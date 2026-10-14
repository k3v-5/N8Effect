//! Tape Stop & Spin-Up DSP Kernel (DSP #36)
//! Rules 5, 8, 9, 13, 16, 32, 34, 35, 46, 47: Zero allocations in audio thread,
//! Hermite cubic tape playback interpolation, variable inertia, progressive LPF,
//! analog wobble/wow, and denormal protection.

use std::f32::consts::PI;
use crate::common::{denormal_guard, BiquadCoeffs, BiquadState};

pub struct RustTapeStop {
    sample_rate: f32,
    max_delay_samples: usize,
    ring_buffers: [Vec<f32>; 2],
    write_index: usize,
    read_index: f64,

    current_speed: f32,
    target_speed: f32,
    target_trigger: bool,
    target_stop_time: f32,
    target_spin_up_time: f32,
    target_curve: i32,
    target_inertia: f32,
    target_mix: f32,

    wobble_phase: f32,
    lpf_coeffs: [BiquadCoeffs; 2],
    lpf_states: [BiquadState; 2],
}

impl RustTapeStop {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let max_samples = (sr * 4.0).max(1024.0) as usize;
        let mut stop = Self {
            sample_rate: sr,
            max_delay_samples: max_samples,
            ring_buffers: [vec![0.0f32; max_samples], vec![0.0f32; max_samples]],
            write_index: 0,
            read_index: 0.0,
            current_speed: 1.0,
            target_speed: 1.0,
            target_trigger: false,
            target_stop_time: 0.6,
            target_spin_up_time: 0.3,
            target_curve: 1,
            target_inertia: 0.35,
            target_mix: 1.0,
            wobble_phase: 0.0,
            lpf_coeffs: [BiquadCoeffs::default(); 2],
            lpf_states: [BiquadState::default(); 2],
        };
        stop.update_filter(18000.0);
        stop
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.sample_rate = sr;
        let max_samples = (sr * 4.0).max(1024.0) as usize;
        if self.max_delay_samples != max_samples {
            self.max_delay_samples = max_samples;
            self.ring_buffers[0] = vec![0.0f32; max_samples];
            self.ring_buffers[1] = vec![0.0f32; max_samples];
        } else {
            self.ring_buffers[0].fill(0.0);
            self.ring_buffers[1].fill(0.0);
        }
        self.reset();
    }

    pub fn reset(&mut self) {
        self.ring_buffers[0].fill(0.0);
        self.ring_buffers[1].fill(0.0);
        self.write_index = 0;
        self.read_index = 0.0;
        self.current_speed = 1.0;
        self.target_speed = if self.target_trigger { 0.0 } else { 1.0 };
        self.wobble_phase = 0.0;
        self.lpf_states[0].reset();
        self.lpf_states[1].reset();
        self.update_filter(18000.0);
    }

    #[inline]
    fn update_filter(&mut self, cutoff: f32) {
        let sr = self.sample_rate;
        let c = cutoff.clamp(20.0, sr * 0.49);
        let coeff = BiquadCoeffs::lowpass(sr, c, 0.707);
        self.lpf_coeffs[0] = coeff;
        self.lpf_coeffs[1] = coeff;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => { // Trigger
                self.target_trigger = value >= 0.5;
                self.target_speed = if self.target_trigger { 0.0 } else { 1.0 };
            }
            2 => self.target_stop_time = value.clamp(0.05, 4.0),
            3 => self.target_spin_up_time = value.clamp(0.05, 2.0),
            4 => self.target_curve = value.round() as i32,
            5 => self.target_inertia = value.clamp(0.0, 1.0),
            6 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => if self.target_trigger { 1.0 } else { 0.0 },
            2 => self.target_stop_time,
            3 => self.target_spin_up_time,
            4 => self.target_curve as f32,
            5 => self.target_inertia,
            6 => self.target_mix,
            _ => 0.0,
        }
    }

    #[inline]
    fn read_hermite(&self, ch: usize, pos: f64) -> f32 {
        let max_d = self.max_delay_samples as f64;
        let mut wrapped_pos = pos % max_d;
        if wrapped_pos < 0.0 {
            wrapped_pos += max_d;
        }

        let i1 = wrapped_pos as usize;
        let i0 = (i1 + self.max_delay_samples - 1) % self.max_delay_samples;
        let i2 = (i1 + 1) % self.max_delay_samples;
        let i3 = (i1 + 2) % self.max_delay_samples;

        let frac = (wrapped_pos - i1 as f64) as f32;

        let buf = &self.ring_buffers[ch];
        let y0 = buf[i0];
        let y1 = buf[i1];
        let y2 = buf[i2];
        let y3 = buf[i3];

        let c0 = y1;
        let c1 = 0.5 * (y2 - y0);
        let c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
        let c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);

        ((c3 * frac + c2) * frac + c1) * frac + c0
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
        if num_samples == 0 || self.max_delay_samples == 0 {
            return;
        }

        let sr = self.sample_rate;
        let stop_rate_delta = 1.0 / (self.target_stop_time.max(0.05) * sr);
        let spin_rate_delta = 1.0 / (self.target_spin_up_time.max(0.05) * sr);
        let max_samples = self.max_delay_samples;

        for s in 0..num_samples {
            let raw_in_l = in_l[s];
            let raw_in_r = if s < in_r.len() { in_r[s] } else { raw_in_l };

            // 1. Escribir audio en el buffer circular
            self.ring_buffers[0][self.write_index] = raw_in_l;
            self.ring_buffers[1][self.write_index] = raw_in_r;

            // 2. Transición de velocidad (Frenado / Arranque)
            if self.target_trigger {
                if self.current_speed > 0.0001 {
                    if self.target_curve == 1 { // Exponencial
                        self.current_speed = (self.current_speed - stop_rate_delta * (self.current_speed + 0.1)).max(0.0);
                    } else if self.target_curve == 2 { // Curva S
                        let t = 1.0 - self.current_speed;
                        let factor = 3.0 * t * (1.0 - t) + 0.2;
                        self.current_speed = (self.current_speed - stop_rate_delta * factor).max(0.0);
                    } else { // Lineal
                        self.current_speed = (self.current_speed - stop_rate_delta).max(0.0);
                    }
                } else {
                    self.current_speed = 0.0;
                }
            } else {
                if self.current_speed < 0.9999 {
                    self.current_speed = (self.current_speed + spin_rate_delta).min(1.0);
                } else {
                    self.current_speed = 1.0;
                }
            }

            // 3. Fluctuación analógica (Wow/Wobble al frenar)
            let mut speed_with_wobble = self.current_speed;
            if self.current_speed > 0.01 && self.current_speed < 0.95 && self.target_inertia > 0.01 {
                self.wobble_phase += (4.0 * self.current_speed) / sr;
                if self.wobble_phase >= 1.0 {
                    self.wobble_phase -= 1.0;
                }
                let wobble = (self.wobble_phase * 2.0 * PI).sin() * 0.08 * self.target_inertia * (1.0 - self.current_speed);
                speed_with_wobble = (self.current_speed + wobble).max(0.0);
            }

            // 4. Lectura con interpolación cúbica Hermite de 4 puntos
            let (wet_l, wet_r) = if self.current_speed > 0.0005 {
                let raw_l = self.read_hermite(0, self.read_index);
                let raw_r = self.read_hermite(1, self.read_index);

                // Filtro pasobajos dinámico que atenúa frecuencias altas al frenar
                let cutoff = (18000.0 * (self.current_speed * self.current_speed)).clamp(250.0, 18000.0);
                self.update_filter(cutoff);
                let fl = self.lpf_states[0].process(raw_l, &self.lpf_coeffs[0]);
                let fr = self.lpf_states[1].process(raw_r, &self.lpf_coeffs[1]);

                // Avanzar puntero de lectura proporcional a la velocidad
                self.read_index += speed_with_wobble as f64;
                if self.read_index >= max_samples as f64 {
                    self.read_index -= max_samples as f64;
                }
                (fl, fr)
            } else {
                (0.0, 0.0)
            };

            self.write_index = (self.write_index + 1) % max_samples;

            let mix = self.target_mix;
            out_l[s] = denormal_guard((1.0 - mix) * raw_in_l + mix * wet_l);
            if s < out_r.len() {
                out_r[s] = denormal_guard((1.0 - mix) * raw_in_r + mix * wet_r);
            }
        }
    }
}
