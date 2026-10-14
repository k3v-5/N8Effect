//! Comb Filter Resonator (Phase 9 - Large Effect Library, Rules 5, 8, 9, 12, 34, 46, 47)
//! High-Q tuned comb resonator with bipolar feedback, feedback damping lowpass,
//! sub-sample fractional delay interpolation, and stereo detuning.

use crate::common::{clamp, lerp};
use crate::optimization::fast_pade_tanh;

const MAX_DELAY_SAMPLES: usize = 4096;

pub struct RustCombResonator {
    sample_rate: f32,
    buffer_l: Vec<f32>,
    buffer_r: Vec<f32>,
    write_pos: usize,

    // Feedback damping lowpass filter states
    damp_state_l: f32,
    damp_state_r: f32,

    // Parameters
    freq_hz: f32,          // 20.0 to 5000.0 Hz
    feedback: f32,         // -0.98 to +0.98 (bipolar: positive odd/even harmonics, negative odd only)
    damping_hz: f32,       // 1000.0 to 20000.0 Hz
    stereo_detune_cents: f32, // 0.0 to 50.0 cents
    mix: f32,              // 0.0 to 1.0

    // Smoothing
    smoothed_freq: f32,
    smoothed_fb: f32,
    smoothed_mix: f32,
}

impl RustCombResonator {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            buffer_l: vec![0.0; MAX_DELAY_SAMPLES],
            buffer_r: vec![0.0; MAX_DELAY_SAMPLES],
            write_pos: 0,
            damp_state_l: 0.0,
            damp_state_r: 0.0,
            freq_hz: 220.0,
            feedback: 0.85,
            damping_hz: 8000.0,
            stereo_detune_cents: 12.0,
            mix: 0.5,
            smoothed_freq: 220.0,
            smoothed_fb: 0.85,
            smoothed_mix: 0.5,
        }
    }

    pub fn reset(&mut self) {
        self.buffer_l.fill(0.0);
        self.buffer_r.fill(0.0);
        self.write_pos = 0;
        self.damp_state_l = 0.0;
        self.damp_state_r = 0.0;
        self.smoothed_freq = self.freq_hz;
        self.smoothed_fb = self.feedback;
        self.smoothed_mix = self.mix;
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn set_freq(&mut self, freq_hz: f32) {
        self.freq_hz = clamp(freq_hz, 20.0, 5000.0);
    }

    pub fn set_feedback(&mut self, fb: f32) {
        self.feedback = clamp(fb, -0.98, 0.98);
    }

    pub fn set_damping(&mut self, damp_hz: f32) {
        self.damping_hz = clamp(damp_hz, 500.0, 20000.0);
    }

    pub fn set_stereo_detune(&mut self, cents: f32) {
        self.stereo_detune_cents = clamp(cents, 0.0, 50.0);
    }

    pub fn set_mix(&mut self, mix: f32) {
        self.mix = clamp(mix, 0.0, 1.0);
    }

    #[inline(always)]
    fn read_fractional(buf: &[f32], delay_samples: f32, write_pos: usize) -> f32 {
        let max_len = buf.len();
        let read_pos = (write_pos as f32 + max_len as f32 - delay_samples) % (max_len as f32);
        let idx0 = read_pos.floor() as usize % max_len;
        let idx1 = (idx0 + 1) % max_len;
        let frac = read_pos - read_pos.floor();
        lerp(buf[idx0], buf[idx1], frac)
    }

    pub fn process_block(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len().min(in_r.len()).min(out_l.len()).min(out_r.len());
        if num_samples == 0 {
            return;
        }

        // Damping 1-pole coefficient
        let damp_fc = clamp(self.damping_hz, 200.0, self.sample_rate * 0.45);
        let damp_coeff = clamp(
            (std::f32::consts::TAU * damp_fc / self.sample_rate).min(1.0),
            0.01,
            0.99,
        );

        // Detune factor for channel R: 2^(cents / 1200)
        let detune_ratio = (self.stereo_detune_cents / 1200.0).exp2();

        for i in 0..num_samples {
            self.smoothed_freq = lerp(self.smoothed_freq, self.freq_hz, 0.005);
            self.smoothed_fb = lerp(self.smoothed_fb, self.feedback, 0.005);
            self.smoothed_mix = lerp(self.smoothed_mix, self.mix, 0.005);

            let dry_l = in_l[i];
            let dry_r = in_r[i];

            // Calculate delay in samples from frequency
            let delay_samples_l = clamp(
                self.sample_rate / self.smoothed_freq.max(20.0),
                2.0,
                (MAX_DELAY_SAMPLES - 4) as f32,
            );
            let freq_r = self.smoothed_freq * detune_ratio;
            let delay_samples_r = clamp(
                self.sample_rate / freq_r.max(20.0),
                2.0,
                (MAX_DELAY_SAMPLES - 4) as f32,
            );

            // Read comb output from delay lines
            let delayed_l = Self::read_fractional(&self.buffer_l, delay_samples_l, self.write_pos);
            let delayed_r = Self::read_fractional(&self.buffer_r, delay_samples_r, self.write_pos);

            // Apply damping lowpass to feedback signal
            self.damp_state_l += damp_coeff * (delayed_l - self.damp_state_l);
            self.damp_state_r += damp_coeff * (delayed_r - self.damp_state_r);

            // Soft saturation in feedback loop to prevent runaway (Rule 12)
            let fb_signal_l = fast_pade_tanh(self.damp_state_l * self.smoothed_fb);
            let fb_signal_r = fast_pade_tanh(self.damp_state_r * self.smoothed_fb);

            // Write input + feedback into buffer
            self.buffer_l[self.write_pos] = dry_l + fb_signal_l;
            self.buffer_r[self.write_pos] = dry_r + fb_signal_r;

            // Output wet signal is the resonated delayed output
            let wet_l = delayed_l;
            let wet_r = delayed_r;

            let m = self.smoothed_mix;
            out_l[i] = (1.0 - m) * dry_l + m * wet_l;
            out_r[i] = (1.0 - m) * dry_r + m * wet_r;

            self.write_pos = (self.write_pos + 1) % MAX_DELAY_SAMPLES;
        }
    }
}
