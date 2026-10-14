//! EventContainerCore - Event Rack Container (DSP #47 / NodeType::EventContainer)
//! Developed for the N8Effect Hybrid C++ / Rust Audio Engine Architecture (Rules 2, 3, 6, 9, 10, 11, 35, 46, 47).
//! Encapsulates audio event capture, event pool (bounded capacity of 64 events), edge triggers,
//! auto-density rhythmic stochastic generator, source-following level calculation,
//! real-time active event rendering, and smooth wet/dry mix.
//! Zero heap allocations in the audio processing thread, cacheline aligned, AVX2 SIMD friendly.

use crate::common::denormal_guard;
use crate::event_manager::RustEventManager;

/// EventContainerCore manages the event engine lifecycle, triggers,
/// source following, and wet/dry mixing inside a contained modular rack.
pub struct EventContainerCore {
    sample_rate: f32,
    max_block_size: usize,
    event_manager: RustEventManager,

    target_trigger: f32,
    last_trigger: f32,
    target_density: f32,
    target_pitch: f32,
    target_duration_ms: f32,
    target_source_follow: f32,
    target_mix: f32,
    current_mix: f32,

    auto_trigger_counter: usize,

    scratch_ev_l: Vec<f32>,
    scratch_ev_r: Vec<f32>,
}

unsafe impl Send for EventContainerCore {}

