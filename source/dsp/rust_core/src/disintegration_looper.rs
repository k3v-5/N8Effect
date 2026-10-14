//! Disintegration Tape Looper (Rules 5, 8, 9, 14, 34, 35, 46, 47)
//! Inspired by William Basinski's "The Disintegration Loops".
//! Recirculates captured audio through an eroding virtual magnetic tape reel with
//! progressive oxide loss (accumulative high-frequency loss), non-linear wow & flutter pitch drift,
//! sporadic splice dropouts, and magnetic tape hysteresis saturation.

use crate::common::{clamp, lerp, denormal_guard, fast_tanh};
use std::f32::consts::PI;

const MAX_LOOP_SAMPLES: usize = 131072; // ~2.97s at 44.1kHz, strictly bounded
const LOOP_MASK: usize = MAX_LOOP_SAMPLES - 1;

pub struct RustDisintegrationLooper {
    sample_rate: f32,

    buf_l: Box<[f32]>,
    buf_r: Box<[f32]>,
    write_idx: usize,

    // Oxide degradation 1-pole lowpass filters
    oxide_state_l: f32,
    oxide_state_r: f32,

    // Wow and Flutter LFO phases
    wow_phase: f32,
    flutter_phase: f32,

    // Dropout burst state
    dropout_gain: f32,
    dropout_timer: u32,
    rng_state: u32,

    // Input envelope follower for silence gating
    env_loop: f32,

    // Parameters
    loop_length_ms: f32,      // 250.0 to 2800.0 ms
    feedback: f32,            // 0.2 to 1.0 (1.0 = infinite decay loop)
    oxide_decay: f32,         // 0.0 to 1.0 (HF loss per cycle)
    wow_flutter: f32,         // 0.0 to 1.0 (pitch instability)
    dropout_intensity: f32,   // 0.0 to 1.0 (random mute bursts)
    tape_warmth: f32,         // 0.0 to 1.0 (saturation amount)
    mix: f32,                 // 0.0 to 1.0

    // Smoothed
    smoothed_fb: f32,
    smoothed_mix: f32,
}

impl RustDisintegrationLooper {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            buf_l: vec![0.0f32; MAX_LOOP_SAMPLES].into_boxed_slice(),
            buf_r: vec![0.0f32; MAX_LOOP_SAMPLES].into_boxed_slice(),
            write_idx: 0,
            oxide_state_l: 0.0,
            oxide_state_r: 0.0,
            wow_phase: 0.0,
            flutter_phase: 0.0,
            dropout_gain: 1.0,
            dropout_timer: 44100,
            rng_state: 0x51f382a9,
            env_loop: 0.0,
            loop_length_ms: 1800.0,
            feedback: 0.92,
            oxide_decay: 0.45,
            wow_flutter: 0.35,
            dropout_intensity: 0.25,
            tape_warmth: 0.6,
            mix: 0.85,
            smoothed_fb: 0.92,
            smoothed_mix: 0.85,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn reset(&mut self) {
        self.buf_l.fill(0.0);
        self.buf_r.fill(0.0);
        self.write_idx = 0;
        self.oxide_state_l = 0.0;
        self.oxide_state_r = 0.0;
        self.wow_phase = 0.0;
        self.flutter_phase = 0.0;
        self.dropout_gain = 1.0;
        self.dropout_timer = (self.sample_rate * 2.0) as u32;
        self.env_loop = 0.0;
        self.smoothed_fb = self.feedback;
        self.smoothed_mix = self.mix;
    }

    #[inline(always)]
    fn next_random(&mut self) -> f32 {
        let mut x = self.rng_state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        self.rng_state = x;
        (x as f32) / (u32::MAX as f32)
    }

