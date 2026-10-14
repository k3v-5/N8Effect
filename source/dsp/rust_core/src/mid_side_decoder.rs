//! Mid/Side to Stereo Decoder DSP Kernel with Mono Bass Maker (DSP #25)
//! Rules 5, 8, 9, 13, 16, 34, 46, 47: Zero allocations in audio thread,
//! continuous stereo width scaling, Butterworth highpass side filter (mono bass),
//! anti-click smoothing, normalized power matrix, and denormal protection.

use crate::common::{denormal_guard, BiquadCoeffs, BiquadState};

const INV_SQRT_2: f32 = 0.70710678;

pub struct RustMidSideDecoder {
    sample_rate: f32,
    target_width: f32,
    current_width: f32,
    target_mono_bass_freq: f32,
    target_mono_bass_active: f32,
    target_gain: f32,
    current_gain: f32,

    side_hp_coeffs: BiquadCoeffs,
    side_hp_state: BiquadState,
}

impl RustMidSideDecoder {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut dec = Self {
            sample_rate: sr,
            target_width: 1.0,
            current_width: 1.0,
            target_mono_bass_freq: 120.0,
            target_mono_bass_active: 1.0,
            target_gain: 1.0,
            current_gain: 1.0,
            side_hp_coeffs: BiquadCoeffs::default(),
            side_hp_state: BiquadState::default(),
        };
        dec.update_filter();
        dec
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.sample_rate = sr;
        self.update_filter();
        self.reset();
        self.current_width = self.target_width;
        self.current_gain = self.target_gain;
    }

    pub fn reset(&mut self) {
        self.side_hp_state.reset();
        self.current_width = self.target_width;
        self.current_gain = self.target_gain;
    }

    #[inline]
    fn update_filter(&mut self) {
        self.side_hp_coeffs = BiquadCoeffs::highpass(self.sample_rate, self.target_mono_bass_freq, 0.707);
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_width = value.clamp(0.0, 2.0),
            2 => {
                self.target_mono_bass_freq = value.clamp(20.0, 400.0);
                self.update_filter();
            }
            3 => self.target_mono_bass_active = value,
            4 => self.target_gain = value.clamp(0.0, 2.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_width,
            2 => self.target_mono_bass_freq,
            3 => self.target_mono_bass_active,
            4 => self.target_gain,
            _ => 0.0,
        }
    }

    #[inline]
    pub fn process_block(
        &mut self,
        in_mid: &[f32],
        in_side: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_mid.len().min(out_l.len());
        if num_samples == 0 {
            return;
        }

        const ALPHA: f32 = 0.005;
        let mono_bass_enabled = self.target_mono_bass_active > 0.5;

        for s in 0..num_samples {
            self.current_width += ALPHA * (self.target_width - self.current_width);
            self.current_gain += ALPHA * (self.target_gain - self.current_gain);

            let mid = in_mid[s];
            let side_raw = if s < in_side.len() { in_side[s] } else { 0.0 };

            // 1. Filtrado Mono Bass Maker en el canal Side si está activo
            let side = if mono_bass_enabled {
                self.side_hp_state.process(side_raw, &self.side_hp_coeffs)
            } else {
                side_raw
            };

            // 2. Ancho estéreo continuo: S' = S * Width
            let side_scaled = side * self.current_width;

            // 3. Matriz inversa: L = (M + S') * 0.7071, R = (M - S') * 0.7071
            let out_l_val = (mid + side_scaled) * INV_SQRT_2 * self.current_gain;
            let out_r_val = (mid - side_scaled) * INV_SQRT_2 * self.current_gain;

            out_l[s] = denormal_guard(out_l_val);
            if s < out_r.len() {
                out_r[s] = denormal_guard(out_r_val);
            }
        }
    }
}
