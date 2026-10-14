//! High-Performance Topology-Preserving Transform (TPT) State Variable Filter (SVF)
//! (DSP #4 / NodeType::Filter)
//! Rules 5, 8, 9, 14, 34, 38, 46, 47:
//! Zero heap allocations in audio processing thread,
//! Exact 2-pole TPT analog modeling with bilinear transform frequency warping,
//! Lowpass, Highpass, and Bandpass modes,
//! Sample-accurate audio-rate FM modulation input (Pin 4),
//! Anti-denormal and NaN/Inf guards.

use std::f32::consts::PI;
use crate::common::denormal_guard;

pub struct RustSimpleFilter {
    sample_rate: f64,
    cutoff_hz: f32,
    resonance: f32,
    mode: i32, // 0: LP, 1: HP, 2: BP

    // Estados de integradores TPT para estéreo
    s1_l: f32,
    s2_l: f32,
    s1_r: f32,
    s2_r: f32,

    // Coeficientes precalculados para modo estático (sin FM)
    base_g: f32,
    base_h: f32,
    base_r: f32,
    dirty_coeffs: bool,
}

impl RustSimpleFilter {
    pub fn new() -> Self {
        let mut filter = Self {
            sample_rate: 44100.0,
            cutoff_hz: 1000.0,
            resonance: 0.707,
            mode: 0,
            s1_l: 0.0,
            s2_l: 0.0,
            s1_r: 0.0,
            s2_r: 0.0,
            base_g: 0.0,
            base_h: 1.0,
            base_r: 1.0 / (2.0 * 0.707),
            dirty_coeffs: true,
        };
        filter.update_coefficients();
        filter
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
        self.dirty_coeffs = true;
        self.update_coefficients();
    }

