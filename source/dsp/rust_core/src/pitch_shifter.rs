//! Hybrid Pitch Shifter DSP Kernel (DSP #27)
//! Rules 3, 5, 8, 9, 13, 14, 17, 34, 46, 47: Zero allocations in audio thread.
//! Dual-Algorithm Architecture:
//! - Mode 0: Dual-head time-domain delay with 4-point Hermite cubic interpolation (<5ms latency).
//! - Mode 1: Spectral Phase Vocoder HD (1024-point STFT, 75% OLA) with Laroche & Dolson Identity Phase Locking.

use std::f32::consts::{PI, TAU};
use crate::common::denormal_guard;
use crate::fft::RustFFT;

#[inline]
fn wrap_pi(mut x: f32) -> f32 {
    while x > PI {
        x -= TAU;
    }
    while x < -PI {
        x += TAU;
    }
    x
}

struct RustPhaseVocoderChannel {
    fft: RustFFT,
    fft_size: usize,
    hop_size: usize,
    ring_size: usize,
    ring_mask: usize,
    ola_norm: f32,

    input_ring: Box<[f32]>,
    output_ring: Box<[f32]>,
    analysis_frame: Box<[f32]>,
    resynth_frame: Box<[f32]>,
    ola_buffer: Box<[f32]>,
    window: Box<[f32]>,

    current_mag: Box<[f32]>,
    current_phase: Box<[f32]>,
    prev_input_phase: Box<[f32]>,
    synth_phase: Box<[f32]>,
    synth_peak_phase: Box<[f32]>,
    omega: Box<[f32]>,
    target_mag: Box<[f32]>,
    target_phase: Box<[f32]>,

    freq_re: Box<[f32]>,
    freq_im: Box<[f32]>,

    peaks: Box<[usize]>,
    peak_for_bin: Box<[usize]>,

    input_write_pos: usize,
    output_read_pos: usize,
    output_write_pos: usize,
    output_available: usize,
    hop_counter: usize,
}

impl RustPhaseVocoderChannel {
    fn new(fft_size: usize, hop_size: usize) -> Self {
        let num_bins = (fft_size / 2) + 1;
        let ring_size = fft_size * 4;
        let ring_mask = ring_size - 1;

        let mut window = vec![0.0f32; fft_size].into_boxed_slice();
        for i in 0..fft_size {
            window[i] = 0.5 * (1.0 - (TAU * (i as f32) / (fft_size as f32)).cos());
        }

        Self {
            fft: RustFFT::new(fft_size),
            fft_size,
            hop_size,
            ring_size,
            ring_mask,
            ola_norm: 1.0 / 1.5, // 2/3 for 75% Hann OLA

            input_ring: vec![0.0f32; ring_size].into_boxed_slice(),
            output_ring: vec![0.0f32; ring_size].into_boxed_slice(),
            analysis_frame: vec![0.0f32; fft_size].into_boxed_slice(),
            resynth_frame: vec![0.0f32; fft_size].into_boxed_slice(),
            ola_buffer: vec![0.0f32; fft_size * 2].into_boxed_slice(),
            window,

            current_mag: vec![0.0f32; num_bins].into_boxed_slice(),
            current_phase: vec![0.0f32; num_bins].into_boxed_slice(),
            prev_input_phase: vec![0.0f32; num_bins].into_boxed_slice(),
            synth_phase: vec![0.0f32; num_bins].into_boxed_slice(),
            synth_peak_phase: vec![0.0f32; num_bins].into_boxed_slice(),
            omega: vec![0.0f32; num_bins].into_boxed_slice(),
            target_mag: vec![0.0f32; num_bins].into_boxed_slice(),
            target_phase: vec![0.0f32; num_bins].into_boxed_slice(),

            freq_re: vec![0.0f32; fft_size].into_boxed_slice(),
            freq_im: vec![0.0f32; fft_size].into_boxed_slice(),

            peaks: vec![0usize; num_bins].into_boxed_slice(),
            peak_for_bin: vec![0usize; num_bins].into_boxed_slice(),

            input_write_pos: 0,
            output_read_pos: 0,
            output_write_pos: 0,
            output_available: 0,
            hop_counter: 0,
        }
    }

