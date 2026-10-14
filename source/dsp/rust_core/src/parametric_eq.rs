//! 3-Band Parametric Equalizer (Low Shelf, Mid Peak, High Shelf) DSP Kernel
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio thread,
//! Robert Bristow-Johnson (RBJ) Audio EQ Cookbook Direct Form II Transposed biquad filters,
//! cascaded per-channel with denormal safety.

use std::f64::consts::PI;
use crate::common::denormal_guard;

#[derive(Clone, Copy, Default)]
struct BiquadCoeffs {
    b0: f32,
    b1: f32,
    b2: f32,
    a1: f32,
    a2: f32,
}

#[derive(Clone, Copy, Default)]
struct BiquadState {
    z1: f32,
    z2: f32,
}

impl BiquadState {
    #[inline(always)]
    fn reset(&mut self) {
        self.z1 = 0.0;
        self.z2 = 0.0;
    }

    #[inline(always)]
    fn process(&mut self, in_val: f32, c: &BiquadCoeffs) -> f32 {
        let out = c.b0 * in_val + self.z1;
        self.z1 = c.b1 * in_val - c.a1 * out + self.z2;
        self.z2 = c.b2 * in_val - c.a2 * out;

        if self.z1.is_nan() || self.z1.is_infinite() {
            self.z1 = 0.0;
        }
        if self.z2.is_nan() || self.z2.is_infinite() {
            self.z2 = 0.0;
        }
        out
    }
}

pub struct RustParametricEq {
    sample_rate: f32,

    low_freq: f32,
    low_gain: f32,
    mid_freq: f32,
    mid_q: f32,
    mid_gain: f32,
    high_freq: f32,
    high_gain: f32,

    low_coeffs: BiquadCoeffs,
    mid_coeffs: BiquadCoeffs,
    high_coeffs: BiquadCoeffs,

    state_low: [BiquadState; 2],
    state_mid: [BiquadState; 2],
    state_high: [BiquadState; 2],
}

