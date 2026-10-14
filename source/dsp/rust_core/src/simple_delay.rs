//! High-Performance Stereo Delay with Smooth Interpolation & Feedback Protection
//! (DSP #5 / NodeType::Delay)
//! Rules 5, 8, 9, 12, 14, 18, 35, 46, 47:
//! Zero heap allocations in audio processing thread,
//! Preallocated circular delay line up to 2.5 seconds,
//! Linear fractional delay interpolation,
//! Configurable feedback limiter with runaway safety (< 0.95),
//! Anti-click smoothing on parameter changes,
//! Denormal & NaN/Inf protection.

use crate::common::denormal_guard;

pub struct RustSimpleDelay {
    sample_rate: f64,
    max_delay_samples: usize,
    write_index: usize,

    buffer_l: Box<[f32]>,
    buffer_r: Box<[f32]>,

    param_delay_time_ms: f32,
    param_feedback: f32,
    param_dry_wet: f32,

    current_delay_samples: f32,
    current_feedback: f32,
}

impl RustSimpleDelay {
    pub fn new() -> Self {
        let initial_sr = 44100.0;
        let initial_max = (initial_sr * 2.5) as usize;
        let mut delay = Self {
            sample_rate: initial_sr,
            max_delay_samples: initial_max,
            write_index: 0,
            buffer_l: vec![0.0f32; initial_max].into_boxed_slice(),
            buffer_r: vec![0.0f32; initial_max].into_boxed_slice(),
            param_delay_time_ms: 250.0,
            param_feedback: 0.4,
            param_dry_wet: 0.5,
            current_delay_samples: (250.0 * 0.001 * initial_sr) as f32,
            current_feedback: 0.4,
        };
        delay.prepare(initial_sr);
        delay
    }

    pub fn prepare(&mut self, sample_rate: f64) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let max_samples = ((self.sample_rate * 2.5) as usize).max(44100);
        if max_samples != self.max_delay_samples {
            self.max_delay_samples = max_samples;
            self.buffer_l = vec![0.0f32; max_samples].into_boxed_slice();
            self.buffer_r = vec![0.0f32; max_samples].into_boxed_slice();
        }
        self.reset();
        self.current_delay_samples = (self.param_delay_time_ms * 0.001 * (self.sample_rate as f32))
            .clamp(1.0, (self.max_delay_samples - 2) as f32);
        self.current_feedback = self.param_feedback.clamp(0.0, 0.95);
    }

    pub fn reset(&mut self) {
        self.buffer_l.fill(0.0);
        self.buffer_r.fill(0.0);
        self.write_index = 0;
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => {
                // DelayTimeMs: 1.0 .. 2000.0
                self.param_delay_time_ms = value.clamp(1.0, 2000.0);
            }
            2 => {
                // Feedback: 0.0 .. 0.95
                self.param_feedback = value.clamp(0.0, 0.95);
            }
            3 => {
                // DryWet: 0.0 .. 1.0
                self.param_dry_wet = value.clamp(0.0, 1.0);
            }
            _ => {}
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            1 => self.param_delay_time_ms,
            2 => self.param_feedback,
            3 => self.param_dry_wet,
            _ => 0.0,
        }
    }

    #[inline]
    pub fn process(
        &mut self,
        in_l: &[f32],
        in_r: &[f32],
        out_l: &mut [f32],
        out_r: &mut [f32],
    ) {
        let num_samples = in_l.len();
        if num_samples == 0 || self.max_delay_samples < 2 {
            return;
        }

        let max_cap = self.max_delay_samples;
        let max_cap_f = max_cap as f32;
        let target_delay_samples = (self.param_delay_time_ms * 0.001 * (self.sample_rate as f32))
            .clamp(1.0, (max_cap - 2) as f32);
        let target_fb = self.param_feedback.clamp(0.0, 0.95);
        let mix = self.param_dry_wet.clamp(0.0, 1.0);

        let mut write_idx = self.write_index;

        for s in 0..num_samples {
            // Anti-click coefficient smoothing (Rule 35)
            self.current_delay_samples += 0.002 * (target_delay_samples - self.current_delay_samples);
            self.current_feedback += 0.002 * (target_fb - self.current_feedback);

            let delay_val = self.current_delay_samples;
            let fb = self.current_feedback;

            let in_val_l = in_l[s];
            let in_val_r = in_r[s];

            // Linear interpolation read
            let mut read_pos = (write_idx as f32) - delay_val;
            while read_pos < 0.0 {
                read_pos += max_cap_f;
            }
            while read_pos >= max_cap_f {
                read_pos -= max_cap_f;
            }

            let idx0 = (read_pos as usize) % max_cap;
            let idx1 = if idx0 + 1 >= max_cap { 0 } else { idx0 + 1 };
            let frac = read_pos - (read_pos as usize as f32);

            let delayed_l = self.buffer_l[idx0] + frac * (self.buffer_l[idx1] - self.buffer_l[idx0]);
            let delayed_r = self.buffer_r[idx0] + frac * (self.buffer_r[idx1] - self.buffer_r[idx0]);

            // Write into circular buffer with denormal guard
            self.buffer_l[write_idx] = denormal_guard(in_val_l + delayed_l * fb);
            self.buffer_r[write_idx] = denormal_guard(in_val_r + delayed_r * fb);

            write_idx += 1;
            if write_idx >= max_cap {
                write_idx = 0;
            }

            out_l[s] = denormal_guard(in_val_l * (1.0 - mix) + delayed_l * mix);
            out_r[s] = denormal_guard(in_val_r * (1.0 - mix) + delayed_r * mix);
        }

        self.write_index = write_idx;
    }
}
