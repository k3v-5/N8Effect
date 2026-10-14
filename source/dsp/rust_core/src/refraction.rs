//! Polyphonic Refraction Dispersion Engine in Rust (DSP #51)
//! Multi-voice prism dispersion with Haas staggering, constant-power stereo detune,
//! resonant harmonic bandpass bank, and 1/sqrt(N) energy conservation (Rules 5, 8, 9, 14, 18, 45, 46, 47).

use crate::common::{fast_sin, fast_tanh, BiquadCoeffs, BiquadState, DelayBuffer};
use std::f32::consts::PI;

pub const MAX_VOICES: usize = 8;

#[derive(Clone, Copy)]
struct VoiceConfig {
    base_delay_samples: f32,
    mod_depth_samples: f32,
    lfo_phase_inc: f32,
    pan_l: f32,
    pan_r: f32,
}

impl Default for VoiceConfig {
    fn default() -> Self {
        Self {
            base_delay_samples: 0.0,
            mod_depth_samples: 0.0,
            lfo_phase_inc: 0.0,
            pan_l: 0.7071,
            pan_r: 0.7071,
        }
    }
}

pub struct RustRefraction {
    sample_rate: f32,

    // Delay lines for 8 stereo voices (120ms max delay)
    voice_delays_l: [DelayBuffer; MAX_VOICES],
    voice_delays_r: [DelayBuffer; MAX_VOICES],

    // Harmonic dispersion resonant bandpass filters
    filters_l: [BiquadState; MAX_VOICES],
    filters_r: [BiquadState; MAX_VOICES],
    filter_coeffs: [BiquadCoeffs; MAX_VOICES],

    // Desynchronized quadrature LFO phases
    lfo_phases: [f32; MAX_VOICES],

    // Parameters
    param_voices: f32,       // 1: VoiceCount (2.0 .. 8.0)
    param_detune: f32,       // 2: Detune (0.0 .. 100.0 cents)
    param_delay_spread: f32, // 3: DelaySpreadMs (0.0 .. 50.0 ms)
    param_stereo_spread: f32,// 4: StereoSpread (0.0 .. 1.0)
    param_harmonic_disp: f32,// 5: HarmonicDisp (0.0 .. 2.0)
    param_resonance: f32,    // 6: Resonance (0.1 .. 10.0 Q)
    param_mix: f32,          // 7: DryWet (0.0 .. 1.0)
}

impl RustRefraction {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        // Capacity: 150ms max delay line at host sample rate (up to 192k => 28800 -> 32768)
        let delay_cap = ((sr * 0.15) as usize).next_power_of_two().max(8192);

        let mut instance = Self {
            sample_rate: sr,
            voice_delays_l: [
                DelayBuffer::new(delay_cap),
                DelayBuffer::new(delay_cap),
                DelayBuffer::new(delay_cap),
                DelayBuffer::new(delay_cap),
                DelayBuffer::new(delay_cap),
                DelayBuffer::new(delay_cap),
                DelayBuffer::new(delay_cap),
                DelayBuffer::new(delay_cap),
            ],
            voice_delays_r: [
                DelayBuffer::new(delay_cap),
                DelayBuffer::new(delay_cap),
                DelayBuffer::new(delay_cap),
                DelayBuffer::new(delay_cap),
                DelayBuffer::new(delay_cap),
                DelayBuffer::new(delay_cap),
                DelayBuffer::new(delay_cap),
                DelayBuffer::new(delay_cap),
            ],
            filters_l: [BiquadState::default(); MAX_VOICES],
            filters_r: [BiquadState::default(); MAX_VOICES],
            filter_coeffs: [BiquadCoeffs::default(); MAX_VOICES],
            lfo_phases: [0.0; MAX_VOICES],
            param_voices: 4.0,
            param_detune: 18.0,
            param_delay_spread: 15.0,
            param_stereo_spread: 0.85,
            param_harmonic_disp: 0.5,
            param_resonance: 1.2,
            param_mix: 0.5,
        };

        for v in 0..MAX_VOICES {
            instance.lfo_phases[v] = (v as f32) * (2.0 * PI / (MAX_VOICES as f32));
        }

