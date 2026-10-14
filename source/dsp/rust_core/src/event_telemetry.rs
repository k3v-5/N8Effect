//! Lock-Free SPSC Event Telemetry Ring Buffer (Roadmap Phase 2, Rules 9, 23, 26, 47)
//! Thread-safe telemetry dispatch from audio thread to GUI without locks or dynamic allocations.

use std::sync::atomic::{AtomicUsize, Ordering};
use std::cell::UnsafeCell;
use crate::event_types::RustEventTelemetryItem;

pub const TELEMETRY_CAPACITY: usize = 1024;
pub const TELEMETRY_MASK: usize = TELEMETRY_CAPACITY - 1;

pub struct RustEventTelemetryBuffer {
    buffer: UnsafeCell<[RustEventTelemetryItem; TELEMETRY_CAPACITY]>,
    write_index: AtomicUsize,
    read_index: AtomicUsize,
}

unsafe impl Sync for RustEventTelemetryBuffer {}
unsafe impl Send for RustEventTelemetryBuffer {}

impl RustEventTelemetryBuffer {
    pub fn new() -> Self {
        Self {
            buffer: UnsafeCell::new([RustEventTelemetryItem::default(); TELEMETRY_CAPACITY]),
            write_index: AtomicUsize::new(0),
            read_index: AtomicUsize::new(0),
        }
    }

    pub fn reset(&self) {
        self.write_index.store(0, Ordering::Relaxed);
        self.read_index.store(0, Ordering::Relaxed);
    }

    pub fn push(&self, item: RustEventTelemetryItem) -> bool {
        let w = self.write_index.load(Ordering::Relaxed);
        let r = self.read_index.load(Ordering::Acquire);

        if w.wrapping_sub(r) >= TELEMETRY_CAPACITY {
            return false; // Bounded overflow protection (Rule 11)
        }

        unsafe {
            let buf = &mut *self.buffer.get();
            buf[w & TELEMETRY_MASK] = item;
        }

        self.write_index.store(w.wrapping_add(1), Ordering::Release);
        true
    }

    pub fn pop(&self, out_item: &mut RustEventTelemetryItem) -> bool {
        let r = self.read_index.load(Ordering::Relaxed);
        let w = self.write_index.load(Ordering::Acquire);

        if r == w {
            return false; // Empty queue
        }

        unsafe {
            let buf = &*self.buffer.get();
            *out_item = buf[r & TELEMETRY_MASK];
        }

        self.read_index.store(r.wrapping_add(1), Ordering::Release);
        true
    }

    pub fn size(&self) -> usize {
        let w = self.write_index.load(Ordering::Relaxed);
        let r = self.read_index.load(Ordering::Relaxed);
        w.saturating_sub(r)
    }
}
