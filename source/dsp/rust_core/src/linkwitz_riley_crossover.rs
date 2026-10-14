//! Linkwitz-Riley LR4 (24 dB/octave) 3-Way Stereo Crossover
//! Uses cascaded 2nd-order Butterworth biquad sections (Q = 0.70710678)
//! with allpass phase compensation on the low band to guarantee a perfectly flat
//! 0 dB magnitude sum across the entire acoustic spectrum.
//! Follows Rules 32, 33, 34, 46, 47 (Zero audio thread allocations, SIMD cacheline friendly).

use std::f64::consts::PI;

pub const Q_BUTTERWORTH: f32 = 0.70710678;

#[derive(Clone, Copy)]
pub struct BiquadSection {
    b0: f32,
    b1: f32,
    b2: f32,
    a1: f32,
    a2: f32,
    z1: f32,
    z2: f32,
}

impl BiquadSection {
    pub const fn new() -> Self {
        Self {
            b0: 1.0,
            b1: 0.0,
            b2: 0.0,
            a1: 0.0,
            a2: 0.0,
            z1: 0.0,
            z2: 0.0,
        }
    }

    pub fn reset(&mut self) {
        self.z1 = 0.0;
        self.z2 = 0.0;
    }

    pub fn set_lowpass(&mut self, sample_rate: f64, freq_hz: f32, q: f32) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let nyquist = sr * 0.499;
        let fc = (freq_hz as f64).clamp(10.0, nyquist);
        let q_val = (q as f64).max(0.01);
        let omega = 2.0 * PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q_val);

        let a0 = 1.0 + alpha;
        self.b0 = ((1.0 - cs) * 0.5 / a0) as f32;
        self.b1 = ((1.0 - cs) / a0) as f32;
        self.b2 = ((1.0 - cs) * 0.5 / a0) as f32;
        self.a1 = (-2.0 * cs / a0) as f32;
        self.a2 = ((1.0 - alpha) / a0) as f32;
    }

    pub fn set_highpass(&mut self, sample_rate: f64, freq_hz: f32, q: f32) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let nyquist = sr * 0.499;
        let fc = (freq_hz as f64).clamp(10.0, nyquist);
        let q_val = (q as f64).max(0.01);
        let omega = 2.0 * PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q_val);

        let a0 = 1.0 + alpha;
        self.b0 = ((1.0 + cs) * 0.5 / a0) as f32;
        self.b1 = (-(1.0 + cs) / a0) as f32;
        self.b2 = ((1.0 + cs) * 0.5 / a0) as f32;
        self.a1 = (-2.0 * cs / a0) as f32;
        self.a2 = ((1.0 - alpha) / a0) as f32;
    }

    pub fn set_allpass(&mut self, sample_rate: f64, freq_hz: f32, q: f32) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let nyquist = sr * 0.499;
        let fc = (freq_hz as f64).clamp(10.0, nyquist);
        let q_val = (q as f64).max(0.01);
        let omega = 2.0 * PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q_val);

        let a0 = 1.0 + alpha;
        self.b0 = ((1.0 - alpha) / a0) as f32;
        self.b1 = (-2.0 * cs / a0) as f32;
        self.b2 = ((1.0 + alpha) / a0) as f32;
        self.a1 = (-2.0 * cs / a0) as f32;
        self.a2 = ((1.0 - alpha) / a0) as f32;
    }

    #[inline(always)]
    pub fn process(&mut self, in_sample: f32) -> f32 {
        let y = self.b0 * in_sample + self.z1;
        self.z1 = self.b1 * in_sample - self.a1 * y + self.z2;
        self.z2 = self.b2 * in_sample - self.a2 * y;

        if self.z1.abs() < 1e-15 { self.z1 = 0.0; }
        if self.z2.abs() < 1e-15 { self.z2 = 0.0; }

        y
    }
}

#[repr(C)]
pub struct RustLinkwitzRileyCrossover3Way {
    sample_rate: f64,
    f_low_mid: f32,
    f_mid_high: f32,

    // 9 biquad sections per channel (0 = L, 1 = R)
    lp_low1: [BiquadSection; 2],
    lp_low2: [BiquadSection; 2],
    hp_rest1: [BiquadSection; 2],
    hp_rest2: [BiquadSection; 2],
    ap_low: [BiquadSection; 2],
    lp_mid1: [BiquadSection; 2],
    lp_mid2: [BiquadSection; 2],
    hp_high1: [BiquadSection; 2],
    hp_high2: [BiquadSection; 2],
}

