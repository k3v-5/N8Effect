//! Shepard-Risset Barberpole Filter & Infinite Motion Comb (Rules 5, 8, 9, 14, 34, 46, 47)
//! Auditory illusion of infinitely rising or falling spectral notches and resonant peaks.
//! Utilizes 8 staggered 2nd-order allpass/bandpass stages with raised-cosine spectral envelope,
//! nonlinear soft-saturating feedback loop, and stereo phase quadrature.

use crate::common::{clamp, lerp};
use crate::optimization::fast_pade_tanh;

const NUM_POLES: usize = 8;

#[derive(Clone, Copy)]
struct BiquadAllpass {
    s1: f32,
    s2: f32,
}

impl BiquadAllpass {
    const fn new() -> Self {
        Self { s1: 0.0, s2: 0.0 }
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.s1 = 0.0;
        self.s2 = 0.0;
    }

    /// 2nd order allpass filter with coefficient a1, a2
    #[inline(always)]
    fn process(&mut self, x: f32, a1: f32, a2: f32) -> f32 {
        let s1_clean = if self.s1.is_nan() || self.s1.is_infinite() { 0.0 } else { clamp(self.s1, -12.0, 12.0) };
        let s2_clean = if self.s2.is_nan() || self.s2.is_infinite() { 0.0 } else { clamp(self.s2, -12.0, 12.0) };

        let y = a2 * x + s1_clean;
        self.s1 = clamp(a1 * (x - y) + s2_clean, -16.0, 16.0);
        self.s2 = clamp(x - a2 * y, -16.0, 16.0);
        y
    }
}

pub struct RustBarberpoleFilter {
    sample_rate: f32,
    phasor_l: f32,
    phasor_r: f32,

    stages_l: [BiquadAllpass; NUM_POLES],
    stages_r: [BiquadAllpass; NUM_POLES],

    feedback_l: f32,
    feedback_r: f32,

    // Parameters
    rate_hz: f32,       // -4.0 to +4.0 Hz (negative = endless fall, positive = endless rise)
    feedback: f32,      // -0.95 to +0.95
    stereo_spread: f32, // 0.0 to 1.0 (quadrature phase offset between L/R)
    f_min: f32,         // 60.0 to 300.0 Hz
    f_max: f32,         // 6000.0 to 18000.0 Hz
    mix: f32,           // 0.0 to 1.0

    // Smoothed
    smoothed_rate: f32,
    smoothed_fb: f32,
    smoothed_mix: f32,
}

