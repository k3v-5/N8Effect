//! Reverse Reverb & Swell Bloom DSP Kernel (DSP #39)
//! Rules 5, 8, 9, 13, 14, 16, 18, 34, 46, 47: Zero allocations in audio thread,
//! preallocated bounded circular buffer, reverse mirror playback,
//! sinusoidal pre-swell bloom with cosine anti-pop taper, 4-stage stereo prime allpass diffusers,
//! biquad lowpass damping filter, and soft tanh feedback loop.

use crate::common::{denormal_guard, fast_tanh, BiquadCoeffs, BiquadState};
use std::f32::consts::{FRAC_PI_2, PI};

pub struct RustReverseReverb {
    sample_rate: f32,
    max_samples: usize,
    ring_l: Box<[f32]>,
    ring_r: Box<[f32]>,
    write_idx: usize,
    grain_phase: usize,

    diffuser_delays: [usize; 4],
    diffuser_l: [Box<[f32]>; 4],
    diffuser_r: [Box<[f32]>; 4],
    diffuser_indices_l: [usize; 4],
    diffuser_indices_r: [usize; 4],

    damping_coeffs: BiquadCoeffs,
    damping_state_l: BiquadState,
    damping_state_r: BiquadState,

    target_swell_time: f32,
    target_diffusion: f32,
    target_damping: f32,
    target_feedback: f32,
    target_mix: f32,
}

impl RustReverseReverb {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let max_samples = ((sr * 2.0) as usize).max(1024);

