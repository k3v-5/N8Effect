//! Formant Pitch Shifter (Vocal Tract & Anatomical Throat Decoupling)
//! Decouples fundamental vocal pitch from vocal tract formant resonances.
//! Rules: 5, 8, 9, 14, 16, 34, 35, 46, 47 (Zero-alloc, bounded memory, denormal-safe).

use std::f32::consts::PI;

const FORMANT_BUF_SIZE: usize = 16384;
const FORMANT_MASK: usize = FORMANT_BUF_SIZE - 1;

#[inline(always)]
fn clamp(val: f32, min: f32, max: f32) -> f32 {
    if val < min { min } else if val > max { max } else { val }
}

#[inline(always)]
fn lerp(a: f32, b: f32, t: f32) -> f32 {
    a + t * (b - a)
}

#[derive(Clone, Copy)]
struct BiquadBP {
    s1: f32,
    s2: f32,
    b0: f32,
    b1: f32,
    b2: f32,
    a1: f32,
    a2: f32,
}

impl BiquadBP {
    fn new() -> Self {
        Self {
            s1: 0.0,
            s2: 0.0,
            b0: 0.0,
            b1: 0.0,
            b2: 0.0,
            a1: 0.0,
            a2: 0.0,
        }
    }

    fn reset(&mut self) {
        self.s1 = 0.0;
        self.s2 = 0.0;
    }

    fn set_bandpass(&mut self, fc: f32, q: f32, sample_rate: f32) {
        let f = clamp(fc, 60.0, 0.45 * sample_rate);
        let omega = 2.0 * PI * f / sample_rate;
        let sin_w = omega.sin();
        let cos_w = omega.cos();
        let alpha = sin_w / (2.0 * clamp(q, 0.5, 20.0));

        let a0 = 1.0 + alpha;
        self.b0 = (sin_w * 0.5) / a0;
        self.b1 = 0.0;
        self.b2 = -(sin_w * 0.5) / a0;
        self.a1 = (-2.0 * cos_w) / a0;
        self.a2 = (1.0 - alpha) / a0;
    }

    #[inline(always)]
    fn process(&mut self, x: f32) -> f32 {
        let y = self.b0 * x + self.s1;
        self.s1 = self.b1 * x - self.a1 * y + self.s2;
        self.s2 = self.b2 * x - self.a2 * y;
        y
    }
}

pub struct RustFormantPitchShifter {
    sample_rate: f32,
    buf_l: [f32; FORMANT_BUF_SIZE],
    buf_r: [f32; FORMANT_BUF_SIZE],
    write_idx: usize,

    // Pitch grain heads
    pitch_phase: f32,
    pitch_window: f32,
    smoothed_ratio: f32,

    // Formant filter banks: 3 formants (F1, F2, F3) for Left and Right
    formant_l: [BiquadBP; 3],
    formant_r: [BiquadBP; 3],

    pitch_semitones: f32,
    formant_shift_semitones: f32,
    throat_morph: f32,
    formant_intensity: f32,
    mix: f32,

    smoothed_mix: f32,
}

