//! Dynamic Tremolo & Auto-Pan (Phase 9 - Large Effect Library, Rules 5, 8, 9, 34, 46, 47)
//! Emulates optical tube tremolo, harmonic split tremolo and dynamic auto-pan.
//! Features audio envelope modulation of rate (envelope-follower rate warping),
//! variable stereo phase offsets (0 deg tremolo to 180 deg ping-pong), and multi-waveform LFOs.

use crate::common::{clamp, lerp};

pub struct RustDynamicTremolo {
    sample_rate: f32,
    lfo_phase: f32,

    // Envelope follower state
    envelope: f32,
    env_attack_coeff: f32,
    env_release_coeff: f32,

    // Crossover 1-pole state for harmonic tremolo mode
    crossover_state_l: f32,
    crossover_state_r: f32,

    // Parameters
    rate_hz: f32,          // 0.1 to 20.0 Hz
    depth: f32,            // 0.0 to 1.0
    shape: u32,            // 0 = Sine, 1 = Optical (photo-resistor), 2 = Triangle, 3 = Harmonic split
    stereo_phase_deg: f32, // 0.0 to 360.0 deg (0 = sync tremolo, 90 = circle pan, 180 = hard ping-pong)
    dynamics: f32,         // -1.0 to +1.0 (envelope modulation of rate: + speeds up, - slows down)
    mix: f32,              // 0.0 to 1.0

    // Smoothing
    smoothed_rate: f32,
    smoothed_depth: f32,
    smoothed_mix: f32,
}

