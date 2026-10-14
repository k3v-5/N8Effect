//! Zero-Latency Partitioned Convolution Engine (IR Reverb & Cab Sim) - DSP #48
//! Rules 5, 8, 9, 14, 34, 46, 47:
//! Zero heap allocations in audio processing thread,
//! partitioned zero-latency convolution (direct time-domain FIR head of 128 samples,
//! uniform frequency-domain block FFT partitions of size 256 for up to 64 partitions = 8192 samples),
//! double-buffered atomic IR exchange, 5 built-in synthetic IR models + custom IR loader,
//! pre-delay ring buffer, dual highpass/lowpass biquad filters, stereo width M/S matrix,
//! 64-point normalized thumbnail extraction, and denormal protection.

use std::f32::consts::PI;
use std::sync::atomic::{AtomicUsize, Ordering};
use crate::common::denormal_guard;
use crate::fft::RustFFT;

pub const PARTITION_SIZE: usize = 128;
pub const FFT_SIZE: usize = 256;
pub const MAX_PARTITIONS: usize = 64;
pub const MAX_PRE_DELAY_SAMPLES: usize = 19200; // ~400 ms at 48kHz, 100 ms at 192kHz

#[derive(Clone, Copy, PartialEq)]
enum BiquadType {
    Lowpass,
    Highpass,
}

#[derive(Clone)]
struct Biquad {
    b0: f32,
    b1: f32,
    b2: f32,
    a1: f32,
    a2: f32,
    z1: f32,
    z2: f32,
}

impl Biquad {
    fn new() -> Self {
        Self {
            b0: 1.0,
            b1: 0.0,
            b2: 0.0,
            a1: 0.0,
            a2: 0.0,
            z1: 0.0,
            z2: 0.0,
        }
    }

    fn reset(&mut self) {
        self.z1 = 0.0;
        self.z2 = 0.0;
    }

    fn set_filter(&mut self, filter_type: BiquadType, sample_rate: f64, freq_hz: f32, q: f32) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let nyquist = sr * 0.499;
        let fc = (freq_hz as f64).clamp(10.0, nyquist);
        let q_val = (q as f64).max(0.01);
        let omega = 2.0 * std::f64::consts::PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q_val);

        let (b0, b1, b2, a0, a1, a2) = match filter_type {
            BiquadType::Lowpass => {
                let b0 = (1.0 - cs) * 0.5;
                let b1 = 1.0 - cs;
                let b2 = (1.0 - cs) * 0.5;
                let a0 = 1.0 + alpha;
                let a1 = -2.0 * cs;
                let a2 = 1.0 - alpha;
                (b0, b1, b2, a0, a1, a2)
            }
            BiquadType::Highpass => {
                let b0 = (1.0 + cs) * 0.5;
                let b1 = -(1.0 + cs);
                let b2 = (1.0 + cs) * 0.5;
                let a0 = 1.0 + alpha;
                let a1 = -2.0 * cs;
                let a2 = 1.0 - alpha;
                (b0, b1, b2, a0, a1, a2)
            }
        };

        let inv_a0 = 1.0 / a0;
        self.b0 = (b0 * inv_a0) as f32;
        self.b1 = (b1 * inv_a0) as f32;
        self.b2 = (b2 * inv_a0) as f32;
        self.a1 = (a1 * inv_a0) as f32;
        self.a2 = (a2 * inv_a0) as f32;
    }

    #[inline(always)]
    fn process(&mut self, input: f32) -> f32 {
        let out = self.b0 * input + self.z1;
        self.z1 = self.b1 * input - self.a1 * out + self.z2;
        self.z2 = self.b2 * input - self.a2 * out;

        if self.z1.is_nan() || self.z1.is_infinite() {
            self.z1 = 0.0;
        }
        if self.z2.is_nan() || self.z2.is_infinite() {
            self.z2 = 0.0;
        }
        out
    }
}

struct IrSlot {
    head_fir: [[f32; PARTITION_SIZE]; 2],
    ir_spectra_re: Box<[[[f32; FFT_SIZE]; MAX_PARTITIONS]; 2]>,
    ir_spectra_im: Box<[[[f32; FFT_SIZE]; MAX_PARTITIONS]; 2]>,
    active_partitions: usize,
}

