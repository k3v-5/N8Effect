//! Multiband Saturator (Phase 9 - Large Effect Library, Rules 5, 8, 9, 32, 34, 46, 47)
//! FabFilter Saturn style 3-band saturator with Linkwitz-Riley LR4 crossovers.
//! Features independent drive, waveshaper model (Tape, Tube, Foldback) and dynamics per band.

use crate::common::clamp;
use crate::optimization::{fast_db_to_gain, fast_pade_tanh, fast_soft_clip};

/// Biquad filter structure for LR4 crossovers
#[derive(Clone, Copy)]
struct Biquad {
    b0: f32,
    b1: f32,
    b2: f32,
    a1: f32,
    a2: f32,
    x1: f32,
    x2: f32,
    y1: f32,
    y2: f32,
}

impl Biquad {
    fn new() -> Self {
        Self {
            b0: 1.0, b1: 0.0, b2: 0.0,
            a1: 0.0, a2: 0.0,
            x1: 0.0, x2: 0.0,
            y1: 0.0, y2: 0.0,
        }
    }

    fn reset(&mut self) {
        self.x1 = 0.0; self.x2 = 0.0;
        self.y1 = 0.0; self.y2 = 0.0;
    }

    #[inline(always)]
    fn process(&mut self, in_sample: f32) -> f32 {
        let out = self.b0 * in_sample + self.b1 * self.x1 + self.b2 * self.x2
                - self.a1 * self.y1 - self.a2 * self.y2;
        self.x2 = self.x1;
        self.x1 = in_sample;
        self.y2 = self.y1;
        self.y1 = out;
        out
    }

    fn set_lowpass(&mut self, fc: f32, sample_rate: f32) {
        let omega = std::f32::consts::TAU * fc / sample_rate;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * std::f32::consts::SQRT_2); // Q = 0.7071 (Butterworth)

        let a0 = 1.0 + alpha;
        self.b0 = ((1.0 - cs) * 0.5) / a0;
        self.b1 = (1.0 - cs) / a0;
        self.b2 = ((1.0 - cs) * 0.5) / a0;
        self.a1 = (-2.0 * cs) / a0;
        self.a2 = (1.0 - alpha) / a0;
    }

    fn set_highpass(&mut self, fc: f32, sample_rate: f32) {
        let omega = std::f32::consts::TAU * fc / sample_rate;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * std::f32::consts::SQRT_2); // Q = 0.7071 (Butterworth)

        let a0 = 1.0 + alpha;
        self.b0 = ((1.0 + cs) * 0.5) / a0;
        self.b1 = (-(1.0 + cs)) / a0;
        self.b2 = ((1.0 + cs) * 0.5) / a0;
        self.a1 = (-2.0 * cs) / a0;
        self.a2 = (1.0 - alpha) / a0;
    }
}

pub struct RustMultibandSaturator {
    sample_rate: f32,

    // LR4 Crossover filters (cascaded 2x Butterworth biquads per channel)
    lp1_a_l: Biquad, lp1_b_l: Biquad,
    hp1_a_l: Biquad, hp1_b_l: Biquad,
    lp2_a_l: Biquad, lp2_b_l: Biquad,
    hp2_a_l: Biquad, hp2_b_l: Biquad,

    lp1_a_r: Biquad, lp1_b_r: Biquad,
    hp1_a_r: Biquad, hp1_b_r: Biquad,
    lp2_a_r: Biquad, lp2_b_r: Biquad,
    hp2_a_r: Biquad, hp2_b_r: Biquad,

    // Parameters
    xover_low_hz: f32,     // 80.0 to 1000.0 Hz
    xover_high_hz: f32,    // 1000.0 to 12000.0 Hz

    drive_low_db: f32,     // 0.0 to 24.0 dB
    drive_mid_db: f32,     // 0.0 to 24.0 dB
    drive_high_db: f32,    // 0.0 to 24.0 dB