        let delays = [113, 163, 241, 353];
        let mut rev = Self {
            sample_rate: sr,
            max_samples,
            ring_l: vec![0.0f32; max_samples].into_boxed_slice(),
            ring_r: vec![0.0f32; max_samples].into_boxed_slice(),
            write_idx: 0,
            grain_phase: 0,
            diffuser_delays: delays,
            diffuser_l: [
                vec![0.0f32; delays[0] + 16].into_boxed_slice(),
                vec![0.0f32; delays[1] + 16].into_boxed_slice(),
                vec![0.0f32; delays[2] + 16].into_boxed_slice(),
                vec![0.0f32; delays[3] + 16].into_boxed_slice(),
            ],
            diffuser_r: [
                vec![0.0f32; delays[0] + 16].into_boxed_slice(),
                vec![0.0f32; delays[1] + 16].into_boxed_slice(),
                vec![0.0f32; delays[2] + 16].into_boxed_slice(),
                vec![0.0f32; delays[3] + 16].into_boxed_slice(),
            ],
            diffuser_indices_l: [0; 4],
            diffuser_indices_r: [0; 4],
            damping_coeffs: BiquadCoeffs::default(),
            damping_state_l: BiquadState::default(),
            damping_state_r: BiquadState::default(),
            target_swell_time: 0.6,
            target_diffusion: 0.7,
            target_damping: 8000.0,
            target_feedback: 0.35,
            target_mix: 1.0,
        };
        rev.update_damping_filter();
        rev
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.sample_rate = sr;
        let new_max = ((sr * 2.0) as usize).max(1024);
        if new_max != self.max_samples {
            self.max_samples = new_max;
            self.ring_l = vec![0.0f32; new_max].into_boxed_slice();
            self.ring_r = vec![0.0f32; new_max].into_boxed_slice();
        }
        self.reset();
        self.update_damping_filter();
    }

    pub fn reset(&mut self) {
        self.ring_l.fill(0.0);
        self.ring_r.fill(0.0);
        self.write_idx = 0;
        self.grain_phase = 0;

        for stage in 0..4 {
            self.diffuser_l[stage].fill(0.0);
            self.diffuser_r[stage].fill(0.0);
            self.diffuser_indices_l[stage] = 0;
            self.diffuser_indices_r[stage] = 0;
        }

        self.damping_state_l.reset();
        self.damping_state_r.reset();
    }

    #[inline]
    fn update_damping_filter(&mut self) {
        self.damping_coeffs = BiquadCoeffs::lowpass(self.sample_rate, self.target_damping, 0.707);
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.target_swell_time = value.clamp(0.1, 2.0),
            2 => self.target_diffusion = value.clamp(0.0, 1.0),
            3 => {
                self.target_damping = value.clamp(500.0, 18000.0);
                self.update_damping_filter();
            }
            4 => self.target_feedback = value.clamp(0.0, 0.85),
            5 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.target_swell_time,
            2 => self.target_diffusion,
            3 => self.target_damping,
            4 => self.target_feedback,
            5 => self.target_mix,
            _ => 0.0,
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
        if num_samples == 0 || self.max_samples == 0 {
            return;
        }

        let sr = self.sample_rate;
        let swell_window_samples = ((self.target_swell_time * sr) as usize).clamp(256, self.max_samples - 16);
        let diffusion = self.target_diffusion * 0.7;
        let feedback = self.target_feedback;
        let mix = self.target_mix;
        let max_samples = self.max_samples;

        for s in 0..num_samples {
            let raw_in_l = in_l[s];
            let raw_in_r = if s < in_r.len() { in_r[s] } else { raw_in_l };

            // 1. Grabar en el buffer circular
            self.ring_l[self.write_idx] = raw_in_l;
            self.ring_r[self.write_idx] = raw_in_r;

            // 2. Lectura en espejo inverso respecto a la posición actual
            let progress = self.grain_phase as f32 / swell_window_samples as f32;

            // Envolvente de hinchamiento inversa (crece de 0 a 1)
            let mut swell_env = (progress * FRAC_PI_2).sin();

            // Taper suave anti-pop en el último 5% de la ventana
            if progress > 0.95 {
                let fade_progress = (progress - 0.95) / 0.05;
                let taper = 0.5 * (1.0 + (fade_progress * PI).cos());
                swell_env *= taper;
            }

            let reverse_offset = swell_window_samples - self.grain_phase;
            let mut read_idx = self.write_idx + max_samples - reverse_offset;
            if read_idx >= max_samples {
                read_idx -= max_samples;
            }

            let mut rev_l = self.ring_l[read_idx] * swell_env;
            let mut rev_r = self.ring_r[read_idx] * swell_env;

            // 3. Difusores pasa-todo en cascada
            for stage in 0..4 {
                let d_len = self.diffuser_delays[stage];

                // Ch L
                let idx_l = self.diffuser_indices_l[stage];
                let del_l = self.diffuser_l[stage][idx_l];
                let out_diff_l = -rev_l * diffusion + del_l;
                self.diffuser_l[stage][idx_l] = rev_l + del_l * diffusion;
                self.diffuser_indices_l[stage] = if idx_l + 1 >= d_len { 0 } else { idx_l + 1 };
                rev_l = out_diff_l;

                // Ch R
                let idx_r = self.diffuser_indices_r[stage];
                let del_r = self.diffuser_r[stage][idx_r];
                let out_diff_r = -rev_r * diffusion + del_r;
                self.diffuser_r[stage][idx_r] = rev_r + del_r * diffusion;
                self.diffuser_indices_r[stage] = if idx_r + 1 >= d_len { 0 } else { idx_r + 1 };
                rev_r = out_diff_r;
            }

            // 4. Filtro de absorción de agudos (Damping)
            rev_l = self.damping_state_l.process(rev_l, &self.damping_coeffs);
            rev_r = self.damping_state_r.process(rev_r, &self.damping_coeffs);

            // Realimentar al buffer para colas extendidas con saturación suave para prevenir blowup
            let safe_rev_l = fast_tanh(rev_l * feedback);
            let safe_rev_r = fast_tanh(rev_r * feedback);
            self.ring_l[self.write_idx] += safe_rev_l;
            self.ring_r[self.write_idx] += safe_rev_r;

            self.write_idx = if self.write_idx + 1 >= max_samples { 0 } else { self.write_idx + 1 };
            self.grain_phase = if self.grain_phase + 1 >= swell_window_samples { 0 } else { self.grain_phase + 1 };

            // 5. Mezcla Dry/Wet
            let wet_l = rev_l * 1.5;
            let wet_r = rev_r * 1.5;

            out_l[s] = denormal_guard((1.0 - mix) * raw_in_l + mix * wet_l);
            if s < out_r.len() {
                out_r[s] = denormal_guard((1.0 - mix) * raw_in_r + mix * wet_r);
            }
        }
    }
}
