//! Microtonal Harmonizer & Non-Equal Temperament Scale Engine
//! Implements dual-voice microtonal pitch shifting with quarter-tone (24-EDO),
//! 19-EDO, 31-EDO, and 5-limit Just Intonation scale quantization.
//! Rules: 5, 8, 9, 14, 16, 34, 35, 46, 47 (Zero-alloc, bounded memory, denormal-safe).

use std::f32::consts::PI;

const HARMONIZER_BUF_SIZE: usize = 16384;
const HARMONIZER_MASK: usize = HARMONIZER_BUF_SIZE - 1;

#[inline(always)]
fn clamp(val: f32, min: f32, max: f32) -> f32 {
    if val < min { min } else if val > max { max } else { val }
}

#[inline(always)]
fn lerp(a: f32, b: f32, t: f32) -> f32 {
    a + t * (b - a)
}

#[derive(Clone, Copy)]
struct PitchVoice {
    phase: f32,        // Phase in window [0.0, 1.0)
    delay_window: f32, // Window length in samples (~50ms)
    smoothed_ratio: f32,
}

impl PitchVoice {
    fn new() -> Self {
        Self {
            phase: 0.0,
            delay_window: 2205.0,
            smoothed_ratio: 1.0,
        }
    }

    fn reset(&mut self) {
        self.phase = 0.0;
        self.smoothed_ratio = 1.0;
    }

    #[inline(always)]
    fn process_sample(
        &mut self,
        buf: &[f32],
        write_idx: usize,
        target_ratio: f32,
    ) -> f32 {
        // Smooth target pitch ratio
        self.smoothed_ratio += 0.001 * (target_ratio - self.smoothed_ratio);
        let ratio = self.smoothed_ratio;

        // Rate of phase change: (1.0 - ratio) / delay_window
        let phase_inc = (1.0 - ratio) / self.delay_window;
        self.phase += phase_inc;
        while self.phase >= 1.0 {
            self.phase -= 1.0;
        }
        while self.phase < 0.0 {
            self.phase += 1.0;
        }

        // Two read heads 180 degrees apart
        let phase1 = self.phase;
        let mut phase2 = self.phase + 0.5;
        if phase2 >= 1.0 {
            phase2 -= 1.0;
        }

        // Delay offsets
        let delay1 = phase1 * self.delay_window;
        let delay2 = phase2 * self.delay_window;

        // Hann window weights: 0.5 * (1 - cos(2*pi*phase))
        let w1 = 0.5 * (1.0 - (2.0 * PI * phase1).cos());
        let w2 = 0.5 * (1.0 - (2.0 * PI * phase2).cos());

        let read_pos1 = ((write_idx as f32) - delay1 + (HARMONIZER_BUF_SIZE as f32)) % (HARMONIZER_BUF_SIZE as f32);
        let read_pos2 = ((write_idx as f32) - delay2 + (HARMONIZER_BUF_SIZE as f32)) % (HARMONIZER_BUF_SIZE as f32);

        let s1 = Self::read_cubic(buf, read_pos1);
        let s2 = Self::read_cubic(buf, read_pos2);

        s1 * w1 + s2 * w2
    }

    #[inline(always)]
    fn read_cubic(buf: &[f32], read_pos: f32) -> f32 {
        let i_pos = read_pos.floor() as isize;
        let frac = read_pos - (i_pos as f32);

        let idx0 = (i_pos.wrapping_sub(1) as usize) & HARMONIZER_MASK;
        let idx1 = (i_pos as usize) & HARMONIZER_MASK;
        let idx2 = (i_pos.wrapping_add(1) as usize) & HARMONIZER_MASK;
        let idx3 = (i_pos.wrapping_add(2) as usize) & HARMONIZER_MASK;

        let xm1 = buf[idx0];
        let x0 = buf[idx1];
        let x1 = buf[idx2];
        let x2 = buf[idx3];

        let c0 = x0;
        let c1 = 0.5 * (x1 - xm1);
        let c2 = xm1 - 2.5 * x0 + 2.0 * x1 - 0.5 * x2;
        let c3 = 0.5 * (x2 - xm1) + 1.5 * (x0 - x1);

        ((c3 * frac + c2) * frac + c1) * frac + c0
    }
}

