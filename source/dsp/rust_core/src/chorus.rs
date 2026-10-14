//! Multi-Voice BBD-Style Stereo Chorus DSP Kernel
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio thread,
//! Quadrature/multi-phase LFO modulated delay lines, cross-panning voices,
//! soft-clipping feedback, and denormal protection.

use std::f32::consts::PI;
use crate::common::{denormal_guard, fast_tanh};

const MAX_CHORUS_BUFFER: usize = 16384;

pub struct RustChorus {
    sample_rate: f32,
    max_delay_samples: usize,
    delay_buffers: [[f32; MAX_CHORUS_BUFFER]; 2],
    write_index: usize,
    feedback_sample: [f32; 2],
    lfo_phase: f32,

    target_rate: f32,
    target_depth: f32,
    target_feedback: f32,
    target_voices: f32,
    target_mix: f32,
}

impl RustChorus {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let max_samples = ((sr as f64 * 0.05) as usize).clamp(16, MAX_CHORUS_BUFFER);
        Self {
            sample_rate: sr,
            max_delay_samples: max_samples,
            delay_buffers: [[0.0; MAX_CHORUS_BUFFER]; 2],
            write_index: 0,
            feedback_sample: [0.0; 2],
            lfo_phase: 0.0,
            target_rate: 1.0,
            target_depth: 0.5,
            target_feedback: 0.2,
            target_voices: 4.0,
            target_mix: 0.5,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.max_delay_samples = ((self.sample_rate as f64 * 0.05) as usize).clamp(16, MAX_CHORUS_BUFFER);
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
            1 => self.target_rate = value.clamp(0.1, 5.0),
            2 => self.target_depth = value.clamp(0.0, 1.0),
            3 => self.target_feedback = value.clamp(0.0, 0.7),
            4 => self.target_voices = value.clamp(1.0, 4.0),
            5 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_rate,
            2 => self.target_depth,
            3 => self.target_feedback,
            4 => self.target_voices,
            5 => self.target_mix,
            _ => 0.0,
        }
    }

    #[inline(always)]
    fn read_interpolated(&self, ch: usize, delay_samples: f32) -> f32 {
        let max_d = self.max_delay_samples as f32;
        let mut read_pos = (self.write_index as f32) - delay_samples;
        read_pos %= max_d;
        if read_pos < 0.0 {
            read_pos += max_d;
        }

        let idx0 = (read_pos as usize) % self.max_delay_samples;
        let idx1 = (idx0 + 1) % self.max_delay_samples;
        let frac = read_pos - (read_pos as usize as f32);

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
        let sr = self.sample_rate;
        let rate = self.target_rate;
        let depth = self.target_depth;
        let feedback = self.target_feedback;
        let num_voices = (self.target_voices.round() as i32).clamp(1, 4);
        let mix = self.target_mix;
        let dry_gain = 1.0 - mix;

        let lfo_inc = (2.0 * PI * rate) / sr;
        let base_delay_ms = 15.0;
        let mod_depth_ms = depth * 8.0;
        let two_pi = 2.0 * PI;
        let half_pi = PI * 0.5;
        let voice_norm = 1.0 / (num_voices as f32).sqrt();

        for s in 0..num_samples {
            let s_l = denormal_guard(in_l[s]);
            let s_r = denormal_guard(if s < in_r.len() { in_r[s] } else { s_l });

            // Escribir en buffers de retardo con feedback
            self.delay_buffers[0][self.write_index] = s_l + self.feedback_sample[0] * feedback;
            self.delay_buffers[1][self.write_index] = s_r + self.feedback_sample[1] * feedback;

            let mut chorus_accum_l = 0.0f32;
            let mut chorus_accum_r = 0.0f32;

            // Renderizar voces moduladas con diferentes fases (0, 90, 180, 270 grados)
            for v in 0..num_voices {
                let voice_phase = self.lfo_phase + (v as f32) * half_pi;
                let lfo_mod = voice_phase.sin();
                let delay_ms = base_delay_ms + mod_depth_ms * lfo_mod;
                let delay_samples = delay_ms * 0.001 * sr;

                let sample_l = self.read_interpolated(0, delay_samples);
                let sample_r = self.read_interpolated(1, delay_samples);

                let pan_l = if v % 2 == 0 { 0.8 } else { 0.2 };
                let pan_r = 1.0 - pan_l;

                chorus_accum_l += sample_l * pan_l;
                chorus_accum_r += sample_r * pan_r;
            }

            chorus_accum_l *= voice_norm;
            chorus_accum_r *= voice_norm;

            self.feedback_sample[0] = fast_tanh(chorus_accum_l);
            self.feedback_sample[1] = fast_tanh(chorus_accum_r);

            out_l[s] = denormal_guard(dry_gain * s_l + mix * chorus_accum_l);
            if s < out_r.len() {
                out_r[s] = denormal_guard(dry_gain * s_r + mix * chorus_accum_r);
            }

            self.write_index = (self.write_index + 1) % self.max_delay_samples;
            self.lfo_phase += lfo_inc;
            if self.lfo_phase >= two_pi {
                self.lfo_phase -= two_pi;
            }
        }
    }
}
