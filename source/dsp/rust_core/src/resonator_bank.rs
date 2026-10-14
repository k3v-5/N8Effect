//! Resonator Bank (6 Modal & Harmonic Parallel Resonators with Tails) DSP Kernel
//! Rules 5, 8, 9, 12, 14, 18, 32, 46, 47: Zero allocations in audio thread,
//! parallel high-Q bandpass biquads, harmonic-to-modal dispersion morphing,
//! spectral tilt brightening, denormal guards and tail energy support.

use std::f32::consts::PI;
use crate::common::denormal_guard;

const NUM_RESONATORS: usize = 6;
const MODAL_MULTIPLIERS: [f32; NUM_RESONATORS] = [1.0, 2.756, 5.404, 8.933, 13.345, 18.64];
const NORM_GAIN: f32 = 0.45;

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

pub struct RustResonatorBank {
    sample_rate: f32,
    filters_l: [BiquadBP; NUM_RESONATORS],
    filters_r: [BiquadBP; NUM_RESONATORS],
    gains: [f32; NUM_RESONATORS],

    fundamental_freq: f32,
    decay_time: f32,
    harmonic_spread: f32,
    brightness: f32,
    dry_wet: f32,
}

impl RustResonatorBank {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut bank = Self {
            sample_rate: sr,
            filters_l: [BiquadBP::default(); NUM_RESONATORS],
            filters_r: [BiquadBP::default(); NUM_RESONATORS],
            gains: [1.0; NUM_RESONATORS],
            fundamental_freq: 220.0,
            decay_time: 1.5,
            harmonic_spread: 0.0,
            brightness: 0.0,
            dry_wet: 0.5,
        };
        bank.update_resonators();
        bank
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
        self.update_resonators();
    }

    pub fn reset(&mut self) {
        for i in 0..NUM_RESONATORS {
            self.filters_l[i].reset();
            self.filters_r[i].reset();
        }
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => {
                self.fundamental_freq = value.clamp(20.0, 2000.0);
                self.update_resonators();
            }
            2 => {
                self.decay_time = value.clamp(0.05, 10.0);
                self.update_resonators();
            }
            3 => {
                self.harmonic_spread = value.clamp(0.0, 1.0);
                self.update_resonators();
            }
            4 => {
                self.brightness = value.clamp(-1.0, 1.0);
                self.update_resonators();
            }
            5 => self.dry_wet = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.fundamental_freq,
            2 => self.decay_time,
            3 => self.harmonic_spread,
            4 => self.brightness,
            5 => self.dry_wet,
            _ => 0.0,
        }
    }

    pub fn get_tail_samples(&self) -> u32 {
        (self.sample_rate * self.decay_time) as u32
    }

    fn update_resonators(&mut self) {
        let f0 = self.fundamental_freq.clamp(20.0, 2000.0);
        let decay = self.decay_time.clamp(0.05, 10.0);
        let spread = self.harmonic_spread.clamp(0.0, 1.0);
        let bright = self.brightness.clamp(-1.0, 1.0);

        let nyquist = self.sample_rate * 0.49;

        for i in 0..NUM_RESONATORS {
            let harmonic_mul = (i + 1) as f32;
            let modal_mul = MODAL_MULTIPLIERS[i];
            let mul = harmonic_mul * (1.0 - spread) + modal_mul * spread;

            let mut freq = f0 * mul;
            if freq > nyquist {
                freq = nyquist;
            }

            let q = (PI * freq * decay * 0.1).clamp(2.0, 180.0);

            self.filters_l[i].set_bandpass(self.sample_rate, freq, q);
            self.filters_r[i].set_bandpass(self.sample_rate, freq * 1.002, q);

            let harmonic_ratio = (i as f32) / ((NUM_RESONATORS - 1) as f32);
            let tilt_db = bright * harmonic_ratio * 18.0;
            self.gains[i] = 10.0f32.powf(tilt_db * 0.05);
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
        let dry_wet = self.dry_wet.clamp(0.0, 1.0);
        let dry_gain = 1.0 - dry_wet;

        for s in 0..num_samples {
            let dry_l = denormal_guard(in_l[s]);
            let dry_r = denormal_guard(if s < in_r.len() { in_r[s] } else { dry_l });

            let mut res_sum_l = 0.0f32;
            let mut res_sum_r = 0.0f32;

            for i in 0..NUM_RESONATORS {
                let gain = self.gains[i];
                res_sum_l += self.filters_l[i].process_sample(dry_l) * gain;
                res_sum_r += self.filters_r[i].process_sample(dry_r) * gain;
            }

            let wet_l = res_sum_l * NORM_GAIN;
            let wet_r = res_sum_r * NORM_GAIN;

            out_l[s] = denormal_guard(dry_l * dry_gain + wet_l * dry_wet);
            if s < out_r.len() {
                out_r[s] = denormal_guard(dry_r * dry_gain + wet_r * dry_wet);
            }
        }
    }
}
