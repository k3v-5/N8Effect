//! Audio Envelope Follower for Dynamic Modulation (Rules 7, 46, 47)
//! Zero allocations in audio thread, ballistic peak tracking, denormal safe.

use crate::common::denormal_guard;

pub struct RustAudioFollower {
    sample_rate: f64,
    attack_ms: f32,
    release_ms: f32,
    attack_coeff: f32,
    release_coeff: f32,
    envelope: f32,
}

impl RustAudioFollower {
    pub fn new() -> Self {
        let mut fol = Self {
            sample_rate: 44100.0,
            attack_ms: 10.0,
            release_ms: 100.0,
            attack_coeff: 0.01,
            release_coeff: 0.001,
            envelope: 0.0,
        };
        fol.update_coefficients();
        fol
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.envelope = 0.0;
        self.update_coefficients();
    }

    pub fn reset(&mut self) {
        self.envelope = 0.0;
    }

    #[inline]
    pub fn set_attack_ms(&mut self, ms: f32) {
        self.attack_ms = ms.max(0.1);
        self.update_coefficients();
    }

    #[inline]
    pub fn set_release_ms(&mut self, ms: f32) {
        self.release_ms = ms.max(1.0);
        self.update_coefficients();
    }

    #[inline]
    pub fn get_current_value(&self) -> f32 {
        self.envelope
    }

    #[inline]
    pub fn process_block(&mut self, input_channels: *const *const f32, num_channels: u32, num_samples: u32) -> f32 {
        if input_channels.is_null() || num_channels == 0 || num_samples == 0 {
            return self.envelope;
        }

        let mut block_peak = 0.0f32;
        let n = num_samples as usize;

        unsafe {
            for ch in 0..(num_channels as usize) {
                let channel_ptr = *input_channels.add(ch);
                if !channel_ptr.is_null() {
                    let slice = std::slice::from_raw_parts(channel_ptr, n);
                    for &s in slice {
                        let mag = denormal_guard(s).abs();
                        if mag > block_peak {
                            block_peak = mag;
                        }
                    }
                }
            }
        }

        // Ballistic tracking filter
        if block_peak > self.envelope {
            self.envelope += self.attack_coeff * (block_peak - self.envelope);
        } else {
            self.envelope += self.release_coeff * (block_peak - self.envelope);
        }

        self.envelope = denormal_guard(self.envelope).clamp(0.0, 1.0);
        self.envelope
    }

    fn update_coefficients(&mut self) {
        let sr = self.sample_rate as f32;
        let attack_samples = (self.attack_ms * 0.001) * sr;
        let release_samples = (self.release_ms * 0.001) * sr;

        self.attack_coeff = 1.0 - (-1.0 / attack_samples.max(1.0)).exp();
        self.release_coeff = 1.0 - (-1.0 / release_samples.max(1.0)).exp();
    }
}