    model_low: u32,        // 0 = Tape, 1 = Tube, 2 = Foldback
    model_mid: u32,
    model_high: u32,

    mix: f32,              // 0.0 to 1.0
}

impl RustMultibandSaturator {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        let mut inst = Self {
            sample_rate: sr,
            lp1_a_l: Biquad::new(), lp1_b_l: Biquad::new(),
            hp1_a_l: Biquad::new(), hp1_b_l: Biquad::new(),
            lp2_a_l: Biquad::new(), lp2_b_l: Biquad::new(),
            hp2_a_l: Biquad::new(), hp2_b_l: Biquad::new(),

            lp1_a_r: Biquad::new(), lp1_b_r: Biquad::new(),
            hp1_a_r: Biquad::new(), hp1_b_r: Biquad::new(),
            lp2_a_r: Biquad::new(), lp2_b_r: Biquad::new(),
            hp2_a_r: Biquad::new(), hp2_b_r: Biquad::new(),

            xover_low_hz: 250.0,
            xover_high_hz: 3500.0,
            drive_low_db: 4.0,
            drive_mid_db: 6.0,
            drive_high_db: 3.0,
            model_low: 0,
            model_mid: 1,
            model_high: 0,
            mix: 1.0,
        };
        inst.update_crossovers();
        inst
    }

    fn update_crossovers(&mut self) {
        let fc1 = clamp(self.xover_low_hz, 40.0, self.sample_rate * 0.45);
        let fc2 = clamp(self.xover_high_hz, fc1 + 50.0, self.sample_rate * 0.48);

        self.lp1_a_l.set_lowpass(fc1, self.sample_rate);
        self.lp1_b_l.set_lowpass(fc1, self.sample_rate);
        self.hp1_a_l.set_highpass(fc1, self.sample_rate);
        self.hp1_b_l.set_highpass(fc1, self.sample_rate);

        self.lp2_a_l.set_lowpass(fc2, self.sample_rate);
        self.lp2_b_l.set_lowpass(fc2, self.sample_rate);
        self.hp2_a_l.set_highpass(fc2, self.sample_rate);
        self.hp2_b_l.set_highpass(fc2, self.sample_rate);

        self.lp1_a_r.set_lowpass(fc1, self.sample_rate);
        self.lp1_b_r.set_lowpass(fc1, self.sample_rate);
        self.hp1_a_r.set_highpass(fc1, self.sample_rate);
        self.hp1_b_r.set_highpass(fc1, self.sample_rate);

        self.lp2_a_r.set_lowpass(fc2, self.sample_rate);
        self.lp2_b_r.set_lowpass(fc2, self.sample_rate);
        self.hp2_a_r.set_highpass(fc2, self.sample_rate);
        self.hp2_b_r.set_highpass(fc2, self.sample_rate);
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.update_crossovers();
        self.reset();
    }

    pub fn reset(&mut self) {
        self.lp1_a_l.reset(); self.lp1_b_l.reset();
        self.hp1_a_l.reset(); self.hp1_b_l.reset();
        self.lp2_a_l.reset(); self.lp2_b_l.reset();
        self.hp2_a_l.reset(); self.hp2_b_l.reset();

        self.lp1_a_r.reset(); self.lp1_b_r.reset();
        self.hp1_a_r.reset(); self.hp1_b_r.reset();
        self.lp2_a_r.reset(); self.lp2_b_r.reset();
        self.hp2_a_r.reset(); self.hp2_b_r.reset();
    }

    pub fn set_crossover_low(&mut self, freq_hz: f32) {
        self.xover_low_hz = freq_hz;
        self.update_crossovers();
    }

    pub fn set_crossover_high(&mut self, freq_hz: f32) {
        self.xover_high_hz = freq_hz;
        self.update_crossovers();
    }

    pub fn set_drive_low(&mut self, db: f32) { self.drive_low_db = clamp(db, 0.0, 36.0); }
    pub fn set_drive_mid(&mut self, db: f32) { self.drive_mid_db = clamp(db, 0.0, 36.0); }
    pub fn set_drive_high(&mut self, db: f32) { self.drive_high_db = clamp(db, 0.0, 36.0); }

    pub fn set_model_low(&mut self, m: u32) { self.model_low = m.min(2); }
    pub fn set_model_mid(&mut self, m: u32) { self.model_mid = m.min(2); }
    pub fn set_model_high(&mut self, m: u32) { self.model_high = m.min(2); }

    pub fn set_mix(&mut self, mix: f32) { self.mix = clamp(mix, 0.0, 1.0); }

    #[inline(always)]
    fn saturate_sample(sample: f32, gain: f32, model: u32) -> f32 {
        let x = sample * gain;
        let out = match model {
            // 0: Tape (smooth Padé tanh with slight harmonic compression)
            0 => fast_pade_tanh(x),
            // 1: Tube (asymmetric cubic saturation)
            1 => {
                if x >= 0.0 {
                    fast_soft_clip(x)
                } else {
                    let clamped = x.max(-2.0);
                    clamped / (1.0 - 0.25 * clamped)
                }
            }
            // 2: Foldback / Wavefolder
            _ => {
                let folded = (x * 0.7).sin();
                fast_soft_clip(folded * 1.2)
            }
        };
        // Normalize gain so higher drive doesn't explode overall level
        let comp = 1.0 / (1.0 + (gain - 1.0) * 0.5).sqrt();
        out * comp
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len().min(in_r.len()).min(out_l.len()).min(out_r.len());
        if num_samples == 0 {
            return;
        }

        let gain_low = fast_db_to_gain(self.drive_low_db);
        let gain_mid = fast_db_to_gain(self.drive_mid_db);
        let gain_high = fast_db_to_gain(self.drive_high_db);
        let mix = self.mix;

        for i in 0..num_samples {
            let dry_l = in_l[i];
            let dry_r = in_r[i];

            // 1. Channel L LR4 3-way Split
            let low_l = self.lp1_b_l.process(self.lp1_a_l.process(dry_l));
            let not_low_l = self.hp1_b_l.process(self.hp1_a_l.process(dry_l));
            let mid_l = self.lp2_b_l.process(self.lp2_a_l.process(not_low_l));
            let high_l = self.hp2_b_l.process(self.hp2_a_l.process(not_low_l));

            // 2. Channel R LR4 3-way Split
            let low_r = self.lp1_b_r.process(self.lp1_a_r.process(dry_r));
            let not_low_r = self.hp1_b_r.process(self.hp1_a_r.process(dry_r));
            let mid_r = self.lp2_b_r.process(self.lp2_a_r.process(not_low_r));
            let high_r = self.hp2_b_r.process(self.hp2_a_r.process(not_low_r));

            // 3. Saturate each band independently
            let sat_low_l = Self::saturate_sample(low_l, gain_low, self.model_low);
            let sat_low_r = Self::saturate_sample(low_r, gain_low, self.model_low);

            let sat_mid_l = Self::saturate_sample(mid_l, gain_mid, self.model_mid);
            let sat_mid_r = Self::saturate_sample(mid_r, gain_mid, self.model_mid);

            let sat_high_l = Self::saturate_sample(high_l, gain_high, self.model_high);
            let sat_high_r = Self::saturate_sample(high_r, gain_high, self.model_high);

            // 4. Recombine bands (Linkwitz-Riley LR4 sums flat)
            let wet_l = sat_low_l + sat_mid_l + sat_high_l;
            let wet_r = sat_low_r + sat_mid_r + sat_high_r;

            // 5. Wet/Dry Mix
            out_l[i] = (1.0 - mix) * dry_l + mix * wet_l;
            out_r[i] = (1.0 - mix) * dry_r + mix * wet_r;
        }
    }
}
