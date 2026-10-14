//! Auto-Wah / Dynamic Analogue-style Envelope Filter DSP Kernel
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio thread,
//! Cytomic/Andy Simper State Variable Filter (SVF), reactive dynamic envelope follower,
//! Lowpass, Bandpass, and Highpass topology modes with external sidechain capability.

use std::f32::consts::PI;
use crate::common::denormal_guard;

#[derive(Clone, Copy, Default)]
struct SvfState {
    ic1eq: f32,
    ic2eq: f32,
}

impl SvfState {
    #[inline(always)]
    fn reset(&mut self) {
        self.ic1eq = 0.0;
        self.ic2eq = 0.0;
    }
}

pub struct RustAutoWah {
    sample_rate: f32,
    svf_state: [SvfState; 2],
    env_level: f32,

    target_base_freq: f32,
    target_sens: f32,
    target_res: f32,
    target_attack_ms: f32,
    target_decay_ms: f32,
    target_mode: f32,
    target_mix: f32,
}

impl RustAutoWah {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 48000.0 };
        Self {
            sample_rate: sr,
            svf_state: [SvfState::default(); 2],
            env_level: 0.0,
            target_base_freq: 350.0,
            target_sens: 0.6,
            target_res: 4.5,
            target_attack_ms: 15.0,
            target_decay_ms: 180.0,
            target_mode: 1.0,
            target_mix: 1.0,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 48000.0 };
        self.reset();
    }

    pub fn reset(&mut self) {
        self.svf_state[0].reset();
        self.svf_state[1].reset();
        self.env_level = 0.0;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_base_freq = value.clamp(60.0, 3000.0),
            2 => self.target_sens = value.clamp(0.0, 1.0),
            3 => self.target_res = value.clamp(0.7, 15.0),
            4 => self.target_attack_ms = value.clamp(1.0, 100.0),
            5 => self.target_decay_ms = value.clamp(10.0, 800.0),
            6 => self.target_mode = value.clamp(0.0, 2.0),
            7 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_base_freq,
            2 => self.target_sens,
            3 => self.target_res,
            4 => self.target_attack_ms,
            5 => self.target_decay_ms,
            6 => self.target_mode,
            7 => self.target_mix,
            _ => 0.0,
        }
    }

    #[inline]
    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        sc_l: Option<&[f32]>,
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len().min(out_l.len());
        let sr = self.sample_rate;
        let base_f = self.target_base_freq.clamp(60.0, 3000.0);
        let sens = self.target_sens.clamp(0.0, 1.0);
        let q = self.target_res.clamp(0.7, 15.0);
        let mode = self.target_mode.round() as i32;
        let mix = self.target_mix.clamp(0.0, 1.0);
        let dry_gain = 1.0 - mix;

        let att_coeff = (-1.0 / (self.target_attack_ms.max(1.0) * 0.001 * sr)).exp();
        let rel_coeff = (-1.0 / (self.target_decay_ms.max(10.0) * 0.001 * sr)).exp();
        let k = 1.0 / q;

        for s in 0..num_samples {
            let s_l = denormal_guard(in_l[s]);
            let s_r = denormal_guard(if s < in_r.len() { in_r[s] } else { s_l });

            // 1. Detección de envolvente
            let detector_in = match sc_l {
                Some(sc) if s < sc.len() => denormal_guard(sc[s]).abs(),
                _ => (s_l.abs() + s_r.abs()) * 0.5,
            };

            if detector_in > self.env_level {
                self.env_level = detector_in + att_coeff * (self.env_level - detector_in);
            } else {
                self.env_level = detector_in + rel_coeff * (self.env_level - detector_in);
            }

            // 2. Modulación de frecuencia de corte
            let octave_shift = sens * (self.env_level * 4.0).clamp(0.0, 3.8);
            let cutoff = (base_f * 2.0f32.powf(octave_shift)).clamp(50.0, sr * 0.45);

            // 3. Filtro SVF (Cytomic / Andy Simper)
            let g = (PI * cutoff / sr).tan();
            let a1 = 1.0 / (1.0 + g * (g + k));
            let a2 = g * a1;
            let a3 = g * a2;

            // Canal L
            let v3_l = s_l - self.svf_state[0].ic2eq;
            let v1_l = a1 * self.svf_state[0].ic1eq + a2 * v3_l;
            let v2_l = self.svf_state[0].ic2eq + a2 * self.svf_state[0].ic1eq + a3 * v3_l;
            self.svf_state[0].ic1eq = 2.0 * v1_l - self.svf_state[0].ic1eq;
            self.svf_state[0].ic2eq = 2.0 * v2_l - self.svf_state[0].ic2eq;

            let proc_l = match mode {
                0 => v2_l,
                1 => v1_l,
                _ => s_l - k * v1_l - v2_l,
            };

            // Canal R
            let v3_r = s_r - self.svf_state[1].ic2eq;
            let v1_r = a1 * self.svf_state[1].ic1eq + a2 * v3_r;
            let v2_r = self.svf_state[1].ic2eq + a2 * self.svf_state[1].ic1eq + a3 * v3_r;
            self.svf_state[1].ic1eq = 2.0 * v1_r - self.svf_state[1].ic1eq;
            self.svf_state[1].ic2eq = 2.0 * v2_r - self.svf_state[1].ic2eq;

            let proc_r = match mode {
                0 => v2_r,
                1 => v1_r,
                _ => s_r - k * v1_r - v2_r,
            };

            out_l[s] = denormal_guard(s_l * dry_gain + proc_l * mix);
            if s < out_r.len() {
                out_r[s] = denormal_guard(s_r * dry_gain + proc_r * mix);
            }
        }
    }
}