    fn reset(&mut self) {
        self.input_ring.fill(0.0);
        self.output_ring.fill(0.0);
        self.analysis_frame.fill(0.0);
        self.resynth_frame.fill(0.0);
        self.ola_buffer.fill(0.0);
        self.current_mag.fill(0.0);
        self.current_phase.fill(0.0);
        self.prev_input_phase.fill(0.0);
        self.synth_phase.fill(0.0);
        self.synth_peak_phase.fill(0.0);
        self.omega.fill(0.0);
        self.target_mag.fill(0.0);
        self.target_phase.fill(0.0);
        self.freq_re.fill(0.0);
        self.freq_im.fill(0.0);
        self.peaks.fill(0);
        self.peak_for_bin.fill(0);

        self.input_write_pos = 0;
        self.output_read_pos = 0;
        self.output_write_pos = 0;
        self.output_available = 0;
        self.hop_counter = 0;
    }

    fn process_sample(&mut self, in_sample: f32, pitch_ratio: f32, phase_locking: f32) -> f32 {
        let clamped_pitch = pitch_ratio.clamp(0.25, 4.0);
        let clamped_locking = phase_locking.clamp(0.0, 1.0);
        let num_bins = (self.fft_size / 2) + 1;
        let bin_freq_factor = TAU * (self.hop_size as f32) / (self.fft_size as f32);

        self.input_ring[self.input_write_pos] = in_sample;
        self.input_write_pos = (self.input_write_pos + 1) & self.ring_mask;
        self.hop_counter += 1;

        if self.hop_counter >= self.hop_size {
            self.hop_counter = 0;

            // 1. Extract analysis window
            let read_start = (self.input_write_pos + self.ring_size - self.fft_size) & self.ring_mask;
            for i in 0..self.fft_size {
                let idx = (read_start + i) & self.ring_mask;
                self.freq_re[i] = self.input_ring[idx] * self.window[i];
                self.freq_im[i] = 0.0;
            }

            // 2. Forward FFT
            self.fft.compute(&mut self.freq_re, &mut self.freq_im, false);

            // 3. Extract magnitudes and phases
            for k in 0..num_bins {
                let re = self.freq_re[k];
                let im = self.freq_im[k];
                self.current_mag[k] = (re * re + im * im).sqrt();
                self.current_phase[k] = im.atan2(re);
            }

            // 4. Prominent peak detection
            let mut num_peaks = 0;
            for k in 1..(num_bins - 1) {
                if self.current_mag[k] > self.current_mag[k - 1]
                    && self.current_mag[k] > self.current_mag[k + 1]
                    && self.current_mag[k] > 1.0e-5
                {
                    self.peaks[num_peaks] = k;
                    num_peaks += 1;
                }
            }

            // 5. Peak influence region assignment
            if num_peaks == 0 {
                for k in 0..num_bins {
                    self.peak_for_bin[k] = k;
                }
            } else {
                for p in 0..num_peaks {
                    let start = if p == 0 {
                        0
                    } else {
                        (self.peaks[p - 1] + self.peaks[p]) / 2 + 1
                    };
                    let end = if p + 1 == num_peaks {
                        num_bins - 1
                    } else {
                        (self.peaks[p] + self.peaks[p + 1]) / 2
                    };
                    let peak_k = self.peaks[p];
                    for k in start..=end {
                        self.peak_for_bin[k] = peak_k;
                    }
                }
            }

            // 6. Peak phase propagation (unwrapped instantaneous frequency)
            for p in 0..num_peaks {
                let pk = self.peaks[p];
                let delta_phi = self.current_phase[pk] - self.prev_input_phase[pk];
                let nominal_omega = (pk as f32) * bin_freq_factor;
                let dev = wrap_pi(delta_phi - nominal_omega);
                let true_omega = nominal_omega + dev;
                self.omega[pk] = true_omega;

                let synth_advance = clamped_pitch * true_omega;
                self.synth_peak_phase[pk] = wrap_pi(self.synth_peak_phase[pk] + synth_advance);
            }

            // 7. Spectral mapping with Identity Phase Locking
            for k in 0..num_bins {
                let src_bin = (k as f32) / clamped_pitch;
                if src_bin >= (num_bins - 1) as f32 {
                    self.target_mag[k] = 0.0;
                    self.target_phase[k] = 0.0;
                    continue;
                }

                let s0 = src_bin as usize;
                let frac = src_bin - (s0 as f32);
                self.target_mag[k] = (1.0 - frac) * self.current_mag[s0] + frac * self.current_mag[s0 + 1];

                // Rigid locked phase relative to governing peak
                let pk = self.peak_for_bin[s0];
                let locked_phase = wrap_pi(self.synth_peak_phase[pk] + (self.current_phase[s0] - self.current_phase[pk]));

                // Independent phase advance fallback
                let delta_phi_s = self.current_phase[s0] - self.prev_input_phase[s0];
                let nominal_omega_s = (s0 as f32) * bin_freq_factor;
                let dev_s = wrap_pi(delta_phi_s - nominal_omega_s);
                let indep_phase = wrap_pi(self.synth_phase[k] + clamped_pitch * (nominal_omega_s + dev_s));

                self.target_phase[k] = (1.0 - clamped_locking) * indep_phase + clamped_locking * locked_phase;
                self.synth_phase[k] = self.target_phase[k];
            }

            self.prev_input_phase[..num_bins].copy_from_slice(&self.current_phase[..num_bins]);

            // 8. Reconstruct complex spectrum with Hermitian symmetry
            for k in 0..num_bins {
                let (sin_k, cos_k) = self.target_phase[k].sin_cos();
                self.freq_re[k] = self.target_mag[k] * cos_k;
                self.freq_im[k] = self.target_mag[k] * sin_k;
            }
            for k in 1..(num_bins - 1) {
                self.freq_re[self.fft_size - k] = self.freq_re[k];
                self.freq_im[self.fft_size - k] = -self.freq_im[k];
            }

            // 9. Inverse FFT
            self.fft.compute(&mut self.freq_re, &mut self.freq_im, true);

            // 10. Overlap-Add
            for i in 0..self.fft_size {
                self.ola_buffer[i] += self.freq_re[i] * self.window[i] * self.ola_norm;
            }

            // 11. Dispatch to output ring buffer
            for i in 0..self.hop_size {
                self.output_ring[self.output_write_pos] = denormal_guard(self.ola_buffer[i]);
                self.output_write_pos = (self.output_write_pos + 1) & self.ring_mask;
            }
            self.output_available = (self.output_available + self.hop_size).min(self.ring_size);

            // Shift OLA buffer
            self.ola_buffer.copy_within(self.hop_size..(self.fft_size * 2), 0);
            self.ola_buffer[(self.fft_size * 2 - self.hop_size)..].fill(0.0);
        }

        // Lock-free output readout
        if self.output_available > 0 {
            let out_val = self.output_ring[self.output_read_pos];
            self.output_read_pos = (self.output_read_pos + 1) & self.ring_mask;
            self.output_available -= 1;
            out_val
        } else {
            0.0
        }
    }
}

