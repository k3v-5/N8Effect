//! High-Performance Polyphase Oversampling & Saturation Engine (DSP #48 / NodeType::Oversampler)
//! Rules 5, 8, 9, 14, 34, 46, 47:
//! Zero heap allocations in audio processing thread,
//! 3-stage cascaded polyphase halfband filter (1x -> 2x, 4x, 8x) with Kaiser-windowed sinc (>90 dB stopband attenuation),
//! linear phase latency compensation (PDC), non-linear saturation (Soft Tape, Hard Clip, Tube Wavefold),
//! dynamic harmonic drive, dual highpass/lowpass biquads, and denormal protection.

use std::f32::consts::PI;
use crate::common::denormal_guard;

pub const COEFFS_ORDER: usize = 24;

#[derive(Clone, Copy, PartialEq)]
enum BiquadType {
    Lowpass,
    Highpass,
}

#[derive(Clone)]
struct Biquad {
    b0: f32,
    b1: f32,
    b2: f32,
    a1: f32,
    a2: f32,
    z1: f32,
    z2: f32,
}

impl Biquad {
    fn new() -> Self {
        Self {
            b0: 1.0,
            b1: 0.0,
            b2: 0.0,
            a1: 0.0,
            a2: 0.0,
            z1: 0.0,
            z2: 0.0,
        }
    }

    fn reset(&mut self) {
        self.z1 = 0.0;
        self.z2 = 0.0;
    }

    fn set_filter(&mut self, filter_type: BiquadType, sample_rate: f64, freq_hz: f32, q: f32) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let nyquist = sr * 0.499;
        let fc = (freq_hz as f64).clamp(10.0, nyquist);
        let q_val = (q as f64).max(0.01);
        let omega = 2.0 * std::f64::consts::PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q_val);

        let (b0, b1, b2, a0, a1, a2) = match filter_type {
            BiquadType::Lowpass => {
                let b0 = (1.0 - cs) * 0.5;
                let b1 = 1.0 - cs;
                let b2 = (1.0 - cs) * 0.5;
                let a0 = 1.0 + alpha;
                let a1 = -2.0 * cs;
                let a2 = 1.0 - alpha;
                (b0, b1, b2, a0, a1, a2)
            }
            BiquadType::Highpass => {
                let b0 = (1.0 + cs) * 0.5;
                let b1 = -(1.0 + cs);
                let b2 = (1.0 + cs) * 0.5;
                let a0 = 1.0 + alpha;
                let a1 = -2.0 * cs;
                let a2 = 1.0 - alpha;
                (b0, b1, b2, a0, a1, a2)
            }
        };

        let inv_a0 = 1.0 / a0;
        self.b0 = (b0 * inv_a0) as f32;
        self.b1 = (b1 * inv_a0) as f32;
        self.b2 = (b2 * inv_a0) as f32;
        self.a1 = (a1 * inv_a0) as f32;
        self.a2 = (a2 * inv_a0) as f32;
    }

    #[inline(always)]
    fn process(&mut self, input: f32) -> f32 {
        let out = self.b0 * input + self.z1;
        self.z1 = self.b1 * input - self.a1 * out + self.z2;
        self.z2 = self.b2 * input - self.a2 * out;

        if self.z1.is_nan() || self.z1.is_infinite() {
            self.z1 = 0.0;
        }
        if self.z2.is_nan() || self.z2.is_infinite() {
            self.z2 = 0.0;
        }
        out
    }
}

#[inline(always)]
fn fast_tanh(x: f32) -> f32 {
    let x2 = x * x;
    x * (27.0 + x2) / (27.0 + 9.0 * x2)
}

pub struct RustOversampler {
    sample_rate: f64,
    coeffs: [f32; COEFFS_ORDER],

    factor: u8, // 0: 1x, 1: 2x, 2: 4x, 3: 8x
    max_block_size: usize,

    stage1_up_l: Box<[f32]>,
    stage1_up_r: Box<[f32]>,
    stage1_down_l: Box<[f32]>,
    stage1_down_r: Box<[f32]>,
    stage1_state_up_l: [f32; COEFFS_ORDER],
    stage1_state_up_r: [f32; COEFFS_ORDER],
    stage1_state_down_l: [f32; COEFFS_ORDER],
    stage1_state_down_r: [f32; COEFFS_ORDER],

