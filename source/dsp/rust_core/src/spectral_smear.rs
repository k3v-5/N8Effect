//! Spectral Smear DSP Kernel (DSP #52)
//! Rules 5, 8, 9, 13, 14, 16, 19, 34, 46, 47: Zero allocations in audio thread,
//! 1024-point Radix-2 STFT with 75% overlap-add (Hop size 256), Hann windowing,
//! asymmetric temporal magnitude persistence, stochastic phase diffusion,
//! stereo decorrelation, spectral tilt damping, and denormal protection.

use std::f32::consts::{PI, TAU};
use crate::common::denormal_guard;
use crate::fft::RustFFT;

pub struct RustSpectralSmear {
    fft: RustFFT,
    window: Box<[f32]>,
    input_ring_l: Box<[f32]>,
    input_ring_r: Box<[f32]>,
    ola_buffer_l: Box<[f32]>,
    ola_buffer_r: Box<[f32]>,
    time_frame_l: Box<[f32]>,
    time_frame_r: Box<[f32]>,
    synth_frame_l: Box<[f32]>,
    synth_frame_r: Box<[f32]>,
    freq_re_l: Box<[f32]>,
    freq_im_l: Box<[f32]>,
    freq_re_r: Box<[f32]>,
    freq_im_r: Box<[f32]>,
    prev_mags_l: Box<[f32]>,
    prev_mags_r: Box<[f32]>,
    smooth_mags_l: Box<[f32]>,
    smooth_mags_r: Box<[f32]>,
    phase_accum_l: Box<[f32]>,
    phase_accum_r: Box<[f32]>,

    input_write_pos: usize,
    hop_counter: usize,
    ola_read_pos: usize,
    rng_state_l: u32,
    rng_state_r: u32,

    sample_rate: f32,
    target_smear_time: f32,
    target_phase_diff: f32,
    target_decorrel: f32,
    target_tilt: f32,
    target_mix: f32,
}

impl RustSpectralSmear {
    const FFT_SIZE: usize = 1024;
    const HOP_SIZE: usize = 256;
    const NUM_BINS: usize = Self::FFT_SIZE / 2;
    const OLA_BUFFER_SIZE: usize = Self::FFT_SIZE * 2;

