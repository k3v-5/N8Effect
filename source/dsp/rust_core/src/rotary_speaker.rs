//! n8_rust_core - Rotary Speaker / Leslie Cabinet Simulator (DSP #43)
//! Dual rotor simulation (Horn treble rotor & Drum bass rotor) with
//! Linkwitz-Riley crossover (800 Hz), Doppler frequency shift delay modulation,
//! counter-rotating amplitude modulation, tube preamp drive, and mechanical inertia
//! (Rules 5, 8, 9, 14, 34, 46, 47).

use crate::common::{self, BiquadCoeffs, BiquadState};
use std::f32::consts::PI;

pub const MAX_DELAY_SAMPLES: usize = 2048;

pub const PARAM_SPEED_MODE: u32 = 1;
pub const PARAM_HORN_SPEED: u32 = 2;
pub const PARAM_DRUM_SPEED: u32 = 3;
pub const PARAM_DRIVE: u32 = 4;
pub const PARAM_SPREAD: u32 = 5;
pub const PARAM_MIX: u32 = 6;

pub struct RustRotarySpeaker {
    sample_rate: f32,
    max_delay_samples: usize,
    horn_delay_buffers: [[f32; MAX_DELAY_SAMPLES]; 2],
    delay_write_idx: usize,

    crossover_hpf: [BiquadState; 2],
    crossover_lpf: [BiquadState; 2],
    crossover_hpf_coeffs: BiquadCoeffs,
    crossover_lpf_coeffs: BiquadCoeffs,

    horn_angle: f32,
    drum_angle: f32,
    current_horn_speed: f32,
    current_drum_speed: f32,

    // Parameters
    target_mode: i32,
    target_horn_rate: f32,
    target_drum_rate: f32,
    target_drive: f32,
    target_spread: f32,
    target_mix: f32,
}

