//! Multi-Model Non-Linear Waveshaping & Distortion DSP Kernel
//! Rules 5, 8, 9, 13, 34, 35, 46, 47: Zero allocations in audio thread,
//! SoftClip (Tanh), HardClip, Wavefold, Tube, Bitcrush, Post-Tone Lowpass Filtering,
//! and denormal protection.

use crate::common::{denormal_guard, fast_tanh, fast_exp_negative, BiquadCoeffs, BiquadState};

pub struct RustDistortion {
    sample_rate: f32,
    tone_coeffs: BiquadCoeffs,
    tone_state: [BiquadState; 2],

    target_drive: f32,
    target_mode: i32,
    target_tone_hz: f32,
    target_mix: f32,
}

impl RustDistortion {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let tone_hz = 10000.0f32;
        let coeffs = BiquadCoeffs::lowpass(sr, tone_hz, 0.707);
        Self {
            sample_rate: sr,
            tone_coeffs: coeffs,
            tone_state: [BiquadState::default(); 2],
            target_drive: 2.0,
            target_mode: 0,
            target_tone_hz: tone_hz,
            target_mix: 1.0,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.update_tone();
        self.reset();
    }

    pub fn reset(&mut self) {
        self.tone_state[0].reset();
        self.tone_state[1].reset();
    }

    fn update_tone(&mut self) {
        self.tone_coeffs = BiquadCoeffs::lowpass(self.sample_rate, self.target_tone_hz, 0.707);
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_drive = value.clamp(1.0, 50.0),
            2 => self.target_mode = (value.round() as i32).clamp(0, 4),
            3 => {
                self.target_tone_hz = value.clamp(500.0, 20000.0);
                self.update_tone();
            }
            4 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_drive,
            2 => self.target_mode as f32,
            3 => self.target_tone_hz,
            4 => self.target_mix,
            _ => 0.0,
        }
    }

    #[inline(always)]
    fn shape_sample(&self, in_val: f32, drive: f32, mode: i32) -> f32 {
        match mode {
            0 => {
                // SoftClip Tanh
                fast_tanh(in_val * drive)
            }
            1 => {
                // HardClip
                (in_val * drive).clamp(-1.0, 1.0)
            }
            2 => {
                // Wavefold
                let mut x = in_val * drive;
                let mut iter = 0;
                while (x > 1.0 || x < -1.0) && iter < 16 {
                    if x > 1.0 {
                        x = 2.0 - x;
                    } else if x < -1.0 {
                        x = -2.0 - x;
                    }
                    iter += 1;
                }
                x.clamp(-1.0, 1.0)
            }
            3 => {
                // Tube
                let x = in_val * drive;
                if x >= 0.0 {
                    1.0 - fast_exp_negative(-x)
                } else {
                    -(1.0 - fast_exp_negative(x)) * 0.8
                }
            }
            4 => {
                // Bitcrush
                let x = in_val * drive;
                let num_levels = 16.0f32; // ldexp(1.0, 4) = 16
                let scaled = x * 0.5 + 0.5;
                let quantized = (scaled * num_levels).round() / num_levels;
                (quantized - 0.5) * 2.0
            }
            _ => fast_tanh(in_val * drive),
        }
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
        if num_samples == 0 {
            return;
        }

        let drive = self.target_drive;
        let mode = self.target_mode;
        let mix = self.target_mix;
        let dry_gain = 1.0 - mix;

        for s in 0..num_samples {
            let s_l = denormal_guard(in_l[s]);
            let s_r = denormal_guard(if s < in_r.len() { in_r[s] } else { s_l });

            // Canal izquierdo
            let shaped_l = self.shape_sample(s_l, drive, mode);
            let wet_l = self.tone_state[0].process(shaped_l, &self.tone_coeffs);
            out_l[s] = denormal_guard(dry_gain * s_l + mix * wet_l);

            // Canal derecho
            if s < out_r.len() {
                let shaped_r = self.shape_sample(s_r, drive, mode);
                let wet_r = self.tone_state[1].process(shaped_r, &self.tone_coeffs);
                out_r[s] = denormal_guard(dry_gain * s_r + mix * wet_r);
            }
        }
    }
}