pub struct RustPitchShifter {
    sample_rate: f32,

    // Time-domain dual-head delay buffers (65536 capacity, mask 65535)
    delay_l: Box<[f32]>,
    delay_r: Box<[f32]>,
    delay_mask: usize,
    write_pos: usize,
    phase: f32,

    // Phase Vocoder channels
    vocoder_l: RustPhaseVocoderChannel,
    vocoder_r: RustPhaseVocoderChannel,

    target_semitones: f32,
    target_fine: f32,
    target_window_ms: f32,
    target_mix: f32,
    target_algorithm: f32,
    target_phase_locking: f32,
}

impl RustPitchShifter {
    const DELAY_CAPACITY: usize = 65536;

    pub fn new() -> Self {
        let delay_mask = Self::DELAY_CAPACITY - 1;
        Self {
            sample_rate: 44100.0,
            delay_l: vec![0.0f32; Self::DELAY_CAPACITY].into_boxed_slice(),
            delay_r: vec![0.0f32; Self::DELAY_CAPACITY].into_boxed_slice(),
            delay_mask,
            write_pos: 0,
            phase: 0.0,

            vocoder_l: RustPhaseVocoderChannel::new(1024, 256),
            vocoder_r: RustPhaseVocoderChannel::new(1024, 256),

            target_semitones: 0.0,
            target_fine: 0.0,
            target_window_ms: 45.0,
            target_mix: 1.0,
            target_algorithm: 0.0,
            target_phase_locking: 1.0,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn reset(&mut self) {
        self.delay_l.fill(0.0);
        self.delay_r.fill(0.0);
        self.write_pos = 0;
        self.phase = 0.0;

        self.vocoder_l.reset();
        self.vocoder_r.reset();
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_semitones = value.clamp(-24.0, 24.0),
            2 => self.target_fine = value.clamp(-100.0, 100.0),
            3 => self.target_window_ms = value.clamp(10.0, 120.0),
            4 => self.target_mix = value.clamp(0.0, 1.0),
            5 => self.target_algorithm = value.clamp(0.0, 1.0),
            6 => self.target_phase_locking = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_semitones,
            2 => self.target_fine,
            3 => self.target_window_ms,
            4 => self.target_mix,
            5 => self.target_algorithm,
            6 => self.target_phase_locking,
            _ => 0.0,
        }
    }

    #[inline]
    fn read_cubic(buf: &[f32], mask: usize, write_idx: usize, delay_samples: f32) -> f32 {
        let clamped_delay = delay_samples.clamp(1.0, 32768.0);
        let read_pos = (write_idx as f32) - clamped_delay;
        let i_pos = read_pos.floor() as isize;
        let frac = read_pos - (i_pos as f32);

        let idx0 = ((i_pos - 1) as usize) & mask;
        let idx1 = (i_pos as usize) & mask;
        let idx2 = ((i_pos + 1) as usize) & mask;
        let idx3 = ((i_pos + 2) as usize) & mask;

        let xm1 = buf[idx0];
        let x0  = buf[idx1];
        let x1  = buf[idx2];
        let x2  = buf[idx3];

        let c0 = x0;
        let c1 = 0.5 * (x1 - xm1);
        let c2 = xm1 - 2.5 * x0 + 2.0 * x1 - 0.5 * x2;
        let c3 = 0.5 * (x2 - xm1) + 1.5 * (x0 - x1);

        ((c3 * frac + c2) * frac + c1) * frac + c0
    }

    #[inline]
    pub fn process(&mut self, in_l: &[f32], in_r: &[f32], out_l: &mut [f32], out_r: &mut [f32]) {
        let total_semitones = self.target_semitones + self.target_fine * 0.01;
        let pitch_ratio = 2.0f32.powf(total_semitones / 12.0);
        let mix = self.target_mix;
        let num_samples = in_l.len();

        if self.target_algorithm >= 0.5 {
            // Mode 1: Phase Vocoder HD with Laroche-Dolson Identity Phase Locking
            let phase_locking = self.target_phase_locking;
            for s in 0..num_samples {
                let sample_l = denormal_guard(in_l[s]);
                let sample_r = denormal_guard(in_r[s]);

                let wet_l = self.vocoder_l.process_sample(sample_l, pitch_ratio, phase_locking);
                let wet_r = self.vocoder_r.process_sample(sample_r, pitch_ratio, phase_locking);

                out_l[s] = denormal_guard(sample_l * (1.0 - mix) + wet_l * mix);
                out_r[s] = denormal_guard(sample_r * (1.0 - mix) + wet_r * mix);
            }
        } else {
            // Mode 0: Dual-head time-domain delay with Hermite cubic interpolation
            let win_size = (self.sample_rate * self.target_window_ms * 0.001).max(64.0);
            let half_win = win_size * 0.5;
            let rate = 1.0 - pitch_ratio;
            let mask = self.delay_mask;

            for s in 0..num_samples {
                let sample_l = denormal_guard(in_l[s]);
                let sample_r = denormal_guard(in_r[s]);

                self.delay_l[self.write_pos] = sample_l;
                self.delay_r[self.write_pos] = sample_r;

                // Head 1
                let mut d1 = self.phase;
                while d1 < 0.0 { d1 += win_size; }
                while d1 >= win_size { d1 -= win_size; }

                // Head 2 (180 degrees out of phase)
                let mut d2 = self.phase + half_win;
                while d2 < 0.0 { d2 += win_size; }
                while d2 >= win_size { d2 -= win_size; }

                // Triangular crossfade envelopes
                let env1 = 1.0 - ((2.0 * d1 / win_size) - 1.0).abs();
                let env2 = 1.0 - ((2.0 * d2 / win_size) - 1.0).abs();

                let wet_l = Self::read_cubic(&self.delay_l, mask, self.write_pos, d1 + 10.0) * env1
                    + Self::read_cubic(&self.delay_l, mask, self.write_pos, d2 + 10.0) * env2;
                let wet_r = Self::read_cubic(&self.delay_r, mask, self.write_pos, d1 + 10.0) * env1
                    + Self::read_cubic(&self.delay_r, mask, self.write_pos, d2 + 10.0) * env2;

                self.phase += rate;
                if self.phase >= win_size {
                    self.phase -= win_size;
                } else if self.phase < 0.0 {
                    self.phase += win_size;
                }

                self.write_pos = (self.write_pos + 1) & mask;

                out_l[s] = denormal_guard(sample_l * (1.0 - mix) + wet_l * mix);
                out_r[s] = denormal_guard(sample_r * (1.0 - mix) + wet_r * mix);
            }
        }
    }
}
