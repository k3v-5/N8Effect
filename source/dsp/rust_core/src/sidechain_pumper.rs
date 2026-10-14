//! Kickstart-Style Sidechain Pumper (Rules 5, 8, 9, 14, 34, 35, 37, 46, 47)
//! Tempo-synced volume ducking & sidechain envelope processor with customizable curves,
//! multiband low-only ducking crossover, phase offset, and external audio sidechain trigger.

use crate::common::{clamp, lerp, denormal_guard, BiquadCoeffs, BiquadState};
use std::f32::consts::PI;

const BUTTERWORTH_Q: f32 = 0.70710678;

pub struct RustSidechainPumper {
    sample_rate: f32,
    internal_phase: f32,
    smoothed_gain: f32,

    // Linkwitz-Riley 4th order (cascaded 2-pole Butterworth) for low-only ducking
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

    // Audio sidechain envelope follower for trigger detection
    sc_env: f32,
    sc_triggered: bool,

    // Parameters
    sync_rate: u32,       // 0: 1/1, 1: 1/2, 2: 1/4, 3: 1/8, 4: 1/16, 5: 1/8T, 6: 1/16T
    depth: f32,           // 0.0 to 1.0 (ducking depth, 1.0 = full silence at beat onset)
    curve_shape: u32,     // 0: Classic EDM, 1: Tight Snappy, 2: Deep Techno, 3: S-Curve, 4: Hard Gate, 5: Linear Ramp
    curve_skew: f32,      // 0.2 to 3.0 (power exponent bending curve)
    phase_offset: f32,    // -0.5 to 0.5 (time shift)
    band_split: f32,      // 0.0: Full Band, 1.0: Low Only ducking
    crossover_hz: f32,    // 40.0 to 800.0 Hz
    trigger_mode: u32,    // 0: Host Sync, 1: Audio Sidechain Trigger, 2: Retrigger
    mix: f32,             // 0.0 to 1.0

    // Host sync context
    host_bpm: f32,
    host_ppq: f64,
    host_playing: bool,
}

impl RustSidechainPumper {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        let mut pumper = Self {
            sample_rate: sr,
            internal_phase: 0.0,
            smoothed_gain: 1.0,

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

            sc_env: 0.0,
            sc_triggered: false,

            sync_rate: 2,         // 1/4 note default (EDM 4-on-the-floor)
            depth: 1.0,           // 100% ducking
            curve_shape: 0,       // Classic EDM Kickstart pump
            curve_skew: 1.0,
            phase_offset: 0.0,
            band_split: 0.0,      // Full band by default
            crossover_hz: 200.0,
            trigger_mode: 0,      // Host sync
            mix: 1.0,