impl RustParametricEq {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut eq = Self {
            sample_rate: sr,
            low_freq: 100.0,
            low_gain: 0.0,
            mid_freq: 1000.0,
            mid_q: 1.0,
            mid_gain: 0.0,
            high_freq: 8000.0,
            high_gain: 0.0,
            low_coeffs: BiquadCoeffs::default(),
            mid_coeffs: BiquadCoeffs::default(),
            high_coeffs: BiquadCoeffs::default(),
            state_low: [BiquadState::default(); 2],
            state_mid: [BiquadState::default(); 2],
            state_high: [BiquadState::default(); 2],
        };
        eq.update_filters();
        eq
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
        self.update_filters();
    }

    pub fn reset(&mut self) {
        for s in &mut self.state_low {
            s.reset();
        }
        for s in &mut self.state_mid {
            s.reset();
        }
        for s in &mut self.state_high {
            s.reset();
        }
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => {
                self.low_freq = value.clamp(20.0, 500.0);
                self.update_low_shelf();
            }
            2 => {
                self.low_gain = value.clamp(-18.0, 18.0);
                self.update_low_shelf();
            }
            3 => {
                self.mid_freq = value.clamp(100.0, 8000.0);
                self.update_mid_peak();
            }
            4 => {
                self.mid_q = value.clamp(0.1, 10.0);
                self.update_mid_peak();
            }
            5 => {
                self.mid_gain = value.clamp(-18.0, 18.0);
                self.update_mid_peak();
            }
            6 => {
                self.high_freq = value.clamp(1000.0, 18000.0);
                self.update_high_shelf();
            }
            7 => {
                self.high_gain = value.clamp(-18.0, 18.0);
                self.update_high_shelf();
            }
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.low_freq,
            2 => self.low_gain,
            3 => self.mid_freq,
            4 => self.mid_q,
            5 => self.mid_gain,
            6 => self.high_freq,
            7 => self.high_gain,
            _ => 0.0,
        }
    }

    fn update_filters(&mut self) {
        self.update_low_shelf();
        self.update_mid_peak();
        self.update_high_shelf();
    }

    fn update_low_shelf(&mut self) {
        let sr = self.sample_rate as f64;
        let nyquist = sr * 0.499;
        let fc = (self.low_freq as f64).clamp(10.0, nyquist);
        let q = 0.70710678f64;
        let a = 10.0f64.powf((self.low_gain as f64) / 40.0);
        let omega = 2.0 * PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q);
        let beta = 2.0 * a.sqrt() * alpha;

        let b0 = a * ((a + 1.0) - (a - 1.0) * cs + beta);
        let b1 = 2.0 * a * ((a - 1.0) - (a + 1.0) * cs);
        let b2 = a * ((a + 1.0) - (a - 1.0) * cs - beta);
        let a0 = (a + 1.0) + (a - 1.0) * cs + beta;
        let a1 = -2.0 * ((a - 1.0) + (a + 1.0) * cs);
        let a2 = (a + 1.0) + (a - 1.0) * cs - beta;

        let inv_a0 = 1.0 / a0;
        self.low_coeffs = BiquadCoeffs {
            b0: (b0 * inv_a0) as f32,
            b1: (b1 * inv_a0) as f32,
            b2: (b2 * inv_a0) as f32,
            a1: (a1 * inv_a0) as f32,
            a2: (a2 * inv_a0) as f32,
        };
    }

    fn update_mid_peak(&mut self) {
        let sr = self.sample_rate as f64;
        let nyquist = sr * 0.499;
        let fc = (self.mid_freq as f64).clamp(10.0, nyquist);
        let q = (self.mid_q as f64).max(0.01);
        let a = 10.0f64.powf((self.mid_gain as f64) / 40.0);
        let omega = 2.0 * PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q);

        let b0 = 1.0 + alpha * a;
        let b1 = -2.0 * cs;
        let b2 = 1.0 - alpha * a;
        let a0 = 1.0 + alpha / a;
        let a1 = -2.0 * cs;
        let a2 = 1.0 - alpha / a;

        let inv_a0 = 1.0 / a0;
        self.mid_coeffs = BiquadCoeffs {
            b0: (b0 * inv_a0) as f32,
            b1: (b1 * inv_a0) as f32,
            b2: (b2 * inv_a0) as f32,
            a1: (a1 * inv_a0) as f32,
            a2: (a2 * inv_a0) as f32,
        };
    }

    fn update_high_shelf(&mut self) {
        let sr = self.sample_rate as f64;
        let nyquist = sr * 0.499;
        let fc = (self.high_freq as f64).clamp(10.0, nyquist);
        let q = 0.70710678f64;
        let a = 10.0f64.powf((self.high_gain as f64) / 40.0);
        let omega = 2.0 * PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q);
        let beta = 2.0 * a.sqrt() * alpha;

        let b0 = a * ((a + 1.0) + (a - 1.0) * cs + beta);
        let b1 = -2.0 * a * ((a - 1.0) + (a + 1.0) * cs);
        let b2 = a * ((a + 1.0) + (a - 1.0) * cs - beta);
        let a0 = (a + 1.0) - (a - 1.0) * cs + beta;
        let a1 = 2.0 * ((a - 1.0) + (a + 1.0) * cs);
        let a2 = (a + 1.0) - (a - 1.0) * cs - beta;

        let inv_a0 = 1.0 / a0;
        self.high_coeffs = BiquadCoeffs {
            b0: (b0 * inv_a0) as f32,
            b1: (b1 * inv_a0) as f32,
            b2: (b2 * inv_a0) as f32,
            a1: (a1 * inv_a0) as f32,
            a2: (a2 * inv_a0) as f32,
        };
    }

    #[inline]
    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len().min(out_l.len());
        let c_low = self.low_coeffs;
        let c_mid = self.mid_coeffs;
        let c_high = self.high_coeffs;

        for s in 0..num_samples {
            let s_l = denormal_guard(in_l[s]);
            let s_r = denormal_guard(if s < in_r.len() { in_r[s] } else { s_l });

            // Canal L
            let l1 = self.state_low[0].process(s_l, &c_low);
            let l2 = self.state_mid[0].process(l1, &c_mid);
            let l3 = self.state_high[0].process(l2, &c_high);
            out_l[s] = denormal_guard(l3);

            // Canal R
            if s < out_r.len() {
                let r1 = self.state_low[1].process(s_r, &c_low);
                let r2 = self.state_mid[1].process(r1, &c_mid);
                let r3 = self.state_high[1].process(r2, &c_high);
                out_r[s] = denormal_guard(r3);
            }
        }
    }
}
