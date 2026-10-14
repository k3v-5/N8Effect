//! Transaural Crosstalk Canceller (RACE 3D Holographic Speaker Widener) (Rules 5, 8, 9, 14, 34, 35, 46, 47)
//! Implements Recursive Ambiophonics Crosstalk Elimination (RACE) to expand stereo monitor imaging
//! beyond the physical speaker triangle (up to 180 degrees) by inverting acoustic contralateral crosstalk.
//! Features fractional cubic delay (80-300 us), head-shadow lowpass diffraction filtering,
//! Mid/Side center-channel vocal anchoring, and zero allocations.

use crate::common::{clamp, lerp, denormal_guard, BiquadCoeffs, BiquadState, DelayBuffer};

const MAX_CROSSTALK_DELAY_SAMPLES: usize = 128; // > 1.3ms at 96kHz, strictly bounded

pub struct RustTransauralCrosstalkCanceller {
    sample_rate: f32,

    delay_l: DelayBuffer,
    delay_r: DelayBuffer,

    // Head-shadow diffraction lowpass filter (1st/2nd order)
    shadow_l: BiquadState,
    shadow_r: BiquadState,
    shadow_coeffs: BiquadCoeffs,

    // Parameters
    delay_us: f32,                // 80.0 to 300.0 microseconds (path length between ears)
    crosstalk_cancel: f32,        // 0.0 to 1.0 (attenuation factor k)
    head_shadow_freq: f32,        // 2000.0 to 12000.0 Hz
    center_preserve: f32,         // 0.0 to 1.0 (Mid channel preservation)
    mix: f32,                     // 0.0 to 1.0

    // Smoothed
    smoothed_cancel: f32,
    smoothed_mix: f32,
}

impl RustTransauralCrosstalkCanceller {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        let mut cancel = Self {
            sample_rate: sr,
            delay_l: DelayBuffer::new(MAX_CROSSTALK_DELAY_SAMPLES),
            delay_r: DelayBuffer::new(MAX_CROSSTALK_DELAY_SAMPLES),
            shadow_l: BiquadState::default(),
            shadow_r: BiquadState::default(),
            shadow_coeffs: BiquadCoeffs::default(),
            delay_us: 180.0,
            crosstalk_cancel: 0.65,
            head_shadow_freq: 6500.0,
            center_preserve: 0.7,
            mix: 1.0,
            smoothed_cancel: 0.65,
            smoothed_mix: 1.0,
        };
        cancel.update_filter();
        cancel
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.update_filter();
        self.reset();
    }

    pub fn reset(&mut self) {
        self.delay_l.reset();
        self.delay_r.reset();
        self.shadow_l.reset();
        self.shadow_r.reset();
        self.smoothed_cancel = self.crosstalk_cancel;
        self.smoothed_mix = self.mix;
    }

    fn update_filter(&mut self) {
        let fc = clamp(self.head_shadow_freq, 1500.0, self.sample_rate * 0.45);
        self.shadow_coeffs = BiquadCoeffs::lowpass(self.sample_rate, fc, 0.7071);
    }

    pub fn set_param(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.delay_us = clamp(value, 60.0, 320.0),
            2 => self.crosstalk_cancel = clamp(value, 0.0, 1.0),
            3 => {
                self.head_shadow_freq = clamp(value, 1500.0, 12000.0);
                self.update_filter();
            }
            4 => self.center_preserve = clamp(value, 0.0, 1.0),
            5 => self.mix = clamp(value, 0.0, 1.0),
            _ => {}
        }
    }

    pub fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: u32,
    ) {
        if in_l.is_null() || in_r.is_null() || out_l.is_null() || out_r.is_null() || num_samples == 0 {
            return;
        }

        let delay_samples = (self.delay_us * 1e-6 * self.sample_rate).clamp(1.0, (MAX_CROSSTALK_DELAY_SAMPLES - 4) as f32);
        let smooth = (-1.0 / (0.005 * self.sample_rate)).exp();

        for i in 0..num_samples as usize {
            let xl = unsafe { *in_l.add(i) };
            let xr = unsafe { *in_r.add(i) };

            self.smoothed_cancel = lerp(self.crosstalk_cancel, self.smoothed_cancel, smooth);
            self.smoothed_mix = lerp(self.mix, self.smoothed_mix, smooth);

            // Silence bypass
            if xl.abs() < 1e-6 && xr.abs() < 1e-6 {
                unsafe {
                    *out_l.add(i) = 0.0;
                    *out_r.add(i) = 0.0;
                }
                continue;
            }

            // Write incoming stereo signals to circular delay lines
            self.delay_l.write(xl);
            self.delay_r.write(xr);

            // Read delayed contralateral channel with fractional cubic interpolation
            let delayed_l = self.delay_l.read_cubic(delay_samples);
            let delayed_r = self.delay_r.read_cubic(delay_samples);

            // Apply head shadow acoustic diffraction filtering
            let shadow_l = self.shadow_l.process(delayed_l, &self.shadow_coeffs);
            let shadow_r = self.shadow_r.process(delayed_r, &self.shadow_coeffs);

            // Mid/Side decomposition to preserve phantom center
            let mid = 0.5 * (xl + xr);
            let side_l = xl - mid;
            let side_r = xr - mid;

            // RACE contralateral cancellation applied to side components
            let k = self.smoothed_cancel;
            let cancel_l = side_l - k * shadow_r;
            let cancel_r = side_r - k * shadow_l;

            // Recombine with preserved Center Mid
            let center_weight = self.center_preserve;
            let wet_l = mid * center_weight + cancel_l;
            let wet_r = mid * center_weight + cancel_r;

            let out_sample_l = denormal_guard(lerp(xl, wet_l, self.smoothed_mix));
            let out_sample_r = denormal_guard(lerp(xr, wet_r, self.smoothed_mix));

            unsafe {
                *out_l.add(i) = out_sample_l;
                *out_r.add(i) = out_sample_r;
            }
        }
    }
}
