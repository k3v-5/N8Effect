//! Pitch Delay & Crystals Shifter (Phase 9 - Large Effect Library, Rules 5, 8, 9, 14, 18, 34, 46, 47)
//! Dual-channel pitch-shifting delay line inspired by Eventide Crystals and MicroPitch.
//! Features pitch transposition (+/- 24 semitones and fine detune), reverse playback option,
//! cross-feedback matrix and damping filter.

use crate::common::{clamp, lerp};
use crate::optimization::fast_pade_tanh;

const MAX_DELAY_SAMPLES: usize = 96000; // 2 seconds @ 48 kHz
const GRAIN_SIZE_SAMPLES: usize = 2048;

pub struct RustPitchDelay {
    sample_rate: f32,
    buffer_l: Vec<f32>,
    buffer_r: Vec<f32>,
    write_pos: usize,

    // Pitch grain phasor states
    grain_phase_l: f32,
    grain_phase_r: f32,

    // Damping states
    damp_l: f32,
    damp_r: f32,

    // Parameters
    delay_ms_l: f32,       // 1.0 to 2000.0 ms
    delay_ms_r: f32,       // 1.0 to 2000.0 ms
    pitch_semitones: f32,  // -24.0 to +24.0
    detune_cents: f32,     // -50.0 to +50.0
    feedback: f32,         // 0.0 to 0.95
    reverse_mode: bool,    // false = forward, true = reverse
    damping_hz: f32,       // 1000.0 to 20000.0 Hz
    mix: f32,              // 0.0 to 1.0

    // Smoothing
    smoothed_mix: f32,
    smoothed_fb: f32,
}

