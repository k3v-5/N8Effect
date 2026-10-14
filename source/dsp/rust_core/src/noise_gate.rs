//! Noise Gate & Downward Expander DSP Kernel
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio thread,
//! anti-chatter hysteresis threshold, sample-accurate hold timer,
//! smooth attack/release ballistic smoothing, range floor, and sidechain HPF detector.

use std::f32::consts::PI;
use crate::common::denormal_guard;

const Q_BUTTERWORTH: f32 = 0.70710678;

#[inline(always)]
fn db_to_linear(db: f32) -> f32 {
    10.0f32.powf(db * 0.05)
}

#[derive(Clone, Copy, Default)]
struct BiquadHP {
    b0: f32,
    b1: f32,
    b2: f32,
    a1: f32,
    a2: f32,
    z1: f32,
    z2: f32,
}

impl BiquadHP {
    #[inline(always)]
    fn reset(&mut self) {
        self.z1 = 0.0;
        self.z2 = 0.0;
    }

    #[inline(always)]
    fn process_sample(&mut self, in_val: f32) -> f32 {
        let out = self.b0 * in_val + self.z1;
        self.z1 = self.b1 * in_val - self.a1 * out + self.z2;
        self.z2 = self.b2 * in_val - self.a2 * out;
        out
    }

    fn set_highpass(&mut self, sample_rate: f32, freq_hz: f32, q: f32) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let nyquist = sr * 0.499;
        let fc = freq_hz.clamp(10.0, nyquist);
        let q_val = q.max(0.01);
        let omega = 2.0 * PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q_val);

        let a0 = 1.0 + alpha;
        let inv_a0 = 1.0 / a0;
        self.b0 = ((1.0 + cs) * 0.5) * inv_a0;
        self.b1 = (-(1.0 + cs)) * inv_a0;
        self.b2 = ((1.0 + cs) * 0.5) * inv_a0;
        self.a1 = (-2.0 * cs) * inv_a0;
        self.a2 = (1.0 - alpha) * inv_a0;
    }
}

pub struct RustNoiseGate {
    sample_rate: f32,
    current_gain: f32,
    attack_coeff: f32,
    release_coeff: f32,
    hold_samples: u32,
    hold_counter: u32,
    gate_open: bool,

    sidechain_filter_l: BiquadHP,
    sidechain_filter_r: BiquadHP,

    threshold_db: f32,
    hysteresis_db: f32,
    attack_ms: f32,
    hold_ms: f32,
    release_ms: f32,
    range_db: f32,
    sidechain_hpf: f32,
}

impl RustNoiseGate {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut gate = Self {
            sample_rate: sr,
            current_gain: 0.0,
            attack_coeff: 0.05,
            release_coeff: 0.005,
            hold_samples: 0,
            hold_counter: 0,
            gate_open: false,
            sidechain_filter_l: BiquadHP::default(),
            sidechain_filter_r: BiquadHP::default(),
            threshold_db: -40.0,
            hysteresis_db: 4.0,
            attack_ms: 1.0,
            hold_ms: 25.0,
            release_ms: 80.0,
            range_db: -80.0,
            sidechain_hpf: 80.0,
        };
        gate.update_filters();
        gate.update_coefficients();
        gate
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
        self.update_filters();
        self.update_coefficients();
    }

    pub fn reset(&mut self) {
        self.current_gain = 0.0;
        self.hold_counter = 0;
        self.gate_open = false;
        self.sidechain_filter_l.reset();
        self.sidechain_filter_r.reset();
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.threshold_db = value.clamp(-80.0, 0.0),
            2 => self.hysteresis_db = value.clamp(0.0, 12.0),
            3 => {
                self.attack_ms = value.clamp(0.05, 50.0);
                self.update_coefficients();
            }
            4 => {
                self.hold_ms = value.clamp(0.0, 500.0);
                self.update_coefficients();
            }
            5 => {
                self.release_ms = value.clamp(5.0, 1000.0);
                self.update_coefficients();
            }
            6 => self.range_db = value.clamp(-80.0, 0.0),
            7 => {
                self.sidechain_hpf = value.clamp(20.0, 1000.0);
                self.update_filters();
            }
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.threshold_db,
            2 => self.hysteresis_db,
            3 => self.attack_ms,
            4 => self.hold_ms,
            5 => self.release_ms,
            6 => self.range_db,
            7 => self.sidechain_hpf,
            _ => 0.0,
        }
    }

    fn update_filters(&mut self) {
        self.sidechain_filter_l.set_highpass(self.sample_rate, self.sidechain_hpf, Q_BUTTERWORTH);
        self.sidechain_filter_r.set_highpass(self.sample_rate, self.sidechain_hpf, Q_BUTTERWORTH);
    }

    fn update_coefficients(&mut self) {
        let att_samples = (self.attack_ms * 0.001) * self.sample_rate;
        let rel_samples = (self.release_ms * 0.001) * self.sample_rate;

        self.attack_coeff = 1.0 - (-1.0 / att_samples.max(1.0)).exp();
        self.release_coeff = 1.0 - (-1.0 / rel_samples.max(1.0)).exp();
        self.hold_samples = (self.hold_ms * 0.001 * self.sample_rate) as u32;
    }

    #[inline]
    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        sc_l: Option<&[f32]>,
        sc_r: Option<&[f32]>,
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len().min(out_l.len());
        let open_threshold_lin = db_to_linear(self.threshold_db);
        let close_threshold_lin = db_to_linear(self.threshold_db - self.hysteresis_db);
        let range_floor_lin = db_to_linear(self.range_db);

        for s in 0..num_samples {
            let raw_in_l = denormal_guard(in_l[s]);
            let raw_in_r = denormal_guard(if s < in_r.len() { in_r[s] } else { raw_in_l });

            let raw_sc_l = match sc_l {
                Some(slice) if s < slice.len() => denormal_guard(slice[s]),
                _ => raw_in_l,
            };
            let raw_sc_r = match sc_r {
                Some(slice) if s < slice.len() => denormal_guard(slice[s]),
                _ => raw_in_r,
            };

            // 1. Detección Sidechain filtrada por HPF
            let filtered_l = self.sidechain_filter_l.process_sample(raw_sc_l);
            let filtered_r = self.sidechain_filter_r.process_sample(raw_sc_r);
            let level = filtered_l.abs().max(filtered_r.abs());

            // 2. Máquina de estados de la compuerta con Histéresis y Hold
            if level >= open_threshold_lin {
                self.gate_open = true;
                self.hold_counter = self.hold_samples;
            } else if level < close_threshold_lin {
                if self.hold_counter > 0 {
                    self.hold_counter -= 1;
                } else {
                    self.gate_open = false;
                }
            }

            // 3. Suavizado balístico
            let target_gain = if self.gate_open { 1.0 } else { range_floor_lin };
            if target_gain > self.current_gain {
                self.current_gain += self.attack_coeff * (target_gain - self.current_gain);
            } else {
                self.current_gain += self.release_coeff * (target_gain - self.current_gain);
            }

            out_l[s] = denormal_guard(raw_in_l * self.current_gain);
            if s < out_r.len() {
                out_r[s] = denormal_guard(raw_in_r * self.current_gain);
            }
        }
    }
}