pub struct RustMicrotonalHarmonizer {
    sample_rate: f32,
    buf_l: [f32; HARMONIZER_BUF_SIZE],
    buf_r: [f32; HARMONIZER_BUF_SIZE],
    write_idx: usize,

    voice1_l: PitchVoice,
    voice1_r: PitchVoice,
    voice2_l: PitchVoice,
    voice2_r: PitchVoice,

    v1_semitones: f32,
    v1_cents: f32,
    v1_pan: f32,
    v2_semitones: f32,
    v2_cents: f32,
    v2_pan: f32,
    scale_mode: u32,
    feedback: f32,
    mix: f32,

    fb_l: f32,
    fb_r: f32,
    smoothed_mix: f32,
}

impl RustMicrotonalHarmonizer {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 8000.0 { sample_rate } else { 44100.0 };
        let mut inst = Self {
            sample_rate: sr,
            buf_l: [0.0; HARMONIZER_BUF_SIZE],
            buf_r: [0.0; HARMONIZER_BUF_SIZE],
            write_idx: 0,
            voice1_l: PitchVoice::new(),
            voice1_r: PitchVoice::new(),
            voice2_l: PitchVoice::new(),
            voice2_r: PitchVoice::new(),
            v1_semitones: 0.0,
            v1_cents: 31.0,
            v1_pan: -0.6,
            v2_semitones: 7.0,
            v2_cents: -14.0,
            v2_pan: 0.6,
            scale_mode: 0,
            feedback: 0.0,
            mix: 0.85,
            fb_l: 0.0,
            fb_r: 0.0,
            smoothed_mix: 0.85,
        };
        inst.prepare(sr);
        inst
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 8000.0 { sample_rate } else { 44100.0 };
        let window_len = (0.050 * self.sample_rate).clamp(512.0, 4096.0);
        self.voice1_l.delay_window = window_len;
        self.voice1_r.delay_window = window_len;
        self.voice2_l.delay_window = window_len;
        self.voice2_r.delay_window = window_len;
        self.reset();
    }

    pub fn reset(&mut self) {
        self.buf_l.fill(0.0);
        self.buf_r.fill(0.0);
        self.write_idx = 0;
        self.voice1_l.reset();
        self.voice1_r.reset();
        self.voice2_l.reset();
        self.voice2_r.reset();
        self.fb_l = 0.0;
        self.fb_r = 0.0;
        self.smoothed_mix = self.mix;
    }

    pub fn set_param(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.v1_semitones = clamp(value, -24.0, 24.0),
            2 => self.v1_cents = clamp(value, -100.0, 100.0),
            3 => self.v1_pan = clamp(value, -1.0, 1.0),
            4 => self.v2_semitones = clamp(value, -24.0, 24.0),
            5 => self.v2_cents = clamp(value, -100.0, 100.0),
            6 => self.v2_pan = clamp(value, -1.0, 1.0),
            7 => self.scale_mode = clamp(value, 0.0, 4.0) as u32,
            8 => self.feedback = clamp(value, 0.0, 0.85),
            9 => self.mix = clamp(value, 0.0, 1.0),
            _ => {}
        }
    }

    fn quantize_pitch(&self, raw_st: f32) -> f32 {
        match self.scale_mode {
            // Mode 0: Free / Chromatic Cents
            0 => raw_st,
            // Mode 1: 24-EDO (Quarter-tones, 50 cents step)
            1 => (raw_st * 2.0).round() * 0.5,
            // Mode 2: 19-EDO (1 step = 12/19 st)
            2 => {
                let step = 12.0 / 19.0;
                (raw_st / step).round() * step
            }
            // Mode 3: 31-EDO (1 step = 12/31 st)
            3 => {
                let step = 12.0 / 31.0;
                (raw_st / step).round() * step
            }
            // Mode 4: Just Intonation 5-limit
            4 => {
                let octave = raw_st.floor() / 12.0;
                let semitone_in_oct = raw_st - octave * 12.0;
                // Just intonation scale in semitones:
                // 1/1(0), 16/15(1.12), 9/8(2.04), 6/5(3.16), 5/4(3.86), 4/3(4.98),
                // 45/32(5.90), 3/2(7.02), 8/5(8.14), 5/3(8.84), 9/5(10.18), 15/8(10.88), 2/1(12.0)
                const JI_STEPS: [f32; 13] = [
                    0.0, 1.117, 2.039, 3.156, 3.863, 4.980, 5.902, 7.020, 8.137, 8.844, 10.176, 10.883, 12.0
                ];
                let mut best_diff = 100.0;
                let mut best_val = semitone_in_oct;
                for &ji in &JI_STEPS {
                    let d = (semitone_in_oct - ji).abs();
                    if d < best_diff {
                        best_diff = d;
                        best_val = ji;
                    }
                }
                octave * 12.0 + best_val
            }
            _ => raw_st,
        }
    }

    pub fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: u32,
    ) {
        if in_l.is_null() || in_r.is_null() || out_l.is_null() || out_r.is_null() || num_samples == 0 {
            return;
        }

        let st1 = self.quantize_pitch(self.v1_semitones + self.v1_cents * 0.01);
        let st2 = self.quantize_pitch(self.v2_semitones + self.v2_cents * 0.01);

        let ratio1 = 2.0f32.powf(st1 / 12.0);
        let ratio2 = 2.0f32.powf(st2 / 12.0);

        let pan_ang1 = ((self.v1_pan + 1.0) * 0.5) * (PI * 0.5);
        let v1_gl = pan_ang1.cos();
        let v1_gr = pan_ang1.sin();

        let pan_ang2 = ((self.v2_pan + 1.0) * 0.5) * (PI * 0.5);
        let v2_gl = pan_ang2.cos();
        let v2_gr = pan_ang2.sin();

        let smooth = (-1.0 / (0.005 * self.sample_rate)).exp();

        for i in 0..num_samples as usize {
            let xl = unsafe { *in_l.add(i) };
            let xr = unsafe { *in_r.add(i) };

            self.smoothed_mix = lerp(self.mix, self.smoothed_mix, smooth);

            // Write into ring buffers with feedback
            let w_idx = self.write_idx;
            self.buf_l[w_idx] = xl + self.fb_l * self.feedback;
            self.buf_r[w_idx] = xr + self.fb_r * self.feedback;

            // Process Voice 1
            let v1_l = self.voice1_l.process_sample(&self.buf_l, w_idx, ratio1);
            let v1_r = self.voice1_r.process_sample(&self.buf_r, w_idx, ratio1);
            let v1_mono = 0.5 * (v1_l + v1_r);

            // Process Voice 2
            let v2_l = self.voice2_l.process_sample(&self.buf_l, w_idx, ratio2);
            let v2_r = self.voice2_r.process_sample(&self.buf_r, w_idx, ratio2);
            let v2_mono = 0.5 * (v2_l + v2_r);

            // Pan and sum
            let wet_l = v1_mono * v1_gl + v2_mono * v2_gl;
            let wet_r = v1_mono * v1_gr + v2_mono * v2_gr;

            // Update feedback states
            self.fb_l = wet_l;
            self.fb_r = wet_r;

            self.write_idx = (w_idx + 1) & HARMONIZER_MASK;

            let yl = lerp(xl, wet_l, self.smoothed_mix);
            let yr = lerp(xr, wet_r, self.smoothed_mix);

            unsafe {
                *out_l.add(i) = yl;
                *out_r.add(i) = yr;
            }
        }
    }
}