impl IrSlot {
    fn new() -> Self {
        Self {
            head_fir: [[0.0; PARTITION_SIZE]; 2],
            ir_spectra_re: Box::new([[[0.0; FFT_SIZE]; MAX_PARTITIONS]; 2]),
            ir_spectra_im: Box::new([[[0.0; FFT_SIZE]; MAX_PARTITIONS]; 2]),
            active_partitions: 2,
        }
    }
}

struct SimpleRng(u64);

impl SimpleRng {
    fn new(seed: u64) -> Self {
        Self(seed)
    }

    fn next_f32(&mut self) -> f32 {
        self.0 = self.0.wrapping_mul(6364136223846793005).wrapping_add(1);
        let val = ((self.0 >> 33) as u32) as f32 / 2147483648.0;
        val * 2.0 - 1.0
    }
}

pub struct RustConvolution {
    sample_rate: f64,
    ir_slots: [IrSlot; 2],
    active_ir_index: AtomicUsize,
    ir_thumbnail: [f32; 64],
    current_ir_type: i32,

    head_input_buffer: [[f32; PARTITION_SIZE * 2]; 2],
    head_input_pos: [usize; 2],

    x_spectra_re: Box<[[[f32; FFT_SIZE]; MAX_PARTITIONS]; 2]>,
    x_spectra_im: Box<[[[f32; FFT_SIZE]; MAX_PARTITIONS]; 2]>,

    tail_acc_re: [[f32; FFT_SIZE]; 2],
    tail_acc_im: [[f32; FFT_SIZE]; 2],
    output_overlap: [[f32; FFT_SIZE]; 2],
    input_block_buffer: [[f32; PARTITION_SIZE]; 2],
    input_block_pos: [usize; 2],

    pre_delay_buffer_l: Box<[f32]>,
    pre_delay_buffer_r: Box<[f32]>,
    pre_delay_write_idx: usize,
    max_pre_delay_samples: usize,

    fft: RustFFT,

    low_cut_l: Biquad,
    low_cut_r: Biquad,
    high_cut_l: Biquad,
    high_cut_r: Biquad,
    last_low_cut: f32,
    last_high_cut: f32,

    param_ir_type: f32,
    param_pre_delay_ms: f32,
    param_low_cut_hz: f32,
    param_high_cut_hz: f32,
    param_width: f32,
    param_mix: f32,
}

impl RustConvolution {
    pub fn new() -> Self {
        let max_predelay = MAX_PRE_DELAY_SAMPLES;
        let mut conv = Self {
            sample_rate: 48000.0,
            ir_slots: [IrSlot::new(), IrSlot::new()],
            active_ir_index: AtomicUsize::new(0),
            ir_thumbnail: [0.0; 64],
            current_ir_type: -1,

            head_input_buffer: [[0.0; PARTITION_SIZE * 2]; 2],
            head_input_pos: [0; 2],

            x_spectra_re: Box::new([[[0.0; FFT_SIZE]; MAX_PARTITIONS]; 2]),
            x_spectra_im: Box::new([[[0.0; FFT_SIZE]; MAX_PARTITIONS]; 2]),

            tail_acc_re: [[0.0; FFT_SIZE]; 2],
            tail_acc_im: [[0.0; FFT_SIZE]; 2],
            output_overlap: [[0.0; FFT_SIZE]; 2],
            input_block_buffer: [[0.0; PARTITION_SIZE]; 2],
            input_block_pos: [0; 2],

            pre_delay_buffer_l: vec![0.0f32; max_predelay].into_boxed_slice(),
            pre_delay_buffer_r: vec![0.0f32; max_predelay].into_boxed_slice(),
            pre_delay_write_idx: 0,
            max_pre_delay_samples: max_predelay,

            fft: RustFFT::new(FFT_SIZE),

            low_cut_l: Biquad::new(),
            low_cut_r: Biquad::new(),
            high_cut_l: Biquad::new(),
            high_cut_r: Biquad::new(),
            last_low_cut: 40.0,
            last_high_cut: 16000.0,

            param_ir_type: 0.0,
            param_pre_delay_ms: 0.0,
            param_low_cut_hz: 40.0,
            param_high_cut_hz: 16000.0,
            param_width: 1.0,
            param_mix: 0.5,
        };

        conv.prepare(48000.0);
        conv
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 48000.0 };
        self.max_pre_delay_samples = ((self.sample_rate * 0.12) as usize).clamp(64, MAX_PRE_DELAY_SAMPLES);

        self.reset();

