//! Stereo Imager & Multiband M/S Processor DSP Kernel
//! Rules 5, 8, 9, 14, 16, 34, 46, 47: Zero allocations in audio thread,
//! Mid/Side stereo width processing, Haas micro-delay, mono-bass elliptical filter,
//! phase correlation telemetry, and denormal safety.

use std::f32::consts::{PI, SQRT_2};
use crate::common::denormal_guard;

const MAX_HAAS_BUFFER_SIZE: usize = 8192;
const INV_SQRT2: f32 = 0.70710678118;

pub struct RustStereoImager {
    sample_rate: f32,
    max_delay_samples: usize,
    haas_buffer: [f32; MAX_HAAS_BUFFER_SIZE],
    haas_write_pos: usize,
    side_hp_state: [f32; 2],
    smoothed_correlation: f32,

    target_width: f32,
    target_haas_ms: f32,
    target_mono_bass: f32,
    target_balance: f32,
    target_mix: f32,
}

impl RustStereoImager {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 48000.0 };
        let max_samples = ((sr as f64 * 0.035) as usize).clamp(16, MAX_HAAS_BUFFER_SIZE);
        Self {
            sample_rate: sr,
            max_delay_samples: max_samples,
            haas_buffer: [0.0; MAX_HAAS_BUFFER_SIZE],
            haas_write_pos: 0,
            side_hp_state: [0.0; 2],
            smoothed_correlation: 1.0,
            target_width: 1.0,
            target_haas_ms: 0.0,
            target_mono_bass: 120.0,
            target_balance: 0.0,
            target_mix: 1.0,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 48000.0 };
        self.max_delay_samples = ((self.sample_rate as f64 * 0.035) as usize).clamp(16, MAX_HAAS_BUFFER_SIZE);
        self.reset();
    }

    pub fn reset(&mut self) {
        self.haas_buffer.fill(0.0);
        self.haas_write_pos = 0;
        self.side_hp_state = [0.0; 2];
        self.smoothed_correlation = 1.0;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_width = value.clamp(0.0, 2.0),
            2 => self.target_haas_ms = value.clamp(0.0, 20.0),
            3 => self.target_mono_bass = value.clamp(20.0, 350.0),
            4 => self.target_balance = value.clamp(-1.0, 1.0),
            5 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_width,
            2 => self.target_haas_ms,
            3 => self.target_mono_bass,
            4 => self.target_balance,
            5 => self.target_mix,
            _ => 0.0,
        }
    }

    pub fn get_phase_correlation(&self) -> f32 {
        self.smoothed_correlation
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
        let width = self.target_width.clamp(0.0, 2.0);
        let max_del = self.max_delay_samples;
        let haas_delay_samples = (self.target_haas_ms * 0.001 * sr).clamp(0.0, (max_del as f32 - 2.0).max(0.0));
        let mono_bass_cutoff = self.target_mono_bass.clamp(20.0, 350.0);
        let pan = self.target_balance.clamp(-1.0, 1.0);
        let mix = self.target_mix.clamp(0.0, 1.0);
        let dry_gain = 1.0 - mix;

        let w_c = 2.0 * PI * mono_bass_cutoff / sr;
        let alpha = 1.0 / (1.0 + w_c);

        let pan_angle = (pan + 1.0) * 0.25 * PI;
        let pan_gain_l = pan_angle.cos() * SQRT_2;
        let pan_gain_r = pan_angle.sin() * SQRT_2;

        let mut sum_dot = 0.0f32;
        let mut sum_l2 = 0.0f32;
        let mut sum_r2 = 0.0f32;

        for s in 0..num_samples {
            let s_l = denormal_guard(in_l[s]);
            let s_r = denormal_guard(if s < in_r.len() { in_r[s] } else { s_l });

            // 1. Matriz M/S
            let mid = (s_l + s_r) * INV_SQRT2;
            let side_raw = (s_l - s_r) * INV_SQRT2;

            // 2. Filtro Elíptico Mono-Bass en Side (elimina estéreo en sub-graves)
            let side_hp = alpha * (self.side_hp_state[1] + side_raw - self.side_hp_state[0]);
            self.side_hp_state[0] = side_raw;
            self.side_hp_state[1] = side_hp;
            let side = side_hp * width;

            // 3. Decodificación M/S
            let mut proc_l = (mid + side) * INV_SQRT2;
            let mut proc_r = (mid - side) * INV_SQRT2;

            // 4. Retardo Haas en el canal R
            if haas_delay_samples > 0.5 && max_del > 2 {
                self.haas_buffer[self.haas_write_pos] = proc_r;
                let mut read_pos = (self.haas_write_pos as f32) - haas_delay_samples;
                if read_pos < 0.0 {
                    read_pos += max_del as f32;
                }

                let idx0 = (read_pos as usize) % max_del;
                let idx1 = (idx0 + 1) % max_del;
                let frac = read_pos - (read_pos as usize as f32);
                proc_r = (1.0 - frac) * self.haas_buffer[idx0] + frac * self.haas_buffer[idx1];

                self.haas_write_pos = (self.haas_write_pos + 1) % max_del;
            }

            // 5. Balance estéreo
            proc_l *= pan_gain_l;
            proc_r *= pan_gain_r;

            // Telemetría de correlación
            sum_dot += proc_l * proc_r;
            sum_l2 += proc_l * proc_l;
            sum_r2 += proc_r * proc_r;

            out_l[s] = denormal_guard(s_l * dry_gain + proc_l * mix);
            if s < out_r.len() {
                out_r[s] = denormal_guard(s_r * dry_gain + proc_r * mix);
            }
        }

        // Actualizar correlación de fase
        if sum_l2 > 1e-6 && sum_r2 > 1e-6 {
            let corr = sum_dot / (sum_l2 * sum_r2).sqrt();
            self.smoothed_correlation += 0.1 * (corr.clamp(-1.0, 1.0) - self.smoothed_correlation);
        }
    }
}
