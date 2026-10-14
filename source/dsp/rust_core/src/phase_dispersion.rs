//! Allpass Phase Dispersion Cascade / Dispersion Phantasm (Rules 5, 8, 9, 14, 34, 46, 47)
//! Cascaded 24-stage second-order allpass filter chain with non-uniform frequency-dependent group delay,
//! feedback recirculation, damping, and stereo decorrelation. Transforms transients into liquid laser chirps
//! and creates ethereal acoustic dispersion halos.

use crate::common::{clamp, lerp};
use crate::optimization::fast_pade_tanh;

const MAX_STAGES: usize = 24;

#[derive(Clone, Copy)]
struct AllpassBiquadState {
    s1: f32,
    s2: f32,
}

impl AllpassBiquadState {
    const fn new() -> Self {
        Self { s1: 0.0, s2: 0.0 }
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.s1 = 0.0;
        self.s2 = 0.0;
    }

    /// Direct Form II Transposed 2nd-order Allpass
    /// H(z) = (r^2 + b1*z^-1 + z^-2) / (1 + b1*z^-1 + r^2*z^-2)
    /// where b1 = -2 * r * cos(w0)
    #[inline(always)]
    fn process(&mut self, x: f32, b1: f32, r2: f32) -> f32 {
        let y = r2 * x + self.s1;
        self.s1 = b1 * x - b1 * y + self.s2;
        self.s2 = x - r2 * y;
        y
    }
}

pub struct RustPhaseDispersion {
    sample_rate: f32,

    // 24 allpass biquad stages per channel
    stages_l: [AllpassBiquadState; MAX_STAGES],
    stages_r: [AllpassBiquadState; MAX_STAGES],

    // Feedback memory and damping states
    feedback_l: f32,
    feedback_r: f32,
    damp_l: f32,
    damp_r: f32,

    // Parameters
    stage_count: usize,     // 6, 12, 18, or 24 stages
    dispersion_freq: f32,   // Center frequency: 100 Hz to 12000 Hz
    spread: f32,            // Dispersion pole spread (0.0 to 1.0)
    damping_hz: f32,        // 1000 Hz to 20000 Hz
    feedback: f32,          // -0.95 to +0.95
    mix: f32,               // 0.0 to 1.0

    // Smoothed values
    smoothed_freq: f32,
    smoothed_spread: f32,
    smoothed_fb: f32,
    smoothed_mix: f32,
}

impl RustPhaseDispersion {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            stages_l: [AllpassBiquadState::new(); MAX_STAGES],
            stages_r: [AllpassBiquadState::new(); MAX_STAGES],
            feedback_l: 0.0,
            feedback_r: 0.0,
            damp_l: 0.0,
            damp_r: 0.0,
            stage_count: 16,
            dispersion_freq: 1200.0,
            spread: 0.65,
            damping_hz: 9000.0,
            feedback: 0.4,
            mix: 0.7,
            smoothed_freq: 1200.0,
            smoothed_spread: 0.65,
            smoothed_fb: 0.4,
            smoothed_mix: 0.7,
        }
    }

    pub fn reset(&mut self) {
        for s in &mut self.stages_l {
            s.reset();
        }
        for s in &mut self.stages_r {
            s.reset();
        }
        self.feedback_l = 0.0;
        self.feedback_r = 0.0;
        self.damp_l = 0.0;
        self.damp_r = 0.0;
        self.smoothed_freq = self.dispersion_freq;
        self.smoothed_spread = self.spread;
        self.smoothed_fb = self.feedback;
        self.smoothed_mix = self.mix;
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn set_stage_count(&mut self, count: f32) {
        let c = count.round() as usize;
        self.stage_count = c.clamp(4, MAX_STAGES);
    }

    pub fn set_dispersion_freq(&mut self, freq: f32) {
        self.dispersion_freq = clamp(freq, 80.0, 14000.0);
    }

    pub fn set_spread(&mut self, spread: f32) {
        self.spread = clamp(spread, 0.05, 0.98);
    }

    pub fn set_damping(&mut self, damp_hz: f32) {
        self.damping_hz = clamp(damp_hz, 500.0, 20000.0);
    }

    pub fn set_feedback(&mut self, fb: f32) {
        self.feedback = clamp(fb, -0.95, 0.95);
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

        let num_stages = self.stage_count.min(MAX_STAGES);
        let damp_coeff = (-2.0 * std::f32::consts::PI * self.damping_hz / self.sample_rate).exp();

        for i in 0..n {
            // Anti-click parameter smoothing (Rule 35)
            self.smoothed_freq = lerp(self.smoothed_freq, self.dispersion_freq, 0.005);
            self.smoothed_spread = lerp(self.smoothed_spread, self.spread, 0.005);
            self.smoothed_fb = lerp(self.smoothed_fb, self.feedback, 0.005);
            self.smoothed_mix = lerp(self.smoothed_mix, self.mix, 0.005);

            let dry_l = in_l[i];
            let dry_r = in_r[i];

            // Sum input with damped feedback (soft-saturated via Pade tanh to guarantee zero runaway)
            let sat_fb_l = fast_pade_tanh(self.feedback_l * self.smoothed_fb);
            let sat_fb_r = fast_pade_tanh(self.feedback_r * self.smoothed_fb);

            let mut cur_l = dry_l + sat_fb_l;
            let mut cur_r = dry_r + sat_fb_r;

            // Cascade allpass stages with geometrically distributed center frequencies and pole radii
            let base_w0 = 2.0 * std::f32::consts::PI * self.smoothed_freq / self.sample_rate;
            let pole_r = 0.88 + 0.10 * self.smoothed_spread; // r between 0.88 and 0.98
            let r2 = pole_r * pole_r;

            for s in 0..num_stages {
                // Stagger frequencies across stages: creates continuous frequency chirp dispersion
                let factor = (s as f32 - (num_stages as f32 * 0.5)) / (num_stages as f32);
                let w0_l = clamp(base_w0 * (1.0 + factor * self.smoothed_spread * 0.85), 0.01, std::f32::consts::PI - 0.05);
                // Stereo detune on Right channel for 3D holographic acoustic smear
                let w0_r = clamp(base_w0 * (1.0 + factor * self.smoothed_spread * 0.92), 0.01, std::f32::consts::PI - 0.05);

                let b1_l = -2.0 * pole_r * w0_l.cos();
                let b1_r = -2.0 * pole_r * w0_r.cos();

                cur_l = self.stages_l[s].process(cur_l, b1_l, r2);
                cur_r = self.stages_r[s].process(cur_r, b1_r, r2);
            }

            // Lowpass damping on feedback path
            self.damp_l = cur_l * (1.0 - damp_coeff) + self.damp_l * damp_coeff;
            self.damp_r = cur_r * (1.0 - damp_coeff) + self.damp_r * damp_coeff;
            self.feedback_l = self.damp_l;
            self.feedback_r = self.damp_r;

            // Output crossfade
            out_l[i] = lerp(dry_l, cur_l, self.smoothed_mix);
            out_r[i] = lerp(dry_r, cur_r, self.smoothed_mix);
        }
    }
}
