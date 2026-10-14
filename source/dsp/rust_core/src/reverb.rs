//! Algorithmic FDN Reverb DSP Kernel (DSP #6)
//! Rules 5, 8, 9, 13, 14, 16, 18, 32, 34, 46, 47: Zero allocations in audio thread,
//! 4-line feedback delay network, orthogonal Householder diffusion matrix,
//! mutually prime delay line lengths, damping lowpass filters, and denormal protection.

use crate::common::{denormal_guard, BiquadCoeffs, BiquadState, DelayBuffer};

pub struct RustReverb {
    sample_rate: f32,
    base_lengths_ms: [f32; 4],
    predelay_line: DelayBuffer,
    fdn_lines: [DelayBuffer; 4],
    damp_coeffs: [BiquadCoeffs; 4],
    damp_states: [BiquadState; 4],

    target_size: f32,
    target_decay: f32,
    target_damping: f32,
    target_predelay: f32,
    target_mix: f32,
}

impl RustReverb {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let max_predelay = ((sr * 0.2) as usize).max(1024);
        let max_fdn = ((sr * 0.5) as usize).max(1024);

        let mut reverb = Self {
            sample_rate: sr,
            base_lengths_ms: [29.7, 37.1, 41.3, 47.9],
            predelay_line: DelayBuffer::new(max_predelay),
            fdn_lines: [
                DelayBuffer::new(max_fdn),
                DelayBuffer::new(max_fdn),
                DelayBuffer::new(max_fdn),
                DelayBuffer::new(max_fdn),
            ],
            damp_coeffs: [BiquadCoeffs::default(); 4],
            damp_states: [BiquadState::default(); 4],
            target_size: 0.7,
            target_decay: 2.5,
            target_damping: 5000.0,
            target_predelay: 20.0,
            target_mix: 0.4,
        };
        reverb.update_filters();
        reverb
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.sample_rate = sr;
        let max_predelay = ((sr * 0.2) as usize).max(1024);
        let max_fdn = ((sr * 0.5) as usize).max(1024);

        self.predelay_line = DelayBuffer::new(max_predelay);
        self.fdn_lines = [
            DelayBuffer::new(max_fdn),
            DelayBuffer::new(max_fdn),
            DelayBuffer::new(max_fdn),
            DelayBuffer::new(max_fdn),
        ];
        self.reset();
        self.update_filters();
    }

    pub fn reset(&mut self) {
        self.predelay_line.reset();
        for line in &mut self.fdn_lines {
            line.reset();
        }
        for state in &mut self.damp_states {
            state.reset();
        }
    }

    #[inline]
    fn update_filters(&mut self) {
        let coeff = BiquadCoeffs::lowpass(self.sample_rate, self.target_damping, 0.707);
        for c in &mut self.damp_coeffs {
            *c = coeff;
        }
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_size = value.clamp(0.1, 1.0),
            2 => self.target_decay = value.clamp(0.2, 10.0),
            3 => {
                self.target_damping = value.clamp(500.0, 20000.0);
                self.update_filters();
            }
            4 => self.target_predelay = value.clamp(0.0, 100.0),
            5 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_size,
            2 => self.target_decay,
            3 => self.target_damping,
            4 => self.target_predelay,
            5 => self.target_mix,
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

        let sr = self.sample_rate;
        let predelay_samples = (self.target_predelay * 0.001) * sr;
        let mix = self.target_mix;
        let t60 = self.target_decay.max(0.1);

        let mut delay_samples = [0.0f32; 4];
        let mut feedback_gains = [0.0f32; 4];
        for i in 0..4 {
            let d_ms = self.base_lengths_ms[i] * self.target_size;
            delay_samples[i] = (d_ms * 0.001) * sr;
            feedback_gains[i] = 10.0f32.powf(-3.0 * (d_ms * 0.001) / t60);
        }

        for s in 0..num_samples {
            let s_in_l = in_l[s];
            let s_in_r = if s < in_r.len() { in_r[s] } else { s_in_l };
            let mono_in = 0.5 * (s_in_l + s_in_r);

            // 1. Predelay
            self.predelay_line.write(mono_in);
            let predelayed = self.predelay_line.read_cubic(predelay_samples);

            // 2. Lectura y amortiguación de las 4 líneas FDN
            let mut line_outputs = [0.0f32; 4];
            for i in 0..4 {
                let read_val = self.fdn_lines[i].read_cubic(delay_samples[i]);
                line_outputs[i] = self.damp_states[i].process(read_val, &self.damp_coeffs[i]);
            }

            // 3. Matriz ortogonal Householder (N=4, H*y = y - 0.5 * sum(y))
            let sum_y = line_outputs[0] + line_outputs[1] + line_outputs[2] + line_outputs[3];
            let half_sum = 0.5 * sum_y;

            let mut diffused = [0.0f32; 4];
            for i in 0..4 {
                diffused[i] = line_outputs[i] - half_sum;
                self.fdn_lines[i].write(predelayed + diffused[i] * feedback_gains[i]);
            }

            // 4. Decodificación estéreo
            let wet_l = 0.5 * (diffused[0] + diffused[2]);
            let wet_r = 0.5 * (diffused[1] + diffused[3]);

            out_l[s] = denormal_guard(s_in_l * (1.0 - mix) + wet_l * mix);
            if s < out_r.len() {
                out_r[s] = denormal_guard(s_in_r * (1.0 - mix) + wet_r * mix);
            }
        }
    }
}
