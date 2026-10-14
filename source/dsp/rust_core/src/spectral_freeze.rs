//! Spectral Freeze DSP Kernel (DSP #12)
//! Rules 5, 8, 9, 13, 14, 16, 19, 34, 46, 47: Zero allocations in audio thread,
//! 512-point Radix-2 FFT spectral analysis, magnitude locking (Freeze),
//! temporal magnitude diffusion (Smear), spectral tilt brightness,
//! phase accumulation for infinite drone synthesis, and denormal protection.

use crate::common::denormal_guard;
use crate::fft::RustFFT;

pub struct RustSpectralFreeze {
    fft: RustFFT,
    time_in_l: Box<[f32]>,
    time_in_r: Box<[f32]>,
    time_out_l: Box<[f32]>,
    time_out_r: Box<[f32]>,
    freq_re_l: Box<[f32]>,
    freq_im_l: Box<[f32]>,
    freq_re_r: Box<[f32]>,
    freq_im_r: Box<[f32]>,
    prev_mags_l: Box<[f32]>,
    prev_mags_r: Box<[f32]>,
    frozen_mags_l: Box<[f32]>,
    frozen_mags_r: Box<[f32]>,
    phase_accum_l: Box<[f32]>,
    phase_accum_r: Box<[f32]>,
    buffer_pos: usize,
    was_frozen: bool,
    has_frozen_audio: bool,

    target_freeze: f32,
    target_smear: f32,
    target_brightness: f32,
    target_mix: f32,
}

impl RustSpectralFreeze {
    pub fn new() -> Self {
        let size = 512;
        let num_bins = size / 2;
        Self {
            fft: RustFFT::new(size),
            time_in_l: vec![0.0f32; size].into_boxed_slice(),
            time_in_r: vec![0.0f32; size].into_boxed_slice(),
            time_out_l: vec![0.0f32; size].into_boxed_slice(),
            time_out_r: vec![0.0f32; size].into_boxed_slice(),
            freq_re_l: vec![0.0f32; size].into_boxed_slice(),
            freq_im_l: vec![0.0f32; size].into_boxed_slice(),
            freq_re_r: vec![0.0f32; size].into_boxed_slice(),
            freq_im_r: vec![0.0f32; size].into_boxed_slice(),
            prev_mags_l: vec![0.0f32; num_bins].into_boxed_slice(),
            prev_mags_r: vec![0.0f32; num_bins].into_boxed_slice(),
            frozen_mags_l: vec![0.0f32; num_bins].into_boxed_slice(),
            frozen_mags_r: vec![0.0f32; num_bins].into_boxed_slice(),
            phase_accum_l: vec![0.0f32; num_bins].into_boxed_slice(),
            phase_accum_r: vec![0.0f32; num_bins].into_boxed_slice(),
            buffer_pos: 0,
            was_frozen: false,
            has_frozen_audio: false,
            target_freeze: 0.0,
            target_smear: 0.0,
            target_brightness: 0.0,
            target_mix: 0.5,
        }
    }

    pub fn prepare(&mut self, _sample_rate: f32) {
        self.reset();
    }

