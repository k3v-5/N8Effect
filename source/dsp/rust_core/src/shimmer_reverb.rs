//! Shimmer Reverb Engine in Rust
//! High-density 4-line FDN delay tank with Householder orthogonal matrix diffusion,
//! dual Schroeder allpass diffusers, dual-head Hermite cubic pitch shifter (+12st, +7st, -12st),
//! fast soft saturation feedback, and Mid/Side stereo widener (Rules 5, 8, 9, 14, 18, 45, 46, 47).

use crate::common::{fast_tanh, BiquadCoeffs, BiquadState, DelayBuffer};

pub struct RustShimmerReverb {
    sample_rate: f32,

    // Base delay times in milliseconds (mutually prime for maximum modal density)
    base_lengths_ms: [f32; 4],

    // Schroeder allpass input diffusers
    diffuser1: DelayBuffer,
    diffuser2: DelayBuffer,

    // 4-line FDN delay tank & damping filters
    fdn_lines: [DelayBuffer; 4],
    damp_coeffs: [BiquadCoeffs; 4],
    damp_states: [BiquadState; 4],

    // Dual-head pitch shifter delay lines (L & R)
    pitch_delay_l: DelayBuffer,
    pitch_delay_r: DelayBuffer,
    pitch_window_samples: f32,
    pitch_phase: f32,

    // Parameters
    param_decay: f32,          // 1: DecayTime (0.2s .. 30.0s)
    param_damping: f32,        // 2: DampingHz (1000Hz .. 20000Hz)
    param_shimmer_mix: f32,    // 3: ShimmerMix (0.0 .. 1.0)
    param_pitch_interval: f32, // 4: PitchInterval (0: +12st, 1: +7st, 2: -12st)
    param_stereo_width: f32,   // 5: StereoWidth (0.0 .. 2.0)
    param_dry_wet: f32,        // 6: DryWet (0.0 .. 1.0)
}

