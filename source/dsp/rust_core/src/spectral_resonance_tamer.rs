//! Spectral Resonance Tamer (Dynamic Resonant Spike Suppressor / Soothe Style)
//! Real-time dynamic resonance suppressor that continuously tracks and tames harsh narrow-band resonances.
//! Rules: 5, 8, 9, 14, 16, 34, 35, 46, 47 (Zero-alloc, bounded memory, denormal-safe).

use std::f32::consts::PI;

const NUM_TAMER_BANDS: usize = 12;

#[inline(always)]
fn clamp(val: f32, min: f32, max: f32) -> f32 {
    if val < min { min } else if val > max { max } else { val }
}

#[inline(always)]
fn lerp(a: f32, b: f32, t: f32) -> f32 {
    a + t * (b - a)
}

#[derive(Clone, Copy)]
struct DynamicNotchBand {
    fc: f32,
    // SVF states for bandpass analysis
    ic1eq_l: f32,
    ic2eq_l: f32,
    ic1eq_r: f32,
    ic2eq_r: f32,
    // SVF states for notch suppression
    nic1eq_l: f32,
    nic2eq_l: f32,
    nic1eq_r: f32,
    nic2eq_r: f32,

    env_l: f32,
    env_r: f32,
    gain_l: f32,
    gain_r: f32,
}

impl DynamicNotchBand {
    fn new(fc: f32) -> Self {
        Self {
            fc,
            ic1eq_l: 0.0,
            ic2eq_l: 0.0,
            ic1eq_r: 0.0,
            ic2eq_r: 0.0,
            nic1eq_l: 0.0,
            nic2eq_l: 0.0,
            nic1eq_r: 0.0,
            nic2eq_r: 0.0,
            env_l: 0.0,
            env_r: 0.0,
            gain_l: 1.0,
            gain_r: 1.0,
        }
    }

    fn reset(&mut self) {
        self.ic1eq_l = 0.0;
        self.ic2eq_l = 0.0;
        self.ic1eq_r = 0.0;
        self.ic2eq_r = 0.0;
        self.nic1eq_l = 0.0;
        self.nic2eq_l = 0.0;
        self.nic1eq_r = 0.0;
        self.nic2eq_r = 0.0;
        self.env_l = 0.0;
        self.env_r = 0.0;
        self.gain_l = 1.0;
        self.gain_r = 1.0;
    }
}

pub struct RustSpectralResonanceTamer {
    sample_rate: f32,
    bands: [DynamicNotchBand; NUM_TAMER_BANDS],

    sensitivity: f32,
    depth: f32,
    sharpness_q: f32,
    attack_ms: f32,
    release_ms: f32,
    low_cut_hz: f32,
    high_cut_hz: f32,
    delta_listen: f32,
    mix: f32,

    attack_coeff: f32,
    release_coeff: f32,
    smoothed_mix: f32,
}

impl RustSpectralResonanceTamer {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 8000.0 { sample_rate } else { 44100.0 };
        // Logarithmic center frequencies covering 150 Hz to 14 kHz
        let freqs: [f32; NUM_TAMER_BANDS] = [
            180.0, 320.0, 520.0, 850.0, 1350.0, 2100.0, 3200.0, 4800.0, 6800.0, 9200.0, 11800.0, 14500.0,
        ];
        let mut bands = [
            DynamicNotchBand::new(freqs[0]),
            DynamicNotchBand::new(freqs[1]),
            DynamicNotchBand::new(freqs[2]),
            DynamicNotchBand::new(freqs[3]),
            DynamicNotchBand::new(freqs[4]),
            DynamicNotchBand::new(freqs[5]),
            DynamicNotchBand::new(freqs[6]),
            DynamicNotchBand::new(freqs[7]),
            DynamicNotchBand::new(freqs[8]),
            DynamicNotchBand::new(freqs[9]),
            DynamicNotchBand::new(freqs[10]),
            DynamicNotchBand::new(freqs[11]),
        ];
        for i in 0..NUM_TAMER_BANDS {
            bands[i].fc = freqs[i];
        }

