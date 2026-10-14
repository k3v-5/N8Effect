//! Transient Shaper Engine in Rust (DSP #33)
//! Independent Punch & Sustain dynamics processing in Wideband & 3-Band Linkwitz-Riley LR4 Multiband modes
//! with analog soft clipping and zero memory allocations (Rules 5, 8, 9, 14, 32, 34, 46, 47).

use crate::common::{fast_tanh, BiquadCoeffs, BiquadState};

pub struct RustLinkwitzRileyCrossover3Way {
    sample_rate: f32,
    f_low_mid: f32,
    f_mid_high: f32,

    // Filter coefficients
    lp_low_coeffs: BiquadCoeffs,
    hp_rest_coeffs: BiquadCoeffs,
    ap_low_coeffs: BiquadCoeffs,
    lp_mid_coeffs: BiquadCoeffs,
    hp_high_coeffs: BiquadCoeffs,

    // Filter states [channel 0 = L, 1 = R]
    lp_low1: [BiquadState; 2],
    lp_low2: [BiquadState; 2],
    hp_rest1: [BiquadState; 2],
    hp_rest2: [BiquadState; 2],
    ap_low: [BiquadState; 2],
    lp_mid1: [BiquadState; 2],
    lp_mid2: [BiquadState; 2],
    hp_high1: [BiquadState; 2],
    hp_high2: [BiquadState; 2],
}

impl RustLinkwitzRileyCrossover3Way {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut xover = Self {
            sample_rate: sr,
            f_low_mid: 250.0,
            f_mid_high: 3000.0,
            lp_low_coeffs: BiquadCoeffs::default(),
            hp_rest_coeffs: BiquadCoeffs::default(),
            ap_low_coeffs: BiquadCoeffs::default(),
            lp_mid_coeffs: BiquadCoeffs::default(),
            hp_high_coeffs: BiquadCoeffs::default(),
            lp_low1: [BiquadState::default(); 2],
            lp_low2: [BiquadState::default(); 2],
            hp_rest1: [BiquadState::default(); 2],
            hp_rest2: [BiquadState::default(); 2],
            ap_low: [BiquadState::default(); 2],
            lp_mid1: [BiquadState::default(); 2],
            lp_mid2: [BiquadState::default(); 2],
            hp_high1: [BiquadState::default(); 2],
            hp_high2: [BiquadState::default(); 2],
        };
        xover.update_coefficients();
        xover
    }

    pub fn reset(&mut self) {
        for ch in 0..2 {
            self.lp_low1[ch].reset();
            self.lp_low2[ch].reset();
            self.hp_rest1[ch].reset();
            self.hp_rest2[ch].reset();
            self.ap_low[ch].reset();
            self.lp_mid1[ch].reset();
            self.lp_mid2[ch].reset();
            self.hp_high1[ch].reset();
            self.hp_high2[ch].reset();
        }
    }

    pub fn set_sample_rate(&mut self, sample_rate: f32) {
        if sample_rate > 0.0 {
            self.sample_rate = sample_rate;
            self.update_coefficients();
        }
    }

    pub fn set_crossover_frequencies(&mut self, f_low_mid_hz: f32, f_mid_high_hz: f32) {
        let nyquist = self.sample_rate * 0.49;
        self.f_low_mid = f_low_mid_hz.clamp(20.0, nyquist * 0.5);
        self.f_mid_high = f_mid_high_hz.clamp(self.f_low_mid * 1.2, nyquist);
        self.update_coefficients();
    }

    fn update_coefficients(&mut self) {
        const Q_BUTTERWORTH: f32 = 0.70710678;
        let sr = self.sample_rate;
        self.lp_low_coeffs = BiquadCoeffs::lowpass(sr, self.f_low_mid, Q_BUTTERWORTH);
        self.hp_rest_coeffs = BiquadCoeffs::highpass(sr, self.f_low_mid, Q_BUTTERWORTH);
        self.ap_low_coeffs = BiquadCoeffs::allpass(sr, self.f_mid_high, Q_BUTTERWORTH);
        self.lp_mid_coeffs = BiquadCoeffs::lowpass(sr, self.f_mid_high, Q_BUTTERWORTH);
        self.hp_high_coeffs = BiquadCoeffs::highpass(sr, self.f_mid_high, Q_BUTTERWORTH);
    }

    #[inline(always)]
    pub fn process_sample(&mut self, ch: usize, in_val: f32) -> (f32, f32, f32) {
        let c = if ch != 0 { 1 } else { 0 };

        // 1. Crossover Low-Mid
        let low1 = self.lp_low1[c].process(in_val, &self.lp_low_coeffs);
        let low_raw = self.lp_low2[c].process(low1, &self.lp_low_coeffs);

        let rest1 = self.hp_rest1[c].process(in_val, &self.hp_rest_coeffs);
        let rest = self.hp_rest2[c].process(rest1, &self.hp_rest_coeffs);

        // Phase alignment for low band relative to Mid-High
        let low_aligned = self.ap_low[c].process(low_raw, &self.ap_low_coeffs);

        // 2. Crossover Mid-High
        let mid1 = self.lp_mid1[c].process(rest, &self.lp_mid_coeffs);
        let mid = self.lp_mid2[c].process(mid1, &self.lp_mid_coeffs);

        let high1 = self.hp_high1[c].process(rest, &self.hp_high_coeffs);
        let high = self.hp_high2[c].process(high1, &self.hp_high_coeffs);

        (low_aligned, mid, high)
    }
}

