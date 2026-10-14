//! Brickwall Limiter & Maximizer DSP Kernel
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio thread,
//! bounded lookahead circular buffer, peak ahead ballistic detection,
//! analog soft-knee True Peak limiting with Padé [5/4] rational approximation.

use crate::common::denormal_guard;

const MAX_LOOKAHEAD_SAMPLES: usize = 2048;

#[inline(always)]
fn db_to_linear(db: f32) -> f32 {
    10.0f32.powf(db * 0.05)
}

#[inline(always)]
fn fast_tanh_pade(x: f32) -> f32 {
    if x >= 3.0 {
        return 1.0;
    }
    if x <= -3.0 {
        return -1.0;
    }
    let x2 = x * x;
    let x4 = x2 * x2;
    let num = x * (945.0 + 105.0 * x2 + x4);
    let den = 945.0 + 420.0 * x2 + 15.0 * x4;
    num / den
}

pub struct RustBrickwallLimiter {
    sample_rate: f32,
    lookahead_buffer_l: [f32; MAX_LOOKAHEAD_SAMPLES],
    lookahead_buffer_r: [f32; MAX_LOOKAHEAD_SAMPLES],
    write_idx: usize,

    release_coeff: f32,
    attack_coeff: f32,
    peak_hold: f32,
    current_gain_reduction: f32,
    target_gain_reduction: f32,

    threshold_db: f32,
    ceiling_db: f32,
    release_ms: f32,
    lookahead_ms: f32,
    auto_makeup: f32,
}

impl RustBrickwallLimiter {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut limiter = Self {
            sample_rate: sr,
            lookahead_buffer_l: [0.0; MAX_LOOKAHEAD_SAMPLES],
            lookahead_buffer_r: [0.0; MAX_LOOKAHEAD_SAMPLES],
            write_idx: 0,
            release_coeff: 0.999,
            attack_coeff: 0.5,
            peak_hold: 0.0,
            current_gain_reduction: 1.0,
            target_gain_reduction: 1.0,
            threshold_db: -1.0,
            ceiling_db: -0.1,
            release_ms: 40.0,
            lookahead_ms: 2.0,
            auto_makeup: 1.0,
        };
        limiter.update_coefficients();
        limiter
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.reset();
        self.update_coefficients();
    }

    pub fn reset(&mut self) {
        self.lookahead_buffer_l.fill(0.0);
        self.lookahead_buffer_r.fill(0.0);
        self.write_idx = 0;
        self.current_gain_reduction = 1.0;
        self.target_gain_reduction = 1.0;
        self.peak_hold = 0.0;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.threshold_db = value.clamp(-24.0, 0.0),
            2 => self.ceiling_db = value.clamp(-12.0, 0.0),
            3 => {
                self.release_ms = value.clamp(2.0, 500.0);
                self.update_coefficients();
            }
            4 => {
                self.lookahead_ms = value.clamp(0.2, 5.0);
                self.update_coefficients();
            }
            5 => self.auto_makeup = if value >= 0.5 { 1.0 } else { 0.0 },
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.threshold_db,
            2 => self.ceiling_db,
            3 => self.release_ms,
            4 => self.lookahead_ms,
            5 => self.auto_makeup,
            _ => 0.0,
        }
    }

    fn update_coefficients(&mut self) {
        let rel_samples = (self.release_ms * 0.001) * self.sample_rate;
        self.release_coeff = (-1.0 / rel_samples.max(1.0)).exp();
        let att_samples = 4.0f32.max((self.lookahead_ms * 0.001 * self.sample_rate) * 0.5);
        self.attack_coeff = 1.0 - (-1.0 / att_samples).exp();
    }

    #[inline]
    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len().min(out_l.len());
        let lookahead_samples = ((self.lookahead_ms * 0.001 * self.sample_rate) as usize)
            .clamp(1, MAX_LOOKAHEAD_SAMPLES - 1);

        let input_gain = if self.auto_makeup > 0.5 {
            db_to_linear(-self.threshold_db)
        } else {
            1.0
        };
        let ceiling_linear = db_to_linear(self.ceiling_db);

        let knee = ceiling_linear * 0.92;
        let headroom = ceiling_linear - knee;
        let inv_headroom = if headroom.abs() > 1e-8 { 1.0 / headroom } else { 1.0 };

        for i in 0..num_samples {
            let raw_in_l = denormal_guard(in_l[i]);
            let raw_in_r = denormal_guard(if i < in_r.len() { in_r[i] } else { raw_in_l });

            // 1. Boost / Threshold
            let boosted_l = raw_in_l * input_gain;
            let boosted_r = raw_in_r * input_gain;

            // 2. Buffer circular de lookahead
            self.lookahead_buffer_l[self.write_idx] = boosted_l;
            self.lookahead_buffer_r[self.write_idx] = boosted_r;

            // 3. Peak ahead detection
            let current_peak = boosted_l.abs().max(boosted_r.abs());
            if current_peak > self.peak_hold {
                self.peak_hold = current_peak;
            } else {
                self.peak_hold *= self.release_coeff;
            }

            // 4. Target Gain Reduction
            if self.peak_hold > ceiling_linear && self.peak_hold > 1e-6 {
                self.target_gain_reduction = ceiling_linear / self.peak_hold;
            } else {
                self.target_gain_reduction = 1.0;
            }

            // 5. Suavizado balístico
            if self.target_gain_reduction < self.current_gain_reduction {
                self.current_gain_reduction +=
                    self.attack_coeff * (self.target_gain_reduction - self.current_gain_reduction);
            } else {
                self.current_gain_reduction +=
                    (1.0 - self.release_coeff) * (self.target_gain_reduction - self.current_gain_reduction);
            }

            // 6. Leer señal retrasada
            let read_idx = (self.write_idx + MAX_LOOKAHEAD_SAMPLES - lookahead_samples)
                % MAX_LOOKAHEAD_SAMPLES;
            let delayed_l = self.lookahead_buffer_l[read_idx];
            let delayed_r = self.lookahead_buffer_r[read_idx];

            self.write_idx = (self.write_idx + 1) % MAX_LOOKAHEAD_SAMPLES;

            // 7. Aplicar ganancia
            let mut limited_l = delayed_l * self.current_gain_reduction;
            let mut limited_r = delayed_r * self.current_gain_reduction;

            // 8. Soft-knee Padé limiter
            if limited_l > knee {
                limited_l = knee + headroom * fast_tanh_pade((limited_l - knee) * inv_headroom);
            } else if limited_l < -knee {
                limited_l = -knee - headroom * fast_tanh_pade((-limited_l - knee) * inv_headroom);
            }

            if limited_r > knee {
                limited_r = knee + headroom * fast_tanh_pade((limited_r - knee) * inv_headroom);
            } else if limited_r < -knee {
                limited_r = -knee - headroom * fast_tanh_pade((-limited_r - knee) * inv_headroom);
            }

            out_l[i] = denormal_guard(limited_l);
            if i < out_r.len() {
                out_r[i] = denormal_guard(limited_r);
            }
        }
    }
}