    pub fn reset(&mut self) {
        self.s1_l = 0.0;
        self.s2_l = 0.0;
        self.s1_r = 0.0;
        self.s2_r = 0.0;
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => {
                // CutoffHz: 20.0 .. 20000.0
                let new_val = value.clamp(20.0, 20000.0);
                if (self.cutoff_hz - new_val).abs() > 0.01 {
                    self.cutoff_hz = new_val;
                    self.dirty_coeffs = true;
                }
            }
            2 => {
                // Resonance: 0.1 .. 10.0
                let new_val = value.clamp(0.1, 10.0);
                if (self.resonance - new_val).abs() > 0.001 {
                    self.resonance = new_val;
                    self.dirty_coeffs = true;
                }
            }
            3 => {
                // Mode: 0: LP, 1: HP, 2: BP
                self.mode = value.round().clamp(0.0, 2.0) as i32;
            }
            _ => {}
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            1 => self.cutoff_hz,
            2 => self.resonance,
            3 => self.mode as f32,
            _ => 0.0,
        }
    }

    #[inline]
    fn update_coefficients(&mut self) {
        if !self.dirty_coeffs {
            return;
        }
        let sr = if self.sample_rate > 0.0 { self.sample_rate as f32 } else { 44100.0 };
        let max_nyquist = (sr * 0.48).max(100.0);
        let cutoff = self.cutoff_hz.clamp(20.0, max_nyquist);

        let wd = 2.0 * PI * cutoff;
        let t = 1.0 / sr;
        let wa = (2.0 / t) * (wd * t * 0.5).tan();
        self.base_g = wa * t * 0.5;
        self.base_r = 1.0 / (2.0 * self.resonance.max(0.05));
        self.base_h = 1.0 + 2.0 * self.base_r * self.base_g + self.base_g * self.base_g;
        self.dirty_coeffs = false;
    }

    #[inline]
    pub fn process(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        fm_mod: Option<&[f32]>,
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len();
        if num_samples == 0 {
            return;
        }

        self.update_coefficients();

        let sr = if self.sample_rate > 0.0 { self.sample_rate as f32 } else { 44100.0 };
        let t = 1.0 / sr;
        let max_nyquist = (sr * 0.48).max(100.0);
        let mode = self.mode;

        if let Some(fm) = fm_mod {
            // Ruta modulada muestra a muestra
            for s in 0..num_samples {
                let fm_val = fm[s];
                let mod_cutoff = (self.cutoff_hz * 2.0f32.powf(fm_val * 2.5)).clamp(20.0, max_nyquist);
                let wd = 2.0 * PI * mod_cutoff;
                let wa = (2.0 / t) * (wd * t * 0.5).tan();
                let g = wa * t * 0.5;
                let r = 1.0 / (2.0 * self.resonance.max(0.05));
                let h = 1.0 + 2.0 * r * g + g * g;

                // Canal Izquierdo
                let inl = in_l[s];
                let hp_l = (inl - (2.0 * r + g) * self.s1_l - self.s2_l) / h;
                let bp_l = g * hp_l + self.s1_l;
                let lp_l = g * bp_l + self.s2_l;
                self.s1_l = 2.0 * bp_l - self.s1_l;
                self.s2_l = 2.0 * lp_l - self.s2_l;

                if self.s1_l.is_nan() || self.s1_l.is_infinite() { self.s1_l = 0.0; }
                if self.s2_l.is_nan() || self.s2_l.is_infinite() { self.s2_l = 0.0; }
                self.s1_l = denormal_guard(self.s1_l);
                self.s2_l = denormal_guard(self.s2_l);

                // Canal Derecho
                let inr = in_r[s];
                let hp_r = (inr - (2.0 * r + g) * self.s1_r - self.s2_r) / h;
                let bp_r = g * hp_r + self.s1_r;
                let lp_r = g * bp_r + self.s2_r;
                self.s1_r = 2.0 * bp_r - self.s1_r;
                self.s2_r = 2.0 * lp_r - self.s2_r;

                if self.s1_r.is_nan() || self.s1_r.is_infinite() { self.s1_r = 0.0; }
                if self.s2_r.is_nan() || self.s2_r.is_infinite() { self.s2_r = 0.0; }
                self.s1_r = denormal_guard(self.s1_r);
                self.s2_r = denormal_guard(self.s2_r);

                out_l[s] = match mode {
                    1 => denormal_guard(hp_l),
                    2 => denormal_guard(bp_l),
                    _ => denormal_guard(lp_l),
                };
                out_r[s] = match mode {
                    1 => denormal_guard(hp_r),
                    2 => denormal_guard(bp_r),
                    _ => denormal_guard(lp_r),
                };
            }
        } else {
            // Ruta estática de alto rendimiento (coeficientes fijos por bloque)
            let g = self.base_g;
            let r = self.base_r;
            let h = self.base_h;

            for s in 0..num_samples {
                let inl = in_l[s];
                let hp_l = (inl - (2.0 * r + g) * self.s1_l - self.s2_l) / h;
                let bp_l = g * hp_l + self.s1_l;
                let lp_l = g * bp_l + self.s2_l;
                self.s1_l = 2.0 * bp_l - self.s1_l;
                self.s2_l = 2.0 * lp_l - self.s2_l;

                if self.s1_l.is_nan() || self.s1_l.is_infinite() { self.s1_l = 0.0; }
                if self.s2_l.is_nan() || self.s2_l.is_infinite() { self.s2_l = 0.0; }
                self.s1_l = denormal_guard(self.s1_l);
                self.s2_l = denormal_guard(self.s2_l);

                let inr = in_r[s];
                let hp_r = (inr - (2.0 * r + g) * self.s1_r - self.s2_r) / h;
                let bp_r = g * hp_r + self.s1_r;
                let lp_r = g * bp_r + self.s2_r;
                self.s1_r = 2.0 * bp_r - self.s1_r;
                self.s2_r = 2.0 * lp_r - self.s2_r;

                if self.s1_r.is_nan() || self.s1_r.is_infinite() { self.s1_r = 0.0; }
                if self.s2_r.is_nan() || self.s2_r.is_infinite() { self.s2_r = 0.0; }
                self.s1_r = denormal_guard(self.s1_r);
                self.s2_r = denormal_guard(self.s2_r);

                out_l[s] = match mode {
                    1 => denormal_guard(hp_l),
                    2 => denormal_guard(bp_l),
                    _ => denormal_guard(lp_l),
                };
                out_r[s] = match mode {
                    1 => denormal_guard(hp_r),
                    2 => denormal_guard(bp_r),
                    _ => denormal_guard(lp_r),
                };
            }
        }
    }
}