pub struct RustTransientShaper {
    sample_rate: f32,
    crossover: RustLinkwitzRileyCrossover3Way,

    // [band 0..3: 0=Low/Wide, 1=Mid, 2=High][channel 0=L, 1=R]
    fast_env: [[f32; 2]; 3],
    slow_env: [[f32; 2]; 3],

    fast_att_coeff: f32,
    fast_rel_coeff: f32,
    slow_att_coeff: f32,
    slow_rel_coeff: f32,

    // Parameters
    param_attack: f32,           // 1: Attack (-1.0 .. 1.0)
    param_sustain: f32,          // 2: Sustain (-1.0 .. 1.0)
    param_speed: f32,            // 3: Speed (0.5 .. 2.0)
    param_soft_clip: f32,        // 4: SoftClip (0.0 or 1.0)
    param_gain: f32,             // 5: OutputGain (-12.0 .. 12.0 dB)
    param_mix: f32,              // 6: Mix (0.0 .. 1.0)
    param_mode: f32,             // 7: Mode (0.0: Wideband, 1.0: Multiband)
    param_low_attack: f32,       // 8: LowAttack (-1.0 .. 1.0)
    param_mid_attack: f32,       // 9: MidAttack (-1.0 .. 1.0)
    param_high_attack: f32,      // 10: HighAttack (-1.0 .. 1.0)
    param_low_sustain: f32,      // 11: LowSustain (-1.0 .. 1.0)
    param_mid_sustain: f32,      // 12: MidSustain (-1.0 .. 1.0)
    param_high_sustain: f32,     // 13: HighSustain (-1.0 .. 1.0)
    param_xover_low_mid: f32,    // 14: CrossoverLowMid (40.0 .. 1000.0 Hz)
    param_xover_mid_high: f32,   // 15: CrossoverMidHigh (1000.0 .. 12000.0 Hz)
}

