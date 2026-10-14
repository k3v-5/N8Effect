//! ModalResonatorNode 8-Mode Acoustic Physical Resonator in Rust
//! High-performance SIMD-friendly physical resonator modeling Bell, Wood, Drum, and String.

use crate::common::DENORMAL_GUARD;
use std::f32::consts::PI;

const BELL_RATIOS: [f32; 8] = [1.0, 2.00, 2.80, 4.07, 5.25, 6.80, 8.40, 10.2];
const WOOD_RATIOS: [f32; 8] = [1.0, 2.76, 5.40, 8.93, 13.3, 18.6, 24.8, 31.9];
const DRUM_RATIOS: [f32; 8] = [1.0, 1.59, 2.14, 2.30, 2.65, 2.92, 3.16, 3.50];
const STRING_RATIOS: [f32; 8] = [1.0, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00];

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct ResonatorState {
    pub y1: f32,
    pub y2: f32,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
struct ModeCoeffs {
    a1: f32,
    a2: f32,
    b0: f32,
    active: bool,
}

#[repr(C)]
pub struct RustModalResonator {
    sample_rate: f32,

    // Filter states for 8 modes x 2 channels
    modes_l: [ResonatorState; 8],
    modes_r: [ResonatorState; 8],

    // Parameters
    param_fund_freq: f32,     // 1: 20.0 .. 2000.0 Hz
    param_structure: f32,     // 2: 0: Bell, 1: Wood, 2: Drum, 3: String
    param_decay_sec: f32,     // 3: 0.05 .. 6.0 s
    param_inharmonicity: f32, // 4: 0.0 .. 1.0
    param_damping: f32,       // 5: 0.0 .. 1.0
    param_mix: f32,           // 6: 0.0 .. 1.0
}

impl RustModalResonator {
    pub fn new(sample_rate: f32) -> Self {
        Self {
            sample_rate: if sample_rate > 0.0 { sample_rate } else { 48000.0 },
            modes_l: [ResonatorState::default(); 8],
            modes_r: [ResonatorState::default(); 8],
            param_fund_freq: 220.0,
            param_structure: 0.0,
            param_decay_sec: 1.2,
            param_inharmonicity: 0.15,
            param_damping: 0.35,
            param_mix: 0.6,
        }
    }

    pub fn reset(&mut self) {
        for m in &mut self.modes_l {
            m.y1 = 0.0;
            m.y2 = 0.0;
        }
        for m in &mut self.modes_r {
            m.y1 = 0.0;
            m.y2 = 0.0;
        }
    }

    pub fn set_sample_rate(&mut self, sample_rate: f32) {
        if sample_rate > 0.0 {
            self.sample_rate = sample_rate;
        }
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.param_fund_freq = value.clamp(20.0, 2000.0),
            2 => self.param_structure = value.clamp(0.0, 3.0),
            3 => self.param_decay_sec = value.clamp(0.05, 6.0),
            4 => self.param_inharmonicity = value.clamp(0.0, 1.0),
            5 => self.param_damping = value.clamp(0.0, 1.0),
            6 => self.param_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            1 => self.param_fund_freq,
            2 => self.param_structure,
            3 => self.param_decay_sec,
            4 => self.param_inharmonicity,
            5 => self.param_damping,
            6 => self.param_mix,
            _ => 0.0,
        }
    }

    fn calculate_coefficients(&self) -> [ModeCoeffs; 8] {
        let fund = self.param_fund_freq.clamp(20.0, 2000.0);
        let struct_type = self.param_structure.round() as i32;
        let decay = self.param_decay_sec.clamp(0.05, 6.0);
        let inharm = self.param_inharmonicity.clamp(0.0, 1.0);
        let damp = self.param_damping.clamp(0.0, 1.0);

        let base_ratios = match struct_type {
            1 => &WOOD_RATIOS,
            2 => &DRUM_RATIOS,
            3 => &STRING_RATIOS,
            _ => &BELL_RATIOS,
        };

        let mut coeffs = [ModeCoeffs::default(); 8];
        let two_pi_over_sr = 2.0 * PI / self.sample_rate;

        for m in 0..8 {
            let mut ratio = base_ratios[m];
            ratio *= 1.0 + inharm * 0.15 * ((m * m) as f32);
            let mode_freq = fund * ratio;

            if mode_freq >= self.sample_rate * 0.46 {
                coeffs[m].active = false;
                continue;
            }

            let mode_decay_sec = (decay / (1.0 + damp * (m as f32) * 1.5)).max(0.005);
            let r = (-PI / (mode_decay_sec * self.sample_rate)).exp().clamp(0.90, 0.9999);
            let w = mode_freq * two_pi_over_sr;

            coeffs[m].a1 = -2.0 * r * w.cos();
            coeffs[m].a2 = r * r;
            coeffs[m].b0 = (1.0 - r) * w.sin() * (1.0 / (1.0 + (m as f32) * 0.35));
            coeffs[m].active = true;
        }

        coeffs
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let n_samples = in_l.len().min(out_l.len());
        if n_samples == 0 {
            return;
        }

        let coeffs = self.calculate_coefficients();
        let mix = self.param_mix.clamp(0.0, 1.0);
        let dry = 1.0 - mix;

        for s in 0..n_samples {
            let s_l = in_l[s];
            let s_r = if s < in_r.len() { in_r[s] } else { s_l };

            let mut res_l = 0.0f32;
            let mut res_r = 0.0f32;

            for m in 0..8 {
                if !coeffs[m].active {
                    continue;
                }

                let a1 = coeffs[m].a1;
                let a2 = coeffs[m].a2;
                let b0 = coeffs[m].b0;

                // Left channel resonator
                let st_l = &mut self.modes_l[m];
                let y_l = b0 * s_l - a1 * st_l.y1 - a2 * st_l.y2 + DENORMAL_GUARD;
                st_l.y2 = st_l.y1;
                st_l.y1 = y_l;
                res_l += y_l;

                // Right channel resonator
                let st_r = &mut self.modes_r[m];
                let y_r = b0 * s_r - a1 * st_r.y1 - a2 * st_r.y2 + DENORMAL_GUARD;
                st_r.y2 = st_r.y1;
                st_r.y1 = y_r;
                res_r += y_r;
            }

            out_l[s] = s_l * dry + res_l * mix;
            if s < out_r.len() {
                out_r[s] = s_r * dry + res_r * mix;
            }
        }
    }
}
