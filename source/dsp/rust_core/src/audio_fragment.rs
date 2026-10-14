//! Audio Fragment Playback Engine (Roadmap Phase 2, Rules 2, 4.1, 46)
//! Granular/slice fragment playback with variable pitch, reverse, and envelope scaling.

use crate::event_capture_buffer::RustEventCaptureBuffer;

pub struct RustAudioFragment {
    capture_offset_samples: f64,
    duration_samples: u64,
    pitch_ratio: f32,
    current_phase: f64,
    reverse: bool,
    is_finished: bool,
}

impl RustAudioFragment {
    pub fn new() -> Self {
        Self {
            capture_offset_samples: 0.0,
            duration_samples: 1024,
            pitch_ratio: 1.0,
            current_phase: 0.0,
            reverse: false,
            is_finished: true,
        }
    }

    pub fn configure(&mut self, capture_offset_samples: f64, duration_samples: u64, pitch_ratio: f32, reverse: bool) {
        self.capture_offset_samples = capture_offset_samples;
        self.duration_samples = duration_samples.max(16);
        self.pitch_ratio = pitch_ratio.clamp(0.125, 8.0);
        self.reverse = reverse;
        self.current_phase = 0.0;
        self.is_finished = false;
    }

    pub fn reset(&mut self) {
        self.current_phase = 0.0;
        self.is_finished = false;
    }

    /// Advances playback by one step and reads stereo sample pair from capture buffer.
    #[inline]
    pub fn get_next_sample(
        &mut self,
        capture: &RustEventCaptureBuffer,
        envelope_gain: f32,
    ) -> (bool, f32, f32) {
        if self.is_finished || self.current_phase >= (self.duration_samples as f64) {
            self.is_finished = true;
            return (false, 0.0, 0.0);
        }

        let dur_f = self.duration_samples as f64;
        let play_offset = if self.reverse { dur_f - self.current_phase } else { self.current_phase };
        let sample_offset = self.capture_offset_samples - play_offset;

        let out_l = capture.read_sample(0, sample_offset) * envelope_gain;
        let out_r = capture.read_sample(1, sample_offset) * envelope_gain;

        self.current_phase += self.pitch_ratio as f64;
        if self.current_phase >= dur_f {
            self.is_finished = true;
        }

        (true, out_l, out_r)
    }

    #[inline]
    pub fn current_phase(&self) -> f64 {
        self.current_phase
    }

    #[inline]
    pub fn duration_samples(&self) -> u64 {
        self.duration_samples
    }

    #[inline]
    pub fn is_finished(&self) -> bool {
        self.is_finished
    }
}
