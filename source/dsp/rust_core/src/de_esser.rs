//! De-Esser (Dynamic Split-Band & Wideband Vocal De-Esser) DSP Kernel
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio thread,
//! precision bandpass sibilance extraction, fast attack envelope follower,
//! anti-click gain reduction smoothing, and dedicated Listen monitoring mode.

use std::f32::consts::PI;
use crate::common::denormal_guard;

#[inline(always)]
fn db_to_linear(db: f32) -> f32 {
    10.0f32.powf(db * 0.05)
}

#[inline(always)]
fn linear_to_db(lin: f32) -> f32 {
    if lin > 1e-5 {
        20.0 * lin.log10()
    } else {
        -100.0
    }
}

#[derive(Clone, Copy, Default)]
struct BiquadBP {
    b0: f32,
    b2: f32,
    a1: f32,
    a2: f32,
    z1: f32,
    z2: f32,
}

impl BiquadBP {
    #[inline(always)]
    fn reset(&mut self) {
        self.z1 = 0.0;
        self.z2 = 0.0;
    }

    #[inline(always)]
    fn process_sample(&mut self, in_val: f32) -> f32 {
        let out = self.b0 * in_val + self.z1;
        self.z1 = -self.a1 * out + self.z2;
        self.z2 = self.b2 * in_val - self.a2 * out;
        out
    }

    fn set_bandpass(&mut self, sample_rate: f32, freq_hz: f32, q: f32) {
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
        self.b0 = alpha * inv_a0;
        self.b2 = -alpha * inv_a0;
        self.a1 = (-2.0 * cs) * inv_a0;
        self.a2 = (1.0 - alpha) * inv_a0;
    }
}

#[derive(Clone, Copy, Default)]
struct EnvelopeDetector {
    sample_rate: f32,
    attack_ms: f32,
    release_ms: f32,
    attack_coeff: f32,
    release_coeff: f32,
    envelope: f32,
}

impl EnvelopeDetector {
    fn new(sample_rate: f32, attack_ms: f32, release_ms: f32) -> Self {
        let mut ed = Self {
            sample_rate: if sample_rate > 0.0 { sample_rate } else { 44100.0 },
            attack_ms: attack_ms.max(0.05),
            release_ms: release_ms.max(0.5),
            attack_coeff: 0.0,
            release_coeff: 0.0,
            envelope: 0.0,
        };
        ed.update_coefficients();
        ed
    }

    fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.envelope = 0.0;
        self.update_coefficients();
    }

    fn reset(&mut self) {
        self.envelope = 0.0;
    }

    fn update_coefficients(&mut self) {
        let att_samples = (self.attack_ms * 0.001) * self.sample_rate;
        self.attack_coeff = 1.0 - (-1.0 / att_samples.max(1.0)).exp();
        let rel_samples = (self.release_ms * 0.001) * self.sample_rate;
        self.release_coeff = 1.0 - (-1.0 / rel_samples.max(1.0)).exp();
    }

    #[inline(always)]
    fn process_sample(&mut self, in_val: f32) -> f32 {
        let input_level = in_val.abs();
        if input_level > self.envelope {
            self.envelope += self.attack_coeff * (input_level - self.envelope);
        } else {
            self.envelope += self.release_coeff * (input_level - self.envelope);
        }
        if self.envelope.is_nan() || self.envelope.is_infinite() {
            self.envelope = 0.0;
        }
        self.envelope
    }
}

pub struct RustDeEsser {
    sample_rate: f32,
    bandpass_filters: [BiquadBP; 2],
    detector: EnvelopeDetector,
    current_gain_reduction: f32,

    target_frequency: f32,
    target_bandwidth: f32,
    target_threshold: f32,
    target_reduction: f32,
    target_mode: f32,
    target_listen: f32,
}

