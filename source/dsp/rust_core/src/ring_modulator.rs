//! 4-Quadrant Ring Modulator DSP Kernel with Multi-Waveform Carrier Oscillator
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio thread,
//! Sine, Triangle, Sawtooth and Square carrier generation, carrier bleed,
//! balanced 4-quadrant multiplication, and denormal protection.

use std::f32::consts::PI;
use crate::common::denormal_guard;

pub struct RustRingModulator {
    sample_rate: f32,
    carrier_phase: f32,
    env_follower: f32,

    target_freq: f32,
    target_wave: i32,
    target_bleed: f32,
    target_mix: f32,
}

impl RustRingModulator {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            carrier_phase: 0.0,
            env_follower: 0.0,
            target_freq: 440.0,
            target_wave: 0,
            target_bleed: 0.0,
            target_mix: 1.0,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn reset(&mut self) {
        self.carrier_phase = 0.0;
        self.env_follower = 0.0;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_freq = value.clamp(1.0, 5000.0),
            2 => self.target_wave = (value.round() as i32).clamp(0, 3),
            3 => self.target_bleed = value.clamp(0.0, 1.0),
            4 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_freq,
            2 => self.target_wave as f32,
            3 => self.target_bleed,
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

        let sr = self.sample_rate;
        let freq = self.target_freq;
        let wave = self.target_wave;
        let bleed = self.target_bleed;
        let mix = self.target_mix;
        let dry_gain = 1.0 - mix;

        let two_pi = 2.0 * PI;
        let inv_two_pi = 1.0 / two_pi;
        let phase_inc = (two_pi * freq) / sr;
        let bleed_factor = bleed * 0.2;

        for s in 0..num_samples {
            let s_l = denormal_guard(in_l[s]);
            let s_r = denormal_guard(if s < in_r.len() { in_r[s] } else { s_l });

            // Generar forma de onda portadora
            let norm_phase = (self.carrier_phase * inv_two_pi).clamp(0.0, 1.0);
            let carrier = match wave {
                0 => self.carrier_phase.sin(),
                1 => {
                    if norm_phase < 0.5 {
                        4.0 * norm_phase - 1.0
                    } else {
                        3.0 - 4.0 * norm_phase
                    }
                }
                2 => 2.0 * norm_phase - 1.0,
                3 => {
                    if norm_phase < 0.5 {
                        1.0
                    } else {
                        -1.0
                    }
                }
                _ => self.carrier_phase.sin(),
            };

            let in_mag = 0.5 * (s_l.abs() + s_r.abs());
            self.env_follower = if in_mag > self.env_follower {
                0.95 * self.env_follower + 0.05 * in_mag
            } else {
                0.999 * self.env_follower + 0.001 * in_mag
            };
            let bleed_gate = (self.env_follower * 30.0).clamp(0.0, 1.0);
            let carrier_bleed_term = bleed_factor * carrier * bleed_gate;

            // Modulación en 4 cuadrantes balanceada con carrier bleed
            let ring_mod_l = s_l * carrier + carrier_bleed_term;
            out_l[s] = denormal_guard(dry_gain * s_l + mix * ring_mod_l);

            if s < out_r.len() {
                let ring_mod_r = s_r * carrier + carrier_bleed_term;
                out_r[s] = denormal_guard(dry_gain * s_r + mix * ring_mod_r);
            }

            self.carrier_phase += phase_inc;
            if self.carrier_phase >= two_pi {
                self.carrier_phase -= two_pi;
            }
        }
    }
}
