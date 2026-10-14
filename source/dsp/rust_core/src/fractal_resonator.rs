//! Fractal Resonator / Golden-Ratio Comb Matrix (Rules 5, 8, 9, 12, 14, 34, 46, 47)
//! 6-line coupled physical resonator with delay lengths tuned to Golden-Ratio powers (phi^k).
//! Interconnected via a unitary Householder scattering feedback matrix with lowpass damping,
//! producing shimmering metallic plates, microtonal glass bells, and infinite acoustic depth.

use crate::common::{clamp, lerp};
use crate::optimization::fast_pade_tanh;

const NUM_LINES: usize = 6;
const MAX_DELAY_PER_LINE: usize = 8192;
const PHI: f32 = 1.61803398875;

struct ResonatorLine {
    buffer: Vec<f32>,
    write_pos: usize,
    damp_state: f32,
}

impl ResonatorLine {
    fn new() -> Self {
        Self {
            buffer: vec![0.0; MAX_DELAY_PER_LINE],
            write_pos: 0,
            damp_state: 0.0,
        }
    }

    #[inline(always)]
    fn reset(&mut self) {
        self.buffer.fill(0.0);
        self.write_pos = 0;
        self.damp_state = 0.0;
    }

    #[inline(always)]
    fn read_frac(&self, delay_samples: f32) -> f32 {
        let max_len = self.buffer.len();
        let read_pos = (self.write_pos as f32 + max_len as f32 - delay_samples) % (max_len as f32);
        let idx0 = read_pos.floor() as usize % max_len;
        let idx1 = (idx0 + 1) % max_len;
        let frac = read_pos - read_pos.floor();
        lerp(self.buffer[idx0], self.buffer[idx1], frac)
    }

    #[inline(always)]
    fn write_sample(&mut self, sample: f32) {
        self.buffer[self.write_pos] = sample;
        self.write_pos = (self.write_pos + 1) % self.buffer.len();
    }
}

pub struct RustFractalResonator {
    sample_rate: f32,
    lines: [ResonatorLine; NUM_LINES],

    // Parameters
    base_freq: f32,      // 20.0 to 4000.0 Hz
    fractal_ratio: f32,  // 0.2 to 2.0 (scaling factor of golden-ratio intervals)
    decay: f32,          // 0.0 to 0.99 (feedback sustain)
    damping_hz: f32,     // 500.0 to 20000.0 Hz (high frequency damping)
    stereo_width: f32,   // 0.0 to 1.0 (spatial distribution of lines)
    mix: f32,            // 0.0 to 1.0

    // Smoothed parameters
    smoothed_freq: f32,
    smoothed_decay: f32,
    smoothed_width: f32,
    smoothed_mix: f32,
}

impl RustFractalResonator {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            lines: [
                ResonatorLine::new(),
                ResonatorLine::new(),
                ResonatorLine::new(),
                ResonatorLine::new(),
                ResonatorLine::new(),
                ResonatorLine::new(),
            ],
            base_freq: 261.63, // Middle C (C4)
            fractal_ratio: 0.618,
            decay: 0.88,
            damping_hz: 6500.0,
            stereo_width: 0.85,
            mix: 0.5,
            smoothed_freq: 261.63,
            smoothed_decay: 0.88,
            smoothed_width: 0.85,
            smoothed_mix: 0.5,
        }
    }

    pub fn reset(&mut self) {
        for l in &mut self.lines {
            l.reset();
        }
        self.smoothed_freq = self.base_freq;
        self.smoothed_decay = self.decay;
        self.smoothed_width = self.stereo_width;
        self.smoothed_mix = self.mix;
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn set_base_freq(&mut self, freq: f32) {
        self.base_freq = clamp(freq, 20.0, 4000.0);
    }

    pub fn set_fractal_ratio(&mut self, ratio: f32) {
        self.fractal_ratio = clamp(ratio, 0.1, 2.0);
    }

    pub fn set_decay(&mut self, decay: f32) {
        self.decay = clamp(decay, 0.0, 0.995);
    }

    pub fn set_damping(&mut self, damp_hz: f32) {
        self.damping_hz = clamp(damp_hz, 400.0, 20000.0);
    }

    pub fn set_stereo_width(&mut self, width: f32) {
        self.stereo_width = clamp(width, 0.0, 1.0);
    }

    pub fn set_mix(&mut self, mix: f32) {
        self.mix = clamp(mix, 0.0, 1.0);
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
        num_samples: usize,
    ) {
        let n = num_samples.min(in_l.len()).min(in_r.len()).min(out_l.len()).min(out_r.len());
        if n == 0 {
            return;
        }

        let damp_coeff = (-2.0 * std::f32::consts::PI * self.damping_hz / self.sample_rate).exp();
        let max_del = (MAX_DELAY_PER_LINE - 4) as f32;

        for i in 0..n {
            // Anti-click smoothing
            self.smoothed_freq = lerp(self.smoothed_freq, self.base_freq, 0.005);
            self.smoothed_decay = lerp(self.smoothed_decay, self.decay, 0.005);
            self.smoothed_width = lerp(self.smoothed_width, self.stereo_width, 0.005);
            self.smoothed_mix = lerp(self.smoothed_mix, self.mix, 0.005);

            let input_mono = (in_l[i] + in_r[i]) * 0.5;

            // Compute delay times based on Golden Ratio powers:
            // tau_k = fs / (f0 * phi^((k - 1) * fractal_ratio * 0.4))
            let mut read_vals = [0.0f32; NUM_LINES];
            let mut sum_read = 0.0f32;

            for k in 0..NUM_LINES {
                let phi_power = (k as f32) * self.fractal_ratio * 0.35;
                let line_freq = self.smoothed_freq * PHI.powf(phi_power);
                let delay_len = clamp(self.sample_rate / line_freq.max(20.0), 4.0, max_del);

                let val = self.lines[k].read_frac(delay_len);
                read_vals[k] = val;
                sum_read += val;
            }

            // Householder unitary scattering matrix:
            // V_k = read_k - (2.0 / N) * sum(read)
            let householder_factor = 2.0 / (NUM_LINES as f32);
            let mut wet_accum_l = 0.0f32;
            let mut wet_accum_r = 0.0f32;

            for k in 0..NUM_LINES {
                let scattered = read_vals[k] - householder_factor * sum_read;

                // 1-pole damping filter in feedback loop
                self.lines[k].damp_state = scattered * (1.0 - damp_coeff) + self.lines[k].damp_state * damp_coeff;
                let damped = self.lines[k].damp_state;

                // Soft-saturate with Pade tanh to guarantee zero explosive runaway
                let fb_sample = fast_pade_tanh(damped * self.smoothed_decay + input_mono * 0.6);
                self.lines[k].write_sample(fb_sample);

                // Stereo spatialization: lines alternate L/R with pan spread
                let pan = if k % 2 == 0 {
                    0.5 - 0.5 * self.smoothed_width * (1.0 - (k as f32) / (NUM_LINES as f32))
                } else {
                    0.5 + 0.5 * self.smoothed_width * (1.0 - (k as f32) / (NUM_LINES as f32))
                };

                wet_accum_l += read_vals[k] * (1.0 - pan);
                wet_accum_r += read_vals[k] * pan;
            }

            // Normalization & mix
            let norm = 1.2 / (NUM_LINES as f32).sqrt();
            let wet_l = wet_accum_l * norm;
            let wet_r = wet_accum_r * norm;

            out_l[i] = lerp(in_l[i], wet_l, self.smoothed_mix);
            out_r[i] = lerp(in_r[i], wet_r, self.smoothed_mix);
        }
    }
}
