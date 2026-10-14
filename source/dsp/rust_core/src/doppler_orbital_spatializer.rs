//! Doppler 3D Orbital Spatializer (Rules 5, 8, 9, 14, 34, 35, 46, 47)
//! Simulates true physical acoustic 3D orbital trajectory around the listener with
//! continuous Doppler frequency shifting, speed-of-sound fractional delay line (343 m/s),
//! distance air absorption lowpass filtering, and inverse-distance attenuation.

use crate::common::{clamp, lerp, denormal_guard, DelayBuffer};
use std::f32::consts::PI;

const SPEED_OF_SOUND: f32 = 343.0; // meters per second
const MAX_ORBIT_DELAY_SAMPLES: usize = 8192; // Supports up to 28 meters radius at 96kHz

pub struct RustDopplerOrbitalSpatializer {
    sample_rate: f32,

    delay_l: DelayBuffer,
    delay_r: DelayBuffer,

    // One-pole air absorption lowpass filters
    air_state_l: f32,
    air_state_r: f32,

    // Orbital phase
    orbit_phase: f32,

    // Parameters
    orbit_rate_hz: f32,       // 0.05 to 10.0 Hz
    orbit_radius_m: f32,      // 0.5 to 12.0 meters
    elevation_deg: f32,       // -45.0 to 45.0 degrees
    doppler_depth: f32,       // 0.0 to 1.0 (modulates pitch shift intensity)
    air_damping: f32,         // 0.0 to 1.0 (air absorption high-shelf attenuation)
    ear_width_m: f32,         // 0.15 to 0.35 meters (binaural ear distance)
    mix: f32,                 // 0.0 to 1.0

    // Smoothed
    smoothed_mix: f32,
}

impl RustDopplerOrbitalSpatializer {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            delay_l: DelayBuffer::new(MAX_ORBIT_DELAY_SAMPLES),
            delay_r: DelayBuffer::new(MAX_ORBIT_DELAY_SAMPLES),
            air_state_l: 0.0,
            air_state_r: 0.0,
            orbit_phase: 0.0,
            orbit_rate_hz: 0.35,
            orbit_radius_m: 3.5,
            elevation_deg: 0.0,
            doppler_depth: 0.8,
            air_damping: 0.5,
            ear_width_m: 0.22,
            mix: 1.0,
            smoothed_mix: 1.0,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn reset(&mut self) {
        self.delay_l.reset();
        self.delay_r.reset();
        self.air_state_l = 0.0;
        self.air_state_r = 0.0;
        self.orbit_phase = 0.0;
        self.smoothed_mix = self.mix;
    }

    pub fn set_param(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.orbit_rate_hz = clamp(value, 0.02, 10.0),
            2 => self.orbit_radius_m = clamp(value, 0.5, 12.0),
            3 => self.elevation_deg = clamp(value, -45.0, 45.0),
            4 => self.doppler_depth = clamp(value, 0.0, 1.0),
            5 => self.air_damping = clamp(value, 0.0, 1.0),
            6 => self.ear_width_m = clamp(value, 0.15, 0.35),
            7 => self.mix = clamp(value, 0.0, 1.0),
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

        let phase_inc = (2.0 * PI * self.orbit_rate_hz / self.sample_rate).max(1e-6);
        let elev_rad = self.elevation_deg * (PI / 180.0);
        let cos_elev = elev_rad.cos();
        let sin_elev = elev_rad.sin();
        let half_ear = self.ear_width_m * 0.5;
        let smooth = (-1.0 / (0.005 * self.sample_rate)).exp();
        let max_delay = (MAX_ORBIT_DELAY_SAMPLES - 8) as f32;

        for i in 0..num_samples as usize {
            let xl = unsafe { *in_l.add(i) };
            let xr = unsafe { *in_r.add(i) };

            self.smoothed_mix = lerp(self.mix, self.smoothed_mix, smooth);

            // Silence bypass
            if xl.abs() < 1e-6 && xr.abs() < 1e-6 {
                unsafe {
                    *out_l.add(i) = 0.0;
                    *out_r.add(i) = 0.0;
                }
                self.orbit_phase = (self.orbit_phase + phase_inc) % (2.0 * PI);
                continue;
            }

            // Write to fractional delay lines
            self.delay_l.write(xl);
            self.delay_r.write(xr);

            // Calculate 3D position on orbit
            let xs = self.orbit_radius_m * cos_elev * self.orbit_phase.sin();
            let ys = self.orbit_radius_m * cos_elev * self.orbit_phase.cos();
            let zs = self.orbit_radius_m * sin_elev;

            // Euclidean distance to Left ear (-half_ear, 0, 0) and Right ear (+half_ear, 0, 0)
            let dx_l = xs + half_ear;
            let dx_r = xs - half_ear;
            let dist_l = (dx_l * dx_l + ys * ys + zs * zs).sqrt().max(0.2);
            let dist_r = (dx_r * dx_r + ys * ys + zs * zs).sqrt().max(0.2);

            // Propagation delay in samples: d / c * sample_rate
            let mean_delay = (self.orbit_radius_m / SPEED_OF_SOUND) * self.sample_rate;
            let raw_delay_l = (dist_l / SPEED_OF_SOUND) * self.sample_rate;
            let raw_delay_r = (dist_r / SPEED_OF_SOUND) * self.sample_rate;

            // Doppler depth scales deviation around mean delay
            let delay_samples_l = lerp(mean_delay, raw_delay_l, self.doppler_depth).clamp(2.0, max_delay);
            let delay_samples_r = lerp(mean_delay, raw_delay_r, self.doppler_depth).clamp(2.0, max_delay);

            // Read with Hermite cubic interpolation
            let delayed_l = self.delay_l.read_cubic(delay_samples_l);
            let delayed_r = self.delay_r.read_cubic(delay_samples_r);

            // Distance inverse attenuation law
            let gain_l = 1.0 / (1.0 + 0.35 * (dist_l - 0.5).max(0.0));
            let gain_r = 1.0 / (1.0 + 0.35 * (dist_r - 0.5).max(0.0));

            // Dynamic air absorption lowpass filter (drops high frequencies over distance)
            let air_alpha_l = (-2.0 * PI * (16000.0 / (1.0 + self.air_damping * 0.4 * dist_l)) / self.sample_rate).exp();
            let air_alpha_r = (-2.0 * PI * (16000.0 / (1.0 + self.air_damping * 0.4 * dist_r)) / self.sample_rate).exp();

            self.air_state_l = lerp(delayed_l * gain_l, self.air_state_l, air_alpha_l);
            self.air_state_r = lerp(delayed_r * gain_r, self.air_state_r, air_alpha_r);

            let wet_l = self.air_state_l;
            let wet_r = self.air_state_r;

            let out_sample_l = denormal_guard(lerp(xl, wet_l, self.smoothed_mix));
            let out_sample_r = denormal_guard(lerp(xr, wet_r, self.smoothed_mix));

            unsafe {
                *out_l.add(i) = out_sample_l;
                *out_r.add(i) = out_sample_r;
            }

            self.orbit_phase = (self.orbit_phase + phase_inc) % (2.0 * PI);
        }
    }
}