impl RustBarberpoleFilter {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            phasor_l: 0.0,
            phasor_r: 0.25, // Default 90 deg stereo quadrature
            stages_l: [BiquadAllpass::new(); NUM_POLES],
            stages_r: [BiquadAllpass::new(); NUM_POLES],
            feedback_l: 0.0,
            feedback_r: 0.0,
            rate_hz: 0.35,  // Slow hypnotic rising spiral
            feedback: 0.72,
            stereo_spread: 0.5,
            f_min: 80.0,
            f_max: 14000.0,
            mix: 0.7,
            smoothed_rate: 0.35,
            smoothed_fb: 0.72,
            smoothed_mix: 0.7,
        }
    }

    pub fn reset(&mut self) {
        self.phasor_l = 0.0;
        self.phasor_r = 0.25;
        for s in &mut self.stages_l { s.reset(); }
        for s in &mut self.stages_r { s.reset(); }
        self.feedback_l = 0.0;
        self.feedback_r = 0.0;
        self.smoothed_rate = self.rate_hz;
        self.smoothed_fb = self.feedback;
        self.smoothed_mix = self.mix;
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn set_rate(&mut self, rate: f32) {
        self.rate_hz = clamp(rate, -5.0, 5.0);
    }

    pub fn set_feedback(&mut self, fb: f32) {
        self.feedback = clamp(fb, -0.95, 0.95);
    }

    pub fn set_stereo_spread(&mut self, spread: f32) {
        self.stereo_spread = clamp(spread, 0.0, 1.0);
    }

    pub fn set_f_min(&mut self, fmin: f32) {
        self.f_min = clamp(fmin, 40.0, 500.0);
    }

    pub fn set_f_max(&mut self, fmax: f32) {
        self.f_max = clamp(fmax, 4000.0, 20000.0);
    }

    pub fn set_mix(&mut self, mix: f32) {
        self.mix = clamp(mix, 0.0, 1.0);
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
        num_samples: usize,
    ) {
        let n = num_samples.min(in_l.len()).min(in_r.len()).min(out_l.len()).min(out_r.len());
        if n == 0 {
            return;
        }

        let f_ratio = (self.f_max / self.f_min).max(2.0);
        let log_ratio = f_ratio.ln();

        for i in 0..n {
            self.smoothed_rate = lerp(self.smoothed_rate, self.rate_hz, 0.005);
            self.smoothed_fb = lerp(self.smoothed_fb, self.feedback, 0.005);
            self.smoothed_mix = lerp(self.smoothed_mix, self.mix, 0.005);

            let phase_inc = self.smoothed_rate / self.sample_rate;
            self.phasor_l = (self.phasor_l + phase_inc).rem_euclid(1.0);
            self.phasor_r = (self.phasor_l + self.stereo_spread * 0.5).rem_euclid(1.0);

            // Input with soft-saturating feedback
            let in_sat_l = in_l[i] + fast_pade_tanh(self.feedback_l * self.smoothed_fb);
            let in_sat_r = in_r[i] + fast_pade_tanh(self.feedback_r * self.smoothed_fb);

            let mut allpass_accum_l = 0.0f32;
            let mut allpass_accum_r = 0.0f32;
            let mut weight_sum_l = 0.0f32;
            let mut weight_sum_r = 0.0f32;

            for k in 0..NUM_POLES {
                let frac_k = k as f32 / NUM_POLES as f32;
                
                // Left channel pole position
                let theta_l = (self.phasor_l + frac_k).rem_euclid(1.0);
                // Right channel pole position
                let theta_r = (self.phasor_r + frac_k).rem_euclid(1.0);

                // Smooth raised-cosine spectral window fading in at f_min and out at f_max
                let w_l = 0.5 * (1.0 - (std::f32::consts::TAU * theta_l).cos());
                let w_r = 0.5 * (1.0 - (std::f32::consts::TAU * theta_r).cos());

                // Exponential frequency mapping
                let freq_l = self.f_min * (theta_l * log_ratio).exp();
                let freq_r = self.f_min * (theta_r * log_ratio).exp();

                // Compute 2nd order allpass coefficients
                let w0_l = (std::f32::consts::TAU * freq_l / self.sample_rate).min(std::f32::consts::PI * 0.95);
                let w0_r = (std::f32::consts::TAU * freq_r / self.sample_rate).min(std::f32::consts::PI * 0.95);

                let alpha_l = (w0_l * 0.5).tan();
                let alpha_r = (w0_r * 0.5).tan();

                // Allpass coefficients: a1 = -2*cos(w0)/(1+alpha), a2 = (1-alpha)/(1+alpha)
                let a1_l = -w0_l.cos();
                let a2_l = (1.0 - alpha_l) / (1.0 + alpha_l);

                let a1_r = -w0_r.cos();
                let a2_r = (1.0 - alpha_r) / (1.0 + alpha_r);

                let y_l = self.stages_l[k].process(in_sat_l, a1_l, a2_l);
                let y_r = self.stages_r[k].process(in_sat_r, a1_r, a2_r);

                allpass_accum_l += y_l * w_l;
                allpass_accum_r += y_r * w_r;
                weight_sum_l += w_l;
                weight_sum_r += w_r;
            }

            let normalized_ap_l = if weight_sum_l > 0.001 { allpass_accum_l / weight_sum_l } else { in_sat_l };
            let normalized_ap_r = if weight_sum_r > 0.001 { allpass_accum_r / weight_sum_r } else { in_sat_r };

            // Phasing comb interference: blend dry and allpass to create moving notches
            let wet_l = 0.5 * (in_sat_l + normalized_ap_l);
            let wet_r = 0.5 * (in_sat_r + normalized_ap_r);

            self.feedback_l = wet_l;
            self.feedback_r = wet_r;

            out_l[i] = lerp(in_l[i], wet_l, self.smoothed_mix);
            out_r[i] = lerp(in_r[i], wet_r, self.smoothed_mix);
        }
    }
}