        instance.update_filters();
        instance
    }

    #[inline(always)]
    pub fn reset(&mut self) {
        for v in 0..MAX_VOICES {
            self.voice_delays_l[v].reset();
            self.voice_delays_r[v].reset();
            self.filters_l[v].reset();
            self.filters_r[v].reset();
            self.lfo_phases[v] = (v as f32) * (2.0 * PI / (MAX_VOICES as f32));
        }
    }

    pub fn set_sample_rate(&mut self, sample_rate: f32) {
        if sample_rate > 0.0 && (sample_rate - self.sample_rate).abs() > 0.01 {
            self.sample_rate = sample_rate;
            self.update_filters();
        }
    }

    #[inline(always)]
    pub fn update_filters(&mut self) {
        let sr = self.sample_rate;
        let q = (0.707 + self.param_resonance * 0.75).clamp(0.5, 12.0);

        for v in 0..MAX_VOICES {
            let base_freq = 350.0 * (1.0 + (v as f32) * self.param_harmonic_disp * 0.75);
            let cutoff = base_freq.clamp(60.0, sr * 0.44);
            self.filter_coeffs[v] = BiquadCoeffs::bandpass(sr, cutoff, q);
        }
    }

    pub fn set_parameter(&mut self, id: u32, value: f32) {
        match id {
            1 => self.param_voices = value.clamp(2.0, 8.0),
            2 => self.param_detune = value.clamp(0.0, 100.0),
            3 => self.param_delay_spread = value.clamp(0.0, 50.0),
            4 => self.param_stereo_spread = value.clamp(0.0, 1.0),
            5 => {
                self.param_harmonic_disp = value.clamp(0.0, 2.0);
                self.update_filters();
            }
            6 => {
                self.param_resonance = value.clamp(0.1, 10.0);
                self.update_filters();
            }
            7 => self.param_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, id: u32) -> f32 {
        match id {
            1 => self.param_voices,
            2 => self.param_detune,
            3 => self.param_delay_spread,
            4 => self.param_stereo_spread,
            5 => self.param_harmonic_disp,
            6 => self.param_resonance,
            7 => self.param_mix,
            _ => 0.0,
        }
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

        let sr = self.sample_rate;
        let mix = self.param_mix;
        let num_voices = (self.param_voices.round() as usize).clamp(2, MAX_VOICES);
        let norm_energy = 1.0 / (num_voices as f32).sqrt();

        let mut configs = [VoiceConfig::default(); MAX_VOICES];
        for v in 0..num_voices {
            let u = if num_voices > 1 {
                (2.0 * (v as f32) - ((num_voices - 1) as f32)) / ((num_voices - 1) as f32)
            } else {
                0.0
            };

            let base_delay_ms = 2.0 + self.param_delay_spread * ((v as f32) / ((num_voices - 1) as f32));
            configs[v].base_delay_samples = (base_delay_ms * 0.001) * sr;
            configs[v].mod_depth_samples = (self.param_detune * 0.01) * (0.00075 * sr);

            let lfo_freq_hz = 0.3 * (1.0 + 0.18 * u);
            configs[v].lfo_phase_inc = 2.0 * PI * lfo_freq_hz / sr;

            let pan_angle = (PI * 0.25) * (1.0 + u * self.param_stereo_spread);
            configs[v].pan_l = pan_angle.cos();
            configs[v].pan_r = pan_angle.sin();
        }

        for s in 0..n_samples {
            let sample_l = in_l[s];
            let sample_r = if s < in_r.len() { in_r[s] } else { sample_l };

            let mut wet_accum_l = 0.0f32;
            let mut wet_accum_r = 0.0f32;

            for v in 0..num_voices {
                self.voice_delays_l[v].write(sample_l);
                self.voice_delays_r[v].write(sample_r);

                let mod_delay = configs[v].base_delay_samples
                    + configs[v].mod_depth_samples * fast_sin(self.lfo_phases[v]);
                let safe_delay = mod_delay.max(2.0);

                let read_l = self.voice_delays_l[v].read_cubic(safe_delay);
                let read_r = self.voice_delays_r[v].read_cubic(safe_delay);

                let filtered_l = self.filters_l[v].process(read_l, &self.filter_coeffs[v]);
                let filtered_r = self.filters_r[v].process(read_r, &self.filter_coeffs[v]);

                let voice_l = read_l * 0.65 + filtered_l * 0.65;
                let voice_r = read_r * 0.65 + filtered_r * 0.65;

                let voice_panned_l = voice_l * configs[v].pan_l;
                let voice_panned_r = voice_r * configs[v].pan_r;

                wet_accum_l += voice_panned_l * norm_energy;
                wet_accum_r += voice_panned_r * norm_energy;

                self.lfo_phases[v] += configs[v].lfo_phase_inc;
                if self.lfo_phases[v] >= 2.0 * PI {
                    self.lfo_phases[v] -= 2.0 * PI;
                }
            }

            let wet_final_l = fast_tanh(wet_accum_l);
            let wet_final_r = fast_tanh(wet_accum_r);

            out_l[s] = sample_l * (1.0 - mix) + wet_final_l * mix;
            if s < out_r.len() {
                out_r[s] = sample_r * (1.0 - mix) + wet_final_r * mix;
            }
        }
    }
}
