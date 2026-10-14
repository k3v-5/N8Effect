//! Dimension Chorus (Phase 9 - Large Effect Library, Rules 5, 8, 9, 34, 46, 47)
//! Emulates the classic Roland SDD-320 Dimension D / Juno BBD chorus.
//! Uses dual complementary delay lines with anti-phase LFO modulation and
//! cross-channel feedback/cancellation to achieve signature spatial widening.

use crate::common::{clamp, lerp};
use crate::optimization::fast_pade_tanh;

const MAX_DELAY_SAMPLES: usize = 4096;

pub struct RustDimensionChorus {
    sample_rate: f32,
    buffer_l: Vec<f32>,
    buffer_r: Vec<f32>,
    write_pos: usize,

    // LFO state
    lfo_phase: f32,

    // Parameters
    mode: u32,             // 0 = Off, 1 = Subtle, 2 = Moderate, 3 = Rich, 4 = All-Buttons In
    rate_hz: f32,          // 0.1 to 5.0 Hz
    depth: f32,            // 0.0 to 1.0
    stereo_spread: f32,    // 0.0 to 2.0
    drive: f32,            // 0.0 to 1.0 (BBD saturation)
    mix: f32,              // 0.0 to 1.0

    // Smoothed values
    smoothed_mix: f32,
    smoothed_depth: f32,
}

impl RustDimensionChorus {
    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            buffer_l: vec![0.0; MAX_DELAY_SAMPLES],
            buffer_r: vec![0.0; MAX_DELAY_SAMPLES],
            write_pos: 0,
            lfo_phase: 0.0,
            mode: 2,
            rate_hz: 0.5,
            depth: 0.6,
            stereo_spread: 1.0,
            drive: 0.2,
            mix: 0.5,
            smoothed_mix: 0.5,
            smoothed_depth: 0.6,
        }
    }

    pub fn reset(&mut self) {
        self.buffer_l.fill(0.0);
        self.buffer_r.fill(0.0);
        self.write_pos = 0;
        self.lfo_phase = 0.0;
        self.smoothed_mix = self.mix;
        self.smoothed_depth = self.depth;
    }

    pub fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = if sample_rate > 1000.0 { sample_rate } else { 44100.0 };
        self.reset();
    }

    pub fn set_mode(&mut self, mode: u32) {
        self.mode = mode.min(4);
        // Preset mode calibrations
        match self.mode {
            1 => { self.rate_hz = 0.25; self.depth = 0.35; }
            2 => { self.rate_hz = 0.50; self.depth = 0.55; }
            3 => { self.rate_hz = 0.75; self.depth = 0.75; }
            4 => { self.rate_hz = 1.10; self.depth = 0.95; }
            _ => {}
        }
    }

    pub fn set_rate(&mut self, rate_hz: f32) {
        self.rate_hz = clamp(rate_hz, 0.05, 5.0);
    }

    pub fn set_depth(&mut self, depth: f32) {
        self.depth = clamp(depth, 0.0, 1.0);
    }

    pub fn set_stereo_spread(&mut self, spread: f32) {
        self.stereo_spread = clamp(spread, 0.0, 2.0);
    }

    pub fn set_drive(&mut self, drive: f32) {
        self.drive = clamp(drive, 0.0, 1.0);
    }

    pub fn set_mix(&mut self, mix: f32) {
        self.mix = clamp(mix, 0.0, 1.0);
    }

    #[inline]
    fn read_interpolated(buf: &[f32], delay_samples: f32, write_pos: usize) -> f32 {
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

        if self.mode == 0 {
            // Bypass mode: straight passthrough
            out_l[..num_samples].copy_from_slice(&in_l[..num_samples]);
            out_r[..num_samples].copy_from_slice(&in_r[..num_samples]);
            return;
        }

        let lfo_inc = (std::f32::consts::TAU * self.rate_hz) / self.sample_rate;
        let base_delay_ms = 5.0f32; // Nominal BBD delay: 5 ms
        let mod_delay_ms = 2.5f32;  // LFO excursion: +/- 2.5 ms
        let base_samples = (base_delay_ms * 0.001) * self.sample_rate;
        let mod_samples = (mod_delay_ms * 0.001) * self.sample_rate;

        for i in 0..num_samples {
            // Parameter smoothing
            self.smoothed_mix = lerp(self.smoothed_mix, self.mix, 0.005);
            self.smoothed_depth = lerp(self.smoothed_depth, self.depth, 0.005);

            let dry_l = in_l[i];
            let dry_r = in_r[i];

            // Optional BBD soft saturation
            let sat_in_l = if self.drive > 0.01 {
                let d = 1.0 + self.drive * 2.0;
                fast_pade_tanh(dry_l * d) / d
            } else {
                dry_l
            };
            let sat_in_r = if self.drive > 0.01 {
                let d = 1.0 + self.drive * 2.0;
                fast_pade_tanh(dry_r * d) / d
            } else {
                dry_r
            };

            // Write to circular delay buffer
            self.buffer_l[self.write_pos] = sat_in_l;
            self.buffer_r[self.write_pos] = sat_in_r;

            // Compute complementary LFOs (180 deg out of phase)
            let mod_l = (self.lfo_phase).sin();
            let mod_r = (self.lfo_phase + std::f32::consts::PI).sin();

            let delay_l = base_samples + mod_l * mod_samples * self.smoothed_depth;
            let delay_r = base_samples + mod_r * mod_samples * self.smoothed_depth;

            // Read wet signals from delay lines
            let wet_l = Self::read_interpolated(&self.buffer_l, delay_l, self.write_pos);
            let wet_r = Self::read_interpolated(&self.buffer_r, delay_r, self.write_pos);

            // Roland Dimension D spatial cross-mixing:
            // Channel L gets: wet_l - (spread * 0.4) * wet_r
            // Channel R gets: wet_r - (spread * 0.4) * wet_l
            let cross = self.stereo_spread * 0.4;
            let spatial_l = wet_l - cross * wet_r;
            let spatial_r = wet_r - cross * wet_l;

            // Dry/Wet mixing
            let m = self.smoothed_mix;
            out_l[i] = (1.0 - m) * dry_l + m * spatial_l;
            out_r[i] = (1.0 - m) * dry_r + m * spatial_r;

            // Advance phase & write pointer
            self.lfo_phase += lfo_inc;
            if self.lfo_phase >= std::f32::consts::TAU {
                self.lfo_phase -= std::f32::consts::TAU;
            }

            self.write_pos = (self.write_pos + 1) % MAX_DELAY_SAMPLES;
        }
    }
}
