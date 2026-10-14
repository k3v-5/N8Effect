//! Dual Bode Frequency Shifter (Quadrature Cross-Feedback Matrix) (Rules 5, 8, 9, 14, 34, 35, 46, 47)
//! Harold Bode Single-Sideband (SSB) frequency shifting with dual quadrature Hilbert networks,
//! independent stereo detune, upper/lower sideband balance, and resonant cross-feedback
//! for metallic clangorous bells, alien inharmonic rings, and spectral spirals.

use crate::common::{clamp, lerp, denormal_guard, fast_tanh, fast_sin};
use std::f32::consts::PI;

// 4-stage 1st-order allpass network for 90-degree Hilbert phase split across 20Hz-20kHz
#[derive(Clone, Copy, Default)]
struct Allpass1st {
    z1: f32,
}

impl Allpass1st {
    #[inline(always)]
    fn process(&mut self, x: f32, coeff: f32) -> f32 {
        let y = -coeff * x + self.z1;
        self.z1 = x + coeff * y;
        denormal_guard(y)
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.z1 = 0.0;
    }
}

pub struct RustDualBodeFrequencyShifter {
    sample_rate: f32,

    // Hilbert allpass networks for Left and Right channels (I and Q branches)
    hilbert_i_l: [Allpass1st; 4],
    hilbert_q_l: [Allpass1st; 4],
    hilbert_i_r: [Allpass1st; 4],
    hilbert_q_r: [Allpass1st; 4],

    // Oscillator quadrature phases
    osc_phase_l: f32,
    osc_phase_r: f32,

    // Feedback states & damping
    fb_state_l: f32,
    fb_state_r: f32,
    damp_state_l: f32,
    damp_state_r: f32,

    env_shifter: f32,

    // Parameters
    shift_hz: f32,            // -1500.0 to 1500.0 Hz
    stereo_spread_hz: f32,    // -500.0 to 500.0 Hz
    feedback: f32,            // -0.95 to 0.95
    damping_hz: f32,          // 500.0 to 12000.0 Hz
    sideband_balance: f32,    // -1.0 (Lower only) to +1.0 (Upper only), 0 = both
    mix: f32,                 // 0.0 to 1.0

    // Smoothed
    smoothed_fb: f32,
    smoothed_mix: f32,
}

impl RustDualBodeFrequencyShifter {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            hilbert_i_l: [Allpass1st::default(); 4],
            hilbert_q_l: [Allpass1st::default(); 4],
            hilbert_i_r: [Allpass1st::default(); 4],
            hilbert_q_r: [Allpass1st::default(); 4],
            osc_phase_l: 0.0,
            osc_phase_r: 0.0,
            fb_state_l: 0.0,
            fb_state_r: 0.0,
            damp_state_l: 0.0,
            damp_state_r: 0.0,
            env_shifter: 0.0,
            shift_hz: 120.0,
            stereo_spread_hz: 25.0,
            feedback: 0.4,
            damping_hz: 5000.0,
            sideband_balance: 0.0,
            mix: 0.75,
            smoothed_fb: 0.4,
            smoothed_mix: 0.75,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn reset(&mut self) {
        for ap in &mut self.hilbert_i_l { ap.reset(); }
        for ap in &mut self.hilbert_q_l { ap.reset(); }
        for ap in &mut self.hilbert_i_r { ap.reset(); }
        for ap in &mut self.hilbert_q_r { ap.reset(); }
        self.osc_phase_l = 0.0;
        self.osc_phase_r = 0.0;
        self.fb_state_l = 0.0;
        self.fb_state_r = 0.0;
        self.damp_state_l = 0.0;
        self.damp_state_r = 0.0;
        self.env_shifter = 0.0;
        self.smoothed_fb = self.feedback;
        self.smoothed_mix = self.mix;
    }

