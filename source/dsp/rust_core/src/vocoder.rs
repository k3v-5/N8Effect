//! n8_rust_core - 16-Band Channel Vocoder (DSP #45)
//! 16-band log filter bank, envelope followers, internal harmonic carrier oscillator
//! (Sawtooth, Pulse, Noise, External/Sidechain), Formant Shifter and soft-clipping waveshaping
//! (Rules 5, 8, 9, 14, 34, 46, 47).

use crate::common::{self, BiquadCoeffs, BiquadState};

pub const NUM_BANDS: usize = 16;

pub const PARAM_CARRIER_MODE: u32 = 1;
pub const PARAM_CARRIER_PITCH: u32 = 2;
pub const PARAM_FORMANT_SHIFT: u32 = 3;
pub const PARAM_BAND_Q: u32 = 4;
pub const PARAM_RELEASE_TIME: u32 = 5;
pub const PARAM_MIX: u32 = 6;

pub struct RustVocoder {
    sample_rate: f32,

    mod_filters: [BiquadState; NUM_BANDS],
    car_filters: [BiquadState; NUM_BANDS],
    mod_coeffs: [BiquadCoeffs; NUM_BANDS],
    car_coeffs: [BiquadCoeffs; NUM_BANDS],
    envelopes: [f32; NUM_BANDS],

    osc_phase: f64,
    rng_state: u32,
    release_coeff: f32,

    // Parameters
    target_mode: i32,
    target_pitch: f32,
    target_shift: f32,
    target_q: f32,
    target_release: f32,
    target_mix: f32,
}