impl RustDeEsser {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut deesser = Self {
            sample_rate: sr,
            bandpass_filters: [BiquadBP::default(); 2],
            detector: EnvelopeDetector::new(sr, 1.0, 35.0),
            current_gain_reduction: 1.0,
            target_frequency: 6500.0,
            target_bandwidth: 1.5,
            target_threshold: -20.0,
            target_reduction: 12.0,
            target_mode: 0.0,
            target_listen: 0.0,
        };
        deesser.update_filters();
        deesser
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
        self.detector.prepare(self.sample_rate);
        self.update_filters();
    }

    pub fn reset(&mut self) {
        self.detector.reset();
        self.current_gain_reduction = 1.0;
        self.bandpass_filters[0].reset();
        self.bandpass_filters[1].reset();
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => {
                self.target_frequency = value.clamp(2000.0, 12000.0);
                self.update_filters();
            }
            2 => {
                self.target_bandwidth = value.clamp(0.5, 5.0);
                self.update_filters();
            }
            3 => self.target_threshold = value.clamp(-50.0, 0.0),
            4 => self.target_reduction = value.clamp(0.0, 30.0),
            5 => self.target_mode = if value >= 0.5 { 1.0 } else { 0.0 },
            6 => self.target_listen = if value >= 0.5 { 1.0 } else { 0.0 },
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_frequency,
            2 => self.target_bandwidth,
            3 => self.target_threshold,
            4 => self.target_reduction,
            5 => self.target_mode,
            6 => self.target_listen,
            _ => 0.0,
        }
    }

    fn update_filters(&mut self) {
        let nyquist = self.sample_rate * 0.49;
        let f = self.target_frequency.clamp(2000.0, nyquist);
        let q = self.target_bandwidth.clamp(0.5, 5.0);
        self.bandpass_filters[0].set_bandpass(self.sample_rate, f, q);
        self.bandpass_filters[1].set_bandpass(self.sample_rate, f, q);
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
        let threshold = self.target_threshold;
        let max_reduction_lin = db_to_linear(-self.target_reduction);
        let is_wideband = self.target_mode >= 0.5;
        let is_listen = self.target_listen >= 0.5;

        for s in 0..num_samples {
            let raw_in_l = denormal_guard(in_l[s]);
            let raw_in_r = denormal_guard(if s < in_r.len() { in_r[s] } else { raw_in_l });

            // 1. Filtrar banda sibilante por canal estéreo
            let sibilant_l = self.bandpass_filters[0].process_sample(raw_in_l);
            let sibilant_r = self.bandpass_filters[1].process_sample(raw_in_r);

            // Modo Listen: audición exclusiva de la banda sibilante
            if is_listen {
                out_l[s] = denormal_guard(sibilant_l * 2.0);
                if s < out_r.len() {
                    out_r[s] = denormal_guard(sibilant_r * 2.0);
                }
                continue;
            }

            // 2. Detección de energía en la banda de sibilancias
            let max_sibilant = sibilant_l.abs().max(sibilant_r.abs());
            let env = self.detector.process_sample(max_sibilant);
            let env_db = linear_to_db(env);

            // 3. Cálculo de Gain Reduction
            let mut target_gr_linear = 1.0f32;
            if env_db > threshold {
                let excess_db = env_db - threshold;
                let gr_db = -(excess_db * 1.5).min(self.target_reduction);
                target_gr_linear = max_reduction_lin.max(db_to_linear(gr_db));
            }

            // Suavizado anti-click del Gain Reduction
            self.current_gain_reduction += 0.08 * (target_gr_linear - self.current_gain_reduction);

            // 4. Aplicación de reducción según modo
            if is_wideband {
                out_l[s] = denormal_guard(raw_in_l * self.current_gain_reduction);
                if s < out_r.len() {
                    out_r[s] = denormal_guard(raw_in_r * self.current_gain_reduction);
                }
            } else {
                let non_sibilant_l = raw_in_l - sibilant_l;
                let non_sibilant_r = raw_in_r - sibilant_r;

                out_l[s] = denormal_guard(non_sibilant_l + sibilant_l * self.current_gain_reduction);
                if s < out_r.len() {
                    out_r[s] = denormal_guard(non_sibilant_r + sibilant_r * self.current_gain_reduction);
                }
            }
        }
    }
}