    pub fn set_param(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.loop_length_ms = clamp(value, 200.0, 2800.0),
            2 => self.feedback = clamp(value, 0.1, 1.0),
            3 => self.oxide_decay = clamp(value, 0.0, 1.0),
            4 => self.wow_flutter = clamp(value, 0.0, 1.0),
            5 => self.dropout_intensity = clamp(value, 0.0, 1.0),
            6 => self.tape_warmth = clamp(value, 0.0, 1.0),
            7 => self.mix = clamp(value, 0.0, 1.0),
            _ => {}
        }
    }

    #[inline(always)]
    fn read_sample_cubic(buf: &[f32], read_pos: f32) -> f32 {
        let i_pos = read_pos.floor() as isize;
        let frac = read_pos - (i_pos as f32);

        let idx0 = (i_pos.wrapping_sub(1) as usize) & LOOP_MASK;
        let idx1 = (i_pos as usize) & LOOP_MASK;
        let idx2 = (i_pos.wrapping_add(1) as usize) & LOOP_MASK;
        let idx3 = (i_pos.wrapping_add(2) as usize) & LOOP_MASK;

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

        let loop_samples = (self.loop_length_ms * 0.001 * self.sample_rate).clamp(100.0, (MAX_LOOP_SAMPLES - 32) as f32);
        let wow_inc = 2.0 * PI * 0.45 / self.sample_rate;
        let flutter_inc = 2.0 * PI * 6.2 / self.sample_rate;
        let smooth = (-1.0 / (0.005 * self.sample_rate)).exp();

        // Oxide lowpass filter alpha: 0 decay -> 18kHz, 1.0 decay -> 2kHz
        let oxide_cutoff = lerp(18000.0, 2200.0, self.oxide_decay);
        let oxide_alpha = (-2.0 * PI * oxide_cutoff / self.sample_rate).exp();

        for i in 0..num_samples as usize {
            let xl = unsafe { *in_l.add(i) };
            let xr = unsafe { *in_r.add(i) };

            self.smoothed_fb = lerp(self.feedback, self.smoothed_fb, smooth);
            self.smoothed_mix = lerp(self.mix, self.smoothed_mix, smooth);

            // Wow & Flutter delay modulation
            let wow_mod = self.wow_phase.sin() * 25.0 * self.wow_flutter;
            let flutter_mod = self.flutter_phase.sin() * 6.0 * self.wow_flutter;
            self.wow_phase = (self.wow_phase + wow_inc) % (2.0 * PI);
            self.flutter_phase = (self.flutter_phase + flutter_inc) % (2.0 * PI);

            let modulated_delay = (loop_samples + wow_mod + flutter_mod).clamp(10.0, (MAX_LOOP_SAMPLES - 16) as f32);
            let read_pos = ((self.write_idx as f32) - modulated_delay + (MAX_LOOP_SAMPLES as f32)) % (MAX_LOOP_SAMPLES as f32);

            // Read degraded audio from loop
            let raw_loop_l = Self::read_sample_cubic(&self.buf_l, read_pos);
            let raw_loop_r = Self::read_sample_cubic(&self.buf_r, read_pos);

            // Dropout burst simulation
            if self.dropout_intensity > 0.01 {
                if self.dropout_timer == 0 {
                    let r = self.next_random();
                    if r < self.dropout_intensity * 0.0008 {
                        // Trigger dropout
                        self.dropout_gain = lerp(0.05, 0.4, self.next_random());
                        self.dropout_timer = ((0.008 + 0.025 * self.next_random()) * self.sample_rate) as u32;
                    }
                } else {
                    self.dropout_timer -= 1;
                    if self.dropout_timer == 0 {
                        self.dropout_gain = 1.0;
                    }
                }
            } else {
                self.dropout_gain = 1.0;
            }

            // Oxide loss lowpass filter
            self.oxide_state_l = lerp(raw_loop_l, self.oxide_state_l, oxide_alpha);
            self.oxide_state_r = lerp(raw_loop_r, self.oxide_state_r, oxide_alpha);

            // Apply tape saturation and dropout
            let drive_gain = 1.0 + self.tape_warmth * 1.5;
            let saturated_l = fast_tanh(self.oxide_state_l * drive_gain) * self.dropout_gain;
            let saturated_r = fast_tanh(self.oxide_state_r * drive_gain) * self.dropout_gain;

            let loop_sig_l = saturated_l * self.smoothed_fb;
            let loop_sig_r = saturated_r * self.smoothed_fb;

            // Track loop energy for silence gating
            let loop_energy = loop_sig_l.abs() + loop_sig_r.abs() + xl.abs() + xr.abs();
            self.env_loop = lerp(loop_energy, self.env_loop, 0.999);

            if self.env_loop < 1e-5 && loop_energy < 1e-5 {
                self.buf_l[self.write_idx] = 0.0;
                self.buf_r[self.write_idx] = 0.0;
                self.write_idx = (self.write_idx + 1) & LOOP_MASK;
                unsafe {
                    *out_l.add(i) = 0.0;
                    *out_r.add(i) = 0.0;
                }
                continue;
            }

            // Write sum to loop buffer
            self.buf_l[self.write_idx] = denormal_guard(xl + loop_sig_l);
            self.buf_r[self.write_idx] = denormal_guard(xr + loop_sig_r);
            self.write_idx = (self.write_idx + 1) & LOOP_MASK;

            let out_sample_l = denormal_guard(lerp(xl, loop_sig_l, self.smoothed_mix));
            let out_sample_r = denormal_guard(lerp(xr, loop_sig_r, self.smoothed_mix));

            unsafe {
                *out_l.add(i) = out_sample_l;
                *out_r.add(i) = out_sample_r;
            }
        }
    }
}
