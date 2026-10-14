//! 3D Spatial Panner DSP Kernel
//! Rules 5, 8, 9, 13, 16, 32, 34, 46, 47: Zero allocations in audio thread,
//! Woodworth spherical ITD with Hermite cubic delay line, ILD head shadowing,
//! pinna notch elevation cues, distance air absorption, and denormal protection.

use std::f32::consts::PI;
use crate::common::{denormal_guard, BiquadCoeffs, BiquadState, DelayBuffer};

pub struct RustSpatialPanner {
    sample_rate: f32,
    itd_delay: [DelayBuffer; 2],

    head_shadow_coeffs: [BiquadCoeffs; 2],
    head_shadow_state: [BiquadState; 2],

    air_absorption_coeffs: [BiquadCoeffs; 2],
    air_absorption_state: [BiquadState; 2],

    pinna_coeffs: [BiquadCoeffs; 2],
    pinna_state: [BiquadState; 2],

    target_azimuth: f32,
    current_azimuth: f32,
    target_elevation: f32,
    current_elevation: f32,
    target_distance: f32,
    current_distance: f32,

    target_mix: f32,
    current_mix: f32,
}

impl RustSpatialPanner {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut panner = Self {
            sample_rate: sr,
            itd_delay: [DelayBuffer::new(256), DelayBuffer::new(256)],
            head_shadow_coeffs: [BiquadCoeffs::default(); 2],
            head_shadow_state: [BiquadState::default(); 2],
            air_absorption_coeffs: [BiquadCoeffs::default(); 2],
            air_absorption_state: [BiquadState::default(); 2],
            pinna_coeffs: [BiquadCoeffs::default(); 2],
            pinna_state: [BiquadState::default(); 2],
            target_azimuth: 0.0,
            current_azimuth: 0.0,
            target_elevation: 0.0,
            current_elevation: 0.0,
            target_distance: 1.0,
            current_distance: 1.0,
            target_mix: 1.0,
            current_mix: 1.0,
        };
        panner.update_filters();
        panner
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.update_filters();
        self.reset();
    }

    pub fn reset(&mut self) {
        self.itd_delay[0].reset();
        self.itd_delay[1].reset();
        self.head_shadow_state[0].reset();
        self.head_shadow_state[1].reset();
        self.air_absorption_state[0].reset();
        self.air_absorption_state[1].reset();
        self.pinna_state[0].reset();
        self.pinna_state[1].reset();
        self.current_azimuth = self.target_azimuth;
        self.current_elevation = self.target_elevation;
        self.current_distance = self.target_distance;
        self.current_mix = self.target_mix;
    }

    pub fn set_coordinates(&mut self, azimuth_deg: f32, elevation_deg: f32, distance_meters: f32) {
        self.target_azimuth = azimuth_deg.clamp(-180.0, 180.0);
        self.target_elevation = elevation_deg.clamp(-90.0, 90.0);
        self.target_distance = distance_meters.clamp(0.1, 10.0);
        self.update_filters();
    }

    #[inline]
    pub fn process_sample_pair(&mut self, in_l: f32, in_r: f32) -> (f32, f32) {
        let alpha = 0.005f32;
        let deg_to_rad = PI / 180.0;
        let sr = self.sample_rate;

        self.current_azimuth += alpha * (self.target_azimuth - self.current_azimuth);
        self.current_elevation += alpha * (self.target_elevation - self.current_elevation);
        self.current_distance += alpha * (self.target_distance - self.current_distance);

        let src_mono = 0.5 * (in_l + in_r);
        let az_rad = self.current_azimuth * deg_to_rad;
        let abs_az = az_rad.abs();
        let sin_az = abs_az.sin();

        let max_itd_sec = 0.000255 * (sin_az + abs_az);
        let itd_samples = max_itd_sec * sr;

        self.itd_delay[0].write(src_mono);
        self.itd_delay[1].write(src_mono);

        let delay_l = if az_rad > 0.0 { itd_samples } else { 0.0 };
        let delay_r = if az_rad < 0.0 { itd_samples } else { 0.0 };

        let mut sig_l = self.itd_delay[0].read_cubic(delay_l);
        let mut sig_r = self.itd_delay[1].read_cubic(delay_r);

        sig_l = self.head_shadow_state[0].process(sig_l, &self.head_shadow_coeffs[0]);
        sig_r = self.head_shadow_state[1].process(sig_r, &self.head_shadow_coeffs[1]);

        sig_l = self.pinna_state[0].process(sig_l, &self.pinna_coeffs[0]);
        sig_r = self.pinna_state[1].process(sig_r, &self.pinna_coeffs[1]);

        sig_l = self.air_absorption_state[0].process(sig_l, &self.air_absorption_coeffs[0]);
        sig_r = self.air_absorption_state[1].process(sig_r, &self.air_absorption_coeffs[1]);

        let dist_gain = 1.0 / self.current_distance.max(0.01).sqrt();
        (sig_l * dist_gain, sig_r * dist_gain)
    }

    fn update_filters(&mut self) {
        let sr = self.sample_rate;
        if sr <= 0.0 {
            return;
        }

        let az_rad = (self.target_azimuth * (PI / 180.0)).abs();
        let sin_az = az_rad.sin();
        let shadow_cutoff = (20000.0 * (1.0 - 0.70 * sin_az)).clamp(100.0, sr * 0.49);

        if self.target_azimuth > 0.0 {
            // Fuente a la derecha: Oído izquierdo (0) recibe sombra
            self.head_shadow_coeffs[0] = BiquadCoeffs::lowpass(sr, shadow_cutoff, 0.707);
            self.head_shadow_coeffs[1] = BiquadCoeffs::allpass(sr, 1000.0, 0.707);
        } else if self.target_azimuth < 0.0 {
            // Fuente a la izquierda: Oído derecho (1) recibe sombra
            self.head_shadow_coeffs[0] = BiquadCoeffs::allpass(sr, 1000.0, 0.707);
            self.head_shadow_coeffs[1] = BiquadCoeffs::lowpass(sr, shadow_cutoff, 0.707);
        } else {
            self.head_shadow_coeffs[0] = BiquadCoeffs::allpass(sr, 1000.0, 0.707);
            self.head_shadow_coeffs[1] = BiquadCoeffs::allpass(sr, 1000.0, 0.707);
        }

        let air_cutoff = (20000.0 / self.target_distance.sqrt()).clamp(3000.0, sr * 0.49);
        self.air_absorption_coeffs[0] = BiquadCoeffs::lowpass(sr, air_cutoff, 0.707);
        self.air_absorption_coeffs[1] = BiquadCoeffs::lowpass(sr, air_cutoff, 0.707);

        let pinna_freq = (8000.0 + self.target_elevation * 25.0).clamp(200.0, sr * 0.49);
        self.pinna_coeffs[0] = BiquadCoeffs::peak(sr, pinna_freq, 2.0, -3.0);
        self.pinna_coeffs[1] = BiquadCoeffs::peak(sr, pinna_freq, 2.0, -3.0);
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => {
                self.target_azimuth = value.clamp(-180.0, 180.0);
                self.update_filters();
            }
            2 => {
                self.target_elevation = value.clamp(-90.0, 90.0);
                self.update_filters();
            }
            3 => {
                self.target_distance = value.clamp(0.1, 10.0);
                self.update_filters();
            }
            4 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_azimuth,
            2 => self.target_elevation,
            3 => self.target_distance,
            4 => self.target_mix,
            _ => 0.0,
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
        if num_samples == 0 {
            return;
        }

        let alpha = 0.005f32;
        let deg_to_rad = PI / 180.0;
        let sr = self.sample_rate;

        for s in 0..num_samples {
            self.current_mix += alpha * (self.target_mix - self.current_mix);
            self.current_azimuth += alpha * (self.target_azimuth - self.current_azimuth);
            self.current_elevation += alpha * (self.target_elevation - self.current_elevation);
            self.current_distance += alpha * (self.target_distance - self.current_distance);

            let s_l = in_l[s];
            let s_r = if s < in_r.len() { in_r[s] } else { s_l };

            // 1. Suma mono para centro espacial
            let src_mono = 0.5 * (s_l + s_r);

            // 2. Cálculo Woodworth ITD
            let az_rad = self.current_azimuth * deg_to_rad;
            let abs_az = az_rad.abs();
            let sin_az = abs_az.sin();

            let max_itd_sec = 0.000255 * (sin_az + abs_az);
            let itd_samples = max_itd_sec * sr;

            self.itd_delay[0].write(src_mono);
            self.itd_delay[1].write(src_mono);

            let delay_l = if az_rad > 0.0 { itd_samples } else { 0.0 };
            let delay_r = if az_rad < 0.0 { itd_samples } else { 0.0 };

            let mut sig_l = self.itd_delay[0].read_cubic(delay_l);
            let mut sig_r = self.itd_delay[1].read_cubic(delay_r);

            // 3. ILD (Head Shadowing)
            sig_l = self.head_shadow_state[0].process(sig_l, &self.head_shadow_coeffs[0]);
            sig_r = self.head_shadow_state[1].process(sig_r, &self.head_shadow_coeffs[1]);

            // 4. Pinna filter
            sig_l = self.pinna_state[0].process(sig_l, &self.pinna_coeffs[0]);
            sig_r = self.pinna_state[1].process(sig_r, &self.pinna_coeffs[1]);

            // 5. Absorción atmosférica
            sig_l = self.air_absorption_state[0].process(sig_l, &self.air_absorption_coeffs[0]);
            sig_r = self.air_absorption_state[1].process(sig_r, &self.air_absorption_coeffs[1]);

            // Ganancia por distancia (1/sqrt(d))
            let dist_gain = 1.0 / self.current_distance.max(0.01).sqrt();
            let spatial_l = sig_l * dist_gain;
            let spatial_r = sig_r * dist_gain;

            let cur_mix = self.current_mix;
            out_l[s] = denormal_guard(s_l * (1.0 - cur_mix) + spatial_l * cur_mix);
            if s < out_r.len() {
                out_r[s] = denormal_guard(s_r * (1.0 - cur_mix) + spatial_r * cur_mix);
            }
        }
    }
}
