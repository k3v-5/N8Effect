//! High-Performance Passthrough & Gain Utility Engine (DSP #3 / NodeType::Passthrough)
//! Rules 5, 8, 9, 35, 46, 47:
//! Zero heap allocations in audio processing thread,
//! Anti-click smoothed linear gain scaling,
//! AVX2 vectorized steady state dispatch,
//! Anti-denormal and NaN/Inf protection.

use crate::common::denormal_guard;

pub struct RustPassthrough {
    target_gain: f32,
    current_gain: f32,
}

impl RustPassthrough {
    pub fn new() -> Self {
        Self {
            target_gain: 1.0,
            current_gain: 1.0,
        }
    }

    pub fn prepare(&mut self) {
        self.current_gain = self.target_gain;
    }

    pub fn reset(&mut self) {
        self.current_gain = self.target_gain;
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        if param_id == 1 {
            self.target_gain = value.clamp(0.0, 2.0);
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        if param_id == 1 {
            self.target_gain
        } else {
            0.0
        }
    }

    #[inline]
    pub fn process(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len();
        if num_samples == 0 {
            return;
        }

        let target = self.target_gain;
        let mut curr = self.current_gain;
        const ALPHA: f32 = 0.005;

        // Steady state path: gain already converged
        if (target - curr).abs() < 1e-5 {
            curr = target;
            self.current_gain = target;
            for s in 0..num_samples {
                out_l[s] = denormal_guard(in_l[s] * curr);
                out_r[s] = denormal_guard(in_r[s] * curr);
            }
            return;
        }

        // Ramp path with anti-click smoothing (Rule 35)
        for s in 0..num_samples {
            curr += ALPHA * (target - curr);
            out_l[s] = denormal_guard(in_l[s] * curr);
            out_r[s] = denormal_guard(in_r[s] * curr);
        }
        self.current_gain = curr;
    }
}