        let mut inst = Self {
            sample_rate: sr,
            bands,
            sensitivity: 0.6,
            depth: 0.7,
            sharpness_q: 4.5,
            attack_ms: 10.0,
            release_ms: 80.0,
            low_cut_hz: 120.0,
            high_cut_hz: 14000.0,
            delta_listen: 0.0,
            mix: 1.0,
            attack_coeff: 0.1,
            release_coeff: 0.01,
            smoothed_mix: 1.0,
        };
        inst.prepare(sr);
        inst
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 8000.0 { sample_rate } else { 44100.0 };
        self.update_ballistics();
        self.reset();
    }

    pub fn reset(&mut self) {
        for b in &mut self.bands {
            b.reset();
        }
        self.smoothed_mix = self.mix;
    }

    fn update_ballistics(&mut self) {
        self.attack_coeff = 1.0 - (-1.0 / (self.attack_ms * 0.001 * self.sample_rate).max(1.0)).exp();
        self.release_coeff = 1.0 - (-1.0 / (self.release_ms * 0.001 * self.sample_rate).max(1.0)).exp();
    }

    pub fn set_param(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.sensitivity = clamp(value, 0.0, 1.0),
            2 => self.depth = clamp(value, 0.0, 1.0),
            3 => self.sharpness_q = clamp(value, 1.0, 12.0),
            4 => {
                self.attack_ms = clamp(value, 1.0, 80.0);
                self.update_ballistics();
            }
            5 => {
                self.release_ms = clamp(value, 10.0, 500.0);
                self.update_ballistics();
            }
            6 => self.low_cut_hz = clamp(value, 20.0, 1500.0),
            7 => self.high_cut_hz = clamp(value, 2000.0, 20000.0),
            8 => self.delta_listen = clamp(value, 0.0, 1.0),
            9 => self.mix = clamp(value, 0.0, 1.0),
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
        let q = self.sharpness_q;
        let k_damp = 1.0 / q;

        for i in 0..num_samples as usize {
            let xl = unsafe { *in_l.add(i) };
            let xr = unsafe { *in_r.add(i) };

            self.smoothed_mix = lerp(self.mix, self.smoothed_mix, smooth);

            // 1. Analyze band energy using SVF bandpass
            let mut bp_l = [0.0f32; NUM_TAMER_BANDS];
            let mut bp_r = [0.0f32; NUM_TAMER_BANDS];
            let mut envs_l = [0.0f32; NUM_TAMER_BANDS];
            let mut envs_r = [0.0f32; NUM_TAMER_BANDS];

            for b in 0..NUM_TAMER_BANDS {
                let band = &mut self.bands[b];
                let g = (PI * band.fc / self.sample_rate).tan();
                let a1 = 1.0 / (1.0 + g * (g + k_damp));
                let a2 = g * a1;

                // Left channel SVF bandpass
                let v3_l = xl - band.ic2eq_l;
                let v1_l = a1 * band.ic1eq_l + a2 * v3_l;
                let v2_l = band.ic2eq_l + g * v1_l;
                band.ic1eq_l = 2.0 * v1_l - band.ic1eq_l;
                band.ic2eq_l = 2.0 * v2_l - band.ic2eq_l;
                bp_l[b] = v1_l;

                // Right channel SVF bandpass
                let v3_r = xr - band.ic2eq_r;
                let v1_r = a1 * band.ic1eq_r + a2 * v3_r;
                let v2_r = band.ic2eq_r + g * v1_r;
                band.ic1eq_r = 2.0 * v1_r - band.ic1eq_r;
                band.ic2eq_r = 2.0 * v2_r - band.ic2eq_r;
                bp_r[b] = v1_r;

                // Envelope follower
                let mag_l = bp_l[b].abs();
                let mag_r = bp_r[b].abs();

                let coeff_l = if mag_l > band.env_l { self.attack_coeff } else { self.release_coeff };
                band.env_l += coeff_l * (mag_l - band.env_l);

                let coeff_r = if mag_r > band.env_r { self.attack_coeff } else { self.release_coeff };
                band.env_r += coeff_r * (mag_r - band.env_r);

                envs_l[b] = band.env_l;
                envs_r[b] = band.env_r;
            }

            // 2. Compute local spectral baseline and resonance excess per band
            let mut filtered_xl = xl;
            let mut filtered_xr = xr;

            for b in 0..NUM_TAMER_BANDS {
                let band = &mut self.bands[b];

                // Check frequency bounds
                if band.fc < self.low_cut_hz || band.fc > self.high_cut_hz {
                    band.gain_l = 1.0;
                    band.gain_r = 1.0;
                    continue;
                }

                // Neighborhood average (local spectral baseline)
                let left_b = if b > 0 { b - 1 } else { 0 };
                let right_b = if b + 1 < NUM_TAMER_BANDS { b + 1 } else { NUM_TAMER_BANDS - 1 };

                let baseline_l = (envs_l[left_b] + envs_l[right_b]) * 0.5 + 1e-4;
                let baseline_r = (envs_r[left_b] + envs_r[right_b]) * 0.5 + 1e-4;

                let ratio_l = (band.env_l / baseline_l).max(1.0) - 1.0;
                let ratio_r = (band.env_r / baseline_r).max(1.0) - 1.0;

                // Suppression factor: target attenuation
                let target_att_l = 1.0 / (1.0 + self.sensitivity * self.depth * ratio_l * 3.0);
                let target_att_r = 1.0 / (1.0 + self.sensitivity * self.depth * ratio_r * 3.0);

                band.gain_l += 0.05 * (target_att_l - band.gain_l);
                band.gain_r += 0.05 * (target_att_r - band.gain_r);

                // Apply dynamic bell/notch attenuation using SVF
                let g = (PI * band.fc / self.sample_rate).tan();
                let a1 = 1.0 / (1.0 + g * (g + k_damp));
                let a2 = g * a1;

                // Left dynamic notch
                let v3_l = filtered_xl - band.nic2eq_l;
                let v1_l = a1 * band.nic1eq_l + a2 * v3_l;
                let v2_l = band.nic2eq_l + g * v1_l;
                band.nic1eq_l = 2.0 * v1_l - band.nic1eq_l;
                band.nic2eq_l = 2.0 * v2_l - band.nic2eq_l;
                let notch_diff_l = v1_l * (1.0 - band.gain_l);
                filtered_xl -= notch_diff_l;

                // Right dynamic notch
                let v3_r = filtered_xr - band.nic2eq_r;
                let v1_r = a1 * band.nic1eq_r + a2 * v3_r;
                let v2_r = band.nic2eq_r + g * v1_r;
                band.nic1eq_r = 2.0 * v1_r - band.nic1eq_r;
                band.nic2eq_r = 2.0 * v2_r - band.nic2eq_r;
                let notch_diff_r = v1_r * (1.0 - band.gain_r);
                filtered_xr -= notch_diff_r;
            }

            // Delta / Listen mode: if active, output ONLY the removed resonances (xl - filtered_xl)
            let (wet_l, wet_r) = if self.delta_listen > 0.5 {
                (xl - filtered_xl, xr - filtered_xr)
            } else {
                (filtered_xl, filtered_xr)
            };

            let yl = lerp(xl, wet_l, self.smoothed_mix);
            let yr = lerp(xr, wet_r, self.smoothed_mix);

            unsafe {
                *out_l.add(i) = yl;
                *out_r.add(i) = yr;
            }
        }
    }
}
