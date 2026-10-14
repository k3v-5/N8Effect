//! n8_rust_core - Dual Harmonic Exciter & Sub-Harmonic Synthesizer (DSP #44)
//! Psychoacoustic air sheen with asymmetric polynomial harmonic generation,
//! allpass phase coupling, and sub-octave flip-flop fundamental synthesis
//! (Rules 5, 8, 9, 14, 34, 46, 47).

use crate::common::{self, BiquadCoeffs, BiquadState};

pub const PARAM_AIR_FREQ: u32 = 1;
pub const PARAM_AIR_DRIVE: u32 = 2;
pub const PARAM_AIR_MIX: u32 = 3;
pub const PARAM_SUB_FREQ: u32 = 4;
pub const PARAM_SUB_DRIVE: u32 = 5;
pub const PARAM_SUB_MIX: u32 = 6;
pub const PARAM_MIX: u32 = 7;

pub struct RustHarmonicExciter {
    sample_rate: f32,

    air_hpf: [BiquadState; 2],
    air_phase_allpass: [BiquadState; 2],
    sub_lpf: [BiquadState; 2],
    sub_post_lpf: [BiquadState; 2],

    air_hpf_coeffs: BiquadCoeffs,
    air_phase_allpass_coeffs: BiquadCoeffs,
    sub_lpf_coeffs: BiquadCoeffs,
    sub_post_lpf_coeffs: BiquadCoeffs,

    sub_flip_flop: [f32; 2],
    prev_sub_sample: [f32; 2],

    // Parameters
    target_air_freq: f32,
    target_air_drive: f32,
    target_air_mix: f32,
    target_sub_freq: f32,
    target_sub_drive: f32,
    target_sub_mix: f32,
    target_mix: f32,
}