impl RustVocoder {
    pub fn new(sample_rate: f64) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate as f32 } else { 44100.0 };
        let mut instance = Self {
            sample_rate: sr,
            mod_filters: [BiquadState::default(); NUM_BANDS],
            car_filters: [BiquadState::default(); NUM_BANDS],
            mod_coeffs: [BiquadCoeffs { b0: 0.0, b1: 0.0, b2: 0.0, a1: 0.0, a2: 0.0 }; NUM_BANDS],
            car_coeffs: [BiquadCoeffs { b0: 0.0, b1: 0.0, b2: 0.0, a1: 0.0, a2: 0.0 }; NUM_BANDS],
            envelopes: [0.0f32; NUM_BANDS],
            osc_phase: 0.0,
            rng_state: 0x55AA1234,
            release_coeff: 0.01,

            target_mode: 0,
            target_pitch: 110.0,
            target_shift: 1.0,
            target_q: 6.0,
            target_release: 0.05,
            target_mix: 1.0,
        };
        instance.update_filters();
        instance.reset();
        instance
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        let sr = if sample_rate > 0.0 { sample_rate as f32 } else { 44100.0 };
        self.sample_rate = sr;
        self.osc_phase = 0.0;
        self.rng_state = 0x55AA1234;
        self.update_filters();
        self.reset();
    }

    pub fn reset(&mut self) {
        for b in 0..NUM_BANDS {
            self.mod_filters[b].reset();
            self.car_filters[b].reset();
            self.envelopes[b] = 0.0;
        }
        self.osc_phase = 0.0;
    }

    #[inline]
    fn update_coeffs(&mut self) {
        let sr = self.sample_rate as f64;
        self.release_coeff = (1.0 - (-1.0 / (self.target_release as f64 * sr)).exp()) as f32;
    }

    #[inline]
    fn update_filters(&mut self) {
        let sr = self.sample_rate;
        let nyquist = sr * 0.48;

        const MIN_FREQ: f64 = 140.0;
        const MAX_FREQ: f64 = 6800.0;

        for b in 0..NUM_BANDS {
            let t = b as f64 / (NUM_BANDS - 1) as f64;
            let fc = (MIN_FREQ * (MAX_FREQ / MIN_FREQ).powf(t)) as f32;

            let mod_fc = fc.min(nyquist);
            self.mod_coeffs[b] = BiquadCoeffs::bandpass(sr, mod_fc, self.target_q);

            let car_fc = (fc * self.target_shift).min(nyquist);
            self.car_coeffs[b] = BiquadCoeffs::bandpass(sr, car_fc, self.target_q);
        }

        self.update_coeffs();
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            PARAM_CARRIER_MODE => self.target_mode = value.round() as i32,
            PARAM_CARRIER_PITCH => self.target_pitch = value.clamp(35.0, 500.0),
            PARAM_FORMANT_SHIFT => {
                self.target_shift = value.clamp(0.5, 2.0);
                self.update_filters();
            }
            PARAM_BAND_Q => {
                self.target_q = value.clamp(1.5, 15.0);
                self.update_filters();
            }
            PARAM_RELEASE_TIME => {
                self.target_release = value.clamp(0.01, 0.25);
                self.update_coeffs();
            }
            PARAM_MIX => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            PARAM_CARRIER_MODE => self.target_mode as f32,
            PARAM_CARRIER_PITCH => self.target_pitch,
            PARAM_FORMANT_SHIFT => self.target_shift,
            PARAM_BAND_Q => self.target_q,
            PARAM_RELEASE_TIME => self.target_release,
            PARAM_MIX => self.target_mix,
            _ => 0.0,
        }
    }

    #[inline(always)]
    fn next_noise(&mut self) -> f32 {
        self.rng_state ^= self.rng_state << 13;
        self.rng_state ^= self.rng_state >> 17;
        self.rng_state ^= self.rng_state << 5;
        ((self.rng_state & 0x7FFFFFFF) as f32 / 1073741824.0) - 1.0
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        sc_l: Option<&[f32]>,
        sc_r: Option<&[f32]>,
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len().min(out_l.len());
        if num_samples == 0 {
            return;
        }

        let sample_rate = if self.sample_rate > 0.0 { self.sample_rate as f64 } else { 44100.0 };
        let phase_increment = self.target_pitch as f64 / sample_rate;

        let mode = self.target_mode;
        let att_coeff = 0.25f32;
        let rel_coeff = self.release_coeff;
        let mix = self.target_mix;

        let has_sidechain = sc_l.is_some();

        for s in 0..num_samples {
            let raw_in_l = in_l[s];
            let raw_in_r = if s < in_r.len() { in_r[s] } else { raw_in_l };

            let mod_sample: f32;
            let car_sample: f32;

            if has_sidechain {
                let raw_sc_l = sc_l.map_or(0.0, |ch| ch[s]);
                let raw_sc_r = sc_r.map_or(raw_sc_l, |ch| ch[s]);
                mod_sample = 0.5 * (raw_sc_l + raw_sc_r);
                car_sample = 0.5 * (raw_in_l + raw_in_r);
            } else {
                mod_sample = if mode == 3 { raw_in_l } else { 0.5 * (raw_in_l + raw_in_r) };

                match mode {
                    0 => {
                        car_sample = (2.0 * self.osc_phase - 1.0) as f32;
                    }
                    1 => {
                        car_sample = if self.osc_phase < 0.5 { 0.9 } else { -0.9 };
                    }
                    2 => {
                        car_sample = self.next_noise();
                    }
                    3 => {
                        car_sample = raw_in_r;
                    }
                    _ => {
                        car_sample = (2.0 * self.osc_phase - 1.0) as f32;
                    }
                }
            }

            self.osc_phase += phase_increment;
            if self.osc_phase >= 1.0 {
                self.osc_phase -= 1.0;
            }

            let mut vocoded_out = 0.0f32;

            for b in 0..NUM_BANDS {
                let mod_band = self.mod_filters[b].process(mod_sample, &self.mod_coeffs[b]);
                let mod_mag = mod_band.abs();

                if mod_mag > self.envelopes[b] {
                    self.envelopes[b] += att_coeff * (mod_mag - self.envelopes[b]);
                } else {
                    self.envelopes[b] += rel_coeff * (mod_mag - self.envelopes[b]);
                }

                let car_band = self.car_filters[b].process(car_sample, &self.car_coeffs[b]);
                vocoded_out += car_band * self.envelopes[b];
            }

            let wet = common::fast_tanh(vocoded_out * 4.5) * 1.3;

            out_l[s] = common::denormal_guard((1.0 - mix) * raw_in_l + mix * wet);
            if s < out_r.len() {
                out_r[s] = common::denormal_guard((1.0 - mix) * raw_in_r + mix * wet);
            }
        }
    }
}