impl RustDynamicTremolo {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        let mut inst = Self {
            sample_rate: sr,
            lfo_phase: 0.0,
            envelope: 0.0,
            env_attack_coeff: 0.0,
            env_release_coeff: 0.0,
            crossover_state_l: 0.0,
            crossover_state_r: 0.0,
            rate_hz: 4.0,
            depth: 0.7,
            shape: 0,
            stereo_phase_deg: 90.0,
            dynamics: 0.3,
            mix: 1.0,
            smoothed_rate: 4.0,
            smoothed_depth: 0.7,
            smoothed_mix: 1.0,
        };
        inst.update_env_coeffs();
        inst
    }

    fn update_env_coeffs(&mut self) {
        let attack_sec = 0.010; // 10 ms attack
        let release_sec = 0.100; // 100 ms release
        self.env_attack_coeff = (-1.0 / (attack_sec * self.sample_rate)).exp();
        self.env_release_coeff = (-1.0 / (release_sec * self.sample_rate)).exp();
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.update_env_coeffs();
        self.reset();
    }

    pub fn reset(&mut self) {
        self.lfo_phase = 0.0;
        self.envelope = 0.0;
        self.crossover_state_l = 0.0;
        self.crossover_state_r = 0.0;
        self.smoothed_rate = self.rate_hz;
        self.smoothed_depth = self.depth;
        self.smoothed_mix = self.mix;
    }

    pub fn set_rate(&mut self, rate_hz: f32) {
        self.rate_hz = clamp(rate_hz, 0.1, 20.0);
    }

    pub fn set_depth(&mut self, depth: f32) {
        self.depth = clamp(depth, 0.0, 1.0);
    }

    pub fn set_shape(&mut self, shape: u32) {
        self.shape = shape.min(3);
    }

    pub fn set_stereo_phase(&mut self, phase_deg: f32) {
        self.stereo_phase_deg = clamp(phase_deg, 0.0, 360.0);
    }

    pub fn set_dynamics(&mut self, dynamics: f32) {
        self.dynamics = clamp(dynamics, -1.0, 1.0);
    }

    pub fn set_mix(&mut self, mix: f32) {
        self.mix = clamp(mix, 0.0, 1.0);
    }

    #[inline]
    fn compute_waveform(phase: f32, shape: u32) -> f32 {
        let p = phase % std::f32::consts::TAU;
        match shape {
            // 0: Sine wave [0.0 .. 1.0]
            0 => 0.5 + 0.5 * p.sin(),
            // 1: Optical (asymmetrical duty cycle with smooth rounded top and sharp cut)
            1 => {
                let s = p.sin();
                if s >= 0.0 {
                    s.powf(0.6)
                } else {
                    0.0
                }
            }
            // 2: Triangle wave [0.0 .. 1.0]
            2 => {
                let norm = p / std::f32::consts::TAU;
                if norm < 0.5 {
                    norm * 2.0
                } else {
                    2.0 - norm * 2.0
                }
            }
            // 3: Harmonic modulation base
            _ => 0.5 + 0.5 * p.sin(),
        }
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len().min(in_r.len()).min(out_l.len()).min(out_r.len());
        if num_samples == 0 {
            return;
        }

        let phase_offset = (self.stereo_phase_deg * std::f32::consts::PI) / 180.0;
        let crossover_alpha = 0.15f32; // ~1 kHz crossover pole at 44.1 kHz

        for i in 0..num_samples {
            self.smoothed_rate = lerp(self.smoothed_rate, self.rate_hz, 0.005);
            self.smoothed_depth = lerp(self.smoothed_depth, self.depth, 0.005);
            self.smoothed_mix = lerp(self.smoothed_mix, self.mix, 0.005);

            let dry_l = in_l[i];
            let dry_r = in_r[i];

            // 1. Envelope detector on input signal
            let input_peak = dry_l.abs().max(dry_r.abs());
            if input_peak > self.envelope {
                self.envelope = input_peak + self.env_attack_coeff * (self.envelope - input_peak);
            } else {
                self.envelope = input_peak + self.env_release_coeff * (self.envelope - input_peak);
            }

            // 2. Dynamics rate modulation: envelope can scale rate up to 3x or down to 0.3x
            let rate_mod = 1.0 + self.dynamics * 2.0 * clamp(self.envelope * 2.0, 0.0, 1.0);
            let effective_rate = clamp(self.smoothed_rate * rate_mod.max(0.1), 0.1, 40.0);
            let lfo_inc = (std::f32::consts::TAU * effective_rate) / self.sample_rate;

            // 3. Compute Left and Right LFO gains
            let wave_l = Self::compute_waveform(self.lfo_phase, self.shape);
            let wave_r = Self::compute_waveform(self.lfo_phase + phase_offset, self.shape);

            let depth = self.smoothed_depth;
            let gain_l = 1.0 - depth * (1.0 - wave_l);
            let gain_r = 1.0 - depth * (1.0 - wave_r);

            // 4. Processing mode
            let (wet_l, wet_r) = if self.shape == 3 {
                // Harmonic Tremolo: split signal into low and high frequencies
                self.crossover_state_l += crossover_alpha * (dry_l - self.crossover_state_l);
                self.crossover_state_r += crossover_alpha * (dry_r - self.crossover_state_r);

                let low_l = self.crossover_state_l;
                let high_l = dry_l - low_l;
                let low_r = self.crossover_state_r;
                let high_r = dry_r - low_r;

                // Modulate low and high in anti-phase
                let low_gain_l = 1.0 - depth * (1.0 - wave_l);
                let high_gain_l = 1.0 - depth * wave_l;
                let low_gain_r = 1.0 - depth * (1.0 - wave_r);
                let high_gain_r = 1.0 - depth * wave_r;

                (low_l * low_gain_l + high_l * high_gain_l,
                 low_r * low_gain_r + high_r * high_gain_r)
            } else {
                (dry_l * gain_l, dry_r * gain_r)
            };

            // 5. Dry / Wet mix
            let m = self.smoothed_mix;
            out_l[i] = (1.0 - m) * dry_l + m * wet_l;
            out_r[i] = (1.0 - m) * dry_r + m * wet_r;

            // Advance phase
            self.lfo_phase += lfo_inc;
            if self.lfo_phase >= std::f32::consts::TAU {
                self.lfo_phase -= std::f32::consts::TAU;
            }
        }
    }
}