impl RustShimmerReverb {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };

        // Buffer sizing: FDN delay up to 500ms, diffusers 50ms, pitch 200ms
        let diffuser_cap = ((sr * 0.06) as usize).next_power_of_two().max(4096);
        let fdn_cap = ((sr * 0.6) as usize).next_power_of_two().max(16384);
        let pitch_cap = ((sr * 0.25) as usize).next_power_of_two().max(16384);

        let mut instance = Self {
            sample_rate: sr,
            base_lengths_ms: [29.7, 37.1, 41.3, 47.9],
            diffuser1: DelayBuffer::new(diffuser_cap),
            diffuser2: DelayBuffer::new(diffuser_cap),
            fdn_lines: [
                DelayBuffer::new(fdn_cap),
                DelayBuffer::new(fdn_cap),
                DelayBuffer::new(fdn_cap),
                DelayBuffer::new(fdn_cap),
            ],
            damp_coeffs: [BiquadCoeffs::default(); 4],
            damp_states: [BiquadState::default(); 4],
            pitch_delay_l: DelayBuffer::new(pitch_cap),
            pitch_delay_r: DelayBuffer::new(pitch_cap),
            pitch_window_samples: (sr * 0.045).max(64.0),
            pitch_phase: 0.0,
            param_decay: 4.0,
            param_damping: 6500.0,
            param_shimmer_mix: 0.5,
            param_pitch_interval: 0.0,
            param_stereo_width: 1.2,
            param_dry_wet: 0.45,
        };

        instance.update_filters();
        instance
    }

    #[inline(always)]
    pub fn reset(&mut self) {
        self.diffuser1.reset();
        self.diffuser2.reset();
        for i in 0..4 {
            self.fdn_lines[i].reset();
            self.damp_states[i].reset();
        }
        self.pitch_delay_l.reset();
        self.pitch_delay_r.reset();
        self.pitch_phase = 0.0;
    }

    pub fn set_sample_rate(&mut self, sample_rate: f32) {
        if sample_rate > 0.0 && (sample_rate - self.sample_rate).abs() > 0.01 {
            self.sample_rate = sample_rate;
            self.pitch_window_samples = (sample_rate * 0.045).max(64.0);
            self.update_filters();
        }
    }

    #[inline(always)]
    pub fn update_filters(&mut self) {
        let sr = self.sample_rate;
        let fc = self.param_damping.clamp(1000.0, 20000.0);
        let coeffs = BiquadCoeffs::lowpass(sr, fc, 0.707);
        for i in 0..4 {
            self.damp_coeffs[i] = coeffs;
        }
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.param_decay = value.clamp(0.2, 30.0),
            2 => {
                self.param_damping = value.clamp(1000.0, 20000.0);
                self.update_filters();
            }
            3 => self.param_shimmer_mix = value.clamp(0.0, 1.0),
            4 => self.param_pitch_interval = value.clamp(0.0, 2.0),
            5 => self.param_stereo_width = value.clamp(0.0, 2.0),
            6 => self.param_dry_wet = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.param_decay,
            2 => self.param_damping,
            3 => self.param_shimmer_mix,
            4 => self.param_pitch_interval,
            5 => self.param_stereo_width,
            6 => self.param_dry_wet,
            _ => 0.0,
        }
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let n_samples = in_l.len().min(out_l.len());
        if n_samples == 0 {
            return;
        }

        let sr = self.sample_rate;
        let mix = self.param_dry_wet;
        let shimmer_mix = self.param_shimmer_mix;
        let stereo_width = self.param_stereo_width;
        let t60 = self.param_decay.clamp(0.2, 30.0);

        // Pitch shift ratio based on interval selector
        let interval_idx = self.param_pitch_interval.round() as i32;
        let pitch_ratio = if interval_idx == 1 {
            1.498307 // +7st (Perfect fifth: 2^(7/12))
        } else if interval_idx == 2 {
            0.5 // -12st (Sub-octave: 2^(-12/12))
        } else {
            2.0 // +12st (Octave up)
        };

        let pitch_rate = 1.0 - pitch_ratio;
        let win_size = self.pitch_window_samples;
        let half_win = win_size * 0.5;

        // Calculate FDN delay lengths in samples and decay gains (strictly bounded <= 0.985)
        let mut delay_samples = [0.0f32; 4];
        let mut feedback_gains = [0.0f32; 4];
        for i in 0..4 {
            let d_sec = self.base_lengths_ms[i] * 0.001;
            delay_samples[i] = d_sec * sr;
            let g = 10.0f32.powf(-3.0 * d_sec / t60);
            feedback_gains[i] = g.min(0.985);
        }

        // Input Schroeder allpass diffusion delay times
        let diff_delay1 = (sr * 0.0032).max(4.0);
        let diff_delay2 = (sr * 0.0086).max(4.0);
        const ALLPASS_COEFF: f32 = 0.618;

        for s in 0..n_samples {
            let dry_l = in_l[s];
            let dry_r = if s < in_r.len() { in_r[s] } else { dry_l };
            let mono_in = 0.5 * (dry_l + dry_r);

            // 1. Dual-stage Schroeder allpass input diffusion
            let ap1_read = self.diffuser1.read_cubic(diff_delay1);
            let ap1_out = -ALLPASS_COEFF * mono_in + ap1_read;
            self.diffuser1.write(mono_in + ALLPASS_COEFF * ap1_out);

            let ap2_read = self.diffuser2.read_cubic(diff_delay2);
            let ap2_out = -ALLPASS_COEFF * ap1_out + ap2_read;
            self.diffuser2.write(ap1_out + ALLPASS_COEFF * ap2_out);

            let diffused_input = ap2_out;

            // 2. Read and lowpass filter the 4 FDN delay lines
            let mut line_outputs = [0.0f32; 4];
            for i in 0..4 {
                let read_val = self.fdn_lines[i].read_cubic(delay_samples[i]);
                line_outputs[i] = self.damp_states[i].process(read_val, &self.damp_coeffs[i]);
            }

            // 3. Lossless Householder orthogonal diffusion matrix: (H * y)_i = y_i - 0.5 * sum(y)
            let sum_y = line_outputs[0] + line_outputs[1] + line_outputs[2] + line_outputs[3];
            let half_sum = 0.5 * sum_y;

            let mut diffused = [0.0f32; 4];
            for i in 0..4 {
                diffused[i] = line_outputs[i] - half_sum;
            }

            // 4. Intermediate stereo extraction from the reverb tank
            let tank_l = 0.5 * (diffused[0] + diffused[2]);
            let tank_r = 0.5 * (diffused[1] + diffused[3]);

            // 5. Dual-head Hermite pitch shifter
            self.pitch_delay_l.write(tank_l);
            self.pitch_delay_r.write(tank_r);

            let mut d1 = self.pitch_phase;
            while d1 < 0.0 {
                d1 += win_size;
            }
            while d1 >= win_size {
                d1 -= win_size;
            }

            let mut d2 = self.pitch_phase + half_win;
            while d2 < 0.0 {
                d2 += win_size;
            }
            while d2 >= win_size {
                d2 -= win_size;
            }

            let env1 = 1.0 - ((2.0 * d1 / win_size) - 1.0).abs();
            let env2 = 1.0 - ((2.0 * d2 / win_size) - 1.0).abs();

            let shimmer_l = self.pitch_delay_l.read_cubic(d1 + 10.0) * env1
                + self.pitch_delay_l.read_cubic(d2 + 10.0) * env2;
            let shimmer_r = self.pitch_delay_r.read_cubic(d1 + 10.0) * env1
                + self.pitch_delay_r.read_cubic(d2 + 10.0) * env2;

            self.pitch_phase += pitch_rate;
            if self.pitch_phase >= win_size {
                self.pitch_phase -= win_size;
            } else if self.pitch_phase < 0.0 {
                self.pitch_phase += win_size;
            }

            // 6. Analog soft saturation return (fast_tanh prevents runaway)
            let return_l = fast_tanh(shimmer_l);
            let return_r = fast_tanh(shimmer_r);

            // Re-injection into FDN delay lines
            let feed0 = (1.0 - shimmer_mix) * diffused[0] + shimmer_mix * return_l;
            let feed1 = (1.0 - shimmer_mix) * diffused[1] + shimmer_mix * return_r;
            let feed2 = (1.0 - shimmer_mix) * diffused[2] + shimmer_mix * return_l;
            let feed3 = (1.0 - shimmer_mix) * diffused[3] + shimmer_mix * return_r;

            self.fdn_lines[0].write(diffused_input + feed0 * feedback_gains[0]);
            self.fdn_lines[1].write(diffused_input + feed1 * feedback_gains[1]);
            self.fdn_lines[2].write(diffused_input + feed2 * feedback_gains[2]);
            self.fdn_lines[3].write(diffused_input + feed3 * feedback_gains[3]);

            // 7. Mid/Side stereo widening
            let mut wet_l = tank_l * (1.0 - shimmer_mix * 0.3) + return_l * (shimmer_mix * 0.5);
            let mut wet_r = tank_r * (1.0 - shimmer_mix * 0.3) + return_r * (shimmer_mix * 0.5);

            let mid_signal = 0.5 * (wet_l + wet_r);
            let side_signal = 0.5 * (wet_l - wet_r);
            wet_l = mid_signal + side_signal * stereo_width;
            wet_r = mid_signal - side_signal * stereo_width;

            // 8. Output Dry/Wet mix
            out_l[s] = dry_l * (1.0 - mix) + wet_l * mix;
            if s < out_r.len() {
                out_r[s] = dry_r * (1.0 - mix) + wet_r * mix;
            }
        }
    }
}
