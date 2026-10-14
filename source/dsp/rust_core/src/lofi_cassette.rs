//! Lo-Fi Cassette & Analog Tape Emulator DSP Kernel (DSP #35)
//! Rules 5, 8, 9, 13, 16, 32, 34, 46, 47: Zero allocations in audio thread,
//! stochastic wow & flutter, magnetic head saturation (cubic polynomial),
//! tape age HF loss, sporadic dropout generator, and denormal protection.

use std::f32::consts::PI;
use crate::common::denormal_guard;

pub struct RustLoFiCassette {
    sample_rate: f32,
    max_delay_samples: usize,
    delay_buffers: [Vec<f32>; 2],
    write_index: usize,

    wow_phase: f32,
    flutter_phase: f32,
    head_filter_l: f32,
    head_filter_r: f32,
    dropout_gain: f32,
    dropout_timer: i32,
    prng_seed: u32,

    target_wow: f32,
    target_flutter: f32,
    target_age: f32,
    target_saturation: f32,
    target_dropouts: f32,
    target_mix: f32,
}

impl RustLoFiCassette {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 48000.0 };
        let max_samples = ((sr * 0.045) as usize).max(1024);
        let mut cassette = Self {
            sample_rate: sr,
            max_delay_samples: max_samples,
            delay_buffers: [vec![0.0f32; max_samples], vec![0.0f32; max_samples]],
            write_index: 0,
            wow_phase: 0.0,
            flutter_phase: 0.0,
            head_filter_l: 0.0,
            head_filter_r: 0.0,
            dropout_gain: 1.0,
            dropout_timer: 0,
            prng_seed: 0x12345678,
            target_wow: 0.2,
            target_flutter: 0.15,
            target_age: 0.3,
            target_saturation: 0.25,
            target_dropouts: 0.05,
            target_mix: 1.0,
        };
        cassette.reset();
        cassette
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 48000.0 };
        self.sample_rate = sr;
        let max_samples = ((sr * 0.045) as usize).max(1024);
        if self.max_delay_samples != max_samples {
            self.max_delay_samples = max_samples;
            self.delay_buffers[0] = vec![0.0f32; max_samples];
            self.delay_buffers[1] = vec![0.0f32; max_samples];
        } else {
            self.delay_buffers[0].fill(0.0);
            self.delay_buffers[1].fill(0.0);
        }
        self.reset();
    }

    pub fn reset(&mut self) {
        self.delay_buffers[0].fill(0.0);
        self.delay_buffers[1].fill(0.0);
        self.write_index = 0;
        self.wow_phase = 0.0;
        self.flutter_phase = 0.0;
        self.head_filter_l = 0.0;
        self.head_filter_r = 0.0;
        self.dropout_gain = 1.0;
        self.dropout_timer = 0;
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_wow = value.clamp(0.0, 1.0),
            2 => self.target_flutter = value.clamp(0.0, 1.0),
            3 => self.target_age = value.clamp(0.0, 1.0),
            4 => self.target_saturation = value.clamp(0.0, 1.0),
            5 => self.target_dropouts = value.clamp(0.0, 1.0),
            6 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_wow,
            2 => self.target_flutter,
            3 => self.target_age,
            4 => self.target_saturation,
            5 => self.target_dropouts,
            6 => self.target_mix,
            _ => 0.0,
        }
    }

    #[inline]
    fn read_hermite(&self, ch: usize, delay_samples: f32) -> f32 {
        let max_d = self.max_delay_samples as f32;
        let mut r_pos = self.write_index as f32 - delay_samples;
        while r_pos < 0.0 {
            r_pos += max_d;
        }

        let i1 = (r_pos as usize) % self.max_delay_samples;
        let i0 = (i1 + self.max_delay_samples - 1) % self.max_delay_samples;
        let i2 = (i1 + 1) % self.max_delay_samples;
        let i3 = (i1 + 2) % self.max_delay_samples;

        let frac = r_pos - (r_pos as usize as f32);

        let buf = &self.delay_buffers[ch];
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
        let wow_amount = self.target_wow;
        let flutter_amount = self.target_flutter;
        let age = self.target_age;
        let sat_drive = 1.0 + self.target_saturation * 3.5;
        let drop_chance = self.target_dropouts * 0.0001;
        let mix = self.target_mix;

        let wow_inc = (2.0 * PI * 0.85) / sr;
        let flutter_inc = (2.0 * PI * 11.3) / sr;

        let head_cutoff = 18000.0 - age * 14500.0;
        let alpha = ((2.0 * PI * head_cutoff) / sr).clamp(0.01, 0.95);
        let base_delay = 150.0f32;
        let max_samples = self.max_delay_samples;
        let two_pi = 2.0 * PI;

        for s in 0..num_samples {
            let s_l = in_l[s];
            let s_r = if s < in_r.len() { in_r[s] } else { s_l };

            // 1. Guardar en buffer circular
            self.delay_buffers[0][self.write_index] = s_l;
            self.delay_buffers[1][self.write_index] = s_r;

            // 2. Modulación de tiempo Wow & Flutter
            self.wow_phase += wow_inc;
            if self.wow_phase > two_pi {
                self.wow_phase -= two_pi;
            }

            self.flutter_phase += flutter_inc;
            if self.flutter_phase > two_pi {
                self.flutter_phase -= two_pi;
            }

            let wow_mod = self.wow_phase.sin() * (wow_amount * 18.0);
            let flutter_mod = self.flutter_phase.sin() * (flutter_amount * 5.0);
            let total_delay = base_delay + wow_mod + flutter_mod;

            // 3. Lectura con interpolación cúbica Hermite (anti-aliasing de pitch)
            let proc_l = self.read_hermite(0, total_delay);
            let proc_r = self.read_hermite(1, total_delay + 1.2);

            self.write_index = (self.write_index + 1) % max_samples;

            // 4. Saturación suave de cinta magnética: x - x^3 / 3
            let mut sat_l = (proc_l * sat_drive).clamp(-1.5, 1.5);
            sat_l = sat_l - (sat_l * sat_l * sat_l) * 0.3333333;

            let mut sat_r = (proc_r * sat_drive).clamp(-1.5, 1.5);
            sat_r = sat_r - (sat_r * sat_r * sat_r) * 0.3333333;

            // 5. Filtro de cabezal de cinta (atenuación HF analógica)
            self.head_filter_l += alpha * (sat_l - self.head_filter_l);
            self.head_filter_r += alpha * (sat_r - self.head_filter_r);

            // 6. Generador de dropouts esporádicos
            if self.dropout_timer > 0 {
                self.dropout_timer -= 1;
                self.dropout_gain += 0.005 * (0.3 - self.dropout_gain);
            } else {
                self.dropout_gain += 0.005 * (1.0 - self.dropout_gain);
                self.prng_seed = self.prng_seed.wrapping_mul(1664525).wrapping_add(1013904223);
                let rnd = (self.prng_seed as f32) * 2.3283064e-10;
                if rnd < drop_chance {
                    self.dropout_timer = (sr * 0.04) as i32;
                }
            }

            let final_l = self.head_filter_l * self.dropout_gain;
            let final_r = self.head_filter_r * self.dropout_gain;

            out_l[s] = denormal_guard(s_l * (1.0 - mix) + final_l * mix);
            if s < out_r.len() {
                out_r[s] = denormal_guard(s_r * (1.0 - mix) + final_r * mix);
            }
        }
    }
}
