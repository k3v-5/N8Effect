//! Spectral Frequency-Split Spatial Dynamic Panner (Rules 5, 8, 9, 14, 34, 35, 46, 47)
//! Splits audio across an LR4 Linkwitz-Riley crossover into independent low and high frequency bands.
//! Pans frequencies in opposite directions, with dynamic spectral tilt tracking that whips
//! spatial positioning based on high/low frequency dominance, plus Haas ITD binaural depth and mono-sub protection.

use crate::common::{clamp, lerp, denormal_guard, BiquadCoeffs, BiquadState, DelayBuffer};
use std::f32::consts::PI;

const BUTTERWORTH_Q: f32 = 0.70710678;
const MAX_HAAS_DELAY_SAMPLES: usize = 256; // > 2.5ms at 96kHz, strictly bounded

pub struct RustFrequencySplitPanner {
    sample_rate: f32,

    // LR4 Crossover (Cascaded 2-pole Butterworth lowpass & highpass)
    lp1_l: BiquadState,
    lp2_l: BiquadState,
    lp1_r: BiquadState,
    lp2_r: BiquadState,
    hp1_l: BiquadState,
    hp2_l: BiquadState,
    hp1_r: BiquadState,
    hp2_r: BiquadState,
    lp_coeffs: BiquadCoeffs,
    hp_coeffs: BiquadCoeffs,

    // Sub-bass mono maker filter (2-pole Butterworth LP)
    sub_l: BiquadState,
    sub_r: BiquadState,
    sub_coeffs: BiquadCoeffs,

    // Envelope followers for dynamic spectral tilt tracking
    env_low: f32,
    env_high: f32,
    smoothed_tilt: f32,

    // Haas ITD microdelay buffers for high band binaural widening
    haas_delay_l: DelayBuffer,
    haas_delay_r: DelayBuffer,

    // Smoothed parameters
    smoothed_low_pan: f32,
    smoothed_high_pan: f32,

    // Parameters
    crossover_hz: f32,     // 100.0 to 5000.0 Hz
    low_pan: f32,          // -1.0 to +1.0 (default -0.7 = left)
    high_pan: f32,         // -1.0 to +1.0 (default +0.7 = right)
    dynamic_amount: f32,   // -1.0 to +1.0 (spectral reactivity)
    inertia_ms: f32,       // 5.0 to 200.0 ms
    haas_spread: f32,      // 0.0 to 1.0 (0 to 1.0 ms micro-delay)
    mono_bass_hz: f32,     // 20.0 to 250.0 Hz
    mix: f32,              // 0.0 to 1.0
}

impl RustFrequencySplitPanner {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        let mut panner = Self {
            sample_rate: sr,

            lp1_l: BiquadState::default(),
            lp2_l: BiquadState::default(),
            lp1_r: BiquadState::default(),
            lp2_r: BiquadState::default(),
            hp1_l: BiquadState::default(),
            hp2_l: BiquadState::default(),
            hp1_r: BiquadState::default(),
            hp2_r: BiquadState::default(),
            lp_coeffs: BiquadCoeffs::default(),
            hp_coeffs: BiquadCoeffs::default(),

            sub_l: BiquadState::default(),
            sub_r: BiquadState::default(),
            sub_coeffs: BiquadCoeffs::default(),

            env_low: 0.0,
            env_high: 0.0,
            smoothed_tilt: 0.0,

            haas_delay_l: DelayBuffer::new(MAX_HAAS_DELAY_SAMPLES),
            haas_delay_r: DelayBuffer::new(MAX_HAAS_DELAY_SAMPLES),

            smoothed_low_pan: -0.7,
            smoothed_high_pan: 0.7,