            host_bpm: 120.0,
            host_ppq: 0.0,
            host_playing: false,
        };
        pumper.update_crossover_coeffs();
        pumper
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.update_crossover_coeffs();
        self.reset();
    }

    pub fn reset(&mut self) {
        self.internal_phase = 0.0;
        self.smoothed_gain = 1.0;
        self.lp1_l.reset();
        self.lp2_l.reset();
        self.lp1_r.reset();
        self.lp2_r.reset();
        self.hp1_l.reset();
        self.hp2_l.reset();
        self.hp1_r.reset();
        self.hp2_r.reset();
        self.sc_env = 0.0;
        self.sc_triggered = false;
    }

    fn update_crossover_coeffs(&mut self) {
        let fc = clamp(self.crossover_hz, 30.0, self.sample_rate * 0.45);
        self.lp_coeffs = BiquadCoeffs::lowpass(self.sample_rate, fc, BUTTERWORTH_Q);
        self.hp_coeffs = BiquadCoeffs::highpass(self.sample_rate, fc, BUTTERWORTH_Q);
    }

    pub fn set_host_sync(&mut self, bpm: f32, ppq: f64, playing: bool) {
        self.host_bpm = if bpm > 20.0 { bpm } else { 120.0 };
        self.host_ppq = ppq;
        self.host_playing = playing;
    }

    pub fn set_param(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.sync_rate = clamp(value, 0.0, 6.0) as u32,
            2 => self.depth = clamp(value, 0.0, 1.0),
            3 => self.curve_shape = clamp(value, 0.0, 5.0) as u32,
            4 => self.curve_skew = clamp(value, 0.2, 3.5),
            5 => self.phase_offset = clamp(value, -0.5, 0.5),
            6 => self.band_split = clamp(value, 0.0, 1.0),
            7 => {
                self.crossover_hz = clamp(value, 40.0, 1200.0);
                self.update_crossover_coeffs();
            }
            8 => self.trigger_mode = clamp(value, 0.0, 2.0) as u32,
            9 => self.mix = clamp(value, 0.0, 1.0),
            _ => {}
        }
    }

    pub fn retrigger(&mut self) {
        self.internal_phase = 0.0;
    }

    #[inline(always)]
    fn get_subdivision_beats(&self) -> f32 {
        match self.sync_rate {
            0 => 4.0,           // 1/1 (whole note = 4 beats)
            1 => 2.0,           // 1/2 (half note = 2 beats)
            2 => 1.0,           // 1/4 (quarter note = 1 beat)
            3 => 0.5,           // 1/8 (eighth note)
            4 => 0.25,          // 1/16 (sixteenth note)
            5 => 1.0 / 3.0,     // 1/8T (triplet)
            6 => 0.125,         // 1/16T
            _ => 1.0,
        }
    }

    #[inline(always)]
    fn compute_envelope_gain(&self, phase: f32) -> f32 {
        // Normal phase in [0.0, 1.0)
        let mut t = phase + self.phase_offset;
        while t < 0.0 { t += 1.0; }
        while t >= 1.0 { t -= 1.0; }

        // Shape curve bending
        let u = if (self.curve_skew - 1.0).abs() > 0.01 {
            t.powf(self.curve_skew)
        } else {
            t
        };

        let raw_duck = match self.curve_shape {
            // 0: Classic EDM Kickstart (sharp initial drop, concave exponential-like recovery)
            0 => {
                if u < 0.7 {
                    let p = u / 0.7;
                    // Ease-in quadratic to rapid full recovery
                    0.5 * (1.0 - (1.0 - p).powi(3)) + 0.5 * p * p
                } else {
                    1.0
                }
            }
            // 1: Tight Snappy Kick (recovers very fast within 35% of the beat)
            1 => {
                if u < 0.35 {
                    let p = u / 0.35;
                    1.0 - (1.0 - p).powi(2)
                } else {
                    1.0
                }
            }
            // 2: Deep Techno / Long Duck (holds low for longer, smooth rise)
            2 => {
                if u < 0.2 {
                    0.0
                } else if u < 0.85 {
                    let p = (u - 0.2) / 0.65;
                    0.5 * (1.0 - (p * PI).cos())
                } else {
                    1.0
                }
            }
            // 3: Smooth S-Curve (Hermite smoothstep)
            3 => {
                u * u * (3.0 - 2.0 * u)
            }
            // 4: Hard Trance Gate (square-ish transition)
            4 => {
                if u < 0.45 {
                    0.0
                } else if u < 0.55 {
                    (u - 0.45) / 0.10
                } else {
                    1.0
                }
            }
            // 5: Linear Ramp
            5 => {
                u
            }
            _ => u,
        };

        // Depth scaling: 0 depth => 1.0 (no duck), 1.0 depth => raw_duck (full duck)
        1.0 - self.depth * (1.0 - clamp(raw_duck, 0.0, 1.0))
    }

    pub fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        sc_l: *const f32,
        sc_r: *const f32,
        num_samples: u32,
    ) {
        if in_l.is_null() || in_r.is_null() || out_l.is_null() || out_r.is_null() || num_samples == 0 {
            return;
        }

        let sub_beats = self.get_subdivision_beats();
        let phase_inc_free = (self.host_bpm / (60.0 * self.sample_rate * sub_beats)).max(1e-6);

        // Anti-click smoothing factor (~0.6ms time constant)
        let smooth_coeff = (-1.0 / (0.0006 * self.sample_rate)).exp();
        let is_low_only = self.band_split > 0.5;

        for i in 0..num_samples as usize {
            let xl = unsafe { *in_l.add(i) };
            let xr = unsafe { *in_r.add(i) };

            // Audio sidechain trigger detection
            if self.trigger_mode == 1 && !sc_l.is_null() {
                let sl = unsafe { *sc_l.add(i) };
                let sr = if !sc_r.is_null() { unsafe { *sc_r.add(i) } } else { sl };
                let sc_in = 0.5 * (sl.abs() + sr.abs());

                // Fast attack, slower release envelope follower
                let sc_att = 0.002;
                let sc_rel = 0.0005;
                if sc_in > self.sc_env {
                    self.sc_env += sc_att * (sc_in - self.sc_env);
                } else {
                    self.sc_env += sc_rel * (sc_in - self.sc_env);
                }

                // Threshold hysteresis for trigger
                if self.sc_env > 0.08 && !self.sc_triggered {
                    self.internal_phase = 0.0;
                    self.sc_triggered = true;
                } else if self.sc_env < 0.03 {
                    self.sc_triggered = false;
                }
            }

            // Current cycle phase determination
            let current_phase = if self.trigger_mode == 0 && self.host_playing {
                let current_ppq = self.host_ppq + (i as f64 * (self.host_bpm as f64) / (60.0 * self.sample_rate as f64));
                let beat_in_sub = (current_ppq / sub_beats as f64) % 1.0;
                if beat_in_sub < 0.0 { (beat_in_sub + 1.0) as f32 } else { beat_in_sub as f32 }
            } else {
                let p = self.internal_phase;
                self.internal_phase = (self.internal_phase + phase_inc_free) % 1.0;
                p
            };

            // Target ducking gain
            let target_gain = self.compute_envelope_gain(current_phase);
            self.smoothed_gain = lerp(target_gain, self.smoothed_gain, smooth_coeff);
            let g = denormal_guard(self.smoothed_gain);

            let (wl, wr) = if is_low_only {
                // Linkwitz-Riley 4th order (cascaded 2-pole Butterworth lowpass & highpass)
                let low1_l = self.lp1_l.process(xl, &self.lp_coeffs);
                let low_l = self.lp2_l.process(low1_l, &self.lp_coeffs);

                let low1_r = self.lp1_r.process(xr, &self.lp_coeffs);
                let low_r = self.lp2_r.process(low1_r, &self.lp_coeffs);

                let high1_l = self.hp1_l.process(xl, &self.hp_coeffs);
                let high_l = self.hp2_l.process(high1_l, &self.hp_coeffs);

                let high1_r = self.hp1_r.process(xr, &self.hp_coeffs);
                let high_r = self.hp2_r.process(high1_r, &self.hp_coeffs);

                // Duck only low band; leave high band intact!
                (low_l * g + high_l, low_r * g + high_r)
            } else {
                // Full-band ducking
                (xl * g, xr * g)
            };

            let yl = denormal_guard(lerp(xl, wl, self.mix));
            let yr = denormal_guard(lerp(xr, wr, self.mix));

            unsafe {
                *out_l.add(i) = yl;
                *out_r.add(i) = yr;
            }
        }

        // Advance free phase accumulator if playing
        if self.trigger_mode == 0 && self.host_playing {
            self.internal_phase = (self.internal_phase + phase_inc_free * num_samples as f32) % 1.0;
        }
    }
}
