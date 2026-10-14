//! Subharmonic Synthesizer & Analog Bass Extender (Rules 5, 8, 9, 14, 34, 35, 46, 47)
//! Inspired by classic sub-octave generators (dbx 120XP style).
//! Isolates low bass fundamental with a 2nd-order bandpass filter, tracks pitch zero-crossings
//! via Schmitt-trigger hysteresis, divides waveform frequencies by 2 (-1 octave) and 4 (-2 octaves),
//! filters harmonics through dedicated lowpass smoothing filters, shapes dynamics through
//! an input envelope follower, and applies warm rational tanh saturation.

use crate::common::{clamp, lerp, denormal_guard, fast_tanh, BiquadCoeffs, BiquadState};

const BUTTERWORTH_Q: f32 = 0.70710678;

pub struct RustSubharmonicSynthesizer {
    sample_rate: f32,

    // Bandpass filter isolating the fundamental tracking band (40 to 140 Hz)
    bp_l: BiquadState,
    bp_r: BiquadState,
    bp_coeffs: BiquadCoeffs,

    // Smoothing lowpass filters for the synthesized square waves (-1 and -2 octaves)
    lp_sub1_l: BiquadState,
    lp_sub1_r: BiquadState,
    lp_sub2_l: BiquadState,
    lp_sub2_r: BiquadState,
    lp_sub_coeffs: BiquadCoeffs,

    // Schmitt trigger zero-crossing states
    last_sign_l: bool,
    last_sign_r: bool,
    ff1_l: bool,
    ff1_r: bool,
    ff2_l: bool,
    ff2_r: bool,
    ff1_counter_l: u32,
    ff1_counter_r: u32,

    // Dynamic envelope follower for natural breathing
    env_in: f32,

    // Parameters
    sub_band_freq: f32,    // 40.0 to 140.0 Hz (tracking cutoff)
    sub1_level: f32,       // 0.0 to 1.0 (-1 octave level)
    sub2_level: f32,       // 0.0 to 1.0 (-2 octaves level)
    drive: f32,            // 1.0 to 4.0
    dry_bass_level: f32,   // 0.0 to 1.5
    mix: f32,              // 0.0 to 1.0

    // Smoothed
    smoothed_sub1: f32,
    smoothed_sub2: f32,
    smoothed_mix: f32,
}