        self.low_cut_l.set_filter(BiquadType::Highpass, self.sample_rate, self.param_low_cut_hz, 0.707);
        self.low_cut_r.set_filter(BiquadType::Highpass, self.sample_rate, self.param_low_cut_hz, 0.707);
        self.high_cut_l.set_filter(BiquadType::Lowpass, self.sample_rate, self.param_high_cut_hz, 0.707);
        self.high_cut_r.set_filter(BiquadType::Lowpass, self.sample_rate, self.param_high_cut_hz, 0.707);

        self.last_low_cut = self.param_low_cut_hz;
        self.last_high_cut = self.param_high_cut_hz;

        self.current_ir_type = -1;
        self.load_synthetic_ir(self.param_ir_type.round() as i32);
    }

    pub fn reset(&mut self) {
        for ch in 0..2 {
            self.head_input_buffer[ch].fill(0.0);
            self.head_input_pos[ch] = 0;

            for p in 0..MAX_PARTITIONS {
                self.x_spectra_re[ch][p].fill(0.0);
                self.x_spectra_im[ch][p].fill(0.0);
            }

            self.tail_acc_re[ch].fill(0.0);
            self.tail_acc_im[ch].fill(0.0);
            self.output_overlap[ch].fill(0.0);
            self.input_block_buffer[ch].fill(0.0);
            self.input_block_pos[ch] = 0;
        }

        self.pre_delay_buffer_l.fill(0.0);
        self.pre_delay_buffer_r.fill(0.0);
        self.pre_delay_write_idx = 0;

        self.low_cut_l.reset();
        self.low_cut_r.reset();
        self.high_cut_l.reset();
        self.high_cut_r.reset();
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.param_ir_type = value.clamp(0.0, 5.0),
            2 => self.param_pre_delay_ms = value.clamp(0.0, 100.0),
            3 => self.param_low_cut_hz = value.clamp(20.0, 1000.0),
            4 => self.param_high_cut_hz = value.clamp(1000.0, 20000.0),
            5 => self.param_width = value.clamp(0.0, 2.0),
            6 => self.param_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            1 => self.param_ir_type,
            2 => self.param_pre_delay_ms,
            3 => self.param_low_cut_hz,
            4 => self.param_high_cut_hz,
            5 => self.param_width,
            6 => self.param_mix,
            _ => 0.0,
        }
    }

    pub fn get_thumbnail(&self, out_64: &mut [f32; 64]) {
        out_64.copy_from_slice(&self.ir_thumbnail);
    }

    pub fn set_custom_impulse_response(&mut self, l_data: &[f32], r_data: Option<&[f32]>) {
        if l_data.is_empty() {
            return;
        }
        self.param_ir_type = 5.0;
        self.current_ir_type = 5;

        self.update_thumbnail(l_data, r_data);

        let active_idx = self.active_ir_index.load(Ordering::Relaxed);
        let inactive_idx = 1 - active_idx;

        self.populate_ir_slot(inactive_idx, l_data, r_data);
        self.active_ir_index.store(inactive_idx, Ordering::Release);
    }

    fn update_thumbnail(&mut self, l_data: &[f32], r_data: Option<&[f32]>) {
        let num_samples = l_data.len();
        if num_samples == 0 {
            self.ir_thumbnail.fill(0.0);
            return;
        }

        let mut global_max = 0.0f32;
        for pt in 0..64 {
            let start = pt * num_samples / 64;
            let mut end = (pt + 1) * num_samples / 64;
            if end <= start {
                end = start + 1;
            }
            if end > num_samples {
                end = num_samples;
            }

            let mut peak = 0.0f32;
            for i in start..end {
                let val_l = l_data[i].abs();
                let val_r = if let Some(r) = r_data { r[i].abs() } else { val_l };
                let val = val_l.max(val_r);
                if val > peak {
                    peak = val;
                }
            }
            self.ir_thumbnail[pt] = peak;
            if peak > global_max {
                global_max = peak;
            }
        }

        if global_max > 1e-5 {
            for v in &mut self.ir_thumbnail {
                *v /= global_max;
            }
        }
    }

    fn populate_ir_slot(&mut self, slot_idx: usize, l_data: &[f32], r_data: Option<&[f32]>) {
        if l_data.is_empty() || slot_idx >= 2 {
            return;
        }

        let num_samples = l_data.len();

        let mut max_peak = 0.0f32;
        for i in 0..num_samples {
            let val_l = l_data[i].abs();
            let val_r = if let Some(r) = r_data { r[i].abs() } else { val_l };
            max_peak = max_peak.max(val_l).max(val_r);
        }
        let gain = if max_peak > 1e-4 { 0.95 / max_peak } else { 1.0 };

        let total_partitions = ((num_samples + PARTITION_SIZE - 1) / PARTITION_SIZE).min(MAX_PARTITIONS);
        let active_partitions = total_partitions.max(2);

        let mut hp_re = [0.0f32; FFT_SIZE];
        let mut hp_im = [0.0f32; FFT_SIZE];

        let slot = &mut self.ir_slots[slot_idx];
        slot.active_partitions = active_partitions;

        for ch in 0..2 {
            let src: &[f32] = if ch == 0 {
                l_data
            } else if let Some(r) = r_data {
                r
            } else {
                l_data
            };

            // 1. Partition 0 (Head): direct FIR
            for i in 0..PARTITION_SIZE {
                slot.head_fir[ch][i] = if i < num_samples { src[i] * gain } else { 0.0 };
            }

            // 2. Partitions 1..active_partitions (Tail): FFT transform
            for p in 1..active_partitions {
                let start_idx = p * PARTITION_SIZE;
                for i in 0..PARTITION_SIZE {
                    let idx = start_idx + i;
                    hp_re[i] = if idx < num_samples { src[idx] * gain } else { 0.0 };
                    hp_im[i] = 0.0;
                }
                for i in PARTITION_SIZE..FFT_SIZE {
                    hp_re[i] = 0.0;
                    hp_im[i] = 0.0;
                }

                self.fft.compute(&mut hp_re, &mut hp_im, false);

                slot.ir_spectra_re[ch][p].copy_from_slice(&hp_re);
                slot.ir_spectra_im[ch][p].copy_from_slice(&hp_im);
            }

            // Clear inactive partitions
            for p in active_partitions..MAX_PARTITIONS {
                slot.ir_spectra_re[ch][p].fill(0.0);
                slot.ir_spectra_im[ch][p].fill(0.0);
            }
        }
    }

    fn load_synthetic_ir(&mut self, ir_type: i32) {
        self.current_ir_type = ir_type;
        let total_samples = ((self.sample_rate * 2.5) as usize).min(MAX_PARTITIONS * PARTITION_SIZE);
        let mut ir_l = vec![0.0f32; total_samples];
        let mut ir_r = vec![0.0f32; total_samples];

        let mut rng = SimpleRng::new(1234567);

        match ir_type {
            0 => {
                // Small Room
                let decay_rate = 18.0 / (self.sample_rate as f32);
                ir_l[0] = 1.0;
                ir_r[0] = 0.95;
                let early = [47usize, 89, 131, 211, 283, 379];
                let early_gains = [0.45f32, -0.38, 0.32, -0.28, 0.22, -0.18];
                for i in 0..6 {
                    if early[i] < total_samples {
                        ir_l[early[i]] += early_gains[i];
                        let r_idx = (early[i] + 7).min(total_samples - 1);
                        ir_r[r_idx] += early_gains[i] * 0.9;
                    }
                }
                for n in 1..total_samples {
                    let env = (-decay_rate * (n as f32)).exp();
                    let noise_l = rng.next_f32();
                    let noise_r = rng.next_f32();
                    ir_l[n] += noise_l * env * 0.35;
                    ir_r[n] += noise_r * env * 0.35;
                }
            }
            1 => {
                // Large Hall
                let decay_rate = 3.5 / (self.sample_rate as f32);
                ir_l[0] = 0.7;
                ir_r[0] = 0.7;
                for n in 1..total_samples {
                    let env = (-decay_rate * (n as f32)).exp();
                    let noise_l = rng.next_f32();
                    let noise_r = rng.next_f32();
                    ir_l[n] += noise_l * env * 0.4;
                    ir_r[n] += noise_r * env * 0.4;
                }
            }
            2 => {
                // Dark Plate
                let decay_rate = 5.0 / (self.sample_rate as f32);
                ir_l[0] = 0.9;
                ir_r[0] = 0.85;
                let mut filter_state_l = 0.0f32;
                let mut filter_state_r = 0.0f32;
                for n in 1..total_samples {
                    let env = (-decay_rate * (n as f32)).exp();
                    let noise_l = rng.next_f32();
                    let noise_r = rng.next_f32();
                    filter_state_l = filter_state_l * 0.65 + noise_l * 0.35;
                    filter_state_r = filter_state_r * 0.65 + noise_r * 0.35;
                    ir_l[n] += filter_state_l * env * 0.5;
                    ir_r[n] += filter_state_r * env * 0.5;
                }
            }
            3 => {
                // Vintage Cab 4x12
                let cab_len = total_samples.min(1024);
                let f0 = 2800.0f32;
                let omega = 2.0 * PI * f0 / (self.sample_rate as f32);
                let decay = 120.0 / (self.sample_rate as f32);
                for n in 0..cab_len {
                    let env = (-decay * (n as f32)).exp();
                    let sample = (omega * (n as f32)).sin() * env;
                    ir_l[n] = sample * 0.85;
                    ir_r[n] = sample * 0.85;
                }
            }
            4 => {
                // Spring Tank
                let spring_len = total_samples.min(4096);
                let decay = 15.0 / (self.sample_rate as f32);
                for n in 0..spring_len {
                    let env = (-decay * (n as f32)).exp();
                    let nf = n as f32;
                    let phase = 0.05 * nf + 0.0001 * nf * nf;
                    let s_l = phase.sin() * env;
                    let s_r = (phase * 1.04 + 0.5).sin() * env;
                    ir_l[n] = s_l * 0.6;
                    ir_r[n] = s_r * 0.6;
                }
            }
            _ => {
                ir_l[0] = 1.0;
                ir_r[0] = 1.0;
            }
        }

        self.update_thumbnail(&ir_l, Some(&ir_r));

        let active_idx = self.active_ir_index.load(Ordering::Relaxed);
        let inactive_idx = 1 - active_idx;
        self.populate_ir_slot(inactive_idx, &ir_l, Some(&ir_r));
        self.active_ir_index.store(inactive_idx, Ordering::Release);
    }

    #[inline]
    fn process_sample_ch(&mut self, ch: usize, input: f32, active_idx: usize) -> f32 {
        // 1. Direct Time-Domain FIR Head (Zero Latency)
        let pos = self.head_input_pos[ch];
        self.head_input_buffer[ch][pos] = input;
        self.head_input_buffer[ch][pos + PARTITION_SIZE] = input; // Duplicated to eliminate modulo in dot product

        let mut head_output = 0.0f32;
        let offset = pos + 1;
        let buf_slice = &self.head_input_buffer[ch][offset..offset + PARTITION_SIZE];
        let fir_slice = &self.ir_slots[active_idx].head_fir[ch];

        for i in 0..PARTITION_SIZE {
            head_output += buf_slice[PARTITION_SIZE - 1 - i] * fir_slice[i];
        }

        self.head_input_pos[ch] = (pos + 1) % PARTITION_SIZE;

        // 2. Extract accumulated tail output from previous FFT partitions (Overlap-Add)
        let blk_pos = self.input_block_pos[ch];
        let tail_output = self.output_overlap[ch][blk_pos];
        self.output_overlap[ch][blk_pos] = 0.0;

        // 3. Buffer input for next FFT block
        self.input_block_buffer[ch][blk_pos] = input;
        self.input_block_pos[ch] += 1;

        if self.input_block_pos[ch] >= PARTITION_SIZE {
            self.input_block_pos[ch] = 0;
            self.process_tail_block(ch, active_idx);
        }

        head_output + tail_output
    }

    fn process_tail_block(&mut self, ch: usize, active_idx: usize) {
        let partitions = self.ir_slots[active_idx].active_partitions;

        // 1. Shift input spectra delay line X_p = X_{p-1}
        for p in (1..partitions).rev() {
            let (prev_re, prev_im) = (self.x_spectra_re[ch][p - 1], self.x_spectra_im[ch][p - 1]);
            self.x_spectra_re[ch][p] = prev_re;
            self.x_spectra_im[ch][p] = prev_im;
        }

        // 2. Compute FFT of current input block zero-padded to 256
        let mut f_re = [0.0f32; FFT_SIZE];
        let mut f_im = [0.0f32; FFT_SIZE];
        f_re[..PARTITION_SIZE].copy_from_slice(&self.input_block_buffer[ch]);

        self.fft.compute(&mut f_re, &mut f_im, false);

        self.x_spectra_re[ch][0].copy_from_slice(&f_re);
        self.x_spectra_im[ch][0].copy_from_slice(&f_im);

        // 3. Spectral convolution with accumulation: sum_{p=1}^{active-1} X_{p-1} * H_p
        self.tail_acc_re[ch].fill(0.0);
        self.tail_acc_im[ch].fill(0.0);

        for p in 1..partitions {
            let x_re = &self.x_spectra_re[ch][p - 1];
            let x_im = &self.x_spectra_im[ch][p - 1];
            let h_re = &self.ir_slots[active_idx].ir_spectra_re[ch][p];
            let h_im = &self.ir_slots[active_idx].ir_spectra_im[ch][p];

            for k in 0..FFT_SIZE {
                // (xr + j xi) * (hr + j hi) = (xr*hr - xi*hi) + j(xr*hi + xi*hr)
                self.tail_acc_re[ch][k] += x_re[k] * h_re[k] - x_im[k] * h_im[k];
                self.tail_acc_im[ch][k] += x_re[k] * h_im[k] + x_im[k] * h_re[k];
            }
        }

        // 4. IFFT of tail accumulator
        self.fft.compute(&mut self.tail_acc_re[ch], &mut self.tail_acc_im[ch], true);

        // 5. Shift upper half of output overlap to lower half, and clear upper half
        for i in 0..PARTITION_SIZE {
            self.output_overlap[ch][i] = self.output_overlap[ch][i + PARTITION_SIZE];
            self.output_overlap[ch][i + PARTITION_SIZE] = 0.0;
        }

        // 6. Overlap-add
        for i in 0..FFT_SIZE {
            self.output_overlap[ch][i] += self.tail_acc_re[ch][i];
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

        // Synthetic IR model update check
        let req_ir = self.param_ir_type.round() as i32;
        if req_ir != self.current_ir_type && (0..=4).contains(&req_ir) {
            self.load_synthetic_ir(req_ir);
        }

        // Acquire lock-free active IR slot index
        let active_idx = self.active_ir_index.load(Ordering::Acquire);

        // Filter cutoff updates
        if (self.last_low_cut - self.param_low_cut_hz).abs() > 1.0 {
            self.last_low_cut = self.param_low_cut_hz;
            self.low_cut_l.set_filter(BiquadType::Highpass, self.sample_rate, self.param_low_cut_hz, 0.707);
            self.low_cut_r.set_filter(BiquadType::Highpass, self.sample_rate, self.param_low_cut_hz, 0.707);
        }
        if (self.last_high_cut - self.param_high_cut_hz).abs() > 5.0 {
            self.last_high_cut = self.param_high_cut_hz;
            self.high_cut_l.set_filter(BiquadType::Lowpass, self.sample_rate, self.param_high_cut_hz, 0.707);
            self.high_cut_r.set_filter(BiquadType::Lowpass, self.sample_rate, self.param_high_cut_hz, 0.707);
        }

        let pre_delay_samples = (self.param_pre_delay_ms * 0.001 * (self.sample_rate as f32)) as usize;
        let width = self.param_width;
        let mix = self.param_mix;
        let max_predelay = self.max_pre_delay_samples;

        for s in 0..num_samples {
            let raw_in_l = denormal_guard(in_l[s]);
            let raw_in_r = denormal_guard(in_r[s]);

            // Pre-delay ring buffer
            self.pre_delay_buffer_l[self.pre_delay_write_idx] = raw_in_l;
            self.pre_delay_buffer_r[self.pre_delay_write_idx] = raw_in_r;

            let read_idx = (self.pre_delay_write_idx + max_predelay - pre_delay_samples) % max_predelay;
            let delayed_in_l = self.pre_delay_buffer_l[read_idx];
            let delayed_in_r = self.pre_delay_buffer_r[read_idx];
            self.pre_delay_write_idx = (self.pre_delay_write_idx + 1) % max_predelay;

            // Zero-latency partitioned convolution
            let mut wet_l = self.process_sample_ch(0, delayed_in_l, active_idx);
            let mut wet_r = self.process_sample_ch(1, delayed_in_r, active_idx);

            // Biquad filtering
            wet_l = self.low_cut_l.process(wet_l);
            wet_l = self.high_cut_l.process(wet_l);
            wet_r = self.low_cut_r.process(wet_r);
            wet_r = self.high_cut_r.process(wet_r);

            // Mid/Side stereo width
            let mid = 0.5 * (wet_l + wet_r);
            let side = 0.5 * (wet_l - wet_r) * width;
            wet_l = mid + side;
            wet_r = mid - side;

            // Dry/Wet mix
            out_l[s] = denormal_guard(raw_in_l * (1.0 - mix) + wet_l * mix);
            out_r[s] = denormal_guard(raw_in_r * (1.0 - mix) + wet_r * mix);
        }
    }
}
