//! High-Performance Noise & Organic Texture Generator with Sidechain Ducking
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio thread,
//! White, Pink, Tape Hiss, Vinyl Crackle, Rain/Ambient modes,
//! Xorshift32 PRNG, Kellet 3-pole pinking filter, tone lowpass, and denormal protection.

use crate::common::{denormal_guard, BiquadCoeffs, BiquadState};

pub struct RustNoiseTexture {
    sample_rate: f32,
    rng_state: u32,
    pink_b: [f32; 6],
    tone_coeffs: BiquadCoeffs,
    tone_state: [BiquadState; 2],
    vinyl_coeffs: BiquadCoeffs,
    vinyl_state: [BiquadState; 2],
    env_follower: f32,
    rain_mod_phase: f32,

    target_mode: i32,
    target_density: f32,
    target_tone: f32,
    target_duck: f32,
    target_level: f32,
    target_mix: f32,
}

impl RustNoiseTexture {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let tone_fc = 8000.0;
        let mut node = Self {
            sample_rate: sr,
            rng_state: 0x12345678,
            pink_b: [0.0; 6],
            tone_coeffs: BiquadCoeffs::lowpass(sr, tone_fc, 0.707),
            tone_state: [BiquadState::default(); 2],
            vinyl_coeffs: BiquadCoeffs::bandpass(sr, 2500.0, 4.0),
            vinyl_state: [BiquadState::default(); 2],
            env_follower: 0.0,
            rain_mod_phase: 0.0,
            target_mode: 3,
            target_density: 0.45,
            target_tone: tone_fc,
            target_duck: 0.5,
            target_level: 0.4,
            target_mix: 1.0,
        };
        node.update_tone();
        node
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.update_tone();
        self.vinyl_coeffs = BiquadCoeffs::bandpass(self.sample_rate, 2500.0, 4.0);
        self.reset();
    }

    pub fn reset(&mut self) {
        self.rng_state = 0x12345678;
        self.pink_b.fill(0.0);
        self.tone_state[0].reset();
        self.tone_state[1].reset();
        self.vinyl_state[0].reset();
        self.vinyl_state[1].reset();
        self.env_follower = 0.0;
        self.rain_mod_phase = 0.0;
    }

    fn update_tone(&mut self) {
        self.tone_coeffs = BiquadCoeffs::lowpass(self.sample_rate, self.target_tone, 0.707);
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_mode = (value.round() as i32).clamp(0, 4),
            2 => self.target_density = value.clamp(0.0, 1.0),
            3 => {
                self.target_tone = value.clamp(300.0, 18000.0);
                self.update_tone();
            }
            4 => self.target_duck = value.clamp(0.0, 1.0),
            5 => self.target_level = value.clamp(0.0, 1.0),
            6 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_mode as f32,
            2 => self.target_density,
            3 => self.target_tone,
            4 => self.target_duck,
            5 => self.target_level,
            6 => self.target_mix,
            _ => 0.0,
        }
    }

    #[inline(always)]
    fn xorshift32(&mut self) -> u32 {
        let mut x = self.rng_state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        self.rng_state = x;
        x
    }

    #[inline(always)]
    fn next_white(&mut self) -> f32 {
        ((self.xorshift32() & 0x7FFFFFFF) as f32 / 1073741824.0) - 1.0
    }

    #[inline(always)]
    fn process_pink(&mut self, white: f32) -> f32 {
        self.pink_b[0] = 0.99886 * self.pink_b[0] + white * 0.0555179;
        self.pink_b[1] = 0.99332 * self.pink_b[1] + white * 0.0750759;
        self.pink_b[2] = 0.96900 * self.pink_b[2] + white * 0.1538520;
        self.pink_b[3] = 0.86650 * self.pink_b[3] + white * 0.3104856;
        self.pink_b[4] = 0.55000 * self.pink_b[4] + white * 0.5329522;
        self.pink_b[5] = -0.7616 * self.pink_b[5] - white * 0.0168980;
        (self.pink_b[0]
            + self.pink_b[1]
            + self.pink_b[2]
            + self.pink_b[3]
            + self.pink_b[4]
            + self.pink_b[5]
            + white * 0.5362)
            * 0.18
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

        let mode = self.target_mode;
        let density = self.target_density;
        let duck_amt = self.target_duck;
        let level = self.target_level;
        let mix = self.target_mix;
        let dry_gain = 1.0 - mix;

        for s in 0..num_samples {
            let raw_in_l = in_l[s];
            let raw_in_r = if s < in_r.len() { in_r[s] } else { raw_in_l };

            // 1. Detección de envolvente para Sidechain Ducking y Noise Gate
            let in_mag = 0.5 * (raw_in_l.abs() + raw_in_r.abs());
            self.env_follower = if in_mag > self.env_follower {
                0.95 * self.env_follower + 0.05 * in_mag
            } else {
                0.999 * self.env_follower + 0.001 * in_mag
            };
            let duck_gain = (1.0 - (self.env_follower * duck_amt * 2.5)).clamp(0.0, 1.0);
            let input_gate = (self.env_follower * 50.0).clamp(0.0, 1.0);

            // 2. Generación de ruido crudo base
            let white_l = self.next_white();
            let white_r = self.next_white();

            let (raw_noise_l, raw_noise_r) = match mode {
                0 => {
                    // White Noise
                    (white_l * 0.4, white_r * 0.4)
                }
                1 => {
                    // Pink Noise (Kellet 3-pole filter)
                    let p_l = self.process_pink(white_l);
                    let p_r = self.process_pink(white_r);
                    (p_l, p_r)
                }
                2 => {
                    // Tape Hiss (Banda acotada cálida)
                    let p_l = self.process_pink(white_l);
                    let p_r = self.process_pink(white_r);
                    (p_l * 0.6 + white_l * 0.15, p_r * 0.6 + white_r * 0.15)
                }
                3 => {
                    // Vinyl Crackle
                    let surface_l = self.process_pink(white_l) * 0.15;
                    let surface_r = self.process_pink(white_r) * 0.15;

                    let mut crackle_l = 0.0f32;
                    let mut crackle_r = 0.0f32;
                    let threshold = (density * 6500.0) as u32;
                    if (self.xorshift32() & 0x0FFFFF) < threshold {
                        let sign = if self.next_white() > 0.0 { 1.0 } else { -1.0 };
                        let spike = sign * (0.6 + 0.4 * self.next_white().abs());
                        crackle_l = spike;
                        crackle_r = spike * 0.85;
                    }

                    crackle_l = self.vinyl_state[0].process(crackle_l, &self.vinyl_coeffs);
                    crackle_r = self.vinyl_state[1].process(crackle_r, &self.vinyl_coeffs);

                    (surface_l + crackle_l * 2.2, surface_r + crackle_r * 2.2)
                }
                4 => {
                    // Rain / Organic Ambient
                    let pink = self.process_pink(white_l);
                    self.rain_mod_phase += 0.0001;
                    let rain_lfo = 0.7 + 0.3 * self.rain_mod_phase.sin();
                    let n_l = pink * rain_lfo * 0.5;
                    let n_r = self.process_pink(white_r) * (1.4 - rain_lfo) * 0.5;
                    (n_l, n_r)
                }
                _ => (white_l * 0.3, white_r * 0.3),
            };

            // 3. Filtrado de tono (Tone LPF)
            let filtered_noise_l = self.tone_state[0].process(raw_noise_l, &self.tone_coeffs) * (level * duck_gain * input_gate);
            let filtered_noise_r = self.tone_state[1].process(raw_noise_r, &self.tone_coeffs) * (level * duck_gain * input_gate);

            // 4. Inyección a la salida y mezcla Dry/Wet
            let wet_l = raw_in_l + filtered_noise_l;
            let wet_r = raw_in_r + filtered_noise_r;

            out_l[s] = denormal_guard(dry_gain * raw_in_l + mix * wet_l);
            if s < out_r.len() {
                out_r[s] = denormal_guard(dry_gain * raw_in_r + mix * wet_r);
            }
        }
    }
}
