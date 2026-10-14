//! Buchla 292 Vactrol Low-Pass Gate & Resonant Strike (Rules 5, 8, 9, 14, 34, 46, 47)
//! Physical modeling of an optocoupler vactrol (LED + CdS photoresistor) coupled with
//! resonant diode-clipped filter and voltage-controlled amplifier.
//! Delivers iconic organic acoustic rings, wooden "Buchla bongo" plucks, and sluggish tails.

use crate::common::{clamp, lerp};
use crate::optimization::fast_pade_tanh;

#[derive(Clone, Copy)]
struct VactrolState {
    value: f32,
    slow_memory: f32,
}

impl VactrolState {
    const fn new() -> Self {
        Self {
            value: 0.0,
            slow_memory: 0.0,
        }
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.value = 0.0;
        self.slow_memory = 0.0;
    }

    /// Update non-linear optocoupler physical dynamics
    #[inline(always)]
    fn update(&mut self, target: f32, decay_ms: f32, sample_rate: f32) -> f32 {
        let target_clamped = clamp(target, 0.0, 1.0);

        if target_clamped > self.value {
            // Optical excitation: very fast attack (~2 ms)
            let attack_coeff = (-1.0 / (0.002 * sample_rate)).exp();
            self.value = target_clamped + attack_coeff * (self.value - target_clamped);
            self.slow_memory = self.value;
        } else {
            // CdS photoresistor sluggish decay: dual-phase memory droop
            // Fast decay initially, then decelerates exponentially into a long acoustic tail
            let base_tau = (decay_ms * 0.001 * sample_rate).max(10.0);
            let sluggishness = 1.0 + (1.0 - self.value) * 3.5;
            let decay_coeff = (-1.0 / (base_tau * sluggishness)).exp();
            
            self.value = decay_coeff * self.value;
            self.slow_memory = 0.9995 * self.slow_memory;
        }

        clamp(self.value, 0.0, 1.0)
    }
}

#[derive(Clone, Copy)]
struct SallenKeyFilter {
    s1: f32,
    s2: f32,
}

impl SallenKeyFilter {
    const fn new() -> Self {
        Self { s1: 0.0, s2: 0.0 }
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.s1 = 0.0;
        self.s2 = 0.0;
    }

    /// Process sample through 2-pole lowpass filter with non-linear diode feedback
    #[inline(always)]
    fn process(&mut self, x: f32, g: f32, res: f32) -> f32 {
        let s1_clean = if self.s1.is_nan() || self.s1.is_infinite() { 0.0 } else { clamp(self.s1, -16.0, 16.0) };
        let s2_clean = if self.s2.is_nan() || self.s2.is_infinite() { 0.0 } else { clamp(self.s2, -16.0, 16.0) };

        // Diode-saturating resonance feedback loop
        let fb_saturated = fast_pade_tanh(s1_clean * res);
        let h = 1.0 / (1.0 + g * (g + 1.2));
        
        let in_eff = x - fb_saturated;
        let y = (s2_clean + g * (s1_clean + g * in_eff)) * h;

        let v1 = g * (in_eff - y);
        let v2 = g * y;

        self.s1 = clamp(s1_clean + 2.0 * v1, -24.0, 24.0);
        self.s2 = clamp(s2_clean + 2.0 * v2, -24.0, 24.0);

        y
    }
}

pub struct RustVactrolLPG {
    sample_rate: f32,
    vactrol_l: VactrolState,
    vactrol_r: VactrolState,
    filter_l: SallenKeyFilter,
    filter_r: SallenKeyFilter,

    // Strike impulse generator
    strike_env: f32,
    strike_triggered: bool,

    // Parameters
    cutoff_base: f32,     // 0.0 to 1.0 (manual gate opening)
    resonance: f32,       // 0.0 to 8.0 (resonance ringing / bongo pitch)
    decay_ms: f32,        // 20.0 to 2500.0 ms
    strike_amount: f32,   // 0.0 to 1.0
    mode_balance: f32,    // 0.0 = VCF only, 0.5 = Both (LPG), 1.0 = VCA only
    drive: f32,           // 1.0 to 5.0
    mix: f32,             // 0.0 to 1.0

    // Smoothed
    smoothed_cutoff: f32,
    smoothed_mix: f32,
}