impl RustLinkwitzRileyCrossover3Way {
    pub fn new() -> Self {
        const INIT_SEC: BiquadSection = BiquadSection::new();
        let mut lr = Self {
            sample_rate: 44100.0,
            f_low_mid: 250.0,
            f_mid_high: 2500.0,
            lp_low1: [INIT_SEC; 2],
            lp_low2: [INIT_SEC; 2],
            hp_rest1: [INIT_SEC; 2],
            hp_rest2: [INIT_SEC; 2],
            ap_low: [INIT_SEC; 2],
            lp_mid1: [INIT_SEC; 2],
            lp_mid2: [INIT_SEC; 2],
            hp_high1: [INIT_SEC; 2],
            hp_high2: [INIT_SEC; 2],
        };
        lr.update_coefficients();
        lr
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
        self.update_coefficients();
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

    pub fn set_crossover_frequencies(&mut self, f_low_mid_hz: f32, f_mid_high_hz: f32) {
        let nyquist = (self.sample_rate * 0.49) as f32;
        self.f_low_mid = f_low_mid_hz.clamp(20.0, nyquist * 0.5);
        self.f_mid_high = f_mid_high_hz.clamp(self.f_low_mid * 1.2, nyquist);
        self.update_coefficients();
    }

    pub fn update_coefficients(&mut self) {
        for ch in 0..2 {
            // Lowpass LR4 at f_low_mid
            self.lp_low1[ch].set_lowpass(self.sample_rate, self.f_low_mid, Q_BUTTERWORTH);
            self.lp_low2[ch].set_lowpass(self.sample_rate, self.f_low_mid, Q_BUTTERWORTH);

            // Highpass LR4 at f_low_mid
            self.hp_rest1[ch].set_highpass(self.sample_rate, self.f_low_mid, Q_BUTTERWORTH);
            self.hp_rest2[ch].set_highpass(self.sample_rate, self.f_low_mid, Q_BUTTERWORTH);

            // Allpass at f_mid_high to align Low band
            self.ap_low[ch].set_allpass(self.sample_rate, self.f_mid_high, Q_BUTTERWORTH);

            // Lowpass LR4 at f_mid_high (Mid band)
            self.lp_mid1[ch].set_lowpass(self.sample_rate, self.f_mid_high, Q_BUTTERWORTH);
            self.lp_mid2[ch].set_lowpass(self.sample_rate, self.f_mid_high, Q_BUTTERWORTH);

            // Highpass LR4 at f_mid_high (High band)
            self.hp_high1[ch].set_highpass(self.sample_rate, self.f_mid_high, Q_BUTTERWORTH);
            self.hp_high2[ch].set_highpass(self.sample_rate, self.f_mid_high, Q_BUTTERWORTH);
        }
    }

    #[inline(always)]
    pub fn process_sample(&mut self, ch: usize, in_sample: f32) -> (f32, f32, f32) {
        let c = if ch != 0 { 1 } else { 0 };

        // 1. Crossover Low-Mid at f_low_mid
        let low1 = self.lp_low1[c].process(in_sample);
        let low_raw = self.lp_low2[c].process(low1);

        let rest1 = self.hp_rest1[c].process(in_sample);
        let rest = self.hp_rest2[c].process(rest1);

        // Phase compensation for Low band relative to f_mid_high
        let low_aligned = self.ap_low[c].process(low_raw);

        // 2. Crossover Mid-High at f_mid_high
        let mid1 = self.lp_mid1[c].process(rest);
        let mid = self.lp_mid2[c].process(mid1);

        let high1 = self.hp_high1[c].process(rest);
        let high = self.hp_high2[c].process(high1);

        (low_aligned, mid, high)
    }

    pub fn process_block(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        low_l: *mut f32,
        low_r: *mut f32,
        mid_l: *mut f32,
        mid_r: *mut f32,
        high_l: *mut f32,
        high_r: *mut f32,
        num_samples: usize,
    ) {
        for s in 0..num_samples {
            // Channel 0 (Left)
            let in_val_l = if !in_l.is_null() { unsafe { *in_l.add(s) } } else { 0.0 };
            let (low_out_l, mid_out_l, high_out_l) = self.process_sample(0, in_val_l);
            if !low_l.is_null() { unsafe { *low_l.add(s) = low_out_l; } }
            if !mid_l.is_null() { unsafe { *mid_l.add(s) = mid_out_l; } }
            if !high_l.is_null() { unsafe { *high_l.add(s) = high_out_l; } }

            // Channel 1 (Right)
            let in_val_r = if !in_r.is_null() { unsafe { *in_r.add(s) } } else { in_val_l };
            let (low_out_r, mid_out_r, high_out_r) = self.process_sample(1, in_val_r);
            if !low_r.is_null() { unsafe { *low_r.add(s) = low_out_r; } }
            if !mid_r.is_null() { unsafe { *mid_r.add(s) = mid_out_r; } }
            if !high_r.is_null() { unsafe { *high_r.add(s) = high_out_r; } }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_linkwitz_riley_crossover_reconstruction() {
        let mut lr = RustLinkwitzRileyCrossover3Way::new();
        lr.prepare(44100.0);
        lr.set_crossover_frequencies(300.0, 3000.0);

        let sr = 44100.0;
        let test_freqs = [100.0f32, 1000.0f32, 5000.0f32];

        for &freq in &test_freqs {
            lr.reset();
            let omega = 2.0 * PI * freq as f64 / sr;
            let mut max_in = 0.0f32;
            let mut max_out = 0.0f32;

            for i in 0..1500 {
                let in_val = (omega * i as f64).sin() as f32;
                let (low, mid, high) = lr.process_sample(0, in_val);
                let sum = low + mid + high;

                if i > 1000 {
                    max_in = max_in.max(in_val.abs());
                    max_out = max_out.max(sum.abs());
                }
            }
            assert!((max_in - max_out).abs() < 0.08, "freq: {}, in: {}, out: {}", freq, max_in, max_out);
        }
    }
}
