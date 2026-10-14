//! TransientDetectorCore - Real-Time Transient & Onset Detection Engine (Analysis Core)
//! Implemented for the N8Effect Hybrid C++ / Rust Audio Engine Architecture (Rules 1, 7, 9, 34, 46, 47).
//! Uses 1st-order pre-emphasis highpass differentiation, dual decoupled fast/slow envelope followers,
//! adaptive energy ratio, and a refractory gate to eliminate false triggers.
//! Zero heap allocations in audio processing, AVX2 SIMD friendly, denormal safe.

use crate::common::denormal_guard;

pub struct TransientDetectorCore {
    sample_rate: f32,
    fast_alpha: f32,
    slow_alpha: f32,
    prev_sample: f32,
    fast_env: f32,
    slow_env: f32,
    sensitivity: f32,
    threshold_ratio: f32,
    refractory_samples_limit: u32,
    refractory_counter: u32,
    onset_strength: f32,
    is_transient: bool,
}

impl Default for TransientDetectorCore {
    fn default() -> Self {
        Self::new(44100.0)
    }
}

impl TransientDetectorCore {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut detector = Self {
            sample_rate: sr,
            fast_alpha: 0.05,
            slow_alpha: 0.005,
            prev_sample: 0.0,
            fast_env: 0.0,
            slow_env: 0.0,
            sensitivity: 0.7,
            threshold_ratio: 2.5,
            refractory_samples_limit: 660,
            refractory_counter: 0,
            onset_strength: 0.0,
            is_transient: false,
        };
        detector.prepare(sr);
        detector
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.fast_alpha = 1.0 - (-1.0 / (0.002 * self.sample_rate)).exp();
        self.slow_alpha = 1.0 - (-1.0 / (0.040 * self.sample_rate)).exp();
        self.refractory_samples_limit = (self.sample_rate * 0.015) as u32; // 15 ms refractory
        self.threshold_ratio = 1.5 + (1.0 - self.sensitivity) * 3.5;
        self.reset();
    }

    pub fn reset(&mut self) {
        self.prev_sample = 0.0;
        self.fast_env = 0.0;
        self.slow_env = 0.0;
        self.refractory_counter = 0;
        self.onset_strength = 0.0;
        self.is_transient = false;
    }

    pub fn set_sensitivity(&mut self, sensitivity: f32) {
        self.sensitivity = sensitivity.clamp(0.0, 1.0);
        self.threshold_ratio = 1.5 + (1.0 - self.sensitivity) * 3.5;
    }

    #[inline(always)]
    pub fn get_sensitivity(&self) -> f32 {
        self.sensitivity
    }

    #[inline(always)]
    pub fn is_transient_detected(&self) -> bool {
        self.is_transient
    }

    #[inline(always)]
    pub fn get_onset_strength(&self) -> f32 {
        self.onset_strength
    }

    pub fn process(&mut self, input: Option<&[f32]>, num_samples: usize) {
        if input.is_none() || num_samples == 0 {
            self.is_transient = false;
            self.onset_strength *= 0.95;
            return;
        }

        let slice = input.unwrap();
        self.is_transient = false;
        let decay_factor = 1.0 - (1.0 / (0.050 * self.sample_rate));

        for s in 0..num_samples {
            let in_val = denormal_guard(slice[s]);

            // 1. Highpass differentiator pre-emphasis
            let hp = in_val - 0.95 * self.prev_sample;
            self.prev_sample = in_val;

            let energy = hp * hp;

            // 2. Fast and slow envelope followers
            self.fast_env += self.fast_alpha * (energy - self.fast_env);
            self.slow_env += self.slow_alpha * (energy - self.slow_env);

            // 3. Transient energy ratio
            let ratio = self.fast_env / (self.slow_env + 1e-6);

            if self.refractory_counter > 0 {
                self.refractory_counter -= 1;
            } else if ratio > self.threshold_ratio && energy > 1e-4 {
                self.is_transient = true;
                self.refractory_counter = self.refractory_samples_limit;
                self.onset_strength = ((ratio - 1.0) / self.threshold_ratio).clamp(0.2, 1.0);
            }

            self.onset_strength = (self.onset_strength * decay_factor).max(0.0);
        }
    }
}