impl RustRotarySpeaker {
    pub fn new(sample_rate: f64) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate as f32 } else { 44100.0 };
        let mut max_delay = ((sr * 0.005) as usize).max(256);
        if max_delay > MAX_DELAY_SAMPLES {
            max_delay = MAX_DELAY_SAMPLES;
        }

        let crossover_hpf_coeffs = BiquadCoeffs::highpass(sr, 800.0, 0.707);
        let crossover_lpf_coeffs = BiquadCoeffs::lowpass(sr, 800.0, 0.707);

        Self {
            sample_rate: sr,
            max_delay_samples: max_delay,
            horn_delay_buffers: [[0.0f32; MAX_DELAY_SAMPLES]; 2],
            delay_write_idx: 0,
            crossover_hpf: [BiquadState::default(); 2],
            crossover_lpf: [BiquadState::default(); 2],
            crossover_hpf_coeffs,
            crossover_lpf_coeffs,
            horn_angle: 0.0,
            drum_angle: 0.0,
            current_horn_speed: 6.8,
            current_drum_speed: 5.8,

            target_mode: 1,
            target_horn_rate: 1.0,
            target_drum_rate: 1.0,
            target_drive: 1.5,
            target_spread: 0.85,
            target_mix: 1.0,
        }
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        let sr = if sample_rate > 0.0 { sample_rate as f32 } else { 44100.0 };
        self.sample_rate = sr;
        let mut max_delay = ((sr * 0.005) as usize).max(256);
        if max_delay > MAX_DELAY_SAMPLES {
            max_delay = MAX_DELAY_SAMPLES;
        }
        self.max_delay_samples = max_delay;

        self.crossover_hpf_coeffs = BiquadCoeffs::highpass(sr, 800.0, 0.707);
        self.crossover_lpf_coeffs = BiquadCoeffs::lowpass(sr, 800.0, 0.707);

        self.horn_angle = 0.0;
        self.drum_angle = 0.0;
        self.current_horn_speed = 6.8;
        self.current_drum_speed = 5.8;

        self.reset();
    }

    pub fn reset(&mut self) {
        for buf in &mut self.horn_delay_buffers {
            buf.fill(0.0);
        }
        self.delay_write_idx = 0;
        for f in &mut self.crossover_hpf {
            f.reset();
        }
        for f in &mut self.crossover_lpf {
            f.reset();
        }
        self.horn_angle = 0.0;
        self.drum_angle = 0.0;
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            PARAM_SPEED_MODE => self.target_mode = value.round() as i32,
            PARAM_HORN_SPEED => self.target_horn_rate = value.clamp(0.5, 1.5),
            PARAM_DRUM_SPEED => self.target_drum_rate = value.clamp(0.5, 1.5),
            PARAM_DRIVE => self.target_drive = value.clamp(1.0, 6.0),
            PARAM_SPREAD => self.target_spread = value.clamp(0.0, 1.0),
            PARAM_MIX => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            PARAM_SPEED_MODE => self.target_mode as f32,
            PARAM_HORN_SPEED => self.target_horn_rate,
            PARAM_DRUM_SPEED => self.target_drum_rate,
            PARAM_DRIVE => self.target_drive,
            PARAM_SPREAD => self.target_spread,
            PARAM_MIX => self.target_mix,
            _ => 0.0,
        }
    }

    #[inline(always)]
    fn read_interpolated(&self, ch: usize, delay_samples: f32) -> f32 {
        let cap_f = self.max_delay_samples as f32;
        let read_pos = (self.delay_write_idx as f32) - delay_samples;
        let mut wrapped_pos = read_pos % cap_f;
        if wrapped_pos < 0.0 {
            wrapped_pos += cap_f;
        }

        let idx0 = wrapped_pos as usize;
        let idx1 = if idx0 + 1 < self.max_delay_samples { idx0 + 1 } else { 0 };
        let frac = wrapped_pos - idx0 as f32;

        (1.0 - frac) * self.horn_delay_buffers[ch][idx0] + frac * self.horn_delay_buffers[ch][idx1]
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len().min(out_l.len());
        if num_samples == 0 || self.max_delay_samples == 0 {
            return;
        }

        let sample_rate = if self.sample_rate > 0.0 { self.sample_rate } else { 44100.0 };
        let dt = 1.0 / sample_rate;

        // Velocidad objetivo según modo
        let mut target_horn_hz = 0.0f32;
        let mut target_drum_hz = 0.0f32;
        if self.target_mode == 0 {
            // Slow (Chorale)
            target_horn_hz = 0.8 * self.target_horn_rate;
            target_drum_hz = 0.7 * self.target_drum_rate;
        } else if self.target_mode == 1 {
            // Fast (Tremolo)
            target_horn_hz = 6.8 * self.target_horn_rate;
            target_drum_hz = 5.8 * self.target_drum_rate;
        }

        // Inercia mecánica
        let horn_inertia_rate = dt * if self.target_mode == 1 { 2.5 } else { 0.9 };
        let drum_inertia_rate = dt * if self.target_mode == 1 { 0.9 } else { 0.35 };

        let drive = self.target_drive;
        let spread = self.target_spread;
        let mix = self.target_mix;
        const TWO_PI: f32 = 2.0 * PI;

        for s in 0..num_samples {
            let raw_in_l = in_l[s];
            let raw_in_r = if s < in_r.len() { in_r[s] } else { raw_in_l };

            // 1. Simulación de preamplificador de válvulas
            let driven_l = common::fast_tanh(raw_in_l * drive);
            let driven_r = common::fast_tanh(raw_in_r * drive);

            // 2. Crossover Linkwitz-Riley 800 Hz
            let horn_in_l = self.crossover_hpf[0].process(driven_l, &self.crossover_hpf_coeffs);
            let horn_in_r = self.crossover_hpf[1].process(driven_r, &self.crossover_hpf_coeffs);
            let drum_in_l = self.crossover_lpf[0].process(driven_l, &self.crossover_lpf_coeffs);
            let drum_in_r = self.crossover_lpf[1].process(driven_r, &self.crossover_lpf_coeffs);

            // Escribir en buffer de delay para Doppler del rotor de agudos
            self.horn_delay_buffers[0][self.delay_write_idx] = horn_in_l;
            self.horn_delay_buffers[1][self.delay_write_idx] = horn_in_r;

            // 3. Actualizar inercia de velocidad
            self.current_horn_speed += horn_inertia_rate * (target_horn_hz - self.current_horn_speed);
            self.current_drum_speed += drum_inertia_rate * (target_drum_hz - self.current_drum_speed);

            self.horn_angle += TWO_PI * self.current_horn_speed * dt;
            if self.horn_angle >= TWO_PI {
                self.horn_angle -= TWO_PI;
            }

            self.drum_angle -= TWO_PI * self.current_drum_speed * dt;
            if self.drum_angle <= -TWO_PI {
                self.drum_angle += TWO_PI;
            }

            // 4. Modulación Doppler e intensidad AM del Horn
            let horn_sin = self.horn_angle.sin();
            let horn_cos = self.horn_angle.cos();

            let doppler_samples_l = (0.001 + 0.0003 * horn_sin * spread) * sample_rate;
            let doppler_samples_r = (0.001 - 0.0003 * horn_sin * spread) * sample_rate;

            let horn_audio_l = self.read_interpolated(0, doppler_samples_l);
            let horn_audio_r = self.read_interpolated(1, doppler_samples_r);

            let horn_am_l = 0.65 + 0.35 * horn_cos;
            let horn_am_r = 0.65 - 0.35 * horn_cos;

            let horn_out_l = horn_audio_l * horn_am_l;
            let horn_out_r = horn_audio_r * horn_am_r;

            // 5. Modulación AM del Drum (Graves)
            let drum_sin = self.drum_angle.sin();
            let drum_am_l = 0.75 + 0.25 * drum_sin * spread;
            let drum_am_r = 0.75 - 0.25 * drum_sin * spread;

            let drum_out_l = drum_in_l * drum_am_l;
            let drum_out_r = drum_in_r * drum_am_r;

            self.delay_write_idx = (self.delay_write_idx + 1) % self.max_delay_samples;

            // 6. Suma de rotores y mezcla Dry/Wet
            let wet_l = (horn_out_l + drum_out_l) * 1.15;
            let wet_r = (horn_out_r + drum_out_r) * 1.15;

            out_l[s] = common::denormal_guard((1.0 - mix) * raw_in_l + mix * wet_l);
            if s < out_r.len() {
                out_r[s] = common::denormal_guard((1.0 - mix) * raw_in_r + mix * wet_r);
            }
        }
    }
}
