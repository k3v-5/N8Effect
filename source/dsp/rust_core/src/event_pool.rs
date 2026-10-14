//! Preallocated Event Pool (Roadmap Phase 2, Rules 9, 10, 11, 46, 47)
//! Zero-allocation fixed-capacity memory pool for audio events.

use crate::event::RustEvent;
use crate::event_types::EventLifecycle;

pub struct RustEventPool {
    storage: Vec<RustEvent>,
    free_indices: Vec<u32>,
    max_events: usize,
    free_count: usize,
}

impl RustEventPool {
    pub const DEFAULT_MAX_EVENTS: usize = 1024;

    pub fn new() -> Self {
        Self {
            storage: Vec::new(),
            free_indices: Vec::new(),
            max_events: 0,
            free_count: 0,
        }
    }

    pub fn prepare(&mut self, max_events: usize) {
        let count = if max_events > 0 { max_events } else { Self::DEFAULT_MAX_EVENTS };
        self.max_events = count;
        self.storage.clear();
        self.storage.reserve_exact(count);
        for _ in 0..count {
            self.storage.push(RustEvent::new());
        }

        self.free_indices.clear();
        self.free_indices.reserve_exact(count);
        for i in 0..count {
            self.free_indices.push(i as u32);
        }

        self.free_count = count;
    }

    pub fn reset(&mut self) {
        self.free_count = self.max_events;
        for i in 0..self.max_events {
            self.free_indices[i] = i as u32;
            self.storage[i].kill();
            self.storage[i].set_state(EventLifecycle::Destroyed);
        }
    }

    /// Acquires an event slot in O(1) without heap allocations.
    pub fn acquire(&mut self) -> *mut RustEvent {
        if self.free_count == 0 {
            return std::ptr::null_mut(); // CPU protection: bounded limit reached safely (Rule 11)
        }

        self.free_count -= 1;
        let index = self.free_indices[self.free_count] as usize;
        &mut self.storage[index] as *mut RustEvent
    }

    /// Releases an event slot back to the pool in O(1).
    pub fn release(&mut self, ptr: *mut RustEvent) {
        if ptr.is_null() || self.storage.is_empty() {
            return;
        }

        let base = self.storage.as_ptr();
        let offset = unsafe { ptr.offset_from(base) };

        if offset < 0 || (offset as usize) >= self.max_events {
            return;
        }

        let index = offset as usize;
        unsafe {
            (*ptr).kill();
            (*ptr).set_state(EventLifecycle::Destroyed);
        }

        if self.free_count < self.max_events {
            self.free_indices[self.free_count] = index as u32;
            self.free_count += 1;
        }
    }

    #[inline]
    pub fn capacity(&self) -> usize {
        self.max_events
    }

    #[inline]
    pub fn active_count(&self) -> usize {
        self.max_events.saturating_sub(self.free_count)
    }

    #[inline]
    pub fn available_count(&self) -> usize {
        self.free_count
    }

    #[inline]
    pub fn get_event_at(&mut self, index: usize) -> *mut RustEvent {
        if index < self.storage.len() {
            &mut self.storage[index] as *mut RustEvent
        } else {
            std::ptr::null_mut()
        }
    }

    #[inline]
    pub fn get_event_index(&self, ev: *const RustEvent) -> usize {
        if ev.is_null() || self.storage.is_empty() {
            return usize::MAX;
        }
        let base = self.storage.as_ptr();
        let offset = unsafe { ev.offset_from(base) };
        if offset >= 0 && (offset as usize) < self.max_events {
            offset as usize
        } else {
            usize::MAX
        }
    }
}