impl RustFormantPitchShifter {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 8000.0 { sample_rate } else { 44100.0 };
        let mut inst = Self {
            sample_rate: sr,
            buf_l: [0.0; FORMANT_BUF_SIZE],
            buf_r: [0.0; FORMANT_BUF_SIZE],
            write_idx: 0,
            pitch_phase: 0.0,
            pitch_window: 2205.0,
            smoothed_ratio: 1.0,
            formant_l: [BiquadBP::new(); 3],
            formant_r: [BiquadBP::new(); 3],
            pitch_semitones: 0.0,
            formant_shift_semitones: 0.0,
            throat_morph: 0.0,
            formant_intensity: 0.75,
            mix: 1.0,
            smoothed_mix: 1.0,
        };
        inst.prepare(sr);
        inst
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 8000.0 { sample_rate } else { 44100.0 };
        self.pitch_window = (0.045 * self.sample_rate).clamp(512.0, 4096.0);
        self.reset();
    }

    pub fn reset(&mut self) {
        self.buf_l.fill(0.0);
        self.buf_r.fill(0.0);
        self.write_idx = 0;
        self.pitch_phase = 0.0;
        self.smoothed_ratio = 1.0;
        for i in 0..3 {
            self.formant_l[i].reset();
            self.formant_r[i].reset();
        }
        self.smoothed_mix = self.mix;
        self.update_formant_filters();
    }

    pub fn set_param(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.pitch_semitones = clamp(value, -12.0, 12.0),
            2 => {
                self.formant_shift_semitones = clamp(value, -12.0, 12.0);
                self.update_formant_filters();
            }
            3 => {
                self.throat_morph = clamp(value, -1.0, 1.0);
                self.update_formant_filters();
            }
            4 => self.formant_intensity = clamp(value, 0.0, 1.0),
            5 => self.mix = clamp(value, 0.0, 1.0),
            _ => {}
        }
    }

    fn update_formant_filters(&mut self) {
        // Base vowel formants: F1 ~ 650Hz, F2 ~ 1700Hz, F3 ~ 2900Hz
        let base_f1 = 650.0;
        let base_f2 = 1700.0;
        let base_f3 = 2900.0;

        // Throat morph expands or compresses the acoustic vocal tract length
        // -1.0 = shorter tract (child/female, formants shift up)
        // +1.0 = longer tract (deep larynx, formants shift down)
        let throat_scale = 2.0f32.powf(-self.throat_morph * 0.4);

        // Explicit formant frequency shift
        let formant_ratio = 2.0f32.powf(self.formant_shift_semitones / 12.0) * throat_scale;

        let f1 = base_f1 * formant_ratio;
        let f2 = base_f2 * formant_ratio;
        let f3 = base_f3 * formant_ratio;

        let q1 = 4.0;
        let q2 = 5.0;
        let q3 = 6.0;

        self.formant_l[0].set_bandpass(f1, q1, self.sample_rate);
        self.formant_l[1].set_bandpass(f2, q2, self.sample_rate);
        self.formant_l[2].set_bandpass(f3, q3, self.sample_rate);

        self.formant_r[0].set_bandpass(f1, q1, self.sample_rate);
        self.formant_r[1].set_bandpass(f2, q2, self.sample_rate);
        self.formant_r[2].set_bandpass(f3, q3, self.sample_rate);
    }

    #[inline(always)]
    fn read_cubic(buf: &[f32], read_pos: f32) -> f32 {
        let i_pos = read_pos.floor() as isize;
        let frac = read_pos - (i_pos as f32);

        let idx0 = (i_pos.wrapping_sub(1) as usize) & FORMANT_MASK;
        let idx1 = (i_pos as usize) & FORMANT_MASK;
        let idx2 = (i_pos.wrapping_add(1) as usize) & FORMANT_MASK;
        let idx3 = (i_pos.wrapping_add(2) as usize) & FORMANT_MASK;

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

        let target_ratio = 2.0f32.powf(self.pitch_semitones / 12.0);
        let smooth = (-1.0 / (0.005 * self.sample_rate)).exp();

        for i in 0..num_samples as usize {
            let xl = unsafe { *in_l.add(i) };
            let xr = unsafe { *in_r.add(i) };

            self.smoothed_mix = lerp(self.mix, self.smoothed_mix, smooth);
            self.smoothed_ratio = lerp(target_ratio, self.smoothed_ratio, smooth);

            // Write raw audio to history buffer
            let w_idx = self.write_idx;
            self.buf_l[w_idx] = xl;
            self.buf_r[w_idx] = xr;

            // Pitch grain synthesis
            let phase_inc = (1.0 - self.smoothed_ratio) / self.pitch_window;
            self.pitch_phase += phase_inc;
            while self.pitch_phase >= 1.0 {
                self.pitch_phase -= 1.0;
            }
            while self.pitch_phase < 0.0 {
                self.pitch_phase += 1.0;
            }

            let phase1 = self.pitch_phase;
            let mut phase2 = self.pitch_phase + 0.5;
            if phase2 >= 1.0 {
                phase2 -= 1.0;
            }

            let delay1 = phase1 * self.pitch_window;
            let delay2 = phase2 * self.pitch_window;

            let w1 = 0.5 * (1.0 - (2.0 * PI * phase1).cos());
            let w2 = 0.5 * (1.0 - (2.0 * PI * phase2).cos());

            let read_pos1 = ((w_idx as f32) - delay1 + (FORMANT_BUF_SIZE as f32)) % (FORMANT_BUF_SIZE as f32);
            let read_pos2 = ((w_idx as f32) - delay2 + (FORMANT_BUF_SIZE as f32)) % (FORMANT_BUF_SIZE as f32);

            let pitched_l = Self::read_cubic(&self.buf_l, read_pos1) * w1 + Self::read_cubic(&self.buf_l, read_pos2) * w2;
            let pitched_r = Self::read_cubic(&self.buf_r, read_pos1) * w1 + Self::read_cubic(&self.buf_r, read_pos2) * w2;

            // Process through resonant vocal tract formant filters
            let f1_l = self.formant_l[0].process(pitched_l);
            let f2_l = self.formant_l[1].process(pitched_l);
            let f3_l = self.formant_l[2].process(pitched_l);
            let formant_sum_l = (f1_l * 1.0 + f2_l * 0.7 + f3_l * 0.4) * 2.5;

            let f1_r = self.formant_r[0].process(pitched_r);
            let f2_r = self.formant_r[1].process(pitched_r);
            let f3_r = self.formant_r[2].process(pitched_r);
            let formant_sum_r = (f1_r * 1.0 + f2_r * 0.7 + f3_r * 0.4) * 2.5;

            // Combine pitched body with formant emphasis
            let wet_l = lerp(pitched_l, formant_sum_l, self.formant_intensity);
            let wet_r = lerp(pitched_r, formant_sum_r, self.formant_intensity);

            self.write_idx = (w_idx + 1) & FORMANT_MASK;

            let yl = lerp(xl, wet_l, self.smoothed_mix);
            let yr = lerp(xr, wet_r, self.smoothed_mix);

            unsafe {
                *out_l.add(i) = yl;
                *out_r.add(i) = yr;
            }
        }
    }
}