impl RustHarmonicExciter {
    pub fn new(sample_rate: f64) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate as f32 } else { 44100.0 };
        let mut instance = Self {
            sample_rate: sr,
            air_hpf: [BiquadState::default(); 2],
            air_phase_allpass: [BiquadState::default(); 2],
            sub_lpf: [BiquadState::default(); 2],
            sub_post_lpf: [BiquadState::default(); 2],

            air_hpf_coeffs: BiquadCoeffs { b0: 0.0, b1: 0.0, b2: 0.0, a1: 0.0, a2: 0.0 },
            air_phase_allpass_coeffs: BiquadCoeffs { b0: 0.0, b1: 0.0, b2: 0.0, a1: 0.0, a2: 0.0 },
            sub_lpf_coeffs: BiquadCoeffs { b0: 0.0, b1: 0.0, b2: 0.0, a1: 0.0, a2: 0.0 },
            sub_post_lpf_coeffs: BiquadCoeffs { b0: 0.0, b1: 0.0, b2: 0.0, a1: 0.0, a2: 0.0 },

            sub_flip_flop: [1.0, 1.0],
            prev_sub_sample: [0.0, 0.0],

            target_air_freq: 6000.0,
            target_air_drive: 2.0,
            target_air_mix: 0.5,
            target_sub_freq: 80.0,
            target_sub_drive: 1.5,
            target_sub_mix: 0.4,
            target_mix: 1.0,
        };
        instance.update_filters();
        instance.reset();
        instance
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        let sr = if sample_rate > 0.0 { sample_rate as f32 } else { 44100.0 };
        self.sample_rate = sr;
        self.update_filters();
        self.reset();
    }

    pub fn reset(&mut self) {
        for f in &mut self.air_hpf {
            f.reset();
        }
        for f in &mut self.air_phase_allpass {
            f.reset();
        }
        for f in &mut self.sub_lpf {
            f.reset();
        }
        for f in &mut self.sub_post_lpf {
            f.reset();
        }
        self.sub_flip_flop = [1.0, 1.0];
        self.prev_sub_sample = [0.0, 0.0];
    }

    fn update_filters(&mut self) {
        let sr = self.sample_rate;
        self.air_hpf_coeffs = BiquadCoeffs::highpass(sr, self.target_air_freq, 0.707);
        self.air_phase_allpass_coeffs = BiquadCoeffs::allpass(sr, self.target_air_freq * 0.8, 1.0);
        self.sub_lpf_coeffs = BiquadCoeffs::lowpass(sr, self.target_sub_freq, 0.707);
        self.sub_post_lpf_coeffs = BiquadCoeffs::lowpass(sr, self.target_sub_freq, 0.707);
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            PARAM_AIR_FREQ => {
                self.target_air_freq = value.clamp(3000.0, 12000.0);
                self.update_filters();
            }
            PARAM_AIR_DRIVE => self.target_air_drive = value.clamp(0.0, 6.0),
            PARAM_AIR_MIX => self.target_air_mix = value.clamp(0.0, 1.0),
            PARAM_SUB_FREQ => {
                self.target_sub_freq = value.clamp(40.0, 120.0);
                self.update_filters();
            }
            PARAM_SUB_DRIVE => self.target_sub_drive = value.clamp(0.0, 6.0),
            PARAM_SUB_MIX => self.target_sub_mix = value.clamp(0.0, 1.0),
            PARAM_MIX => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            PARAM_AIR_FREQ => self.target_air_freq,
            PARAM_AIR_DRIVE => self.target_air_drive,
            PARAM_AIR_MIX => self.target_air_mix,
            PARAM_SUB_FREQ => self.target_sub_freq,
            PARAM_SUB_DRIVE => self.target_sub_drive,
            PARAM_SUB_MIX => self.target_sub_mix,
            PARAM_MIX => self.target_mix,
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
        let num_samples = in_l.len().min(out_l.len());
        if num_samples == 0 {
            return;
        }

        let air_drive = 1.0 + self.target_air_drive;
        let air_mix = self.target_air_mix;
        let sub_drive = 1.0 + self.target_sub_drive;
        let sub_mix = self.target_sub_mix;
        let mix = self.target_mix;

        for s in 0..num_samples {
            let raw_in_l = in_l[s];
            let raw_in_r = if s < in_r.len() { in_r[s] } else { raw_in_l };

            // 1. Rama de Agudos (Air Exciter): Paso-alto -> saturación asimétrica -> rotación de fase
            let mut high_l = self.air_hpf[0].process(raw_in_l, &self.air_hpf_coeffs);
            let mut high_r = self.air_hpf[1].process(raw_in_r, &self.air_hpf_coeffs);

            // Generador asimétrico de armónicos pares e impares
            high_l = (high_l * air_drive)
                + 0.35 * (high_l * high_l * air_drive)
                - 0.15 * (high_l * high_l * high_l * air_drive);
            high_r = (high_r * air_drive)
                + 0.35 * (high_r * high_r * air_drive)
                - 0.15 * (high_r * high_r * high_r * air_drive);

            // Rotación de fase pasa-todo para acoplamiento armónico natural
            high_l = self.air_phase_allpass[0].process(high_l, &self.air_phase_allpass_coeffs);
            high_r = self.air_phase_allpass[1].process(high_r, &self.air_phase_allpass_coeffs);

            // 2. Rama de Subgraves (Sub-Harmonic Synthesizer)
            let low_l = self.sub_lpf[0].process(raw_in_l, &self.sub_lpf_coeffs);
            let low_r = self.sub_lpf[1].process(raw_in_r, &self.sub_lpf_coeffs);

            // Detección de cruce por cero para conmutación de sub-octava limpia (f0 / 2)
            if low_l > 0.0 && self.prev_sub_sample[0] <= 0.0 {
                self.sub_flip_flop[0] = -self.sub_flip_flop[0];
            }
            if low_r > 0.0 && self.prev_sub_sample[1] <= 0.0 {
                self.sub_flip_flop[1] = -self.sub_flip_flop[1];
            }
            self.prev_sub_sample[0] = low_l;
            self.prev_sub_sample[1] = low_r;

            let mut sub_l = low_l.abs() * self.sub_flip_flop[0] * sub_drive;
            let mut sub_r = low_r.abs() * self.sub_flip_flop[1] * sub_drive;

            // Suavizar onda para conservar solo el fundamental subgrave
            sub_l = self.sub_post_lpf[0].process(sub_l, &self.sub_post_lpf_coeffs);
            sub_r = self.sub_post_lpf[1].process(sub_r, &self.sub_post_lpf_coeffs);

            // 3. Mezcla de las bandas excitadas con el audio original
            let wet_l = raw_in_l + high_l * air_mix + sub_l * sub_mix;
            let wet_r = raw_in_r + high_r * air_mix + sub_r * sub_mix;

            out_l[s] = common::denormal_guard((1.0 - mix) * raw_in_l + mix * wet_l);
            if s < out_r.len() {
                out_r[s] = common::denormal_guard((1.0 - mix) * raw_in_r + mix * wet_r);
            }
        }
    }
}
