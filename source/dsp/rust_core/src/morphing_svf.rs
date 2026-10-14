//! Nonlinear Morphing State-Variable Filter (Topology Warp) (Rules 5, 8, 9, 14, 34, 46, 47)
//! Zero-Delay Feedback (ZDF/TPT) SVF with continuous 360-degree topology morphing
//! (LP -> BP -> HP -> Notch -> Peak), soft-saturating non-linear feedback loop (self-oscillating without blowup),
//! input drive saturation, and audio-rate FM cross-modulation input.

use crate::common::{clamp, lerp};
use crate::optimization::fast_pade_tanh;

#[derive(Clone, Copy)]
struct SvfChannelState {
    s1: f32,
    s2: f32,
}

impl SvfChannelState {
    const fn new() -> Self {
        Self { s1: 0.0, s2: 0.0 }
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.s1 = 0.0;
        self.s2 = 0.0;
    }

    /// Process one sample with ZDF/TPT trapezoidal integration and saturating resonance loop
    #[inline(always)]
    fn process(&mut self, x: f32, g: f32, r: f32, morph: f32, drive: f32) -> f32 {
        // Non-linear input saturation
        let x_driven = fast_pade_tanh(x * drive);

        let s1_clean = if self.s1.is_nan() || self.s1.is_infinite() { 0.0 } else { clamp(self.s1, -16.0, 16.0) };
        let s2_clean = if self.s2.is_nan() || self.s2.is_infinite() { 0.0 } else { clamp(self.s2, -16.0, 16.0) };

        // ZDF TPT trapezoidal integrator equations with saturating feedback:
        let s1_sat = fast_pade_tanh(s1_clean);
        let s2_sat = fast_pade_tanh(s2_clean);
        let h = 1.0 / (1.0 + 2.0 * r * g + g * g);
        let hp = (x_driven - (2.0 * r + g) * s1_sat - s2_sat) * h;
        
        let bp = g * hp + s1_clean;
        let s1_next = g * hp + bp;
        self.s1 = clamp(s1_next, -24.0, 24.0);
        
        let lp = g * bp + s2_clean;
        let s2_next = g * bp + lp;
        self.s2 = clamp(s2_next, -24.0, 24.0);

        // Notch = HP + LP
        let notch = hp + lp;
        // Peaking Bell = LP - HP (or LP + HP - BP)
        let peak = lp - hp;

        // Continuous 360-degree morphing around topology ring:
        // morph in [0.0, 1.0]:
        // 0.00 .. 0.20: LP -> BP
        // 0.20 .. 0.40: BP -> HP
        // 0.40 .. 0.60: HP -> Notch
        // 0.60 .. 0.80: Notch -> Peak
        // 0.80 .. 1.00: Peak -> LP
        let m = morph * 5.0;
        let sector = m.floor() as usize;
        let frac = m - m.floor();

        match sector {
            0 => lerp(lp, bp, frac),
            1 => lerp(bp, hp, frac),
            2 => lerp(hp, notch, frac),
            3 => lerp(notch, peak, frac),
            _ => lerp(peak, lp, frac),
        }
    }
}

pub struct RustMorphingSvf {
    sample_rate: f32,
    state_l: SvfChannelState,
    state_r: SvfChannelState,

    // Parameters
    cutoff_hz: f32,   // 20.0 to 20000.0 Hz
    resonance: f32,   // 0.1 to 30.0 (high values self-oscillate with warm saturation)
    morph: f32,       // 0.0 to 1.0 (continuous topology sweep)
    drive: f32,       // 1.0 to 10.0 (input warmth & saturation)
    fm_depth: f32,    // 0.0 to 1.0 (audio-rate modulation intensity)

    // Smoothed values
    smoothed_cutoff: f32,
    smoothed_res: f32,
    smoothed_morph: f32,
    smoothed_drive: f32,
}

impl RustMorphingSvf {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            state_l: SvfChannelState::new(),
            state_r: SvfChannelState::new(),
            cutoff_hz: 1200.0,
            resonance: 1.5,
            morph: 0.0,
            drive: 1.2,
            fm_depth: 0.0,
            smoothed_cutoff: 1200.0,
            smoothed_res: 1.5,
            smoothed_morph: 0.0,
            smoothed_drive: 1.2,
        }
    }

    pub fn reset(&mut self) {
        self.state_l.reset();
        self.state_r.reset();
        self.smoothed_cutoff = self.cutoff_hz;
        self.smoothed_res = self.resonance;
        self.smoothed_morph = self.morph;
        self.smoothed_drive = self.drive;
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn set_cutoff(&mut self, cutoff: f32) {
        self.cutoff_hz = clamp(cutoff, 20.0, 20000.0);
    }

    pub fn set_resonance(&mut self, res: f32) {
        self.resonance = clamp(res, 0.1, 30.0);
    }

    pub fn set_morph(&mut self, morph: f32) {
        self.morph = clamp(morph, 0.0, 1.0);
    }

    pub fn set_drive(&mut self, drive: f32) {
        self.drive = clamp(drive, 1.0, 10.0);
    }

    pub fn set_fm_depth(&mut self, fm: f32) {
        self.fm_depth = clamp(fm, 0.0, 1.0);
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        fm_in: Option<&[f32]>,
        out_l: &mut [f32],
        out_r: &mut [f32],
        num_samples: usize,
    ) {
        let n = num_samples.min(in_l.len()).min(in_r.len()).min(out_l.len()).min(out_r.len());
        if n == 0 {
            return;
        }

        for i in 0..n {
            // Anti-click smoothing (Rule 35)
            self.smoothed_cutoff = lerp(self.smoothed_cutoff, self.cutoff_hz, 0.005);
            self.smoothed_res = lerp(self.smoothed_res, self.resonance, 0.005);
            self.smoothed_morph = lerp(self.smoothed_morph, self.morph, 0.005);
            self.smoothed_drive = lerp(self.smoothed_drive, self.drive, 0.005);

            // Compute base frequency with optional audio-rate FM
            let mut mod_freq = self.smoothed_cutoff;
            if let Some(fm) = fm_in {
                if i < fm.len() {
                    // Exponential FM mod: 2^(fm * 3.0 octaves)
                    let octaves = fm[i] * self.fm_depth * 3.5;
                    mod_freq *= 2.0f32.powf(octaves);
                }
            }
            mod_freq = clamp(mod_freq, 20.0, (self.sample_rate * 0.48).min(20000.0));

            // TPT integrator pole frequency g = tan(pi * fc / fs)
            let g = (std::f32::consts::PI * mod_freq / self.sample_rate).tan().max(0.0001);
            // Damping factor R = 1 / (2 * Q)
            let r = 1.0 / (2.0 * self.smoothed_res.max(0.1));

            out_l[i] = self.state_l.process(in_l[i], g, r, self.smoothed_morph, self.smoothed_drive);
            out_r[i] = self.state_r.process(in_r[i], g, r, self.smoothed_morph, self.smoothed_drive);
        }
    }
}
