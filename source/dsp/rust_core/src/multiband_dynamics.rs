//! Multiband Dynamics (3-Band OTT-style Upward & Downward Dynamics) DSP Kernel
//! Rules 5, 8, 9, 14, 32, 33, 34, 46, 47: Zero allocations in audio thread,
//! 4th-order Linkwitz-Riley (LR4) flat-magnitude crossover, decoupled ballistic envelope detectors,
//! aggressive upward and downward compression.

use std::f32::consts::PI;
use crate::common::denormal_guard;

const Q_BUTTERWORTH: f32 = 0.70710678;

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
struct BiquadDF2T {
    b0: f32,
    b1: f32,
    b2: f32,
    a1: f32,
    a2: f32,
    z1: f32,
    z2: f32,
}

impl BiquadDF2T {
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

    fn set_lowpass(&mut self, sample_rate: f32, freq_hz: f32, q: f32) {
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
        self.b0 = ((1.0 - cs) * 0.5) * inv_a0;
        self.b1 = (1.0 - cs) * inv_a0;
        self.b2 = ((1.0 - cs) * 0.5) * inv_a0;
        self.a1 = (-2.0 * cs) * inv_a0;
        self.a2 = (1.0 - alpha) * inv_a0;
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

    fn set_allpass(&mut self, sample_rate: f32, freq_hz: f32, q: f32) {
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
        self.b0 = (1.0 - alpha) * inv_a0;
        self.b1 = (-2.0 * cs) * inv_a0;
        self.b2 = (1.0 + alpha) * inv_a0;
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

    fn set_attack_ms(&mut self, ms: f32) {
        self.attack_ms = ms.max(0.05);
        self.update_coefficients();
    }

    fn set_release_ms(&mut self, ms: f32) {
        self.release_ms = ms.max(0.5);
        self.update_coefficients();
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

pub struct RustMultibandDynamics {
    sample_rate: f32,

    // Crossover LR4 (2 channels)
    lp_low1: [BiquadDF2T; 2],
    lp_low2: [BiquadDF2T; 2],
    hp_rest1: [BiquadDF2T; 2],
    hp_rest2: [BiquadDF2T; 2],
    ap_low: [BiquadDF2T; 2],
    lp_mid1: [BiquadDF2T; 2],
    lp_mid2: [BiquadDF2T; 2],
    hp_high1: [BiquadDF2T; 2],
    hp_high2: [BiquadDF2T; 2],

    // Envelope detectors (2 channels x 3 bands)
    env_low: [EnvelopeDetector; 2],
    env_mid: [EnvelopeDetector; 2],
    env_high: [EnvelopeDetector; 2],

    // Parameters
    f_low_mid: f32,
    f_mid_high: f32,
    low_dynamics: f32,
    mid_dynamics: f32,
    high_dynamics: f32,
    dry_wet: f32,
}

impl RustMultibandDynamics {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut mb = Self {
            sample_rate: sr,
            lp_low1: [BiquadDF2T::default(); 2],
            lp_low2: [BiquadDF2T::default(); 2],
            hp_rest1: [BiquadDF2T::default(); 2],
            hp_rest2: [BiquadDF2T::default(); 2],
            ap_low: [BiquadDF2T::default(); 2],
            lp_mid1: [BiquadDF2T::default(); 2],
            lp_mid2: [BiquadDF2T::default(); 2],
            hp_high1: [BiquadDF2T::default(); 2],
            hp_high2: [BiquadDF2T::default(); 2],

            env_low: [
                EnvelopeDetector::new(sr, 20.0, 150.0),
                EnvelopeDetector::new(sr, 20.0, 150.0),
            ],
            env_mid: [
                EnvelopeDetector::new(sr, 10.0, 100.0),
                EnvelopeDetector::new(sr, 10.0, 100.0),
            ],
            env_high: [
                EnvelopeDetector::new(sr, 5.0, 60.0),
                EnvelopeDetector::new(sr, 5.0, 60.0),
            ],

            f_low_mid: 250.0,
            f_mid_high: 2500.0,
            low_dynamics: 1.0,
            mid_dynamics: 1.0,
            high_dynamics: 1.0,
            dry_wet: 1.0,
        };
        mb.update_crossovers();
        mb
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
        for ch in 0..2 {
            self.env_low[ch].prepare(self.sample_rate);
            self.env_low[ch].set_attack_ms(20.0);
            self.env_low[ch].set_release_ms(150.0);

            self.env_mid[ch].prepare(self.sample_rate);
            self.env_mid[ch].set_attack_ms(10.0);
            self.env_mid[ch].set_release_ms(100.0);

            self.env_high[ch].prepare(self.sample_rate);
            self.env_high[ch].set_attack_ms(5.0);
            self.env_high[ch].set_release_ms(60.0);
        }
        self.update_crossovers();
    }

    pub fn reset(&mut self) {
        for ch in 0..2 {
            self.lp_low1[ch].reset();
            self.lp_low2[ch].reset();
            self.hp_rest1[ch].reset();
            self.hp_rest2[ch].reset();
            self.ap_low[ch].reset();
            self.lp_mid1[ch].reset();
            self.lp_mid2[ch].reset();
            self.hp_high1[ch].reset();
            self.hp_high2[ch].reset();

            self.env_low[ch].reset();
            self.env_mid[ch].reset();
            self.env_high[ch].reset();
        }
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => {
                self.f_low_mid = value.clamp(60.0, 1000.0);
                self.update_crossovers();
            }
            2 => {
                self.f_mid_high = value.clamp(800.0, 12000.0);
                self.update_crossovers();
            }
            3 => self.low_dynamics = value.clamp(0.0, 2.0),
            4 => self.mid_dynamics = value.clamp(0.0, 2.0),
            5 => self.high_dynamics = value.clamp(0.0, 2.0),
            6 => self.dry_wet = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.f_low_mid,
            2 => self.f_mid_high,
            3 => self.low_dynamics,
            4 => self.mid_dynamics,
            5 => self.high_dynamics,
            6 => self.dry_wet,
            _ => 0.0,
        }
    }

    fn update_crossovers(&mut self) {
        let nyquist = self.sample_rate * 0.49;
        let clamped_low = self.f_low_mid.clamp(20.0, nyquist * 0.5);
        let clamped_high = self.f_mid_high.clamp(clamped_low * 1.2, nyquist);

        for ch in 0..2 {
            self.lp_low1[ch].set_lowpass(self.sample_rate, clamped_low, Q_BUTTERWORTH);
            self.lp_low2[ch].set_lowpass(self.sample_rate, clamped_low, Q_BUTTERWORTH);

            self.hp_rest1[ch].set_highpass(self.sample_rate, clamped_low, Q_BUTTERWORTH);
            self.hp_rest2[ch].set_highpass(self.sample_rate, clamped_low, Q_BUTTERWORTH);

            self.ap_low[ch].set_allpass(self.sample_rate, clamped_high, Q_BUTTERWORTH);

            self.lp_mid1[ch].set_lowpass(self.sample_rate, clamped_high, Q_BUTTERWORTH);
            self.lp_mid2[ch].set_lowpass(self.sample_rate, clamped_high, Q_BUTTERWORTH);

            self.hp_high1[ch].set_highpass(self.sample_rate, clamped_high, Q_BUTTERWORTH);
            self.hp_high2[ch].set_highpass(self.sample_rate, clamped_high, Q_BUTTERWORTH);
        }
    }

    #[inline(always)]
    fn process_band_dynamics(
        sample: f32,
        detector: &mut EnvelopeDetector,
        dynamics_amount: f32,
        down_thresh_db: f32,
        up_thresh_db: f32,
    ) -> f32 {
        let env_linear = detector.process_sample(sample);
        let env_db = linear_to_db(env_linear);

        let mut gain_db = 0.0f32;

        if env_db > down_thresh_db {
            let over_db = env_db - down_thresh_db;
            let ratio = 1.0 + 3.0 * dynamics_amount;
            let compressed_over = over_db / ratio;
            gain_db -= over_db - compressed_over;
        } else if env_db < up_thresh_db && env_db > -70.0 {
            let under_db = up_thresh_db - env_db;
            let boost_factor = (under_db * 0.4 * dynamics_amount).clamp(0.0, 15.0);
            gain_db += boost_factor;
        }

        let linear_gain = db_to_linear(gain_db);
        sample * linear_gain
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
        let low_dyn = self.low_dynamics.clamp(0.0, 2.0);
        let mid_dyn = self.mid_dynamics.clamp(0.0, 2.0);
        let high_dyn = self.high_dynamics.clamp(0.0, 2.0);
        let dry_wet = self.dry_wet.clamp(0.0, 1.0);
        let dry_gain = 1.0 - dry_wet;

        for s in 0..num_samples {
            let dry_l = denormal_guard(in_l[s]);
            let dry_r = denormal_guard(if s < in_r.len() { in_r[s] } else { dry_l });

            // Channel L Crossover
            let low1_l = self.lp_low1[0].process_sample(dry_l);
            let low_raw_l = self.lp_low2[0].process_sample(low1_l);
            let rest1_l = self.hp_rest1[0].process_sample(dry_l);
            let rest_l = self.hp_rest2[0].process_sample(rest1_l);
            let low_aligned_l = self.ap_low[0].process_sample(low_raw_l);
            let mid1_l = self.lp_mid1[0].process_sample(rest_l);
            let mid_l = self.lp_mid2[0].process_sample(mid1_l);
            let high1_l = self.hp_high1[0].process_sample(rest_l);
            let high_l = self.hp_high2[0].process_sample(high1_l);

            // Channel R Crossover
            let low1_r = self.lp_low1[1].process_sample(dry_r);
            let low_raw_r = self.lp_low2[1].process_sample(low1_r);
            let rest1_r = self.hp_rest1[1].process_sample(dry_r);
            let rest_r = self.hp_rest2[1].process_sample(rest1_r);
            let low_aligned_r = self.ap_low[1].process_sample(low_raw_r);
            let mid1_r = self.lp_mid1[1].process_sample(rest_r);
            let mid_r = self.lp_mid2[1].process_sample(mid1_r);
            let high1_r = self.hp_high1[1].process_sample(rest_r);
            let high_r = self.hp_high2[1].process_sample(high1_r);

            // Dynamics processing
            let proc_low_l = Self::process_band_dynamics(low_aligned_l, &mut self.env_low[0], low_dyn, -20.0, -40.0);
            let proc_low_r = Self::process_band_dynamics(low_aligned_r, &mut self.env_low[1], low_dyn, -20.0, -40.0);

            let proc_mid_l = Self::process_band_dynamics(mid_l, &mut self.env_mid[0], mid_dyn, -18.0, -36.0);
            let proc_mid_r = Self::process_band_dynamics(mid_r, &mut self.env_mid[1], mid_dyn, -18.0, -36.0);

            let proc_high_l = Self::process_band_dynamics(high_l, &mut self.env_high[0], high_dyn, -16.0, -34.0);
            let proc_high_r = Self::process_band_dynamics(high_r, &mut self.env_high[1], high_dyn, -16.0, -34.0);

            let wet_l = proc_low_l + proc_mid_l + proc_high_l;
            let wet_r = proc_low_r + proc_mid_r + proc_high_r;

            out_l[s] = denormal_guard(dry_l * dry_gain + wet_l * dry_wet);
            if s < out_r.len() {
                out_r[s] = denormal_guard(dry_r * dry_gain + wet_r * dry_wet);
            }
        }
    }
}
