//! Event Manager & Orchestrator (Roadmap Phase 2, Rules 2, 3, 10, 11, 12, 35, 46, 47)
//! Audio event capture, spawning, tree generation limits, lifecycle advancement, and audio rendering.

use std::sync::atomic::{AtomicBool, Ordering};
use crate::event::RustEvent;
use crate::event_types::{RustEventAttributes, RustEventTelemetryItem};
use crate::event_capture_buffer::RustEventCaptureBuffer;
use crate::event_pool::RustEventPool;
use crate::event_telemetry::RustEventTelemetryBuffer;

pub const MAX_GENERATIONS: u32 = 4; // CPU protection against exponential multiplication (Rule 11)

pub struct RustEventManager {
    sample_rate: f64,
    max_block_size: usize,
    next_event_id: u32,

    editor_active: AtomicBool,

    capture_buffer: RustEventCaptureBuffer,
    event_pool: RustEventPool,
    active_events: Vec<*mut RustEvent>,
    telemetry_buffer: RustEventTelemetryBuffer,

    scratch_l: Vec<f32>,
    scratch_r: Vec<f32>,
}

unsafe impl Send for RustEventManager {}

impl RustEventManager {
    pub fn new() -> Self {
        Self {
            sample_rate: 44100.0,
            max_block_size: 512,
            next_event_id: 1,
            editor_active: AtomicBool::new(false),
            capture_buffer: RustEventCaptureBuffer::new(),
            event_pool: RustEventPool::new(),
            active_events: Vec::new(),
            telemetry_buffer: RustEventTelemetryBuffer::new(),
            scratch_l: Vec::new(),
            scratch_r: Vec::new(),
        }
    }

    pub fn prepare(&mut self, sample_rate: f64, max_block_size: usize, pool_capacity: usize) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let blk = if max_block_size > 0 { max_block_size } else { 512 };
        let cap = if pool_capacity > 0 { pool_capacity } else { RustEventPool::DEFAULT_MAX_EVENTS };

        self.sample_rate = sr;
        self.max_block_size = blk;

        self.capture_buffer.prepare(sr, 5.0, 2);
        self.event_pool.prepare(cap);

        self.active_events.clear();
        self.active_events.reserve_exact(cap);

        self.scratch_l.clear();
        self.scratch_l.resize(blk, 0.0);
        self.scratch_r.clear();
        self.scratch_r.resize(blk, 0.0);