impl EventContainerCore {
    /// Default pool capacity for event container (Rule 47: Bounded memory)
    pub const DEFAULT_POOL_CAPACITY: usize = 64;

    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let mut core = Self {
            sample_rate: sr,
            max_block_size: 512,
            event_manager: RustEventManager::new(),
            target_trigger: 0.0,
            last_trigger: 0.0,
            target_density: 0.5,
            target_pitch: 1.0,
            target_duration_ms: 150.0,
            target_source_follow: 0.0,
            target_mix: 0.7,
            current_mix: 0.7,
            auto_trigger_counter: 0,
            scratch_ev_l: Vec::new(),
            scratch_ev_r: Vec::new(),
        };
        core.prepare(sr, 512, Self::DEFAULT_POOL_CAPACITY);
        core
    }

    pub fn prepare(&mut self, sample_rate: f32, max_block_size: usize, pool_capacity: usize) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.max_block_size = if max_block_size > 0 { max_block_size } else { 512 };
        let cap = if pool_capacity > 0 { pool_capacity } else { Self::DEFAULT_POOL_CAPACITY };

        self.event_manager.prepare(self.sample_rate as f64, self.max_block_size, cap);

        if self.scratch_ev_l.len() < self.max_block_size {
            self.scratch_ev_l.clear();
            self.scratch_ev_l.resize(self.max_block_size, 0.0);
            self.scratch_ev_r.clear();
            self.scratch_ev_r.resize(self.max_block_size, 0.0);
        }

        self.reset();
        self.current_mix = self.target_mix;
        self.last_trigger = self.target_trigger;
    }

    pub fn reset(&mut self) {
        self.event_manager.reset();
        self.auto_trigger_counter = 0;
        self.current_mix = self.target_mix;
        self.last_trigger = self.target_trigger;
        self.scratch_ev_l.fill(0.0);
        self.scratch_ev_r.fill(0.0);
    }

    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            1 => self.target_trigger = value,
            2 => self.target_density = value.clamp(0.0, 1.0),
            3 => self.target_pitch = value.clamp(0.25, 4.0),
            4 => self.target_duration_ms = value.clamp(10.0, 1000.0),
            5 => self.target_source_follow = value.clamp(0.0, 1.0),
            6 => self.target_mix = value.clamp(0.0, 1.0),
            _ => {}
        }
    }

    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            1 => self.target_trigger,
            2 => self.target_density,
            3 => self.target_pitch,
            4 => self.target_duration_ms,
            5 => self.target_source_follow,
            6 => self.target_mix,
            _ => 0.0,
        }
    }

    /// Captures input audio, calculates RMS source level, checks manual trigger & auto density,
    /// and spawns events accordingly without heap allocations.
    pub fn capture_and_trigger(
        &mut self,
        in_l: Option<&[f32]>,
        in_r: Option<&[f32]>,
        num_samples: usize,
    ) -> f32 {
        if let Some(l) = in_l {
            let r = in_r.unwrap_or(l);
            self.event_manager.capture_input_audio(l, r, num_samples);
        }

        // Calculate RMS level of source for Source Following (Rule 3)
        let mut sum_sq = 0.0f32;
        if let Some(l) = in_l {
            for s in 0..num_samples {
                let sample_val = l[s];
                sum_sq += sample_val * sample_val;
            }
        }
        let source_level = (sum_sq / (num_samples.max(1) as f32)).sqrt();

        // Edge detector for manual trigger parameter
        if self.target_trigger >= 0.5 && self.last_trigger < 0.5 {
            let dur_samples = (self.target_duration_ms * 0.001 * self.sample_rate).max(1.0) as u64;
            self.event_manager.spawn_event(
                2, // EventType::Grain
                dur_samples as f64,
                dur_samples,
                self.target_pitch,
                1.0,
                0.0,
                self.target_source_follow,
                2, // SourceDisappearanceMode::Fade
                false,
            );
        }
        self.last_trigger = self.target_trigger;

        // Auto density stochastic generator if Density > 0.05
        if self.target_density > 0.05 {
            self.auto_trigger_counter += num_samples;
            let trigger_interval = (self.sample_rate * (1.0 - self.target_density * 0.85) * 0.25) as usize;
            let min_interval = 256.max(trigger_interval);
            if self.auto_trigger_counter >= min_interval {
                self.auto_trigger_counter = 0;
                let dur_samples = (self.target_duration_ms * 0.001 * self.sample_rate).max(1.0) as u64;
                self.event_manager.spawn_event(
                    2, // EventType::Grain
                    dur_samples as f64,
                    dur_samples,
                    self.target_pitch,
                    0.85,
                    0.0,
                    self.target_source_follow,
                    2, // SourceDisappearanceMode::Fade
                    false,
                );
            }
        }

        source_level
    }

    /// Injects audio input + triggered events into forward buffer to feed an inner processor.
    pub fn inject_forward(
        &mut self,
        in_l: Option<&[f32]>,
        in_r: Option<&[f32]>,
        forward_l: &mut [f32],
        forward_r: &mut [f32],
        num_samples: usize,
    ) {
        let source_level = self.capture_and_trigger(in_l, in_r, num_samples);
        let limit = num_samples.min(forward_l.len()).min(forward_r.len());
        forward_l[..limit].fill(0.0);
        forward_r[..limit].fill(0.0);
        self.event_manager.render(&mut forward_l[..limit], &mut forward_r[..limit], limit, source_level, 0);
    }

    /// Mixes wet return signal from inner processor with dry input signal using anti-click smoothing.
    pub fn process_return(
        &mut self,
        in_l: Option<&[f32]>,
        in_r: Option<&[f32]>,
        wet_l: &[f32],
        wet_r: &[f32],
        out_l: &mut [f32],
        mut out_r: Option<&mut [f32]>,
        num_samples: usize,
    ) {
        let alpha = 0.005f32;
        let limit = num_samples.min(wet_l.len()).min(wet_r.len()).min(out_l.len());

        for s in 0..limit {
            self.current_mix += alpha * (self.target_mix - self.current_mix);
            let dry_l = denormal_guard(in_l.map_or(0.0, |b| b[s]));
            let dry_r = denormal_guard(in_r.map_or(dry_l, |b| b[s]));
            let w_l = denormal_guard(wet_l[s]);
            let w_r = denormal_guard(wet_r[s]);

            out_l[s] = dry_l * (1.0 - self.current_mix) + w_l * self.current_mix;
            if let Some(ref mut r_buf) = out_r {
                if s < r_buf.len() {
                    r_buf[s] = dry_r * (1.0 - self.current_mix) + w_r * self.current_mix;
                }
            }
        }
    }

    /// Direct processing path when no inner processor is attached:
    /// Captures input, updates triggers, renders events into scratch buffer, and blends with dry.
    pub fn process_direct(
        &mut self,
        in_l: Option<&[f32]>,
        in_r: Option<&[f32]>,
        out_l: &mut [f32],
        mut out_r: Option<&mut [f32]>,
        num_samples: usize,
    ) {
        let source_level = self.capture_and_trigger(in_l, in_r, num_samples);
        let scratch_capacity = self.scratch_ev_l.len();
        if scratch_capacity == 0 {
            return;
        }

        let alpha = 0.005f32;

        // Process in scratch-sized chunks to ensure zero allocations even if host sends larger blocks
        let chunk_size = scratch_capacity;
        for chunk_start in (0..num_samples).step_by(chunk_size) {
            let chunk_len = (num_samples - chunk_start).min(chunk_size);

            self.scratch_ev_l[..chunk_len].fill(0.0);
            self.scratch_ev_r[..chunk_len].fill(0.0);

            self.event_manager.render(
                &mut self.scratch_ev_l[..chunk_len],
                &mut self.scratch_ev_r[..chunk_len],
                chunk_len,
                source_level,
                0,
            );

            for i in 0..chunk_len {
                let s = chunk_start + i;
                self.current_mix += alpha * (self.target_mix - self.current_mix);

                let dry_l = denormal_guard(in_l.map_or(0.0, |b| b[s]));
                let dry_r = denormal_guard(in_r.map_or(dry_l, |b| b[s]));
                let w_l = denormal_guard(self.scratch_ev_l[i]);
                let w_r = denormal_guard(self.scratch_ev_r[i]);

                out_l[s] = dry_l * (1.0 - self.current_mix) + w_l * self.current_mix;
                if let Some(ref mut r_buf) = out_r {
                    if s < r_buf.len() {
                        r_buf[s] = dry_r * (1.0 - self.current_mix) + w_r * self.current_mix;
                    }
                }
            }
        }
    }

    #[inline]
    pub fn get_event_manager_mut(&mut self) -> *mut RustEventManager {
        &mut self.event_manager as *mut _
    }

    #[inline]
    pub fn get_event_manager(&self) -> *const RustEventManager {
        &self.event_manager as *const _
    }

    #[inline]
    pub fn get_active_event_count(&self) -> usize {
        self.event_manager.get_active_event_count()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_event_container_lifecycle_and_trigger() {
        let mut core = EventContainerCore::new(44100.0);
        core.set_parameter(4, 80.0); // Duration 80ms
        core.set_parameter(3, 1.25); // Pitch 1.25
        core.set_parameter(6, 1.0);  // Mix 100%

        let in_audio = vec![0.8f32; 256];
        let mut out_l = vec![0.0f32; 256];
        let mut out_r = vec![0.0f32; 256];

        // Block 1: Feed audio into capture buffer
        core.process_direct(Some(&in_audio), Some(&in_audio), &mut out_l, Some(&mut out_r), 256);

        // Block 2: Manual trigger
        core.set_parameter(1, 1.0);
        core.process_direct(Some(&in_audio), Some(&in_audio), &mut out_l, Some(&mut out_r), 256);

        // Assert event is active
        assert!(core.get_active_event_count() >= 1);

        // Subsequent blocks: trigger reset to 0.0
        core.set_parameter(1, 0.0);
        for _ in 0..10 {
            core.process_direct(Some(&in_audio), Some(&in_audio), &mut out_l, Some(&mut out_r), 256);
            for s in 0..256 {
                assert!(!out_l[s].is_nan() && !out_l[s].is_infinite());
                assert!(!out_r[s].is_nan() && !out_r[s].is_infinite());
            }
        }

        // Reset
        core.reset();
        assert_eq!(core.get_active_event_count(), 0);
    }

    #[test]
    fn test_event_container_forward_return_pipeline() {
        let mut core = EventContainerCore::new(44100.0);
        core.set_parameter(1, 1.0);
        let in_audio = vec![0.7f32; 256];
        let mut fwd_l = vec![0.0f32; 256];
        let mut fwd_r = vec![0.0f32; 256];
        let mut out_l = vec![0.0f32; 256];
        let mut out_r = vec![0.0f32; 256];

        // Inject forward
        core.inject_forward(Some(&in_audio), Some(&in_audio), &mut fwd_l, &mut fwd_r, 256);
        assert!(core.get_active_event_count() >= 1);

        // Simulate inner processor by multiplying wet by 0.5
        let wet_l: Vec<f32> = fwd_l.iter().map(|&x| x * 0.5).collect();
        let wet_r: Vec<f32> = fwd_r.iter().map(|&x| x * 0.5).collect();

        // Process return
        core.process_return(Some(&in_audio), Some(&in_audio), &wet_l, &wet_r, &mut out_l, Some(&mut out_r), 256);
        for s in 0..256 {
            assert!(!out_l[s].is_nan());
            assert!(!out_r[s].is_nan());
        }
    }
}
