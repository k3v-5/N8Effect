//! Dynamic EQ Engine in Rust (DSP #54)
//! 4-Band Dynamic Parametric Equalizer with Envelope Tracking, Threshold Expansion/Compression,
//! and Surgical Dynamic Biquad Filtering (Rules 5, 8, 9, 14, 34, 46, 47).

use crate::common::DENORMAL_GUARD;
use std::f32::consts::PI;

#[derive(Clone, Copy, Default)]
struct Df1BiquadState {
    x1: f32,
    x2: f32,
    y1: f32,
    y2: f32,
}

impl Df1BiquadState {
    #[inline(always)]
    fn reset(&mut self) {
        self.x1 = 0.0;
        self.x2 = 0.0;
        self.y1 = 0.0;
        self.y2 = 0.0;
    }

    #[inline(always)]
    fn process(&mut self, input: f32, b0: f32, b1: f32, b2: f32, a1: f32, a2: f32) -> f32 {
        let y = b0 * input + b1 * self.x1 + b2 * self.x2 - a1 * self.y1 - a2 * self.y2 + DENORMAL_GUARD;
        self.x2 = self.x1;
        self.x1 = input;
        self.y2 = self.y1;
        self.y1 = y;
        y
    }
}

pub struct RustDynamicEq {
    sample_rate: f32,

    // Filters: [channel 0..2][band 0..4]
    filters: [[Df1BiquadState; 4]; 2],

    // Envelope followers per band
    env_levels: [f32; 4],

    // Parameters
    param_b1_freq: f32,   // 1: 20 .. 500 Hz (default 100)
    param_b1_gain: f32,   // 2: -18 .. 18 dB (default 0)
    param_b1_thresh: f32, // 3: -60 .. 0 dB (default -18)
    param_b2_freq: f32,   // 4: 80 .. 4000 Hz (default 500)
    param_b2_gain: f32,   // 5: -18 .. 18 dB (default 0)
    param_b2_thresh: f32, // 6: -60 .. 0 dB (default -18)
    param_b3_freq: f32,   // 7: 300 .. 12000 Hz (default 2500)
    param_b3_gain: f32,   // 8: -18 .. 18 dB (default 0)
    param_b3_thresh: f32, // 9: -60 .. 0 dB (default -18)
    param_b4_freq: f32,   // 10: 1000 .. 20000 Hz (default 8000)
    param_b4_gain: f32,   // 11: -18 .. 18 dB (default 0)
    param_b4_thresh: f32, // 12: -60 .. 0 dB (default -18)
    param_ratio: f32,     // 13: 1.0 .. 10.0 (default 2.5)
    param_attack: f32,    // 14: 1.0 .. 100.0 ms (default 10.0)
    param_release: f32,   // 15: 10.0 .. 1000.0 ms (default 100.0)
    param_mix: f32,       // 16: 0.0 .. 1.0 (default 1.0)
}

