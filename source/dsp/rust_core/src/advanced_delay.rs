//! Advanced Stereo Delay DSP Kernel (DSP #20)
//! Rules 5, 8, 9, 12, 13, 14, 16, 18, 32, 34, 46, 47: Zero allocations in audio thread,
//! Hermite cubic interpolation, damping lowpass filter in feedback loop,
//! Ping-Pong cross-channel routing, soft saturation via fast_tanh, and denormal protection.

use crate::common::{denormal_guard, fast_tanh, BiquadCoeffs, BiquadState, DelayBuffer};

pub struct RustAdvancedDelay {
    sample_rate: f32,
    delay_l: DelayBuffer,
    delay_r: DelayBuffer,
    damp_coeffs: [BiquadCoeffs; 2],
    damp_states: [BiquadState; 2],

    target_time_l: f32,
    target_time_r: f32,
    target_feedback: f32,
    target_damping: f32,
    target_ping_pong: f32,
    target_mix: f32,
}

impl RustAdvancedDelay {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let max_samples = (sr * 3.0) as usize;
        let mut delay = Self {
            sample_rate: sr,
            delay_l: DelayBuffer::new(max_samples),
            delay_r: DelayBuffer::new(max_samples),
            damp_coeffs: [BiquadCoeffs::default(); 2],
            damp_states: [BiquadState::default(); 2],
            target_time_l: 300.0,
            target_time_r: 450.0,
            target_feedback: 0.45,
            target_damping: 6000.0,
            target_ping_pong: 0.0,
            target_mix: 0.5,
        };
        delay.update_damping();
        delay
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.sample_rate = sr;
        let max_samples = (sr * 3.0) as usize;
        self.delay_l = DelayBuffer::new(max_samples);
        self.delay_r = DelayBuffer::new(max_samples);
        self.reset();
        self.update_damping();
    }

    pub fn reset(&mut self) {
        self.delay_l.reset();
        self.delay_r.reset();
        self.damp_states[0].reset();
        self.damp_states[1].reset();
    }

    #[inline]
    fn update_damping(&mut self) {
        let coeff = BiquadCoeffs::lowpass(self.sample_rate, self.target_damping, 0.707);
        self.damp_coeffs[0] = coeff;
        self.damp_coeffs[1] = coeff;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_time_l = value.clamp(1.0, 2500.0),
            2 => self.target_time_r = value.clamp(1.0, 2500.0),
            3 => self.target_feedback = value.clamp(0.0, 0.95),
            4 => {
                self.target_damping = value.clamp(1000.0, 20000.0);
                self.update_damping();
            }
            5 => self.target_ping_pong = if value > 0.5 { 1.0 } else { 0.0 },
            6 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_time_l,
            2 => self.target_time_r,
            3 => self.target_feedback,
            4 => self.target_damping,
            5 => self.target_ping_pong,
            6 => self.target_mix,
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

        let delay_samples_l = (self.target_time_l * 0.001) * self.sample_rate;
        let delay_samples_r = (self.target_time_r * 0.001) * self.sample_rate;
        let fb = self.target_feedback.clamp(0.0, 0.95);
        let ping_pong = self.target_ping_pong > 0.5;
        let mix = self.target_mix;

        for s in 0..num_samples {
            let s_in_l = in_l[s];
            let s_in_r = if s < in_r.len() { in_r[s] } else { s_in_l };

            // Lectura con interpolación cúbica (Hermite)
            let mut delayed_l = self.delay_l.read_cubic(delay_samples_l);
            let mut delayed_r = self.delay_r.read_cubic(delay_samples_r);

            // Filtrado de amortiguación en el lazo de feedback
            delayed_l = self.damp_states[0].process(delayed_l, &self.damp_coeffs[0]);
            delayed_r = self.damp_states[1].process(delayed_r, &self.damp_coeffs[1]);

            // Normal vs Ping-Pong
            let (mut fb_in_l, mut fb_in_r) = if ping_pong {
                (s_in_l + delayed_r * fb, s_in_r + delayed_l * fb)
            } else {
                (s_in_l + delayed_l * fb, s_in_r + delayed_r * fb)
            };

            // Saturación suave anti-runaway / anti-pop (fast_tanh)
            fb_in_l = fast_tanh(fb_in_l);
            fb_in_r = fast_tanh(fb_in_r);

            self.delay_l.write(fb_in_l);
            self.delay_r.write(fb_in_r);

            out_l[s] = denormal_guard(s_in_l * (1.0 - mix) + delayed_l * mix);
            if s < out_r.len() {
                out_r[s] = denormal_guard(s_in_r * (1.0 - mix) + delayed_r * mix);
            }
        }
    }
}
