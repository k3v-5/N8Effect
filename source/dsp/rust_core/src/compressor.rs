//! VCA Compressor with Soft-Knee & Stereo Sidechain DSP Kernel
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio thread,
//! Analog VCA soft-knee curve, ballistics envelope tracking, anti-click smoothing,
//! and denormal protection.

use crate::common::denormal_guard;

pub struct RustCompressor {
    sample_rate: f32,
    envelope: f32,
    current_gr: f32,

    target_threshold: f32,
    target_ratio: f32,
    target_attack_ms: f32,
    target_release_ms: f32,
    target_makeup_db: f32,
    target_knee_db: f32,

    attack_coeff: f32,
    release_coeff: f32,
}

impl RustCompressor {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 48000.0 };
        let mut comp = Self {
            sample_rate: sr,
            envelope: 0.0,
            current_gr: 1.0,
            target_threshold: -18.0,
            target_ratio: 4.0,
            target_attack_ms: 15.0,
            target_release_ms: 100.0,
            target_makeup_db: 0.0,
            target_knee_db: 4.0,
            attack_coeff: 0.01,
            release_coeff: 0.001,
        };
        comp.update_coefficients();
        comp
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 48000.0 };
        self.reset();
        self.update_coefficients();
    }

    pub fn reset(&mut self) {
        self.envelope = 0.0;
        self.current_gr = 1.0;
    }

    fn update_coefficients(&mut self) {
        let att_samples = (self.target_attack_ms.max(0.05) * 0.001) * self.sample_rate;
        let rel_samples = (self.target_release_ms.max(0.5) * 0.001) * self.sample_rate;

        self.attack_coeff = 1.0 - (-1.0 / att_samples.max(1.0)).exp();
        self.release_coeff = 1.0 - (-1.0 / rel_samples.max(1.0)).exp();
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_threshold = value.clamp(-60.0, 0.0),
            2 => self.target_ratio = value.clamp(1.0, 20.0),
            3 => {
                self.target_attack_ms = value.clamp(0.1, 200.0);
                self.update_coefficients();
            }
            4 => {
                self.target_release_ms = value.clamp(5.0, 1000.0);
                self.update_coefficients();
            }
            5 => self.target_makeup_db = value.clamp(0.0, 30.0),
            6 => self.target_knee_db = value.clamp(0.0, 20.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_threshold,
            2 => self.target_ratio,
            3 => self.target_attack_ms,
            4 => self.target_release_ms,
            5 => self.target_makeup_db,
            6 => self.target_knee_db,
            _ => 0.0,
        }
    }

    #[inline]
    fn linear_to_db(lin: f32) -> f32 {
        if lin > 1e-5 {
            20.0 * lin.log10()
        } else {
            -100.0
        }
    }

    #[inline]
    fn db_to_linear(db: f32) -> f32 {
        10.0f32.powf(db * 0.05)
    }

    #[inline]
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
        let makeup_linear = Self::db_to_linear(self.target_makeup_db);
        let threshold = self.target_threshold;
        let ratio = self.target_ratio;
        let knee = self.target_knee_db;
        let half_knee = knee * 0.5;
        let att_coeff = self.attack_coeff;
        let rel_coeff = self.release_coeff;

        for s in 0..num_samples {
            let s_l = denormal_guard(in_l[s]);
            let s_r = denormal_guard(if s < in_r.len() { in_r[s] } else { s_l });

            let sc_s_l = match sc_l {
                Some(sc) if s < sc.len() => denormal_guard(sc[s]),
                _ => s_l,
            };
            let sc_s_r = match sc_r {
                Some(sc) if s < sc.len() => denormal_guard(sc[s]),
                _ => match sc_l {
                    Some(sc) if s < sc.len() => denormal_guard(sc[s]),
                    _ => s_r,
                },
            };

            let max_sample = sc_s_l.abs().max(sc_s_r.abs());

            if max_sample > self.envelope {
                self.envelope += att_coeff * (max_sample - self.envelope);
            } else {
                self.envelope += rel_coeff * (max_sample - self.envelope);
            }

            if self.envelope.is_nan() || self.envelope.is_infinite() {
                self.envelope = 0.0;
            }

            let env_db = Self::linear_to_db(self.envelope);

            let mut target_gr_db = 0.0f32;
            if knee > 0.0 && env_db > (threshold - half_knee) && env_db < (threshold + half_knee) {
                let x = env_db - threshold + half_knee;
                target_gr_db = ((1.0 / ratio - 1.0) * (x * x)) / (2.0 * knee);
            } else if env_db >= (threshold + half_knee) {
                target_gr_db = (threshold - env_db) * (1.0 - 1.0 / ratio);
            }

            let target_gr_lin = Self::db_to_linear(target_gr_db);
            self.current_gr += 0.05 * (target_gr_lin - self.current_gr);

            let final_gain = self.current_gr * makeup_linear;

            out_l[s] = denormal_guard(s_l * final_gain);
            if s < out_r.len() {
                out_r[s] = denormal_guard(s_r * final_gain);
            }
        }
    }
}