    pub fn new() -> Self {
        let fft_size = Self::FFT_SIZE;
        let num_bins = Self::NUM_BINS;
        let ola_size = Self::OLA_BUFFER_SIZE;

        let mut window = vec![0.0f32; fft_size].into_boxed_slice();
        let denom = (fft_size - 1) as f32;
        for i in 0..fft_size {
            window[i] = 0.5 * (1.0 - (TAU * (i as f32) / denom).cos());
        }

        Self {
            fft: RustFFT::new(fft_size),
            window,
            input_ring_l: vec![0.0f32; fft_size].into_boxed_slice(),
            input_ring_r: vec![0.0f32; fft_size].into_boxed_slice(),
            ola_buffer_l: vec![0.0f32; ola_size].into_boxed_slice(),
            ola_buffer_r: vec![0.0f32; ola_size].into_boxed_slice(),
            time_frame_l: vec![0.0f32; fft_size].into_boxed_slice(),
            time_frame_r: vec![0.0f32; fft_size].into_boxed_slice(),
            synth_frame_l: vec![0.0f32; fft_size].into_boxed_slice(),
            synth_frame_r: vec![0.0f32; fft_size].into_boxed_slice(),
            freq_re_l: vec![0.0f32; fft_size].into_boxed_slice(),
            freq_im_l: vec![0.0f32; fft_size].into_boxed_slice(),
            freq_re_r: vec![0.0f32; fft_size].into_boxed_slice(),
            freq_im_r: vec![0.0f32; fft_size].into_boxed_slice(),
            prev_mags_l: vec![0.0f32; num_bins].into_boxed_slice(),
            prev_mags_r: vec![0.0f32; num_bins].into_boxed_slice(),
            smooth_mags_l: vec![0.0f32; num_bins].into_boxed_slice(),
            smooth_mags_r: vec![0.0f32; num_bins].into_boxed_slice(),
            phase_accum_l: vec![0.0f32; num_bins].into_boxed_slice(),
            phase_accum_r: vec![0.0f32; num_bins].into_boxed_slice(),

            input_write_pos: 0,
            hop_counter: 0,
            ola_read_pos: 0,
            rng_state_l: 123456789,
            rng_state_r: 987654321,

            sample_rate: 48000.0,
            target_smear_time: 2.5,
            target_phase_diff: 0.7,
            target_decorrel: 0.85,
            target_tilt: 0.0,
            target_mix: 0.5,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn reset(&mut self) {
        self.input_ring_l.fill(0.0);
        self.input_ring_r.fill(0.0);
        self.ola_buffer_l.fill(0.0);
        self.ola_buffer_r.fill(0.0);
        self.time_frame_l.fill(0.0);
        self.time_frame_r.fill(0.0);
        self.synth_frame_l.fill(0.0);
        self.synth_frame_r.fill(0.0);
        self.freq_re_l.fill(0.0);
        self.freq_im_l.fill(0.0);
        self.freq_re_r.fill(0.0);
        self.freq_im_r.fill(0.0);
        self.prev_mags_l.fill(0.0);
        self.prev_mags_r.fill(0.0);
        self.smooth_mags_l.fill(0.0);
        self.smooth_mags_r.fill(0.0);
        self.phase_accum_l.fill(0.0);
        self.phase_accum_r.fill(0.0);

        self.input_write_pos = 0;
        self.hop_counter = 0;
        self.ola_read_pos = 0;
        self.rng_state_l = 123456789;
        self.rng_state_r = 987654321;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_smear_time = value.clamp(0.05, 10.0),
            2 => self.target_phase_diff = value.clamp(0.0, 1.0),
            3 => self.target_decorrel = value.clamp(0.0, 1.0),
            4 => self.target_tilt = value.clamp(-1.0, 1.0),
            5 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_smear_time,
            2 => self.target_phase_diff,
            3 => self.target_decorrel,
            4 => self.target_tilt,
            5 => self.target_mix,
            _ => 0.0,
        }
    }

    #[inline]
    pub fn process(&mut self, in_l: &[f32], in_r: &[f32], out_l: &mut [f32], out_r: &mut [f32]) {
        let sr = self.sample_rate;
        let mix = self.target_mix;
        let smear_time = self.target_smear_time.clamp(0.05, 10.0);
        let phase_diff = self.target_phase_diff.clamp(0.0, 1.0);
        let decorrel = self.target_decorrel.clamp(0.0, 1.0);
        let tilt = self.target_tilt.clamp(-1.0, 1.0);

        // Asymmetric magnitude persistence: alpha = exp(-H / (fs * T))
        let alpha_blur = (-(Self::HOP_SIZE as f32) / (sr * smear_time)).exp().clamp(0.0, 0.998);

        // COLA amplitude normalization for 75% Hann overlap: 2/3
        const COLA_NORMALIZATION: f32 = 2.0 / 3.0;

        let num_samples = in_l.len();
        for s in 0..num_samples {
            let sample_l = denormal_guard(in_l[s]);
            let sample_r = denormal_guard(in_r[s]);

            // 1. Write to circular input buffer
            self.input_ring_l[self.input_write_pos] = sample_l;
            self.input_ring_r[self.input_write_pos] = sample_r;
            self.input_write_pos = (self.input_write_pos + 1) % Self::FFT_SIZE;

            // 2. Read from OLA buffer
            let mut wet_l = self.ola_buffer_l[self.ola_read_pos];
            let mut wet_r = self.ola_buffer_r[self.ola_read_pos];
            self.ola_buffer_l[self.ola_read_pos] = 0.0;
            self.ola_buffer_r[self.ola_read_pos] = 0.0;
            self.ola_read_pos = (self.ola_read_pos + 1) % Self::OLA_BUFFER_SIZE;

            // 3. STFT hop trigger
            self.hop_counter += 1;
            if self.hop_counter >= Self::HOP_SIZE {
                self.hop_counter = 0;

                // Unroll chronological samples from circular buffer and apply analysis window
                for i in 0..Self::FFT_SIZE {
                    let ring_idx = (self.input_write_pos + i) % Self::FFT_SIZE;
                    let w = self.window[i];
                    self.freq_re_l[i] = self.input_ring_l[ring_idx] * w;
                    self.freq_im_l[i] = 0.0;
                    self.freq_re_r[i] = self.input_ring_r[ring_idx] * w;
                    self.freq_im_r[i] = 0.0;
                }

                // Forward FFT
                self.fft.compute(&mut self.freq_re_l, &mut self.freq_im_l, false);
                self.fft.compute(&mut self.freq_re_r, &mut self.freq_im_r, false);

                // 4. Spectral bin processing
                for k in 0..Self::NUM_BINS {
                    let re_l = self.freq_re_l[k];
                    let im_l = self.freq_im_l[k];
                    let re_r = self.freq_re_r[k];
                    let im_r = self.freq_im_r[k];

                    let mag_l = (re_l * re_l + im_l * im_l).sqrt();
                    let mag_r = (re_r * re_r + im_r * im_r).sqrt();
                    let in_phase_l = im_l.atan2(re_l);
                    let in_phase_r = im_r.atan2(re_r);

                    // Asymmetric persistence (immediate attack, ethereal tail)
                    let s_mag_l = if mag_l >= self.prev_mags_l[k] {
                        mag_l
                    } else {
                        alpha_blur * self.prev_mags_l[k] + (1.0 - alpha_blur) * mag_l
                    };
                    let s_mag_r = if mag_r >= self.prev_mags_r[k] {
                        mag_r
                    } else {
                        alpha_blur * self.prev_mags_r[k] + (1.0 - alpha_blur) * mag_r
                    };
                    self.prev_mags_l[k] = s_mag_l;
                    self.prev_mags_r[k] = s_mag_r;

                    self.smooth_mags_l[k] = s_mag_l;
                    self.smooth_mags_r[k] = s_mag_r;

                    // Spectral Tilt
                    let norm_freq = (k as f32) / (Self::NUM_BINS as f32);
                    let tilt_gain = 10.0f32.powf((tilt * (norm_freq - 0.5) * 16.0) * 0.05);
                    let s_mag_l = s_mag_l * tilt_gain;
                    let s_mag_r = s_mag_r * tilt_gain;

                    // Stochastic phase diffusion & stereo decorrelation via LCG RNG
                    self.rng_state_l = self.rng_state_l.wrapping_mul(1664525).wrapping_add(1013904223);
                    self.rng_state_r = self.rng_state_r.wrapping_mul(1664525).wrapping_add(1013904223);

                    let rand_l = (((self.rng_state_l & 0x00FFFFFF) as f32) / 16777216.0 - 0.5) * TAU;
                    let rand_r = (((self.rng_state_r & 0x00FFFFFF) as f32) / 16777216.0 - 0.5) * TAU;

                    let delta_omega = (TAU * ((k * Self::HOP_SIZE) as f32)) / (Self::FFT_SIZE as f32);

                    self.phase_accum_l[k] += delta_omega + rand_l * (phase_diff * 0.75);
                    let decorr_rand = rand_r * decorrel + rand_l * (1.0 - decorrel);
                    self.phase_accum_r[k] += delta_omega + decorr_rand * (phase_diff * 0.75);

                    // Wrap phase in [-PI, +PI]
                    while self.phase_accum_l[k] > PI { self.phase_accum_l[k] -= TAU; }
                    while self.phase_accum_l[k] < -PI { self.phase_accum_l[k] += TAU; }
                    while self.phase_accum_r[k] > PI { self.phase_accum_r[k] -= TAU; }
                    while self.phase_accum_r[k] < -PI { self.phase_accum_r[k] += TAU; }

                    // Smooth blend of natural and diffused phases
                    let final_phase_l = (1.0 - phase_diff) * in_phase_l + phase_diff * self.phase_accum_l[k];
                    let final_phase_r = (1.0 - phase_diff) * in_phase_r + phase_diff * self.phase_accum_r[k];

                    let (sin_l, cos_l) = final_phase_l.sin_cos();
                    let (sin_r, cos_r) = final_phase_r.sin_cos();

                    self.freq_re_l[k] = s_mag_l * cos_l;
                    self.freq_im_l[k] = s_mag_l * sin_l;
                    self.freq_re_r[k] = s_mag_r * cos_r;
                    self.freq_im_r[k] = s_mag_r * sin_r;

                    // Hermitian conjugate symmetry for inverse C2R synthesis
                    if k > 0 && k < Self::NUM_BINS {
                        self.freq_re_l[Self::FFT_SIZE - k] = self.freq_re_l[k];
                        self.freq_im_l[Self::FFT_SIZE - k] = -self.freq_im_l[k];
                        self.freq_re_r[Self::FFT_SIZE - k] = self.freq_re_r[k];
                        self.freq_im_r[Self::FFT_SIZE - k] = -self.freq_im_r[k];
                    }
                }

                // 5. Inverse IFFT
                self.fft.compute(&mut self.freq_re_l, &mut self.freq_im_l, true);
                self.fft.compute(&mut self.freq_re_r, &mut self.freq_im_r, true);

                // 6. Cumulative overlap-add with synthesis window & COLA normalization
                for i in 0..Self::FFT_SIZE {
                    let ola_idx = (self.ola_read_pos + i) % Self::OLA_BUFFER_SIZE;
                    let w = self.window[i];
                    self.ola_buffer_l[ola_idx] += self.freq_re_l[i] * w * COLA_NORMALIZATION;
                    self.ola_buffer_r[ola_idx] += self.freq_re_r[i] * w * COLA_NORMALIZATION;
                }
            }

            // Numerical stability & anti-NaN
            if !wet_l.is_finite() { wet_l = 0.0; }
            if !wet_r.is_finite() { wet_r = 0.0; }

            out_l[s] = denormal_guard(sample_l * (1.0 - mix) + wet_l * mix);
            out_r[s] = denormal_guard(sample_r * (1.0 - mix) + wet_r * mix);
        }
    }
}