impl RustDynamicEq {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 48000.0 };
        Self {
            sample_rate: sr,
            filters: [[Df1BiquadState::default(); 4]; 2],
            env_levels: [0.0; 4],
            param_b1_freq: 100.0,
            param_b1_gain: 0.0,
            param_b1_thresh: -18.0,
            param_b2_freq: 500.0,
            param_b2_gain: 0.0,
            param_b2_thresh: -18.0,
            param_b3_freq: 2500.0,
            param_b3_gain: 0.0,
            param_b3_thresh: -18.0,
            param_b4_freq: 8000.0,
            param_b4_gain: 0.0,
            param_b4_thresh: -18.0,
            param_ratio: 2.5,
            param_attack: 10.0,
            param_release: 100.0,
            param_mix: 1.0,
        }
    }

    pub fn reset(&mut self) {
        for ch in 0..2 {
            for b in 0..4 {
                self.filters[ch][b].reset();
            }
        }
        self.env_levels.fill(0.0);
    }

    pub fn set_sample_rate(&mut self, sample_rate: f32) {
        if sample_rate > 0.0 {
            self.sample_rate = sample_rate;
        }
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.param_b1_freq = value.clamp(20.0, 500.0),
            2 => self.param_b1_gain = value.clamp(-18.0, 18.0),
            3 => self.param_b1_thresh = value.clamp(-60.0, 0.0),
            4 => self.param_b2_freq = value.clamp(80.0, 4000.0),
            5 => self.param_b2_gain = value.clamp(-18.0, 18.0),
            6 => self.param_b2_thresh = value.clamp(-60.0, 0.0),
            7 => self.param_b3_freq = value.clamp(300.0, 12000.0),
            8 => self.param_b3_gain = value.clamp(-18.0, 18.0),
            9 => self.param_b3_thresh = value.clamp(-60.0, 0.0),
            10 => self.param_b4_freq = value.clamp(1000.0, 20000.0),
            11 => self.param_b4_gain = value.clamp(-18.0, 18.0),
            12 => self.param_b4_thresh = value.clamp(-60.0, 0.0),
            13 => self.param_ratio = value.clamp(1.0, 10.0),
            14 => self.param_attack = value.clamp(1.0, 100.0),
            15 => self.param_release = value.clamp(10.0, 1000.0),
            16 => self.param_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.param_b1_freq,
            2 => self.param_b1_gain,
            3 => self.param_b1_thresh,
            4 => self.param_b2_freq,
            5 => self.param_b2_gain,
            6 => self.param_b2_thresh,
            7 => self.param_b3_freq,
            8 => self.param_b3_gain,
            9 => self.param_b3_thresh,
            10 => self.param_b4_freq,
            11 => self.param_b4_gain,
            12 => self.param_b4_thresh,
            13 => self.param_ratio,
            14 => self.param_attack,
            15 => self.param_release,
            16 => self.param_mix,
            _ => 0.0,
        }
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let n_samples = in_l.len().min(out_l.len());
        if n_samples == 0 {
            return;
        }

        let sr = self.sample_rate;
        let att_coeff = (-1.0 / ((self.param_attack.max(1.0) * 0.001) * sr)).exp();
        let rel_coeff = (-1.0 / ((self.param_release.max(10.0) * 0.001) * sr)).exp();
        let ratio = self.param_ratio.max(1.0);
        let mix = self.param_mix.clamp(0.0, 1.0);

        let freqs = [
            self.param_b1_freq,
            self.param_b2_freq,
            self.param_b3_freq,
            self.param_b4_freq,
        ];
        let base_gains = [
            self.param_b1_gain,
            self.param_b2_gain,
            self.param_b3_gain,
            self.param_b4_gain,
        ];
        let thresholds = [
            self.param_b1_thresh,
            self.param_b2_thresh,
            self.param_b3_thresh,
            self.param_b4_thresh,
        ];

        // Precompute frequency angular constants per band
        let mut cosw0_arr = [0.0f32; 4];
        let mut alpha_arr = [0.0f32; 4];
        const Q: f32 = 1.0;

        for b in 0..4 {
            let f0 = freqs[b].clamp(20.0, sr * 0.48);
            let w0 = 2.0 * PI * f0 / sr;
            let cs = w0.cos();
            let sn = w0.sin();
            cosw0_arr[b] = cs;
            alpha_arr[b] = sn / (2.0 * Q);
        }

        for s in 0..n_samples {
            let s_l = in_l[s];
            let s_r = if s < in_r.len() { in_r[s] } else { s_l };

            let mut proc_l = s_l;
            let mut proc_r = s_r;

            for b in 0..4 {
                // 1. Detect band envelope
                let in_mag = (proc_l.abs() + proc_r.abs()) * 0.5;
                let env = if in_mag > self.env_levels[b] {
                    in_mag + att_coeff * (self.env_levels[b] - in_mag)
                } else {
                    in_mag + rel_coeff * (self.env_levels[b] - in_mag)
                };
                self.env_levels[b] = env;

                // 2. Dynamic reduction calculation
                let env_db = 20.0 * (env.max(1.0e-5)).log10();
                let over_threshold = env_db - thresholds[b];
                let dynamic_reduction = if over_threshold > 0.0 {
                    over_threshold * (1.0 - 1.0 / ratio)
                } else {
                    0.0
                };

                let effective_gain_db = base_gains[b] - dynamic_reduction;
                let gain_linear = 10.0f32.powf(effective_gain_db * 0.05);

                // 3. Compute biquad coefficients
                let cosw0 = cosw0_arr[b];
                let alpha = alpha_arr[b];
                let a_val = gain_linear.max(0.01).sqrt();

                let (b0, b1, b2, a0, a1, a2) = if b == 0 {
                    // Low Shelf
                    let ap1 = a_val + 1.0;
                    let am1 = a_val - 1.0;
                    let two_sqrt_a_alpha = 2.0 * a_val.sqrt() * alpha;
                    let b0 = a_val * (ap1 - am1 * cosw0 + two_sqrt_a_alpha);
                    let b1 = 2.0 * a_val * (am1 - ap1 * cosw0);
                    let b2 = a_val * (ap1 - am1 * cosw0 - two_sqrt_a_alpha);
                    let a0 = ap1 + am1 * cosw0 + two_sqrt_a_alpha;
                    let a1 = -2.0 * (am1 + ap1 * cosw0);
                    let a2 = ap1 + am1 * cosw0 - two_sqrt_a_alpha;
                    (b0, b1, b2, a0, a1, a2)
                } else if b == 3 {
                    // High Shelf
                    let ap1 = a_val + 1.0;
                    let am1 = a_val - 1.0;
                    let two_sqrt_a_alpha = 2.0 * a_val.sqrt() * alpha;
                    let b0 = a_val * (ap1 + am1 * cosw0 + two_sqrt_a_alpha);
                    let b1 = -2.0 * a_val * (am1 + ap1 * cosw0);
                    let b2 = a_val * (ap1 + am1 * cosw0 - two_sqrt_a_alpha);
                    let a0 = ap1 - am1 * cosw0 + two_sqrt_a_alpha;
                    let a1 = 2.0 * (am1 - ap1 * cosw0);
                    let a2 = ap1 - am1 * cosw0 - two_sqrt_a_alpha;
                    (b0, b1, b2, a0, a1, a2)
                } else {
                    // Peaking Bell
                    let b0 = 1.0 + alpha * a_val;
                    let b1 = -2.0 * cosw0;
                    let b2 = 1.0 - alpha * a_val;
                    let a0 = 1.0 + alpha / a_val;
                    let a1 = -2.0 * cosw0;
                    let a2 = 1.0 - alpha / a_val;
                    (b0, b1, b2, a0, a1, a2)
                };

                let inv_a0 = 1.0 / a0;
                let nb0 = b0 * inv_a0;
                let nb1 = b1 * inv_a0;
                let nb2 = b2 * inv_a0;
                let na1 = a1 * inv_a0;
                let na2 = a2 * inv_a0;

                proc_l = self.filters[0][b].process(proc_l, nb0, nb1, nb2, na1, na2);
                proc_r = self.filters[1][b].process(proc_r, nb0, nb1, nb2, na1, na2);
            }

            out_l[s] = s_l * (1.0 - mix) + proc_l * mix;
            if s < out_r.len() {
                out_r[s] = s_r * (1.0 - mix) + proc_r * mix;
            }
        }
    }
}