impl RustSubharmonicSynthesizer {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        let mut synth = Self {
            sample_rate: sr,
            bp_l: BiquadState::default(),
            bp_r: BiquadState::default(),
            bp_coeffs: BiquadCoeffs::default(),
            lp_sub1_l: BiquadState::default(),
            lp_sub1_r: BiquadState::default(),
            lp_sub2_l: BiquadState::default(),
            lp_sub2_r: BiquadState::default(),
            lp_sub_coeffs: BiquadCoeffs::default(),
            last_sign_l: false,
            last_sign_r: false,
            ff1_l: false,
            ff1_r: false,
            ff2_l: false,
            ff2_r: false,
            ff1_counter_l: 0,
            ff1_counter_r: 0,
            env_in: 0.0,
            sub_band_freq: 85.0,
            sub1_level: 0.7,
            sub2_level: 0.4,
            drive: 1.2,
            dry_bass_level: 1.0,
            mix: 1.0,
            smoothed_sub1: 0.7,
            smoothed_sub2: 0.4,
            smoothed_mix: 1.0,
        };
        synth.update_filters();
        synth
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.update_filters();
        self.reset();
    }

    pub fn reset(&mut self) {
        self.bp_l.reset();
        self.bp_r.reset();
        self.lp_sub1_l.reset();
        self.lp_sub1_r.reset();
        self.lp_sub2_l.reset();
        self.lp_sub2_r.reset();
        self.last_sign_l = false;
        self.last_sign_r = false;
        self.ff1_l = false;
        self.ff1_r = false;
        self.ff2_l = false;
        self.ff2_r = false;
        self.ff1_counter_l = 0;
        self.ff1_counter_r = 0;
        self.env_in = 0.0;
        self.smoothed_sub1 = self.sub1_level;
        self.smoothed_sub2 = self.sub2_level;
        self.smoothed_mix = self.mix;
    }

    fn update_filters(&mut self) {
        let fc = clamp(self.sub_band_freq, 35.0, 160.0);
        self.bp_coeffs = BiquadCoeffs::bandpass(self.sample_rate, fc, 1.2);
        // Lowpass smooths synthesized subharmonics below tracking frequency
        self.lp_sub_coeffs = BiquadCoeffs::lowpass(self.sample_rate, fc * 0.9, BUTTERWORTH_Q);
    }

    pub fn set_param(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => {
                self.sub_band_freq = clamp(value, 35.0, 160.0);
                self.update_filters();
            }
            2 => self.sub1_level = clamp(value, 0.0, 1.2),
            3 => self.sub2_level = clamp(value, 0.0, 1.2),
            4 => self.drive = clamp(value, 1.0, 4.0),
            5 => self.dry_bass_level = clamp(value, 0.0, 2.0),
            6 => self.mix = clamp(value, 0.0, 1.0),
            _ => {}
        }
    }

    pub fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: u32,
    ) {
        if in_l.is_null() || in_r.is_null() || out_l.is_null() || out_r.is_null() || num_samples == 0 {
            return;
        }

        let smooth = (-1.0 / (0.005 * self.sample_rate)).exp();
        let env_alpha = (-1.0 / (0.012 * self.sample_rate)).exp();
        let schmitt_high = 0.02;
        let schmitt_low = -0.02;

        for i in 0..num_samples as usize {
            let xl = unsafe { *in_l.add(i) };
            let xr = unsafe { *in_r.add(i) };

            self.smoothed_sub1 = lerp(self.sub1_level, self.smoothed_sub1, smooth);
            self.smoothed_sub2 = lerp(self.sub2_level, self.smoothed_sub2, smooth);
            self.smoothed_mix = lerp(self.mix, self.smoothed_mix, smooth);

            // Filter tracking fundamental band
            let bass_l = self.bp_l.process(xl, &self.bp_coeffs);
            let bass_r = self.bp_r.process(xr, &self.bp_coeffs);

            let in_mag = 0.5 * (bass_l.abs() + bass_r.abs());
            self.env_in = lerp(in_mag, self.env_in, env_alpha);

            // Silence gate (Rule 38)
            if self.env_in < 1.0e-5 && in_mag < 1.0e-5 {
                unsafe {
                    *out_l.add(i) = 0.0;
                    *out_r.add(i) = 0.0;
                }
                continue;
            }

            // Left channel flip-flop frequency division
            if bass_l > schmitt_high && !self.last_sign_l {
                self.last_sign_l = true;
                self.ff1_l = !self.ff1_l;
                self.ff1_counter_l += 1;
                if self.ff1_counter_l >= 2 {
                    self.ff1_counter_l = 0;
                    self.ff2_l = !self.ff2_l;
                }
            } else if bass_l < schmitt_low && self.last_sign_l {
                self.last_sign_l = false;
            }

            // Right channel flip-flop frequency division
            if bass_r > schmitt_high && !self.last_sign_r {
                self.last_sign_r = true;
                self.ff1_r = !self.ff1_r;
                self.ff1_counter_r += 1;
                if self.ff1_counter_r >= 2 {
                    self.ff1_counter_r = 0;
                    self.ff2_r = !self.ff2_r;
                }
            } else if bass_r < schmitt_low && self.last_sign_r {
                self.last_sign_r = false;
            }

            let sq1_l = if self.ff1_l { 1.0 } else { -1.0 };
            let sq1_r = if self.ff1_r { 1.0 } else { -1.0 };
            let sq2_l = if self.ff2_l { 1.0 } else { -1.0 };
            let sq2_r = if self.ff2_r { 1.0 } else { -1.0 };

            // Smooth square waves into deep sine waves with lowpass filtering
            let sine1_l = self.lp_sub1_l.process(sq1_l, &self.lp_sub_coeffs);
            let sine1_r = self.lp_sub1_r.process(sq1_r, &self.lp_sub_coeffs);
            let sine2_l = self.lp_sub2_l.process(sq2_l, &self.lp_sub_coeffs);
            let sine2_r = self.lp_sub2_r.process(sq2_r, &self.lp_sub_coeffs);

            // Dynamically scale synthesized waves with input envelope
            let sub_raw_l = (sine1_l * self.smoothed_sub1 + sine2_l * self.smoothed_sub2) * self.env_in;
            let sub_raw_r = (sine1_r * self.smoothed_sub1 + sine2_r * self.smoothed_sub2) * self.env_in;

            // Apply analog drive warmth
            let sub_driven_l = fast_tanh(sub_raw_l * self.drive);
            let sub_driven_r = fast_tanh(sub_raw_r * self.drive);

            let wet_l = sub_driven_l + bass_l * self.dry_bass_level;
            let wet_r = sub_driven_r + bass_r * self.dry_bass_level;

            let out_sample_l = denormal_guard(lerp(xl, wet_l, self.smoothed_mix));
            let out_sample_r = denormal_guard(lerp(xr, wet_r, self.smoothed_mix));

            unsafe {
                *out_l.add(i) = out_sample_l;
                *out_r.add(i) = out_sample_r;
            }
        }
    }
}
