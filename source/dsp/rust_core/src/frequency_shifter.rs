//! High-Performance Frequency Shifter DSP Kernel using 90-Degree Hilbert Allpass Network & Weaver SSB
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio thread,
//! 4-stage cascaded allpass I/Q networks, bipolar shift, fast saturation, and denormal protection.

use std::f32::consts::PI;
use crate::common::{denormal_guard, fast_tanh};

const COEFS_A: [f32; 4] = [0.161758, 0.733029, 0.945350, 0.990598];
const COEFS_B: [f32; 4] = [0.479401, 0.876243, 0.976599, 0.997500];

pub struct RustFrequencyShifter {
    sample_rate: f32,
    ap_history_a: [[f32; 4]; 2],
    ap_history_b: [[f32; 4]; 2],
    feedback_sample: [f32; 2],
    quad_phase: f32,

    target_shift_hz: f32,
    target_feedback: f32,
    target_mix: f32,
}

impl RustFrequencyShifter {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            ap_history_a: [[0.0; 4]; 2],
            ap_history_b: [[0.0; 4]; 2],
            feedback_sample: [0.0; 2],
            quad_phase: 0.0,
            target_shift_hz: 50.0,
            target_feedback: 0.0,
            target_mix: 1.0,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn reset(&mut self) {
        self.ap_history_a = [[0.0; 4]; 2];
        self.ap_history_b = [[0.0; 4]; 2];
        self.feedback_sample = [0.0; 2];
        self.quad_phase = 0.0;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_shift_hz = value.clamp(-1000.0, 1000.0),
            2 => self.target_feedback = value.clamp(0.0, 0.85),
            3 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_shift_hz,
            2 => self.target_feedback,
            3 => self.target_mix,
            _ => 0.0,
        }
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
        if num_samples == 0 {
            return;
        }

        let sr = self.sample_rate;
        let shift = self.target_shift_hz;
        let feedback = self.target_feedback;
        let mix = self.target_mix;
        let dry_gain = 1.0 - mix;

        let two_pi = 2.0 * PI;
        let phase_inc = (two_pi * shift) / sr;
        let abs_phase_inc = phase_inc.abs();

        for s in 0..num_samples {
            let s_l = denormal_guard(in_l[s]);
            let s_r = denormal_guard(if s < in_r.len() { in_r[s] } else { s_l });

            let cos_val = self.quad_phase.cos();
            let sin_val = self.quad_phase.sin();

            // Canal izquierdo
            let mut sig_l = s_l + self.feedback_sample[0] * feedback;
            sig_l = fast_tanh(sig_l);

            let mut sig_i_l = sig_l;
            for stage in 0..4 {
                let a = COEFS_A[stage];
                let y = a * sig_i_l + self.ap_history_a[0][stage];
                self.ap_history_a[0][stage] = sig_i_l - a * y;
                sig_i_l = y;
            }

            let mut sig_q_l = sig_l;
            for stage in 0..4 {
                let b = COEFS_B[stage];
                let y = b * sig_q_l + self.ap_history_b[0][stage];
                self.ap_history_b[0][stage] = sig_q_l - b * y;
                sig_q_l = y;
            }

            let shifted_l = if shift >= 0.0 {
                sig_i_l * cos_val - sig_q_l * sin_val
            } else {
                sig_i_l * cos_val + sig_q_l * sin_val
            };

            self.feedback_sample[0] = shifted_l;
            out_l[s] = denormal_guard(dry_gain * s_l + mix * shifted_l);

            // Canal derecho
            if s < out_r.len() {
                let mut sig_r = s_r + self.feedback_sample[1] * feedback;
                sig_r = fast_tanh(sig_r);

                let mut sig_i_r = sig_r;
                for stage in 0..4 {
                    let a = COEFS_A[stage];
                    let y = a * sig_i_r + self.ap_history_a[1][stage];
                    self.ap_history_a[1][stage] = sig_i_r - a * y;
                    sig_i_r = y;
                }

                let mut sig_q_r = sig_r;
                for stage in 0..4 {
                    let b = COEFS_B[stage];
                    let y = b * sig_q_r + self.ap_history_b[1][stage];
                    self.ap_history_b[1][stage] = sig_q_r - b * y;
                    sig_q_r = y;
                }

                let shifted_r = if shift >= 0.0 {
                    sig_i_r * cos_val - sig_q_r * sin_val
                } else {
                    sig_i_r * cos_val + sig_q_r * sin_val
                };

                self.feedback_sample[1] = shifted_r;
                out_r[s] = denormal_guard(dry_gain * s_r + mix * shifted_r);
            }

            self.quad_phase += abs_phase_inc;
            if self.quad_phase >= two_pi {
                self.quad_phase -= two_pi;
            }
        }
    }
}
