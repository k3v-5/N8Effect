//! Multi-Formant Resonant Vocal Filter with Continuous Vowel Morphing (A-E-I-O-U)
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio thread,
//! 3-band parallel vocal formant resonators per stereo channel, warmth saturation,
//! continuous morphing interpolation, and denormal protection.

use crate::common::{denormal_guard, fast_tanh, BiquadCoeffs, BiquadState};

#[derive(Clone, Copy)]
struct FormantFreqs {
    f1: f32,
    f2: f32,
    f3: f32,
    gain1: f32,
    gain2: f32,
    gain3: f32,
}

const VOWEL_TABLE: [FormantFreqs; 5] = [
    FormantFreqs { f1: 800.0, f2: 1200.0, f3: 2500.0, gain1: 1.0, gain2: 0.70, gain3: 0.35 }, // A
    FormantFreqs { f1: 400.0, f2: 2000.0, f3: 2800.0, gain1: 1.0, gain2: 0.80, gain3: 0.30 }, // E
    FormantFreqs { f1: 250.0, f2: 2300.0, f3: 3000.0, gain1: 1.0, gain2: 0.85, gain3: 0.40 }, // I
    FormantFreqs { f1: 500.0, f2: 850.0,  f3: 2400.0, gain1: 1.0, gain2: 0.65, gain3: 0.25 }, // O
    FormantFreqs { f1: 300.0, f2: 700.0,  f3: 2200.0, gain1: 1.0, gain2: 0.60, gain3: 0.20 }, // U
];

pub struct RustFormantFilter {
    sample_rate: f32,
    coeffs: [BiquadCoeffs; 3],
    current_gains: [f32; 3],
    state_l: [BiquadState; 3],
    state_r: [BiquadState; 3],

    target_vowel: f32,
    target_resonance: f32,
    target_shift: f32,
    target_warmth: f32,
    target_mix: f32,
}

impl RustFormantFilter {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut filter = Self {
            sample_rate: sr,
            coeffs: [BiquadCoeffs::default(); 3],
            current_gains: [1.0, 0.7, 0.35],
            state_l: [BiquadState::default(); 3],
            state_r: [BiquadState::default(); 3],
            target_vowel: 0.0,
            target_resonance: 6.0,
            target_shift: 1.0,
            target_warmth: 0.3,
            target_mix: 1.0,
        };
        filter.update_filters();
        filter
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.update_filters();
        self.reset();
    }

    pub fn reset(&mut self) {
        for s in &mut self.state_l {
            s.reset();
        }
        for s in &mut self.state_r {
            s.reset();
        }
    }

    fn update_filters(&mut self) {
        let sr = self.sample_rate;
        let nyquist = sr * 0.48;

        let v = self.target_vowel.clamp(0.0, 3.999);
        let idx0 = v as usize;
        let idx1 = (idx0 + 1).min(4);
        let frac = v - (idx0 as f32);

        let tab0 = &VOWEL_TABLE[idx0];
        let tab1 = &VOWEL_TABLE[idx1];

        let f1 = ((tab0.f1 + frac * (tab1.f1 - tab0.f1)) * self.target_shift).clamp(100.0, nyquist);
        let f2 = ((tab0.f2 + frac * (tab1.f2 - tab0.f2)) * self.target_shift).clamp(200.0, nyquist);
        let f3 = ((tab0.f3 + frac * (tab1.f3 - tab0.f3)) * self.target_shift).clamp(500.0, nyquist);

        self.current_gains[0] = tab0.gain1 + frac * (tab1.gain1 - tab0.gain1);
        self.current_gains[1] = tab0.gain2 + frac * (tab1.gain2 - tab0.gain2);
        self.current_gains[2] = tab0.gain3 + frac * (tab1.gain3 - tab0.gain3);

        let q = self.target_resonance.clamp(1.0, 25.0);

        self.coeffs[0] = BiquadCoeffs::bandpass(sr, f1, q);
        self.coeffs[1] = BiquadCoeffs::bandpass(sr, f2, q);
        self.coeffs[2] = BiquadCoeffs::bandpass(sr, f3, q);
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => {
                self.target_vowel = value.clamp(0.0, 4.0);
                self.update_filters();
            }
            2 => {
                self.target_resonance = value.clamp(1.0, 25.0);
                self.update_filters();
            }
            3 => {
                self.target_shift = value.clamp(0.5, 2.0);
                self.update_filters();
            }
            4 => self.target_warmth = value.clamp(0.0, 1.0),
            5 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_vowel,
            2 => self.target_resonance,
            3 => self.target_shift,
            4 => self.target_warmth,
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

        let warmth = self.target_warmth;
        let mix = self.target_mix;
        let dry_gain = 1.0 - mix;
        let g = self.current_gains;

        for s in 0..num_samples {
            let s_l = denormal_guard(in_l[s]);
            let s_r = denormal_guard(if s < in_r.len() { in_r[s] } else { s_l });

            // Canal izquierdo: 3 formantes en paralelo
            let mut formant_sum_l = g[0] * self.state_l[0].process(s_l, &self.coeffs[0])
                                  + g[1] * self.state_l[1].process(s_l, &self.coeffs[1])
                                  + g[2] * self.state_l[2].process(s_l, &self.coeffs[2]);

            // Canal derecho: 3 formantes en paralelo
            let mut formant_sum_r = g[0] * self.state_r[0].process(s_r, &self.coeffs[0])
                                  + g[1] * self.state_r[1].process(s_r, &self.coeffs[1])
                                  + g[2] * self.state_r[2].process(s_r, &self.coeffs[2]);

            if warmth > 0.01 {
                let drive = 1.0 + warmth * 2.5;
                formant_sum_l = fast_tanh(formant_sum_l * drive) * 1.15;
                formant_sum_r = fast_tanh(formant_sum_r * drive) * 1.15;
            }

            let wet_l = formant_sum_l * 1.8;
            let wet_r = formant_sum_r * 1.8;

            out_l[s] = denormal_guard(dry_gain * s_l + mix * wet_l);
            if s < out_r.len() {
                out_r[s] = denormal_guard(dry_gain * s_r + mix * wet_r);
            }
        }
    }
}
