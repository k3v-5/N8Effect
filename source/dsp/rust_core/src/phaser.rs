//! 6-Stage Analogue-Style Allpass Phaser DSP Kernel
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio thread,
//! Quadrature LFO sweeping, cascaded 1st-order allpass filters, soft-clipping feedback,
//! and denormal safety.

use std::f32::consts::PI;
use crate::common::{denormal_guard, fast_tanh};

pub struct RustPhaser {
    sample_rate: f32,
    ap_history_x: [[f32; 6]; 2],
    ap_history_y: [[f32; 6]; 2],
    feedback_sample: [f32; 2],
    lfo_phase: f32,

    target_rate: f32,
    target_depth: f32,
    target_feedback: f32,
    target_base_freq: f32,
    target_mix: f32,
}

impl RustPhaser {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            ap_history_x: [[0.0; 6]; 2],
            ap_history_y: [[0.0; 6]; 2],
            feedback_sample: [0.0; 2],
            lfo_phase: 0.0,
            target_rate: 0.5,
            target_depth: 0.75,
            target_feedback: 0.6,
            target_base_freq: 800.0,
            target_mix: 0.5,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn reset(&mut self) {
        self.ap_history_x = [[0.0; 6]; 2];
        self.ap_history_y = [[0.0; 6]; 2];
        self.feedback_sample = [0.0; 2];
        self.lfo_phase = 0.0;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_rate = value.clamp(0.05, 8.0),
            2 => self.target_depth = value.clamp(0.0, 1.0),
            3 => self.target_feedback = value.clamp(0.0, 0.95),
            4 => self.target_base_freq = value.clamp(100.0, 3000.0),
            5 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_rate,
            2 => self.target_depth,
            3 => self.target_feedback,
            4 => self.target_base_freq,
            5 => self.target_mix,
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
        let sr = self.sample_rate;
        let rate = self.target_rate;
        let depth = self.target_depth;
        let feedback = self.target_feedback;
        let base_freq = self.target_base_freq;
        let mix = self.target_mix;
        let dry_gain = 1.0 - mix;

        let lfo_inc = (2.0 * PI * rate) / sr;
        let two_pi = 2.0 * PI;
        let half_pi = PI * 0.5;

        for s in 0..num_samples {
            // LFO estéreo en cuadratura (90 grados de desfase entre canales)
            let lfo_l = 0.5 + 0.5 * self.lfo_phase.sin();
            let lfo_r = 0.5 + 0.5 * (self.lfo_phase + half_pi).sin();
            self.lfo_phase += lfo_inc;
            if self.lfo_phase >= two_pi {
                self.lfo_phase -= two_pi;
            }

            let s_l = denormal_guard(in_l[s]);
            let s_r = denormal_guard(if s < in_r.len() { in_r[s] } else { s_l });

            // Canal L
            let swept_f_l = base_freq * 4.0f32.powf(lfo_l * depth);
            let clamped_f_l = swept_f_l.clamp(50.0, sr * 0.45);
            let w0_l = (PI * clamped_f_l / sr).tan();
            let a_l = (w0_l - 1.0) / (w0_l + 1.0);

            let mut sig_l = s_l + self.feedback_sample[0] * feedback;
            sig_l = fast_tanh(sig_l);

            for stage in 0..6 {
                let x = sig_l;
                let y = a_l * x + self.ap_history_x[0][stage] - a_l * self.ap_history_y[0][stage];
                self.ap_history_x[0][stage] = x;
                self.ap_history_y[0][stage] = y;
                sig_l = y;
            }
            self.feedback_sample[0] = sig_l;
            out_l[s] = denormal_guard(dry_gain * s_l + mix * sig_l);

            // Canal R
            if s < out_r.len() {
                let swept_f_r = base_freq * 4.0f32.powf(lfo_r * depth);
                let clamped_f_r = swept_f_r.clamp(50.0, sr * 0.45);
                let w0_r = (PI * clamped_f_r / sr).tan();
                let a_r = (w0_r - 1.0) / (w0_r + 1.0);

                let mut sig_r = s_r + self.feedback_sample[1] * feedback;
                sig_r = fast_tanh(sig_r);

                for stage in 0..6 {
                    let x = sig_r;
                    let y = a_r * x + self.ap_history_x[1][stage] - a_r * self.ap_history_y[1][stage];
                    self.ap_history_x[1][stage] = x;
                    self.ap_history_y[1][stage] = y;
                    sig_r = y;
                }
                self.feedback_sample[1] = sig_r;
                out_r[s] = denormal_guard(dry_gain * s_r + mix * sig_r);
            }
        }
    }
}