impl RustTransientShaper {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut shaper = Self {
            sample_rate: sr,
            crossover: RustLinkwitzRileyCrossover3Way::new(sr),
            fast_env: [[0.0; 2]; 3],
            slow_env: [[0.0; 2]; 3],
            fast_att_coeff: 0.1,
            fast_rel_coeff: 0.01,
            slow_att_coeff: 0.01,
            slow_rel_coeff: 0.002,
            param_attack: 0.0,
            param_sustain: 0.0,
            param_speed: 1.0,
            param_soft_clip: 1.0,
            param_gain: 0.0,
            param_mix: 1.0,
            param_mode: 0.0,
            param_low_attack: 0.0,
            param_mid_attack: 0.0,
            param_high_attack: 0.0,
            param_low_sustain: 0.0,
            param_mid_sustain: 0.0,
            param_high_sustain: 0.0,
            param_xover_low_mid: 250.0,
            param_xover_mid_high: 3000.0,
        };
        shaper.update_coeffs();
        shaper
    }

    pub fn reset(&mut self) {
        self.crossover.reset();
        self.fast_env = [[0.0; 2]; 3];
        self.slow_env = [[0.0; 2]; 3];
    }

    pub fn set_sample_rate(&mut self, sample_rate: f32) {
        if sample_rate > 0.0 {
            self.sample_rate = sample_rate;
            self.crossover.set_sample_rate(sample_rate);
            self.update_coeffs();
        }
    }

    fn update_coeffs(&mut self) {
        let sr = self.sample_rate;
        let speed = self.param_speed.clamp(0.5, 2.0);

        // Fast envelope (Attack): 2.0 ms attack, 15.0 ms release
        self.fast_att_coeff = 1.0 - (-1.0 / ((0.002 / speed) * sr)).exp();
        self.fast_rel_coeff = 1.0 - (-1.0 / ((0.015 / speed) * sr)).exp();

        // Slow envelope (Sustain): 25.0 ms attack, 120.0 ms release
        self.slow_att_coeff = 1.0 - (-1.0 / ((0.025 / speed) * sr)).exp();
        self.slow_rel_coeff = 1.0 - (-1.0 / ((0.120 / speed) * sr)).exp();
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.param_attack = value.clamp(-1.0, 1.0),
            2 => self.param_sustain = value.clamp(-1.0, 1.0),
            3 => {
                self.param_speed = value.clamp(0.5, 2.0);
                self.update_coeffs();
            }
            4 => self.param_soft_clip = if value >= 0.5 { 1.0 } else { 0.0 },
            5 => self.param_gain = value.clamp(-12.0, 12.0),
            6 => self.param_mix = value.clamp(0.0, 1.0),
            7 => self.param_mode = if value >= 0.5 { 1.0 } else { 0.0 },
            8 => self.param_low_attack = value.clamp(-1.0, 1.0),
            9 => self.param_mid_attack = value.clamp(-1.0, 1.0),
            10 => self.param_high_attack = value.clamp(-1.0, 1.0),
            11 => self.param_low_sustain = value.clamp(-1.0, 1.0),
            12 => self.param_mid_sustain = value.clamp(-1.0, 1.0),
            13 => self.param_high_sustain = value.clamp(-1.0, 1.0),
            14 => {
                self.param_xover_low_mid = value.clamp(40.0, 1000.0);
                self.crossover
                    .set_crossover_frequencies(self.param_xover_low_mid, self.param_xover_mid_high);
            }
            15 => {
                self.param_xover_mid_high = value.clamp(1000.0, 12000.0);
                self.crossover
                    .set_crossover_frequencies(self.param_xover_low_mid, self.param_xover_mid_high);
            }
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.param_attack,
            2 => self.param_sustain,
            3 => self.param_speed,
            4 => self.param_soft_clip,
            5 => self.param_gain,
            6 => self.param_mix,
            7 => self.param_mode,
            8 => self.param_low_attack,
            9 => self.param_mid_attack,
            10 => self.param_high_attack,
            11 => self.param_low_sustain,
            12 => self.param_mid_sustain,
            13 => self.param_high_sustain,
            14 => self.param_xover_low_mid,
            15 => self.param_xover_mid_high,
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

        let out_gain_lin = 10.0f32.powf(self.param_gain / 20.0);
        let soft_clip = self.param_soft_clip >= 0.5;
        let mix = self.param_mix.clamp(0.0, 1.0);
        let is_multiband = self.param_mode >= 0.5;

        if !is_multiband {
            // Mode 1: Wideband
            let attack_amount = self.param_attack * 2.0;
            let sustain_amount = self.param_sustain * 1.5;

            for s in 0..n_samples {
                let raw_in_l = in_l[s];
                let raw_in_r = if s < in_r.len() { in_r[s] } else { raw_in_l };

                let in_l_mag = raw_in_l.abs();
                let in_r_mag = raw_in_r.abs();

                self.fast_env[0][0] = if in_l_mag > self.fast_env[0][0] {
                    self.fast_env[0][0] + self.fast_att_coeff * (in_l_mag - self.fast_env[0][0])
                } else {
                    self.fast_env[0][0] + self.fast_rel_coeff * (in_l_mag - self.fast_env[0][0])
                };
                self.slow_env[0][0] = if in_l_mag > self.slow_env[0][0] {
                    self.slow_env[0][0] + self.slow_att_coeff * (in_l_mag - self.slow_env[0][0])
                } else {
                    self.slow_env[0][0] + self.slow_rel_coeff * (in_l_mag - self.slow_env[0][0])
                };

                self.fast_env[0][1] = if in_r_mag > self.fast_env[0][1] {
                    self.fast_env[0][1] + self.fast_att_coeff * (in_r_mag - self.fast_env[0][1])
                } else {
                    self.fast_env[0][1] + self.fast_rel_coeff * (in_r_mag - self.fast_env[0][1])
                };
                self.slow_env[0][1] = if in_r_mag > self.slow_env[0][1] {
                    self.slow_env[0][1] + self.slow_att_coeff * (in_r_mag - self.slow_env[0][1])
                } else {
                    self.slow_env[0][1] + self.slow_rel_coeff * (in_r_mag - self.slow_env[0][1])
                };

                let delta_l = self.fast_env[0][0] - self.slow_env[0][0];
                let delta_r = self.fast_env[0][1] - self.slow_env[0][1];

                let mut gain_l = 1.0 + attack_amount * delta_l + sustain_amount * self.slow_env[0][0];
                let mut gain_r = 1.0 + attack_amount * delta_r + sustain_amount * self.slow_env[0][1];

                gain_l = gain_l.clamp(0.0, 4.0);
                gain_r = gain_r.clamp(0.0, 4.0);

                let mut wet_l = raw_in_l * gain_l * out_gain_lin;
                let mut wet_r = raw_in_r * gain_r * out_gain_lin;

                if soft_clip {
                    wet_l = fast_tanh(wet_l);
                    wet_r = fast_tanh(wet_r);
                }

                out_l[s] = (1.0 - mix) * raw_in_l + mix * wet_l;
                if s < out_r.len() {
                    out_r[s] = (1.0 - mix) * raw_in_r + mix * wet_r;
                }
            }
        } else {
            // Mode 2: Multiband LR4 (3-Band)
            let low_att_amount = (self.param_low_attack + self.param_attack).clamp(-1.0, 1.0) * 2.0;
            let mid_att_amount = (self.param_mid_attack + self.param_attack).clamp(-1.0, 1.0) * 2.0;
            let high_att_amount = (self.param_high_attack + self.param_attack).clamp(-1.0, 1.0) * 2.0;

            let low_sus_amount = (self.param_low_sustain + self.param_sustain).clamp(-1.0, 1.0) * 1.5;
            let mid_sus_amount = (self.param_mid_sustain + self.param_sustain).clamp(-1.0, 1.0) * 1.5;
            let high_sus_amount = (self.param_high_sustain + self.param_sustain).clamp(-1.0, 1.0) * 1.5;

            let att_amounts = [low_att_amount, mid_att_amount, high_att_amount];
            let sus_amounts = [low_sus_amount, mid_sus_amount, high_sus_amount];

            for s in 0..n_samples {
                let raw_in_l = in_l[s];
                let raw_in_r = if s < in_r.len() { in_r[s] } else { raw_in_l };

                let (band_l_low, band_l_mid, band_l_high) = self.crossover.process_sample(0, raw_in_l);
                let (band_r_low, band_r_mid, band_r_high) = self.crossover.process_sample(1, raw_in_r);

                let bands_l = [band_l_low, band_l_mid, band_l_high];
                let bands_r = [band_r_low, band_r_mid, band_r_high];

                let mut sum_wet_l = 0.0f32;
                let mut sum_wet_r = 0.0f32;

                for b in 0..3 {
                    let b_l_mag = bands_l[b].abs();
                    let b_r_mag = bands_r[b].abs();

                    self.fast_env[b][0] = if b_l_mag > self.fast_env[b][0] {
                        self.fast_env[b][0] + self.fast_att_coeff * (b_l_mag - self.fast_env[b][0])
                    } else {
                        self.fast_env[b][0] + self.fast_rel_coeff * (b_l_mag - self.fast_env[b][0])
                    };
                    self.slow_env[b][0] = if b_l_mag > self.slow_env[b][0] {
                        self.slow_env[b][0] + self.slow_att_coeff * (b_l_mag - self.slow_env[b][0])
                    } else {
                        self.slow_env[b][0] + self.slow_rel_coeff * (b_l_mag - self.slow_env[b][0])
                    };

                    self.fast_env[b][1] = if b_r_mag > self.fast_env[b][1] {
                        self.fast_env[b][1] + self.fast_att_coeff * (b_r_mag - self.fast_env[b][1])
                    } else {
                        self.fast_env[b][1] + self.fast_rel_coeff * (b_r_mag - self.fast_env[b][1])
                    };
                    self.slow_env[b][1] = if b_r_mag > self.slow_env[b][1] {
                        self.slow_env[b][1] + self.slow_att_coeff * (b_r_mag - self.slow_env[b][1])
                    } else {
                        self.slow_env[b][1] + self.slow_rel_coeff * (b_r_mag - self.slow_env[b][1])
                    };

                    let delta_l = self.fast_env[b][0] - self.slow_env[b][0];
                    let delta_r = self.fast_env[b][1] - self.slow_env[b][1];

                    let mut gain_l = 1.0 + att_amounts[b] * delta_l + sus_amounts[b] * self.slow_env[b][0];
                    let mut gain_r = 1.0 + att_amounts[b] * delta_r + sus_amounts[b] * self.slow_env[b][1];

                    gain_l = gain_l.clamp(0.0, 4.0);
                    gain_r = gain_r.clamp(0.0, 4.0);

                    sum_wet_l += bands_l[b] * gain_l;
                    sum_wet_r += bands_r[b] * gain_r;
                }

                let mut wet_l = sum_wet_l * out_gain_lin;
                let mut wet_r = sum_wet_r * out_gain_lin;

                if soft_clip {
                    wet_l = fast_tanh(wet_l);
                    wet_r = fast_tanh(wet_r);
                }

                out_l[s] = (1.0 - mix) * raw_in_l + mix * wet_l;
                if s < out_r.len() {
                    out_r[s] = (1.0 - mix) * raw_in_r + mix * wet_r;
                }
            }
        }
    }
}
