//! Stereo to Mid/Side Encoder DSP Kernel (DSP #24)
//! Rules 5, 8, 9, 13, 16, 34, 46, 47: Zero allocations in audio thread,
//! normalized 1/sqrt(2) power conservation, anti-click gain smoothing,
//! and denormal protection.

use crate::common::denormal_guard;

const INV_SQRT_2: f32 = 0.70710678;

pub struct RustMidSideEncoder {
    target_gain: f32,
    current_gain: f32,
}

impl RustMidSideEncoder {
    pub fn new() -> Self {
        Self {
            target_gain: 1.0,
            current_gain: 1.0,
        }
    }

    pub fn prepare(&mut self, _sample_rate: f32) {
        self.current_gain = self.target_gain;
    }

    pub fn reset(&mut self) {
        self.current_gain = self.target_gain;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        if id == 1 {
            self.target_gain = value.clamp(0.0, 2.0);
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        if id == 1 {
            self.target_gain
        } else {
            0.0
        }
    }

    #[inline]
    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_mid: &mut [f32],
        out_side: &mut [f32],
    ) {
        let num_samples = in_l.len().min(out_mid.len());
        if num_samples == 0 {
            return;
        }

        const ALPHA: f32 = 0.005;

        for s in 0..num_samples {
            self.current_gain += ALPHA * (self.target_gain - self.current_gain);
            let raw_l = in_l[s];
            let raw_r = if s < in_r.len() { in_r[s] } else { raw_l };

            let mid = (raw_l + raw_r) * INV_SQRT_2 * self.current_gain;
            let side = (raw_l - raw_r) * INV_SQRT_2 * self.current_gain;

            out_mid[s] = denormal_guard(mid);
            if s < out_side.len() {
                out_side[s] = denormal_guard(side);
            }
        }
    }
}