    pub fn set_param(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.shift_hz = clamp(value, -2000.0, 2000.0),
            2 => self.stereo_spread_hz = clamp(value, -500.0, 500.0),
            3 => self.feedback = clamp(value, -0.95, 0.95),
            4 => self.damping_hz = clamp(value, 400.0, 14000.0),
            5 => self.sideband_balance = clamp(value, -1.0, 1.0),
            6 => self.mix = clamp(value, 0.0, 1.0),
            _ => {}
        }
    }

    #[inline(always)]
    fn process_hilbert_pair(
        x: f32,
        branch_i: &mut [Allpass1st; 4],
        branch_q: &mut [Allpass1st; 4],
    ) -> (f32, f32) {
        // Standard wideband 90-degree phase shift coefficients
        const COEFFS_I: [f32; 4] = [0.161758, 0.733029, 0.945350, 0.990598];
        const COEFFS_Q: [f32; 4] = [0.479401, 0.872496, 0.977251, 0.997499];

        let mut yi = x;
        for k in 0..4 {
            yi = branch_i[k].process(yi, COEFFS_I[k]);
        }

        let mut yq = x;
        for k in 0..4 {
            yq = branch_q[k].process(yq, COEFFS_Q[k]);
        }

        (yi, yq)
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

        let shift_l = self.shift_hz - 0.5 * self.stereo_spread_hz;
        let shift_r = self.shift_hz + 0.5 * self.stereo_spread_hz;

        let phase_inc_l = 2.0 * PI * shift_l / self.sample_rate;
        let phase_inc_r = 2.0 * PI * shift_r / self.sample_rate;

        let damp_alpha = (-2.0 * PI * self.damping_hz / self.sample_rate).exp();
        let smooth = (-1.0 / (0.005 * self.sample_rate)).exp();

        // Upper vs lower sideband weights
        let up_weight = (self.sideband_balance + 1.0) * 0.5;   // 0 to 1
        let down_weight = (1.0 - self.sideband_balance) * 0.5; // 1 to 0

        for i in 0..num_samples as usize {
            let xl = unsafe { *in_l.add(i) };
            let xr = unsafe { *in_r.add(i) };

            self.smoothed_fb = lerp(self.feedback, self.smoothed_fb, smooth);
            self.smoothed_mix = lerp(self.mix, self.smoothed_mix, smooth);

            let in_mag = xl.abs() + xr.abs();
            self.env_shifter = lerp(in_mag + self.fb_state_l.abs() + self.fb_state_r.abs(), self.env_shifter, 0.999);

            // Silence gating (Rule 38)
            if self.env_shifter < 1.0e-5 && in_mag < 1.0e-5 {
                unsafe {
                    *out_l.add(i) = 0.0;
                    *out_r.add(i) = 0.0;
                }
                continue;
            }

            // Cross-feedback input sum
            let input_l = xl + self.fb_state_r * self.smoothed_fb;
            let input_r = xr + self.fb_state_l * self.smoothed_fb;

            // Generate quadrature Hilbert signals
            let (il, ql) = Self::process_hilbert_pair(input_l, &mut self.hilbert_i_l, &mut self.hilbert_q_l);
            let (ir, qr) = Self::process_hilbert_pair(input_r, &mut self.hilbert_i_r, &mut self.hilbert_q_r);

            // Quadrature carrier oscillator
            let cos_l = fast_sin(self.osc_phase_l + PI * 0.5);
            let sin_l = fast_sin(self.osc_phase_l);
            let cos_r = fast_sin(self.osc_phase_r + PI * 0.5);
            let sin_r = fast_sin(self.osc_phase_r);

            self.osc_phase_l = (self.osc_phase_l + phase_inc_l) % (2.0 * PI);
            self.osc_phase_r = (self.osc_phase_r + phase_inc_r) % (2.0 * PI);

            // Single-sideband computation
            let up_l = il * cos_l - ql * sin_l;
            let down_l = il * cos_l + ql * sin_l;
            let shifted_l = up_l * up_weight + down_l * down_weight;

            let up_r = ir * cos_r - qr * sin_r;
            let down_r = ir * cos_r + qr * sin_r;
            let shifted_r = up_r * up_weight + down_r * down_weight;

            // Damping lowpass filter on feedback
            self.damp_state_l = lerp(shifted_l, self.damp_state_l, damp_alpha);
            self.damp_state_r = lerp(shifted_r, self.damp_state_r, damp_alpha);

            // Non-linear saturation in feedback loop to prevent runaway
            self.fb_state_l = fast_tanh(self.damp_state_l);
            self.fb_state_r = fast_tanh(self.damp_state_r);

            let out_sample_l = denormal_guard(lerp(xl, shifted_l, self.smoothed_mix));
            let out_sample_r = denormal_guard(lerp(xr, shifted_r, self.smoothed_mix));

            unsafe {
                *out_l.add(i) = out_sample_l;
                *out_r.add(i) = out_sample_r;
            }
        }
    }
}