impl RustVactrolLPG {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            vactrol_l: VactrolState::new(),
            vactrol_r: VactrolState::new(),
            filter_l: SallenKeyFilter::new(),
            filter_r: SallenKeyFilter::new(),
            strike_env: 0.0,
            strike_triggered: false,
            cutoff_base: 0.2,
            resonance: 2.5,
            decay_ms: 280.0,
            strike_amount: 0.8,
            mode_balance: 0.5, // Default classic Buchla LPG Both mode
            drive: 1.2,
            mix: 1.0,
            smoothed_cutoff: 0.2,
            smoothed_mix: 1.0,
        }
    }

    pub fn reset(&mut self) {
        self.vactrol_l.reset();
        self.vactrol_r.reset();
        self.filter_l.reset();
        self.filter_r.reset();
        self.strike_env = 0.0;
        self.smoothed_cutoff = self.cutoff_base;
        self.smoothed_mix = self.mix;
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn set_cutoff(&mut self, val: f32) {
        self.cutoff_base = clamp(val, 0.0, 1.0);
    }

    pub fn set_resonance(&mut self, val: f32) {
        self.resonance = clamp(val, 0.0, 8.0);
    }

    pub fn set_decay_ms(&mut self, val: f32) {
        self.decay_ms = clamp(val, 20.0, 2500.0);
    }

    pub fn set_strike(&mut self, val: f32) {
        self.strike_amount = clamp(val, 0.0, 1.0);
        if val > 0.01 {
            self.strike_triggered = true;
        }
    }

    pub fn trigger_strike(&mut self) {
        self.strike_env = 1.0;
    }

    pub fn set_mode_balance(&mut self, val: f32) {
        self.mode_balance = clamp(val, 0.0, 1.0);
    }

    pub fn set_drive(&mut self, val: f32) {
        self.drive = clamp(val, 1.0, 5.0);
    }

    pub fn set_mix(&mut self, val: f32) {
        self.mix = clamp(val, 0.0, 1.0);
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        strike_in: Option<&[f32]>,
        out_l: &mut [f32],
        out_r: &mut [f32],
        num_samples: usize,
    ) {
        let n = num_samples.min(in_l.len()).min(in_r.len()).min(out_l.len()).min(out_r.len());
        if n == 0 {
            return;
        }

        let strike_decay = (-1.0 / (0.005 * self.sample_rate)).exp(); // ~5 ms strike impulse

        for i in 0..n {
            self.smoothed_cutoff = lerp(self.smoothed_cutoff, self.cutoff_base, 0.005);
            self.smoothed_mix = lerp(self.smoothed_mix, self.mix, 0.005);

            // Audio-rate strike impulse input or manual trigger
            let mut strike_val = self.strike_env;
            if let Some(s_in) = strike_in {
                if i < s_in.len() && s_in[i].abs() > 0.05 {
                    strike_val = strike_val.max(s_in[i].abs() * self.strike_amount);
                }
            }
            if self.strike_triggered {
                strike_val = 1.0 * self.strike_amount;
                self.strike_triggered = false;
            }
            self.strike_env *= strike_decay;

            // Combined optical target: manual base opening + dynamic strike ping
            let control_target = clamp(self.smoothed_cutoff + strike_val, 0.0, 1.0);

            let vactrol_level_l = self.vactrol_l.update(control_target, self.decay_ms, self.sample_rate);
            let vactrol_level_r = self.vactrol_r.update(control_target, self.decay_ms, self.sample_rate);

            // Exponential frequency mapping from vactrol state: 20 Hz to 18000 Hz
            let fc_l = 25.0 * (720.0f32).powf(vactrol_level_l);
            let fc_r = 25.0 * (720.0f32).powf(vactrol_level_r);

            let g_l = (std::f32::consts::PI * fc_l / self.sample_rate).tan().max(0.001);
            let g_r = (std::f32::consts::PI * fc_r / self.sample_rate).tan().max(0.001);

            // VCA amplitude gain: natural perception curve (power of 1.6)
            let vca_gain_l = vactrol_level_l.powf(1.6);
            let vca_gain_r = vactrol_level_r.powf(1.6);

            // Input saturation
            let driven_l = fast_pade_tanh(in_l[i] * self.drive);
            let driven_r = fast_pade_tanh(in_r[i] * self.drive);

            // VCF lowpass filtering
            let filtered_l = self.filter_l.process(driven_l, g_l, self.resonance);
            let filtered_r = self.filter_r.process(driven_r, g_r, self.resonance);

            // Blend based on mode_balance:
            // mode_balance = 0.0 -> pure VCF (filtered_l)
            // mode_balance = 0.5 -> classic Buchla LPG (filtered_l * vca_gain_l)
            // mode_balance = 1.0 -> pure VCA (driven_l * vca_gain_l)
            let (wet_l, wet_r) = if self.mode_balance <= 0.5 {
                let t = self.mode_balance * 2.0; // 0.0 to 1.0
                let comb_l = filtered_l * lerp(1.0, vca_gain_l, t);
                let comb_r = filtered_r * lerp(1.0, vca_gain_r, t);
                (comb_l, comb_r)
            } else {
                let t = (self.mode_balance - 0.5) * 2.0; // 0.0 to 1.0
                let comb_l = lerp(filtered_l * vca_gain_l, driven_l * vca_gain_l, t);
                let comb_r = lerp(filtered_r * vca_gain_r, driven_r * vca_gain_r, t);
                (comb_l, comb_r)
            };

            out_l[i] = lerp(in_l[i], wet_l, self.smoothed_mix);
            out_r[i] = lerp(in_r[i], wet_r, self.smoothed_mix);
        }
    }
}
