//! Spectral Processor DSP Kernel (DSP #29)
//! Rules 5, 8, 9, 13, 14, 16, 19, 34, 46, 47: Zero allocations in audio thread,
//! 512-point Radix-2 FFT spectral analysis, threshold-based spectral gating,
//! tilt frequency weighting, Hermitian conjugate synthesis, and denormal protection.

use crate::common::denormal_guard;
use crate::fft::RustFFT;

pub struct RustSpectralProcessor {
    fft: RustFFT,
    time_in_l: Box<[f32]>,
    time_in_r: Box<[f32]>,
    time_out_l: Box<[f32]>,
    time_out_r: Box<[f32]>,
    freq_re_l: Box<[f32]>,
    freq_im_l: Box<[f32]>,
    freq_re_r: Box<[f32]>,
    freq_im_r: Box<[f32]>,
    buffer_pos: usize,

    target_gate_db: f32,
    target_tilt: f32,
    target_mix: f32,
}

impl RustSpectralProcessor {
    pub fn new() -> Self {
        let size = 512;
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
            buffer_pos: 0,
            target_gate_db: -50.0,
            target_tilt: 0.0,
            target_mix: 1.0,
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
        self.buffer_pos = 0;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_gate_db = value.clamp(-80.0, 0.0),
            2 => self.target_tilt = value.clamp(-6.0, 6.0),
            3 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_gate_db,
            2 => self.target_tilt,
            3 => self.target_mix,
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

        let mix = self.target_mix;
        let fft_size = 512;
        let num_bins = fft_size / 2;
        let gate_threshold = 10.0f32.powf(self.target_gate_db * 0.05);
        let tilt = self.target_tilt;

        for s in 0..num_samples {
            let in_l_val = in_l[s];
            let in_r_val = if s < in_r.len() { in_r[s] } else { in_l_val };

            self.time_in_l[self.buffer_pos] = in_l_val;
            self.time_in_r[self.buffer_pos] = in_r_val;

            let wet_l = self.time_out_l[self.buffer_pos];
            let wet_r = self.time_out_r[self.buffer_pos];

            self.buffer_pos += 1;
            if self.buffer_pos >= fft_size {
                self.buffer_pos = 0;

                // 1. Forward FFT (no windowing, consistent with C++ implementation)
                self.freq_re_l.copy_from_slice(&self.time_in_l);
                self.freq_im_l.fill(0.0);
                self.fft.compute(&mut self.freq_re_l, &mut self.freq_im_l, false);

                self.freq_re_r.copy_from_slice(&self.time_in_r);
                self.freq_im_r.fill(0.0);
                self.fft.compute(&mut self.freq_re_r, &mut self.freq_im_r, false);

                // 2. Frequency domain processing: Gating and Spectral Tilt
                for k in 0..num_bins {
                    let rl = self.freq_re_l[k];
                    let il = self.freq_im_l[k];
                    let mag_l = (rl * rl + il * il).sqrt();

                    let rr = self.freq_re_r[k];
                    let ir = self.freq_im_r[k];
                    let mag_r = (rr * rr + ir * ir).sqrt();

                    let norm_freq = (k as f32) / (num_bins as f32);
                    let tilt_gain = 10.0f32.powf((tilt * (norm_freq - 0.5)) * 0.05);

                    if mag_l < gate_threshold {
                        self.freq_re_l[k] *= 0.05;
                        self.freq_im_l[k] *= 0.05;
                    } else {
                        self.freq_re_l[k] *= tilt_gain;
                        self.freq_im_l[k] *= tilt_gain;
                    }

                    if mag_r < gate_threshold {
                        self.freq_re_r[k] *= 0.05;
                        self.freq_im_r[k] *= 0.05;
                    } else {
                        self.freq_re_r[k] *= tilt_gain;
                        self.freq_im_r[k] *= tilt_gain;
                    }

                    // Conjugate symmetry for upper half
                    if k > 0 && k < num_bins {
                        self.freq_re_l[fft_size - k] = self.freq_re_l[k];
                        self.freq_im_l[fft_size - k] = -self.freq_im_l[k];

                        self.freq_re_r[fft_size - k] = self.freq_re_r[k];
                        self.freq_im_r[fft_size - k] = -self.freq_im_r[k];
                    }
                }

                // 3. Inverse FFT
                self.fft.compute(&mut self.freq_re_l, &mut self.freq_im_l, true);
                self.time_out_l.copy_from_slice(&self.freq_re_l);

                self.fft.compute(&mut self.freq_re_r, &mut self.freq_im_r, true);
                self.time_out_r.copy_from_slice(&self.freq_re_r);
            }

            out_l[s] = denormal_guard(in_l_val * (1.0 - mix) + wet_l * mix);
            if s < out_r.len() {
                out_r[s] = denormal_guard(in_r_val * (1.0 - mix) + wet_r * mix);
            }
        }
    }
}