            crossover_hz: 650.0,
            low_pan: -0.7,
            high_pan: 0.7,
            dynamic_amount: 0.6,
            inertia_ms: 25.0,
            haas_spread: 0.4,
            mono_bass_hz: 90.0,
            mix: 1.0,
        };
        panner.update_filters();
        panner
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.update_filters();
        self.reset();
    }

    pub fn reset(&mut self) {
        self.lp1_l.reset();
        self.lp2_l.reset();
        self.lp1_r.reset();
        self.lp2_r.reset();
        self.hp1_l.reset();
        self.hp2_l.reset();
        self.hp1_r.reset();
        self.hp2_r.reset();
        self.sub_l.reset();
        self.sub_r.reset();
        self.env_low = 0.0;
        self.env_high = 0.0;
        self.smoothed_tilt = 0.0;
        self.haas_delay_l.reset();
        self.haas_delay_r.reset();
        self.smoothed_low_pan = self.low_pan;
        self.smoothed_high_pan = self.high_pan;
    }

    fn update_filters(&mut self) {
        let fc = clamp(self.crossover_hz, 80.0, self.sample_rate * 0.45);
        self.lp_coeffs = BiquadCoeffs::lowpass(self.sample_rate, fc, BUTTERWORTH_Q);
        self.hp_coeffs = BiquadCoeffs::highpass(self.sample_rate, fc, BUTTERWORTH_Q);

        let f_sub = clamp(self.mono_bass_hz, 20.0, 300.0);
        self.sub_coeffs = BiquadCoeffs::lowpass(self.sample_rate, f_sub, BUTTERWORTH_Q);
    }

    pub fn set_param(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => {
                self.crossover_hz = clamp(value, 100.0, 5000.0);
                self.update_filters();
            }
            2 => self.low_pan = clamp(value, -1.0, 1.0),
            3 => self.high_pan = clamp(value, -1.0, 1.0),
            4 => self.dynamic_amount = clamp(value, -1.0, 1.0),
            5 => self.inertia_ms = clamp(value, 5.0, 200.0),
            6 => self.haas_spread = clamp(value, 0.0, 1.0),
            7 => {
                self.mono_bass_hz = clamp(value, 20.0, 250.0);
                self.update_filters();
            }
            8 => self.mix = clamp(value, 0.0, 1.0),
            _ => {}
        }
    }

    #[inline(always)]
    fn compute_equal_power_gains(pan: f32) -> (f32, f32) {
        let p = clamp(pan, -1.0, 1.0);
        let angle = (p + 1.0) * (PI * 0.25); // 0 to PI/2
        (angle.cos(), angle.sin())
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

        // Ballistics time constants
        let inertia_sec = (self.inertia_ms * 0.001).max(0.002);
        let env_alpha = (-1.0 / (inertia_sec * self.sample_rate)).exp();
        let pan_smooth = (-1.0 / (0.005 * self.sample_rate)).exp(); // 5ms smoothing for pan pots

        // Max Haas delay in samples (~0.9 ms max)
        let max_haas_samples = (0.0009 * self.sample_rate).min((MAX_HAAS_DELAY_SAMPLES - 8) as f32);

        for i in 0..num_samples as usize {
            let xl = unsafe { *in_l.add(i) };
            let xr = unsafe { *in_r.add(i) };

            // 1. Linkwitz-Riley 4th order (24 dB/oct) band split
            let low1_l = self.lp1_l.process(xl, &self.lp_coeffs);
            let low_l = self.lp2_l.process(low1_l, &self.lp_coeffs);

            let low1_r = self.lp1_r.process(xr, &self.lp_coeffs);
            let low_r = self.lp2_r.process(low1_r, &self.lp_coeffs);

            let high1_l = self.hp1_l.process(xl, &self.hp_coeffs);
            let high_l = self.hp2_l.process(high1_l, &self.hp_coeffs);

            let high1_r = self.hp1_r.process(xr, &self.hp_coeffs);
            let high_r = self.hp2_r.process(high1_r, &self.hp_coeffs);

            // 2. Dynamic energy envelopes and spectral tilt tracking
            let low_energy = low_l.abs() + low_r.abs();
            let high_energy = high_l.abs() + high_r.abs();

            self.env_low = lerp(low_energy, self.env_low, env_alpha);
            self.env_high = lerp(high_energy, self.env_high, env_alpha);

            let total_energy = self.env_low + self.env_high;
            let current_tilt = if total_energy > 1.0e-5 {
                (self.env_high - self.env_low) / total_energy
            } else {
                0.0
            };
            self.smoothed_tilt = lerp(current_tilt, self.smoothed_tilt, env_alpha);

            // 3. Dynamic panning modulation
            // Tilt > 0 (high frequency dominant): pushes HighPan further, moves LowPan opposite
            // Tilt < 0 (low frequency dominant): locks LowPan hard into position
            let target_low_pan = clamp(self.low_pan - self.dynamic_amount * self.smoothed_tilt, -1.0, 1.0);
            let target_high_pan = clamp(self.high_pan + self.dynamic_amount * self.smoothed_tilt, -1.0, 1.0);

            self.smoothed_low_pan = lerp(target_low_pan, self.smoothed_low_pan, pan_smooth);
            self.smoothed_high_pan = lerp(target_high_pan, self.smoothed_high_pan, pan_smooth);

            let (g_low_l, g_low_r) = Self::compute_equal_power_gains(self.smoothed_low_pan);
            let (g_high_l, g_high_r) = Self::compute_equal_power_gains(self.smoothed_high_pan);

            // 4. Sub-bass mono anchoring
            let sub_sig_l = self.sub_l.process(low_l, &self.sub_coeffs);
            let sub_sig_r = self.sub_r.process(low_r, &self.sub_coeffs);
            let sub_mono = 0.5 * (sub_sig_l + sub_sig_r);

            // Low band output with mono-sub protection
            let low_mid_l = low_l - sub_sig_l;
            let low_mid_r = low_r - sub_sig_r;
            let low_panned_l = low_mid_l * g_low_l + sub_mono;
            let low_panned_r = low_mid_r * g_low_r + sub_mono;

            // 5. High band panning with Haas ITD micro-delay
            let mut high_panned_l = high_l * g_high_l;
            let mut high_panned_r = high_r * g_high_r;

            if self.haas_spread > 0.01 {
                self.haas_delay_l.write(high_panned_l);
                self.haas_delay_r.write(high_panned_r);

                let haas_delay_amount = self.haas_spread * max_haas_samples * self.smoothed_high_pan.abs();
                if self.smoothed_high_pan > 0.0 {
                    // Panned Right: High frequencies reach Right ear first, delay Left channel
                    high_panned_l = self.haas_delay_l.read_cubic(haas_delay_amount.max(1.0));
                } else if self.smoothed_high_pan < 0.0 {
                    // Panned Left: High frequencies reach Left ear first, delay Right channel
                    high_panned_r = self.haas_delay_r.read_cubic(haas_delay_amount.max(1.0));
                }
            }

            // 6. Sum bands and wet/dry mix
            let wet_l = low_panned_l + high_panned_l;
            let wet_r = low_panned_r + high_panned_r;

            let out_sample_l = denormal_guard(lerp(xl, wet_l, self.mix));
            let out_sample_r = denormal_guard(lerp(xr, wet_r, self.mix));

            unsafe {
                *out_l.add(i) = out_sample_l;
                *out_r.add(i) = out_sample_r;
            }
        }
    }
}
