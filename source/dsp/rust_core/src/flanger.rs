//! High-Performance Flanger DSP Kernel with Bipolar Feedback & Deep Comb Filtering
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio thread,
//! Quadrature LFO modulated delay lines, bipolar feedback saturation with cubic soft-clipping,
//! and denormal protection.

use std::f32::consts::PI;
use crate::common::{denormal_guard, fast_soft_clip};

const MAX_FLANGER_BUFFER: usize = 8192;

pub struct RustFlanger {
    sample_rate: f32,
    max_delay_samples: usize,
    delay_buffers: [[f32; MAX_FLANGER_BUFFER]; 2],
    write_index: usize,
    feedback_sample: [f32; 2],
    lfo_phase: f32,

    target_rate: f32,
    target_depth: f32,
    target_delay_ms: f32,
    target_feedback: f32,
    target_mix: f32,
}

impl RustFlanger {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let max_samples = ((sr as f64 * 0.02) as usize).clamp(16, MAX_FLANGER_BUFFER);
        Self {
            sample_rate: sr,
            max_delay_samples: max_samples,
            delay_buffers: [[0.0; MAX_FLANGER_BUFFER]; 2],
            write_index: 0,
            feedback_sample: [0.0; 2],
            lfo_phase: 0.0,
            target_rate: 0.25,
            target_depth: 0.8,
            target_delay_ms: 2.0,
            target_feedback: 0.7,
            target_mix: 0.5,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.max_delay_samples = ((self.sample_rate as f64 * 0.02) as usize).clamp(16, MAX_FLANGER_BUFFER);
        self.reset();
    }

    pub fn reset(&mut self) {
        self.delay_buffers[0].fill(0.0);
        self.delay_buffers[1].fill(0.0);
        self.feedback_sample = [0.0; 2];
        self.write_index = 0;
        self.lfo_phase = 0.0;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_rate = value.clamp(0.05, 5.0),
            2 => self.target_depth = value.clamp(0.0, 1.0),
            3 => self.target_delay_ms = value.clamp(0.2, 10.0),
            4 => self.target_feedback = value.clamp(-0.95, 0.95),
            5 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_rate,
            2 => self.target_depth,
            3 => self.target_delay_ms,
            4 => self.target_feedback,
            5 => self.target_mix,
            _ => 0.0,
        }
    }

    #[inline(always)]
    fn read_interpolated(&self, ch: usize, delay_samples: f32) -> f32 {
        let max_d = self.max_delay_samples as f32;
        let read_pos = (self.write_index as f32) - delay_samples;
        let mut wrapped_pos = read_pos % max_d;
        if wrapped_pos < 0.0 {
            wrapped_pos += max_d;
        }

        let idx0 = (wrapped_pos as usize) % self.max_delay_samples;
        let idx1 = (idx0 + 1) % self.max_delay_samples;
        let frac = wrapped_pos - (idx0 as f32);

        (1.0 - frac) * self.delay_buffers[ch][idx0] + frac * self.delay_buffers[ch][idx1]
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
        if num_samples == 0 || self.max_delay_samples == 0 {
            return;
        }

        let sr = self.sample_rate;
        let rate = self.target_rate;
        let depth = self.target_depth;
        let base_delay_ms = self.target_delay_ms;
        let feedback = self.target_feedback;
        let mix = self.target_mix;
        let dry_gain = 1.0 - mix;

        let lfo_inc = (2.0 * PI * rate) / sr;
        let max_mod_ms = base_delay_ms * depth * 0.85;
        let half_pi = PI * 0.5;
        let two_pi = 2.0 * PI;

        for s in 0..num_samples {
            let s_l = denormal_guard(in_l[s]);
            let s_r = denormal_guard(if s < in_r.len() { in_r[s] } else { s_l });

            // LFO triangular/senoidal suave
            let lfo_l = 0.5 + 0.5 * self.lfo_phase.sin();
            let lfo_r = 0.5 + 0.5 * (self.lfo_phase + half_pi).sin();

            // Canal izquierdo
            let total_delay_ms_l = (base_delay_ms + max_mod_ms * (2.0 * lfo_l - 1.0)).max(0.1);
            let delay_samples_l = total_delay_ms_l * 0.001 * sr;
            let write_val_l = s_l + self.feedback_sample[0] * feedback;
            self.delay_buffers[0][self.write_index] = fast_soft_clip(write_val_l);
            let wet_l = self.read_interpolated(0, delay_samples_l);
            self.feedback_sample[0] = wet_l;
            out_l[s] = denormal_guard(dry_gain * s_l + mix * wet_l);

            // Canal derecho
            let total_delay_ms_r = (base_delay_ms + max_mod_ms * (2.0 * lfo_r - 1.0)).max(0.1);
            let delay_samples_r = total_delay_ms_r * 0.001 * sr;
            let write_val_r = s_r + self.feedback_sample[1] * feedback;
            self.delay_buffers[1][self.write_index] = fast_soft_clip(write_val_r);
            let wet_r = self.read_interpolated(1, delay_samples_r);
            self.feedback_sample[1] = wet_r;
            if s < out_r.len() {
                out_r[s] = denormal_guard(dry_gain * s_r + mix * wet_r);
            }

            self.write_index = (self.write_index + 1) % self.max_delay_samples;
            self.lfo_phase += lfo_inc;
            if self.lfo_phase >= two_pi {
                self.lfo_phase -= two_pi;
            }
        }
    }
}