    stage2_up_l: Box<[f32]>,
    stage2_up_r: Box<[f32]>,
    stage2_down_l: Box<[f32]>,
    stage2_down_r: Box<[f32]>,
    stage2_state_up_l: [f32; COEFFS_ORDER],
    stage2_state_up_r: [f32; COEFFS_ORDER],
    stage2_state_down_l: [f32; COEFFS_ORDER],
    stage2_state_down_r: [f32; COEFFS_ORDER],

    stage3_up_l: Box<[f32]>,
    stage3_up_r: Box<[f32]>,
    stage3_down_l: Box<[f32]>,
    stage3_down_r: Box<[f32]>,
    stage3_state_up_l: [f32; COEFFS_ORDER],
    stage3_state_up_r: [f32; COEFFS_ORDER],
    stage3_state_down_l: [f32; COEFFS_ORDER],
    stage3_state_down_r: [f32; COEFFS_ORDER],

    oversampled_l: Box<[f32]>,
    oversampled_r: Box<[f32]>,

    low_cut_l: Biquad,
    low_cut_r: Biquad,
    high_cut_l: Biquad,
    high_cut_r: Biquad,
    last_low_cut: f32,
    last_high_cut: f32,

    param_factor: f32,
    param_drive_db: f32,
    param_sat_mode: f32,
    param_low_cut_hz: f32,
    param_high_cut_hz: f32,
    param_mix: f32,
}

impl RustOversampler {
    pub fn new() -> Self {
        let coeffs = Self::compute_coefficients();
        let max_block = 512;

        let mut node = Self {
            sample_rate: 48000.0,
            coeffs,
            factor: 1, // 2x default
            max_block_size: max_block,

            stage1_up_l: vec![0.0f32; max_block * 2].into_boxed_slice(),
            stage1_up_r: vec![0.0f32; max_block * 2].into_boxed_slice(),
            stage1_down_l: vec![0.0f32; max_block * 2].into_boxed_slice(),
            stage1_down_r: vec![0.0f32; max_block * 2].into_boxed_slice(),
            stage1_state_up_l: [0.0; COEFFS_ORDER],
            stage1_state_up_r: [0.0; COEFFS_ORDER],
            stage1_state_down_l: [0.0; COEFFS_ORDER],
            stage1_state_down_r: [0.0; COEFFS_ORDER],

            stage2_up_l: vec![0.0f32; max_block * 4].into_boxed_slice(),
            stage2_up_r: vec![0.0f32; max_block * 4].into_boxed_slice(),
            stage2_down_l: vec![0.0f32; max_block * 4].into_boxed_slice(),
            stage2_down_r: vec![0.0f32; max_block * 4].into_boxed_slice(),
            stage2_state_up_l: [0.0; COEFFS_ORDER],
            stage2_state_up_r: [0.0; COEFFS_ORDER],
            stage2_state_down_l: [0.0; COEFFS_ORDER],
            stage2_state_down_r: [0.0; COEFFS_ORDER],

            stage3_up_l: vec![0.0f32; max_block * 8].into_boxed_slice(),
            stage3_up_r: vec![0.0f32; max_block * 8].into_boxed_slice(),
            stage3_down_l: vec![0.0f32; max_block * 8].into_boxed_slice(),
            stage3_down_r: vec![0.0f32; max_block * 8].into_boxed_slice(),
            stage3_state_up_l: [0.0; COEFFS_ORDER],
            stage3_state_up_r: [0.0; COEFFS_ORDER],
            stage3_state_down_l: [0.0; COEFFS_ORDER],
            stage3_state_down_r: [0.0; COEFFS_ORDER],

            oversampled_l: vec![0.0f32; max_block * 8].into_boxed_slice(),
            oversampled_r: vec![0.0f32; max_block * 8].into_boxed_slice(),

            low_cut_l: Biquad::new(),
            low_cut_r: Biquad::new(),
            high_cut_l: Biquad::new(),
            high_cut_r: Biquad::new(),
            last_low_cut: 30.0,
            last_high_cut: 18000.0,

            param_factor: 1.0, // 2x
            param_drive_db: 0.0,
            param_sat_mode: 1.0, // Soft tape
            param_low_cut_hz: 30.0,
            param_high_cut_hz: 18000.0,
            param_mix: 1.0,
        };

        node.prepare(48000.0, 512);
        node
    }