    pub fn reset(&mut self) {
        self.time_in_l.fill(0.0);
        self.time_in_r.fill(0.0);
        self.time_out_l.fill(0.0);
        self.time_out_r.fill(0.0);
        self.freq_re_l.fill(0.0);
        self.freq_im_l.fill(0.0);
        self.freq_re_r.fill(0.0);
        self.freq_im_r.fill(0.0);
        self.prev_mags_l.fill(0.0);
        self.prev_mags_r.fill(0.0);
        self.frozen_mags_l.fill(0.0);
        self.frozen_mags_r.fill(0.0);
        self.phase_accum_l.fill(0.0);
        self.phase_accum_r.fill(0.0);
        self.buffer_pos = 0;
        self.was_frozen = false;
        self.has_frozen_audio = false;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_freeze = if value >= 0.5 { 1.0 } else { 0.0 },
            2 => self.target_smear = value.clamp(0.0, 0.98),
            3 => self.target_brightness = value.clamp(-1.0, 1.0),
            4 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_freeze,
            2 => self.target_smear,
            3 => self.target_brightness,
            4 => self.target_mix,
            _ => 0.0,
        }
    }

    #[inline]
    pub fn process(&mut self, in_l: &[f32], in_r: &[f32], out_l: &mut [f32], out_r: &mut [f32]) {
        let is_frozen = self.target_freeze >= 0.5;
        let smear = self.target_smear.clamp(0.0, 0.98);
        let brightness = self.target_brightness.clamp(-1.0, 1.0);
        let dry_wet = self.target_mix.clamp(0.0, 1.0);

        let fft_size = 512;
        let num_bins = 256;

        let num_samples = in_l.len();
        for s in 0..num_samples {
            let dry_sample_l = denormal_guard(in_l[s]);
            let dry_sample_r = denormal_guard(in_r[s]);

            self.time_in_l[self.buffer_pos] = dry_sample_l;
            self.time_in_r[self.buffer_pos] = dry_sample_r;

            let wet_l = self.time_out_l[self.buffer_pos];
            let wet_r = self.time_out_r[self.buffer_pos];

            self.buffer_pos += 1;
            if self.buffer_pos >= fft_size {
                self.buffer_pos = 0;

                self.freq_re_l.copy_from_slice(&self.time_in_l);
                self.freq_im_l.fill(0.0);
                self.fft.compute(&mut self.freq_re_l, &mut self.freq_im_l, false);

                self.freq_re_r.copy_from_slice(&self.time_in_r);
                self.freq_im_r.fill(0.0);
                self.fft.compute(&mut self.freq_re_r, &mut self.freq_im_r, false);

                let mut total_energy = 0.0f32;
                for k in 0..num_bins {
                    let re_l = self.freq_re_l[k];
                    let im_l = self.freq_im_l[k];
                    let re_r = self.freq_re_r[k];
                    let im_r = self.freq_im_r[k];
                    let mag_l = (re_l * re_l + im_l * im_l).sqrt();
                    let mag_r = (re_r * re_r + im_r * im_r).sqrt();
                    total_energy += mag_l + mag_r;
                }

                if is_frozen {
                    if !self.was_frozen || (!self.has_frozen_audio && total_energy > 1e-3) {
                        for k in 0..num_bins {
                            let re_l = self.freq_re_l[k];
                            let im_l = self.freq_im_l[k];
                            let re_r = self.freq_re_r[k];
                            let im_r = self.freq_im_r[k];
                            self.frozen_mags_l[k] = (re_l * re_l + im_l * im_l).sqrt();
                            self.frozen_mags_r[k] = (re_r * re_r + im_r * im_r).sqrt();
                        }
                        if total_energy > 1e-3 {
                            self.has_frozen_audio = true;
                        }
                    }
                } else {
                    self.has_frozen_audio = false;
                }

                for k in 0..num_bins {
                    let mut mag_l: f32;
                    let mut mag_r: f32;

                    if is_frozen && self.has_frozen_audio {
                        mag_l = self.frozen_mags_l[k];
                        mag_r = self.frozen_mags_r[k];

                        let bin_freq = (k as f32) * 0.13 + 0.07;
                        self.phase_accum_l[k] += bin_freq;
                        self.phase_accum_r[k] += bin_freq * 1.01;
                    } else {
                        let re_l = self.freq_re_l[k];
                        let im_l = self.freq_im_l[k];
                        let re_r = self.freq_re_r[k];
                        let im_r = self.freq_im_r[k];
                        mag_l = (re_l * re_l + im_l * im_l).sqrt();
                        mag_r = (re_r * re_r + im_r * im_r).sqrt();

                        self.frozen_mags_l[k] = mag_l;
                        self.frozen_mags_r[k] = mag_r;

                        if smear > 0.0 {
                            mag_l = (1.0 - smear) * mag_l + smear * self.prev_mags_l[k];
                            mag_r = (1.0 - smear) * mag_r + smear * self.prev_mags_r[k];
                            self.prev_mags_l[k] = mag_l;
                            self.prev_mags_r[k] = mag_r;
                        }

                        self.phase_accum_l[k] = im_l.atan2(re_l);
                        self.phase_accum_r[k] = im_r.atan2(re_r);
                    }

                    let norm_freq = (k as f32) / (num_bins as f32);
                    let tilt_gain = 10.0f32.powf((brightness * (norm_freq - 0.5) * 18.0) * 0.05);

                    mag_l *= tilt_gain;
                    mag_r *= tilt_gain;

                    let (sin_l, cos_l) = self.phase_accum_l[k].sin_cos();
                    let (sin_r, cos_r) = self.phase_accum_r[k].sin_cos();

                    self.freq_re_l[k] = mag_l * cos_l;
                    self.freq_im_l[k] = mag_l * sin_l;

                    self.freq_re_r[k] = mag_r * cos_r;
                    self.freq_im_r[k] = mag_r * sin_r;

                    if k > 0 && k < num_bins {
                        self.freq_re_l[fft_size - k] = self.freq_re_l[k];
                        self.freq_im_l[fft_size - k] = -self.freq_im_l[k];

                        self.freq_re_r[fft_size - k] = self.freq_re_r[k];
                        self.freq_im_r[fft_size - k] = -self.freq_im_r[k];
                    }
                }

                self.was_frozen = is_frozen;

                self.fft.compute(&mut self.freq_re_l, &mut self.freq_im_l, true);
                self.fft.compute(&mut self.freq_re_r, &mut self.freq_im_r, true);

                self.time_out_l.copy_from_slice(&self.freq_re_l);
                self.time_out_r.copy_from_slice(&self.freq_re_r);
            }

            out_l[s] = denormal_guard(dry_sample_l * (1.0 - dry_wet) + wet_l * dry_wet);
            out_r[s] = denormal_guard(dry_sample_r * (1.0 - dry_wet) + wet_r * dry_wet);
        }
    }
}
