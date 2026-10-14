//! Bitcrusher & Vintage Lo-Fi Sample Rate Reducer DSP Kernel
//! Rules 5, 8, 9, 14, 34, 46, 47: Zero allocations in audio thread,
//! variable bit-depth quantizer (2-16 bits), sample-and-hold decimator with stochastic jitter,
//! input drive saturation (soft-clip / Padé tanh) and post-quantization lowpass anti-aliasing filter.

use std::f32::consts::PI;
use crate::common::denormal_guard;

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

#[inline(always)]
fn fast_soft_clip(x: f32) -> f32 {
    if x >= 1.5 {
        return 1.0;
    }
    if x <= -1.5 {
        return -1.0;
    }
    x - (4.0 / 27.0) * (x * x * x)
}

#[derive(Clone, Copy, Default)]
struct BiquadLP {
    b0: f32,
    b1: f32,
    b2: f32,
    a1: f32,
    a2: f32,
    z1: f32,
    z2: f32,
}

impl BiquadLP {
    #[inline(always)]
    fn reset(&mut self) {
        self.z1 = 0.0;
        self.z2 = 0.0;
    }

    #[inline(always)]
    fn process_sample(&mut self, in_val: f32) -> f32 {
        let out = self.b0 * in_val + self.z1;
        self.z1 = self.b1 * in_val - self.a1 * out + self.z2;
        self.z2 = self.b2 * in_val - self.a2 * out;
        out
    }

    fn set_lowpass(&mut self, sample_rate: f32, freq_hz: f32, q: f32) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let nyquist = sr * 0.499;
        let fc = freq_hz.clamp(10.0, nyquist);
        let q_val = q.max(0.01);
        let omega = 2.0 * PI * fc / sr;
        let sn = omega.sin();
        let cs = omega.cos();
        let alpha = sn / (2.0 * q_val);

        let a0 = 1.0 + alpha;
        let inv_a0 = 1.0 / a0;
        self.b0 = ((1.0 - cs) * 0.5) * inv_a0;
        self.b1 = (1.0 - cs) * inv_a0;
        self.b2 = ((1.0 - cs) * 0.5) * inv_a0;
        self.a1 = (-2.0 * cs) * inv_a0;
        self.a2 = (1.0 - alpha) * inv_a0;
    }
}

pub struct RustBitcrusher {
    sample_rate: f32,
    phase_acc: f32,
    held_sample_l: f32,
    held_sample_r: f32,
    rng_state: u32,

    post_filter_l: BiquadLP,
    post_filter_r: BiquadLP,

    target_bit_depth: f32,
    target_downsample: f32,
    target_jitter: f32,
    target_anti_aliasing: f32,
    target_drive: f32,
    target_mix: f32,
}

impl RustBitcrusher {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut bc = Self {
            sample_rate: sr,
            phase_acc: 0.0,
            held_sample_l: 0.0,
            held_sample_r: 0.0,
            rng_state: 0x12345678,
            post_filter_l: BiquadLP::default(),
            post_filter_r: BiquadLP::default(),
            target_bit_depth: 8.0,
            target_downsample: 4.0,
            target_jitter: 0.05,
            target_anti_aliasing: 0.0,
            target_drive: 0.0,
            target_mix: 1.0,
        };
        bc.update_filter();
        bc
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.phase_acc = 0.0;
        self.held_sample_l = 0.0;
        self.held_sample_r = 0.0;
        self.rng_state = 0x12345678;
        self.update_filter();
    }

    pub fn reset(&mut self) {
        self.phase_acc = 0.0;
        self.held_sample_l = 0.0;
        self.held_sample_r = 0.0;
        self.post_filter_l.reset();
        self.post_filter_r.reset();
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_bit_depth = value.clamp(2.0, 16.0),
            2 => self.target_downsample = value.clamp(1.0, 50.0),
            3 => self.target_jitter = value.clamp(0.0, 1.0),
            4 => {
                self.target_anti_aliasing = value.clamp(0.0, 1.0);
                self.update_filter();
            }
            5 => self.target_drive = value.clamp(0.0, 24.0),
            6 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_bit_depth,
            2 => self.target_downsample,
            3 => self.target_jitter,
            4 => self.target_anti_aliasing,
            5 => self.target_drive,
            6 => self.target_mix,
            _ => 0.0,
        }
    }

    fn update_filter(&mut self) {
        let cutoff = 18000.0 * (1500.0f32 / 18000.0f32).powf(self.target_anti_aliasing);
        self.post_filter_l.set_lowpass(self.sample_rate, cutoff, 0.707);
        self.post_filter_r.set_lowpass(self.sample_rate, cutoff, 0.707);
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
        let drive_gain = 10.0f32.powf(self.target_drive * 0.05);
        let bits = self.target_bit_depth.clamp(2.0, 16.0);
        let num_levels = 2.0f32.powf(bits - 1.0);
        let inv_levels = 1.0 / num_levels;
        let downsample_factor = self.target_downsample.max(1.0);
        let jitter_amount = self.target_jitter;
        let mix = self.target_mix.clamp(0.0, 1.0);
        let use_filter = self.target_anti_aliasing > 0.02;

        for s in 0..num_samples {
            let raw_in_l = denormal_guard(in_l[s]);
            let raw_in_r = denormal_guard(if s < in_r.len() { in_r[s] } else { raw_in_l });

            // 1. Acumulador de fase para diezmado de sample rate
            self.phase_acc += 1.0;

            let mut jitter_offset = 0.0f32;
            if jitter_amount > 0.001 {
                self.rng_state = self.rng_state.wrapping_mul(1664525).wrapping_add(1013904223);
                jitter_offset =
                    (((self.rng_state & 0xFFFF) as f32) / 65535.0 - 0.5) * jitter_amount * 2.0;
            }

            if self.phase_acc >= (downsample_factor + jitter_offset) {
                self.phase_acc -= downsample_factor;
                if self.phase_acc < 0.0 {
                    self.phase_acc = 0.0;
                }

                // 2. Saturación suave de entrada (Drive)
                let mut driven_l = raw_in_l * drive_gain;
                let mut driven_r = raw_in_r * drive_gain;

                if self.target_drive > 0.1 {
                    driven_l = fast_tanh_pade(driven_l);
                    driven_r = fast_tanh_pade(driven_r);
                } else {
                    driven_l = fast_soft_clip(driven_l);
                    driven_r = fast_soft_clip(driven_r);
                }

                // 3. Cuantización de profundidad de bits
                driven_l = driven_l.clamp(-1.0, 1.0);
                driven_r = driven_r.clamp(-1.0, 1.0);

                self.held_sample_l = (driven_l * num_levels).round() * inv_levels;
                self.held_sample_r = (driven_r * num_levels).round() * inv_levels;
            }

            let mut wet_l = self.held_sample_l;
            let mut wet_r = self.held_sample_r;

            // 4. Filtro pasa-bajos opcional
            if use_filter {
                wet_l = self.post_filter_l.process_sample(wet_l);
                wet_r = self.post_filter_r.process_sample(wet_r);
            }

            // 5. Mezcla Dry/Wet
            out_l[s] = denormal_guard((1.0 - mix) * raw_in_l + mix * wet_l);
            if s < out_r.len() {
                out_r[s] = denormal_guard((1.0 - mix) * raw_in_r + mix * wet_r);
            }
        }
    }
}