impl RustPitchDelay {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            buffer_l: vec![0.0; MAX_DELAY_SAMPLES],
            buffer_r: vec![0.0; MAX_DELAY_SAMPLES],
            write_pos: 0,
            grain_phase_l: 0.0,
            grain_phase_r: 0.0,
            damp_l: 0.0,
            damp_r: 0.0,
            delay_ms_l: 375.0,
            delay_ms_r: 500.0,
            pitch_semitones: 12.0, // Default 1 octave up
            detune_cents: 7.0,
            feedback: 0.45,
            reverse_mode: false,
            damping_hz: 8000.0,
            mix: 0.5,
            smoothed_mix: 0.5,
            smoothed_fb: 0.45,
        }
    }

    pub fn reset(&mut self) {
        self.buffer_l.fill(0.0);
        self.buffer_r.fill(0.0);
        self.write_pos = 0;
        self.grain_phase_l = 0.0;
        self.grain_phase_r = 0.0;
        self.damp_l = 0.0;
        self.damp_r = 0.0;
        self.smoothed_mix = self.mix;
        self.smoothed_fb = self.feedback;
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn set_delay_l(&mut self, ms: f32) { self.delay_ms_l = clamp(ms, 1.0, 2000.0); }
    pub fn set_delay_r(&mut self, ms: f32) { self.delay_ms_r = clamp(ms, 1.0, 2000.0); }
    pub fn set_pitch(&mut self, semi: f32) { self.pitch_semitones = clamp(semi, -24.0, 24.0); }
    pub fn set_detune(&mut self, cents: f32) { self.detune_cents = clamp(cents, -50.0, 50.0); }
    pub fn set_feedback(&mut self, fb: f32) { self.feedback = clamp(fb, 0.0, 0.95); }
    pub fn set_reverse(&mut self, rev: bool) { self.reverse_mode = rev; }
    pub fn set_damping(&mut self, damp_hz: f32) { self.damping_hz = clamp(damp_hz, 500.0, 20000.0); }
    pub fn set_mix(&mut self, mix: f32) { self.mix = clamp(mix, 0.0, 1.0); }

    #[inline(always)]
    fn read_sample(buf: &[f32], offset: f32, write_pos: usize) -> f32 {
        let max_len = buf.len();
        let read_pos = (write_pos as f32 + max_len as f32 - offset) % (max_len as f32);
        let idx0 = read_pos.floor() as usize % max_len;
        let idx1 = (idx0 + 1) % max_len;
        let frac = read_pos - read_pos.floor();
        lerp(buf[idx0], buf[idx1], frac)
    }

    #[inline(always)]
    fn pitch_shift_grain(
        buf: &[f32],
        base_delay_samples: f32,
        phase: f32,
        write_pos: usize,
        reverse: bool,
    ) -> f32 {
        let grain_size = GRAIN_SIZE_SAMPLES as f32;
        // Phase 1 (0.0 .. 1.0) and Phase 2 (0.5 offset)
        let p1 = phase % 1.0;
        let p2 = (phase + 0.5) % 1.0;

        // Triangular/Hann window for crossfading
        let win1 = 0.5 * (1.0 - (std::f32::consts::TAU * p1).cos());
        let win2 = 0.5 * (1.0 - (std::f32::consts::TAU * p2).cos());

        let offset1 = if reverse {
            base_delay_samples + (1.0 - p1) * grain_size
        } else {
            base_delay_samples + p1 * grain_size
        };

        let offset2 = if reverse {
            base_delay_samples + (1.0 - p2) * grain_size
        } else {
            base_delay_samples + p2 * grain_size
        };

        let s1 = Self::read_sample(buf, offset1, write_pos);
        let s2 = Self::read_sample(buf, offset2, write_pos);

        s1 * win1 + s2 * win2
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

        // Delay times in samples
        let delay_samps_l = (self.delay_ms_l * 0.001 * self.sample_rate)
            .clamp(10.0, (MAX_DELAY_SAMPLES - GRAIN_SIZE_SAMPLES - 10) as f32);
        let delay_samps_r = (self.delay_ms_r * 0.001 * self.sample_rate)
            .clamp(10.0, (MAX_DELAY_SAMPLES - GRAIN_SIZE_SAMPLES - 10) as f32);

        // Pitch ratios: 2^(semi / 12)
        let ratio_l = ((self.pitch_semitones - self.detune_cents * 0.01) / 12.0).exp2();
        let ratio_r = ((self.pitch_semitones + self.detune_cents * 0.01) / 12.0).exp2();

        // Speed difference determines grain phasor rate: (1 - ratio) / grain_size
        let phase_inc_l = (1.0 - ratio_l) / (GRAIN_SIZE_SAMPLES as f32);
        let phase_inc_r = (1.0 - ratio_r) / (GRAIN_SIZE_SAMPLES as f32);

        // Damping filter
        let damp_fc = clamp(self.damping_hz, 200.0, self.sample_rate * 0.45);
        let damp_coeff = clamp(
            (std::f32::consts::TAU * damp_fc / self.sample_rate).min(1.0),
            0.01,
            0.99,
        );

        for i in 0..num_samples {
            self.smoothed_mix = lerp(self.smoothed_mix, self.mix, 0.005);
            self.smoothed_fb = lerp(self.smoothed_fb, self.feedback, 0.005);

            let dry_l = in_l[i];
            let dry_r = in_r[i];

            // Pitch-shifted grains read from delay buffer
            let shifted_l = Self::pitch_shift_grain(
                &self.buffer_l,
                delay_samps_l,
                self.grain_phase_l,
                self.write_pos,
                self.reverse_mode,
            );
            let shifted_r = Self::pitch_shift_grain(
                &self.buffer_r,
                delay_samps_r,
                self.grain_phase_r,
                self.write_pos,
                self.reverse_mode,
            );

            // Damping lowpass on feedback signal
            self.damp_l += damp_coeff * (shifted_l - self.damp_l);
            self.damp_r += damp_coeff * (shifted_r - self.damp_r);

            // Cross-feedback with soft saturation (Rule 12)
            let fb_l = fast_pade_tanh(self.damp_r * self.smoothed_fb);
            let fb_r = fast_pade_tanh(self.damp_l * self.smoothed_fb);

            self.buffer_l[self.write_pos] = dry_l + fb_l;
            self.buffer_r[self.write_pos] = dry_r + fb_r;

            let m = self.smoothed_mix;
            out_l[i] = (1.0 - m) * dry_l + m * shifted_l;
            out_r[i] = (1.0 - m) * dry_r + m * shifted_r;

            // Advance grain phase phasors
            self.grain_phase_l = (self.grain_phase_l + phase_inc_l).rem_euclid(1.0);
            self.grain_phase_r = (self.grain_phase_r + phase_inc_r).rem_euclid(1.0);

            self.write_pos = (self.write_pos + 1) % MAX_DELAY_SAMPLES;
        }
    }
}
