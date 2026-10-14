//! Adaptive Noise Floor Estimator & Schmitt Trigger Hysteresis
//! Implements continuous statistical tracking of environmental background noise,
//! eliminates chattering and spurious cutoffs in natural audio tails/decays,
//! and provides real-time zero-allocation source silence detection.
//! Follows Rules 1, 3, 9, 27, 46, 47 (Zero allocations, bounded physics, real-time safety).

pub const MIN_NOISE_FLOOR: f32 = 1.58489e-5; // -96 dBFS
pub const MAX_NOISE_FLOOR: f32 = 0.01;       // -40 dBFS
pub const DEFAULT_FLOOR: f32  = 1.0e-4;      // -80 dBFS

#[repr(C)]
pub struct RustAdaptiveNoiseFloorEstimator {
    sample_rate: f64,
    attack_coeff: f32,
    decay_coeff: f32,
    current_floor: f32,
    smoothed_level: f32,
    source_active: bool,
    silent_block_counter: u32,
    debounce_blocks: u32,
}

impl RustAdaptiveNoiseFloorEstimator {
    pub fn new() -> Self {
        let mut est = Self {
            sample_rate: 44100.0,
            attack_coeff: 0.00001,
            decay_coeff: 0.0005,
            current_floor: DEFAULT_FLOOR,
            smoothed_level: 0.0,
            source_active: false,
            silent_block_counter: 0,
            debounce_blocks: 4,
        };
        est.reset();
        est
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        // Slow attack upward (1.5 seconds) to avoid tracking musical energy
        self.attack_coeff = (1.0 - (-1.0 / (self.sample_rate * 1.5)).exp()) as f32;
        // Fast decay downward towards noise minimums (0.06 seconds)
        self.decay_coeff = (1.0 - (-1.0 / (self.sample_rate * 0.06)).exp()) as f32;
        self.reset();
    }

    pub fn reset(&mut self) {
        self.current_floor = DEFAULT_FLOOR;
        self.smoothed_level = 0.0;
        self.source_active = false;
        self.silent_block_counter = 0;
    }

    /// Update noise floor estimation from RMS energy of incoming audio block
    pub fn update_block(&mut self, block_rms: f32, num_samples: u32) {
        let rms = if block_rms.abs() < 1e-15 { 0.0 } else { block_rms };

        // Level smoothing to reject immediate transient spikes
        let block_coeff = num_samples as f32 / (self.sample_rate * 0.05) as f32;
        let clamped_coeff = block_coeff.clamp(0.01, 1.0);
        self.smoothed_level += (rms - self.smoothed_level) * clamped_coeff;
        if self.smoothed_level.abs() < 1e-15 {
            self.smoothed_level = 0.0;
        }

        // Asymmetric tracker updating statistical noise floor
        if self.smoothed_level < self.current_floor {
            // Rapid decay towards detected minimum
            let alpha = (self.decay_coeff * num_samples as f32).clamp(0.001, 0.5);
            self.current_floor += (self.smoothed_level - self.current_floor) * alpha;
        } else {
            // Ultra-slow rise to adapt to acoustic environment changes
            let alpha = (self.attack_coeff * num_samples as f32).clamp(0.00001, 0.01);
            self.current_floor += (self.smoothed_level - self.current_floor) * alpha;
        }

        // Bound strictly within physical constraints
        self.current_floor = self.current_floor.clamp(MIN_NOISE_FLOOR, MAX_NOISE_FLOOR);

        // Schmitt trigger state machine:
        // High Threshold = Floor + 8.0 dB (factor 2.5x)
        // Low Threshold  = Floor + 2.0 dB (factor 1.25x)
        let high_threshold = self.current_floor * 2.5;
        let low_threshold = self.current_floor * 1.25;

        if rms > high_threshold {
            self.source_active = true;
            self.silent_block_counter = 0;
        } else if rms < low_threshold {
            self.silent_block_counter += 1;
            if self.silent_block_counter >= self.debounce_blocks {
                self.source_active = false;
            }
        }
        // In hysteresis band [low_threshold, high_threshold], source_active remains unchanged
    }

    #[inline(always)]
    pub fn is_source_active(&self) -> bool {
        self.source_active
    }

    #[inline(always)]
    pub fn is_source_silent(&self) -> bool {
        !self.source_active
    }

    #[inline(always)]
    pub fn get_noise_floor(&self) -> f32 {
        self.current_floor
    }

    #[inline(always)]
    pub fn get_smoothed_level(&self) -> f32 {
        self.smoothed_level
    }

    pub fn set_debounce_blocks(&mut self, blocks: u32) {
        self.debounce_blocks = blocks.clamp(1, 32);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_adaptive_noise_floor_hysteresis() {
        let mut est = RustAdaptiveNoiseFloorEstimator::new();
        est.prepare(44100.0);

        assert!(est.get_noise_floor() >= 1.0e-5);

        // Feed strong signal: activates source
        for _ in 0..10 {
            est.update_block(0.2, 128);
        }
        assert!(est.is_source_active());
        assert!(!est.is_source_silent());

        // Intermediate level inside hysteresis zone between 1.25x and 2.5x
        for _ in 0..10 {
            let mid_level = est.get_noise_floor() * 1.8;
            est.update_block(mid_level, 128);
            assert!(est.is_source_active());
        }

        // Feed silence: after debounce (4 blocks) declares silence
        for _ in 0..6 {
            est.update_block(0.0, 128);
        }
        assert!(est.is_source_silent());
    }
}