    fn compute_coefficients() -> [f32; COEFFS_ORDER] {
        let mut coeffs = [0.0f32; COEFFS_ORDER];
        let alpha = 6.0f32;
        let m = (COEFFS_ORDER / 2) as i32;

        for i in 0..COEFFS_ORDER {
            let n = (i as i32) - m;
            if n == 0 {
                coeffs[i] = 1.0;
            } else if n % 2 == 0 {
                coeffs[i] = 0.0;
            } else {
                let sinc = (PI * 0.5 * (n as f32)).sin() / (PI * (n as f32));
                let kaiser_ratio = (n as f32) / (m as f32);
                let kaiser_arg = 1.0 - kaiser_ratio * kaiser_ratio;
                let win = if kaiser_arg > 0.0 {
                    (alpha * (kaiser_arg.sqrt() - 1.0)).exp()
                } else {
                    0.0
                };
                coeffs[i] = sinc * win * 2.0;
            }
        }
        coeffs
    }

    pub fn prepare(&mut self, sample_rate: f64, max_block_size: usize) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 48000.0 };
        let max_block = max_block_size.max(1024);
        self.max_block_size = max_block;

        self.stage1_up_l = vec![0.0f32; max_block * 2].into_boxed_slice();
        self.stage1_up_r = vec![0.0f32; max_block * 2].into_boxed_slice();
        self.stage1_down_l = vec![0.0f32; max_block * 2].into_boxed_slice();
        self.stage1_down_r = vec![0.0f32; max_block * 2].into_boxed_slice();

        self.stage2_up_l = vec![0.0f32; max_block * 4].into_boxed_slice();
        self.stage2_up_r = vec![0.0f32; max_block * 4].into_boxed_slice();
        self.stage2_down_l = vec![0.0f32; max_block * 4].into_boxed_slice();
        self.stage2_down_r = vec![0.0f32; max_block * 4].into_boxed_slice();

        self.stage3_up_l = vec![0.0f32; max_block * 8].into_boxed_slice();
        self.stage3_up_r = vec![0.0f32; max_block * 8].into_boxed_slice();
        self.stage3_down_l = vec![0.0f32; max_block * 8].into_boxed_slice();
        self.stage3_down_r = vec![0.0f32; max_block * 8].into_boxed_slice();

        self.oversampled_l = vec![0.0f32; max_block * 8].into_boxed_slice();
        self.oversampled_r = vec![0.0f32; max_block * 8].into_boxed_slice();

        self.reset();

        self.low_cut_l.set_filter(BiquadType::Highpass, self.sample_rate, self.param_low_cut_hz, 0.707);
        self.low_cut_r.set_filter(BiquadType::Highpass, self.sample_rate, self.param_low_cut_hz, 0.707);
        self.high_cut_l.set_filter(BiquadType::Lowpass, self.sample_rate, self.param_high_cut_hz, 0.707);
        self.high_cut_r.set_filter(BiquadType::Lowpass, self.sample_rate, self.param_high_cut_hz, 0.707);

        self.last_low_cut = self.param_low_cut_hz;
        self.last_high_cut = self.param_high_cut_hz;
    }

    pub fn reset(&mut self) {
        self.stage1_state_up_l.fill(0.0);
        self.stage1_state_up_r.fill(0.0);
        self.stage1_state_down_l.fill(0.0);
        self.stage1_state_down_r.fill(0.0);

        self.stage2_state_up_l.fill(0.0);
        self.stage2_state_up_r.fill(0.0);
        self.stage2_state_down_l.fill(0.0);
        self.stage2_state_down_r.fill(0.0);

        self.stage3_state_up_l.fill(0.0);
        self.stage3_state_up_r.fill(0.0);
        self.stage3_state_down_l.fill(0.0);
        self.stage3_state_down_r.fill(0.0);

        self.low_cut_l.reset();
        self.low_cut_r.reset();
        self.high_cut_l.reset();
        self.high_cut_r.reset();
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => {
                let f = value.clamp(0.0, 3.0);
                self.param_factor = f;
                let old_factor = self.factor;
                self.factor = f.round() as u8;
                if self.factor != old_factor {
                    self.reset();
                }
            }
            2 => self.param_drive_db = value.clamp(0.0, 24.0),
            3 => self.param_sat_mode = value.clamp(0.0, 3.0),
            4 => self.param_low_cut_hz = value.clamp(20.0, 500.0),
            5 => self.param_high_cut_hz = value.clamp(2000.0, 20000.0),
            6 => self.param_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            1 => self.param_factor,
            2 => self.param_drive_db,
            3 => self.param_sat_mode,
            4 => self.param_low_cut_hz,
            5 => self.param_high_cut_hz,
            6 => self.param_mix,
            _ => 0.0,
        }
    }

    pub fn get_latency_in_samples(&self) -> usize {
        match self.factor {
            0 => 0,  // 1x bypass
            1 => 6,  // 2x
            2 => 9,  // 4x
            3 => 11, // 8x
            _ => 0,
        }
    }

    fn upsample_stage(
        coeffs: &[f32; COEFFS_ORDER],
        in_l: &[f32],
        in_r: &[f32],
        n_in: usize,
        out_l: &mut [f32],
        out_r: &mut [f32],
        state_l: &mut [f32; COEFFS_ORDER],
        state_r: &mut [f32; COEFFS_ORDER],
    ) {
        let m = COEFFS_ORDER / 2;

        for i in 0..n_in {
            let s_l = in_l[i];
            let s_r = in_r[i];

            for k in (1..COEFFS_ORDER).rev() {
                state_l[k] = state_l[k - 1];
                state_r[k] = state_r[k - 1];
            }
            state_l[0] = s_l;
            state_r[0] = s_r;

            out_l[i * 2] = state_l[m];
            out_r[i * 2] = state_r[m];

            let mut acc_l = 0.0f32;
            let mut acc_r = 0.0f32;
            let mut k = 1;
            while k < COEFFS_ORDER {
                acc_l += state_l[k] * coeffs[k];
                acc_r += state_r[k] * coeffs[k];
                k += 2;
            }
            out_l[i * 2 + 1] = denormal_guard(acc_l);
            out_r[i * 2 + 1] = denormal_guard(acc_r);
        }
    }

    fn downsample_stage(
        coeffs: &[f32; COEFFS_ORDER],
        in_l: &[f32],
        in_r: &[f32],
        n_in: usize,
        out_l: &mut [f32],
        out_r: &mut [f32],
        state_l: &mut [f32; COEFFS_ORDER],
        state_r: &mut [f32; COEFFS_ORDER],
    ) {
        let n_out = n_in / 2;
        let m = COEFFS_ORDER / 2;

        for i in 0..n_out {
            for sub in 0..2 {
                let s_l = in_l[i * 2 + sub];
                let s_r = in_r[i * 2 + sub];
                for k in (1..COEFFS_ORDER).rev() {
                    state_l[k] = state_l[k - 1];
                    state_r[k] = state_r[k - 1];
                }
                state_l[0] = s_l;
                state_r[0] = s_r;
            }

            let mut acc_l = state_l[m] * 0.5;
            let mut acc_r = state_r[m] * 0.5;
            let mut k = 1;
            while k < COEFFS_ORDER {
                acc_l += state_l[k] * coeffs[k] * 0.25;
                acc_r += state_r[k] * coeffs[k] * 0.25;
                k += 2;
            }

            out_l[i] = denormal_guard(acc_l);
            out_r[i] = denormal_guard(acc_r);
        }
    }

    #[inline]
    pub fn process(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len();
        if num_samples == 0 {
            return;
        }

        // Filter updates
        if (self.last_low_cut - self.param_low_cut_hz).abs() > 1.0 {
            self.last_low_cut = self.param_low_cut_hz;
            self.low_cut_l.set_filter(BiquadType::Highpass, self.sample_rate, self.param_low_cut_hz, 0.707);
            self.low_cut_r.set_filter(BiquadType::Highpass, self.sample_rate, self.param_low_cut_hz, 0.707);
        }
        if (self.last_high_cut - self.param_high_cut_hz).abs() > 5.0 {
            self.last_high_cut = self.param_high_cut_hz;
            self.high_cut_l.set_filter(BiquadType::Lowpass, self.sample_rate, self.param_high_cut_hz, 0.707);
            self.high_cut_r.set_filter(BiquadType::Lowpass, self.sample_rate, self.param_high_cut_hz, 0.707);
        }

        let factor = self.factor;
        let in_drive = 10.0f32.powf(self.param_drive_db / 20.0);
        let sat_mode = self.param_sat_mode.round() as i32;
        let mix = self.param_mix;
        let coeffs = self.coeffs;

        // Fast path for 1x bypass
        if factor == 0 {
            for s in 0..num_samples {
                let mut l = in_l[s] * in_drive;
                let mut r = in_r[s] * in_drive;

                if sat_mode == 1 {
                    l = fast_tanh(l);
                    r = fast_tanh(r);
                } else if sat_mode == 2 {
                    l = l.clamp(-1.0, 1.0);
                    r = r.clamp(-1.0, 1.0);
                } else if sat_mode == 3 {
                    let fl = (l * 1.5707963).sin();
                    let fr = (r * 1.5707963).sin();
                    l = fast_tanh(fl);
                    r = fast_tanh(fr);
                }

                let out_gain = if in_drive > 1.0 { 1.0 / in_drive.sqrt() } else { 1.0 };
                let mut wet_l = denormal_guard(l * out_gain);
                let mut wet_r = denormal_guard(r * out_gain);

                wet_l = self.low_cut_l.process(wet_l);
                wet_l = self.high_cut_l.process(wet_l);
                wet_r = self.low_cut_r.process(wet_r);
                wet_r = self.high_cut_r.process(wet_r);

                out_l[s] = denormal_guard(in_l[s] * (1.0 - mix) + wet_l * mix);
                out_r[s] = denormal_guard(in_r[s] * (1.0 - mix) + wet_r * mix);
            }
            return;
        }

        // 1. Upsampling
        let num_over_samples = match factor {
            1 => {
                // 1x -> 2x
                let num_2x = num_samples * 2;
                Self::upsample_stage(
                    &coeffs,
                    in_l,
                    in_r,
                    num_samples,
                    &mut self.stage1_up_l[..num_2x],
                    &mut self.stage1_up_r[..num_2x],
                    &mut self.stage1_state_up_l,
                    &mut self.stage1_state_up_r,
                );
                self.oversampled_l[..num_2x].copy_from_slice(&self.stage1_up_l[..num_2x]);
                self.oversampled_r[..num_2x].copy_from_slice(&self.stage1_up_r[..num_2x]);
                num_2x
            }
            2 => {
                // 1x -> 2x -> 4x
                let num_2x = num_samples * 2;
                let num_4x = num_2x * 2;
                Self::upsample_stage(
                    &coeffs,
                    in_l,
                    in_r,
                    num_samples,
                    &mut self.stage1_up_l[..num_2x],
                    &mut self.stage1_up_r[..num_2x],
                    &mut self.stage1_state_up_l,
                    &mut self.stage1_state_up_r,
                );
                Self::upsample_stage(
                    &coeffs,
                    &self.stage1_up_l[..num_2x],
                    &self.stage1_up_r[..num_2x],
                    num_2x,
                    &mut self.stage2_up_l[..num_4x],
                    &mut self.stage2_up_r[..num_4x],
                    &mut self.stage2_state_up_l,
                    &mut self.stage2_state_up_r,
                );
                self.oversampled_l[..num_4x].copy_from_slice(&self.stage2_up_l[..num_4x]);
                self.oversampled_r[..num_4x].copy_from_slice(&self.stage2_up_r[..num_4x]);
                num_4x
            }
            3 => {
                // 1x -> 2x -> 4x -> 8x
                let num_2x = num_samples * 2;
                let num_4x = num_2x * 2;
                let num_8x = num_4x * 2;
                Self::upsample_stage(
                    &coeffs,
                    in_l,
                    in_r,
                    num_samples,
                    &mut self.stage1_up_l[..num_2x],
                    &mut self.stage1_up_r[..num_2x],
                    &mut self.stage1_state_up_l,
                    &mut self.stage1_state_up_r,
                );
                Self::upsample_stage(
                    &coeffs,
                    &self.stage1_up_l[..num_2x],
                    &self.stage1_up_r[..num_2x],
                    num_2x,
                    &mut self.stage2_up_l[..num_4x],
                    &mut self.stage2_up_r[..num_4x],
                    &mut self.stage2_state_up_l,
                    &mut self.stage2_state_up_r,
                );
                Self::upsample_stage(
                    &coeffs,
                    &self.stage2_up_l[..num_4x],
                    &self.stage2_up_r[..num_4x],
                    num_4x,
                    &mut self.stage3_up_l[..num_8x],
                    &mut self.stage3_up_r[..num_8x],
                    &mut self.stage3_state_up_l,
                    &mut self.stage3_state_up_r,
                );
                self.oversampled_l[..num_8x].copy_from_slice(&self.stage3_up_l[..num_8x]);
                self.oversampled_r[..num_8x].copy_from_slice(&self.stage3_up_r[..num_8x]);
                num_8x
            }
            _ => num_samples,
        };

        // 2. Non-linear saturation processing at oversampled rate (anti-aliased)
        if sat_mode > 0 || in_drive > 1.01 {
            let out_gain = if in_drive > 1.0 { 1.0 / in_drive.sqrt() } else { 1.0 };
            for s in 0..num_over_samples {
                let mut l = self.oversampled_l[s] * in_drive;
                let mut r = self.oversampled_r[s] * in_drive;

                if sat_mode == 1 {
                    l = fast_tanh(l);
                    r = fast_tanh(r);
                } else if sat_mode == 2 {
                    l = l.clamp(-1.0, 1.0);
                    r = r.clamp(-1.0, 1.0);
                } else if sat_mode == 3 {
                    let fl = (l * 1.5707963).sin();
                    let fr = (r * 1.5707963).sin();
                    l = fast_tanh(fl);
                    r = fast_tanh(fr);
                }

                self.oversampled_l[s] = denormal_guard(l * out_gain);
                self.oversampled_r[s] = denormal_guard(r * out_gain);
            }
        }

        // 3. Downsampling decimation
        let mut decimated_l = [0.0f32; 1024];
        let mut decimated_r = [0.0f32; 1024];

        match factor {
            1 => {
                // 2x -> 1x
                Self::downsample_stage(
                    &coeffs,
                    &self.oversampled_l[..num_over_samples],
                    &self.oversampled_r[..num_over_samples],
                    num_over_samples,
                    &mut decimated_l[..num_samples],
                    &mut decimated_r[..num_samples],
                    &mut self.stage1_state_down_l,
                    &mut self.stage1_state_down_r,
                );
            }
            2 => {
                // 4x -> 2x -> 1x
                let num_2x = num_over_samples / 2;
                Self::downsample_stage(
                    &coeffs,
                    &self.oversampled_l[..num_over_samples],
                    &self.oversampled_r[..num_over_samples],
                    num_over_samples,
                    &mut self.stage1_down_l[..num_2x],
                    &mut self.stage1_down_r[..num_2x],
                    &mut self.stage2_state_down_l,
                    &mut self.stage2_state_down_r,
                );
                Self::downsample_stage(
                    &coeffs,
                    &self.stage1_down_l[..num_2x],
                    &self.stage1_down_r[..num_2x],
                    num_2x,
                    &mut decimated_l[..num_samples],
                    &mut decimated_r[..num_samples],
                    &mut self.stage1_state_down_l,
                    &mut self.stage1_state_down_r,
                );
            }
            3 => {
                // 8x -> 4x -> 2x -> 1x
                let num_4x = num_over_samples / 2;
                let num_2x = num_4x / 2;
                Self::downsample_stage(
                    &coeffs,
                    &self.oversampled_l[..num_over_samples],
                    &self.oversampled_r[..num_over_samples],
                    num_over_samples,
                    &mut self.stage2_down_l[..num_4x],
                    &mut self.stage2_down_r[..num_4x],
                    &mut self.stage3_state_down_l,
                    &mut self.stage3_state_down_r,
                );
                Self::downsample_stage(
                    &coeffs,
                    &self.stage2_down_l[..num_4x],
                    &self.stage2_down_r[..num_4x],
                    num_4x,
                    &mut self.stage1_down_l[..num_2x],
                    &mut self.stage1_down_r[..num_2x],
                    &mut self.stage2_state_down_l,
                    &mut self.stage2_state_down_r,
                );
                Self::downsample_stage(
                    &coeffs,
                    &self.stage1_down_l[..num_2x],
                    &self.stage1_down_r[..num_2x],
                    num_2x,
                    &mut decimated_l[..num_samples],
                    &mut decimated_r[..num_samples],
                    &mut self.stage1_state_down_l,
                    &mut self.stage1_state_down_r,
                );
            }
            _ => {}
        }

        // 4. Post-filtering & Dry/Wet Mix
        for s in 0..num_samples {
            let mut wet_l = decimated_l[s];
            let mut wet_r = decimated_r[s];

            wet_l = self.low_cut_l.process(wet_l);
            wet_l = self.high_cut_l.process(wet_l);
            wet_r = self.low_cut_r.process(wet_r);
            wet_r = self.high_cut_r.process(wet_r);

            let raw_in_l = in_l[s];
            let raw_in_r = in_r[s];

            out_l[s] = denormal_guard(raw_in_l * (1.0 - mix) + wet_l * mix);
            out_r[s] = denormal_guard(raw_in_r * (1.0 - mix) + wet_r * mix);
        }
    }
}