        self.next_event_id = 1;
    }

    pub fn reset(&mut self) {
        self.capture_buffer.reset();
        self.event_pool.reset();
        self.active_events.clear();
        self.telemetry_buffer.reset();
        self.scratch_l.fill(0.0);
        self.scratch_r.fill(0.0);
    }

    pub fn capture_input_audio(&mut self, in_l: &[f32], in_r: &[f32], num_samples: usize) {
        self.capture_buffer.write_channels(in_l, in_r, num_samples);
    }

    pub fn spawn_event(
        &mut self,
        event_type: u16,
        capture_offset_samples: f64,
        duration_samples: u64,
        pitch_ratio: f32,
        gain: f32,
        pan: f32,
        source_follow: f32,
        disappearance_mode: u8,
        reverse: bool,
    ) -> *mut RustEvent {
        let ev_ptr = self.event_pool.acquire();
        if ev_ptr.is_null() {
            return std::ptr::null_mut(); // Bounded pool limit reached safely (Rule 11)
        }

        let mut attrs = RustEventAttributes::default();
        attrs.id = self.next_event_id;
        self.next_event_id = self.next_event_id.wrapping_add(1);
        attrs.event_type = event_type;
        attrs.pitch_ratio = pitch_ratio;
        attrs.gain = gain;
        attrs.pan = pan;
        attrs.source_follow = source_follow;
        attrs.disappearance_mode = disappearance_mode;
        attrs.generation = 0;
        attrs.parent_id = 0;
        attrs.energy = 1.0;
        attrs.attack_samples = (self.sample_rate * 0.005).max(1.0) as u32; // 5ms anti-click
        attrs.release_samples = (self.sample_rate * 0.020).max(1.0) as u32; // 20ms anti-click

        unsafe {
            (*ev_ptr).initialize(attrs, capture_offset_samples, duration_samples, reverse);
            (*ev_ptr).start();
        }

        self.active_events.push(ev_ptr);
        ev_ptr
    }

    pub fn spawn_child_event(
        &mut self,
        parent: &RustEvent,
        offset_shift_samples: f64,
        pitch_multiplier: f32,
        energy_decay: f32,
    ) -> *mut RustEvent {
        let parent_attrs = parent.get_attributes();
        if parent_attrs.generation >= MAX_GENERATIONS {
            return std::ptr::null_mut(); // Multi-generation limit reached (Rule 11)
        }

        let child_ptr = self.event_pool.acquire();
        if child_ptr.is_null() {
            return std::ptr::null_mut();
        }

        let mut attrs = *parent_attrs;
        attrs.id = self.next_event_id;
        self.next_event_id = self.next_event_id.wrapping_add(1);
        attrs.generation = parent_attrs.generation + 1;
        attrs.parent_id = parent_attrs.id;
        attrs.energy = parent_attrs.energy * energy_decay;
        attrs.pitch_ratio = parent_attrs.pitch_ratio * pitch_multiplier;
        attrs.gain = parent_attrs.gain * attrs.energy;

        unsafe {
            (*child_ptr).initialize(attrs, offset_shift_samples, parent_attrs.lifetime_samples, false);
            (*child_ptr).start();
        }

        self.active_events.push(child_ptr);
        child_ptr
    }

    pub fn render(
        &mut self,
        out_l: &mut [f32],
        out_r: &mut [f32],
        num_samples: usize,
        source_level: f32,
        source_silence_override: i32,
    ) {
        if out_l.is_empty() || out_r.is_empty() || num_samples == 0 {
            return;
        }

        if self.active_events.is_empty() {
            return; // Idle event bypass: zero active events (Rules 9, 10, 47)
        }

        let limit = num_samples.min(self.scratch_l.len()).min(out_l.len()).min(out_r.len());

        self.scratch_l[..limit].fill(0.0);
        self.scratch_r[..limit].fill(0.0);

        let editor_on = self.editor_active.load(Ordering::Relaxed);

        let mut i = 0;
        while i < self.active_events.len() {
            let ev_ptr = self.active_events[i];
            if !ev_ptr.is_null() && unsafe { (*ev_ptr).is_active() } {
                unsafe {
                    (*ev_ptr).render(
                        &self.capture_buffer,
                        &mut self.scratch_l[..limit],
                        &mut self.scratch_r[..limit],
                        limit,
                        source_level,
                        source_silence_override,
                    );

                    if editor_on {
                        let attrs = (*ev_ptr).get_attributes();
                        let mut t_item = RustEventTelemetryItem::default();
                        t_item.pan = attrs.pan;
                        t_item.pitch_ratio = attrs.pitch_ratio;
                        t_item.energy = attrs.energy * attrs.gain;
                        t_item.distance = attrs.distance;
                        t_item.azimuth = attrs.azimuth;
                        t_item.event_type = attrs.event_type;
                        t_item.generation = attrs.generation as u8;
                        t_item.is_alive = true;
                        self.telemetry_buffer.push(t_item);
                    }
                }
                i += 1;
            } else {
                // Event died or was killed: return to pool immediately (Rule 10)
                self.event_pool.release(ev_ptr);

                // O(1) swap_remove for safe real-time removal without shifts
                self.active_events.swap_remove(i);
            }
        }

        // Sum scratch buffers into target output buffers
        for s in 0..limit {
            out_l[s] += self.scratch_l[s];
            out_r[s] += self.scratch_r[s];
        }
    }

    #[inline]
    pub fn set_editor_active(&self, active: bool) {
        self.editor_active.store(active, Ordering::Relaxed);
    }

    #[inline]
    pub fn is_editor_active(&self) -> bool {
        self.editor_active.load(Ordering::Relaxed)
    }

    #[inline]
    pub fn get_active_event_count(&self) -> usize {
        self.active_events.len()
    }

    #[inline]
    pub fn get_pool_capacity(&self) -> usize {
        self.event_pool.capacity()
    }

    #[inline]
    pub fn get_capture_buffer(&self) -> &RustEventCaptureBuffer {
        &self.capture_buffer
    }

    #[inline]
    pub fn get_capture_buffer_mut(&mut self) -> &mut RustEventCaptureBuffer {
        &mut self.capture_buffer
    }

    #[inline]
    pub fn get_telemetry_buffer(&self) -> &RustEventTelemetryBuffer {
        &self.telemetry_buffer
    }

    #[inline]
    pub fn get_event_pool(&self) -> &RustEventPool {
        &self.event_pool
    }

    #[inline]
    pub fn get_event_pool_mut(&mut self) -> &mut RustEventPool {
        &mut self.event_pool
    }
}
