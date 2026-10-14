//! RustAudioBufferPool - Sovereign Lock-Free Audio Buffer Pool (Rules 9, 10, 11, 14, 46, 47)
//! Developed for the N8Effect Hybrid C++ / Rust Audio Engine Architecture.
//! Features 64-byte cacheline alignment for AVX2/AVX-512, lock-free bitmask allocation
//! via atomic bit-scan (trailing_zeros), bounded capacity, and zero heap allocations
//! during real-time processing.

use std::sync::atomic::{AtomicU64, Ordering};

pub const MAX_POOL_BUFFERS: usize = 64;
pub const MAX_CHANNELS: usize = 2;
pub const DEFAULT_MAX_BLOCK_SIZE: usize = 2048;

/// 64-byte cacheline-aligned stereo audio buffer
#[repr(align(64))]
pub struct AlignedAudioBuffer {
    data: Vec<f32>,
    max_block_size: usize,
}

impl AlignedAudioBuffer {
    pub fn new(max_block_size: usize) -> Self {
        let size = max_block_size.max(512);
        Self {
            data: vec![0.0f32; size * MAX_CHANNELS],
            max_block_size: size,
        }
    }

    pub fn resize_block(&mut self, max_block_size: usize) {
        let size = max_block_size.max(512);
        self.max_block_size = size;
        self.data.resize(size * MAX_CHANNELS, 0.0);
        self.data.fill(0.0);
    }

    #[inline(always)]
    pub fn clear(&mut self, num_samples: usize) {
        let limit = (num_samples * MAX_CHANNELS).min(self.data.len());
        self.data[..limit].fill(0.0);
    }

    #[inline(always)]
    pub fn channel_ptrs_mut(&mut self) -> (*mut f32, *mut f32) {
        let ptr = self.data.as_mut_ptr();
        (ptr, unsafe { ptr.add(self.max_block_size) })
    }

    #[inline(always)]
    pub fn channel_ptrs(&self) -> (*const f32, *const f32) {
        let ptr = self.data.as_ptr();
        (ptr, unsafe { ptr.add(self.max_block_size) })
    }
}

/// Thread-safe, lock-free preallocated audio buffer pool for real-time DAG execution.
pub struct RustAudioBufferPool {
    buffers: Vec<AlignedAudioBuffer>,
    allocated_mask: AtomicU64,
    capacity: usize,
    max_block_size: usize,
}

unsafe impl Send for RustAudioBufferPool {}
unsafe impl Sync for RustAudioBufferPool {}

impl RustAudioBufferPool {
    pub fn new(capacity: usize, max_block_size: usize) -> Self {
        let cap = capacity.clamp(1, MAX_POOL_BUFFERS);
        let bs = max_block_size.max(512);

        let mut buffers = Vec::with_capacity(cap);
        for _ in 0..cap {
            buffers.push(AlignedAudioBuffer::new(bs));
        }

        Self {
            buffers,
            allocated_mask: AtomicU64::new(0),
            capacity: cap,
            max_block_size: bs,
        }
    }

    pub fn prepare(&mut self, max_block_size: usize) {
        self.max_block_size = max_block_size.max(512);
        for buf in &mut self.buffers {
            buf.resize_block(self.max_block_size);
        }
        self.reset();
    }

    pub fn reset(&mut self) {
        self.allocated_mask.store(0, Ordering::Release);
        for buf in &mut self.buffers {
            buf.data.fill(0.0);
        }
    }

    /// Lock-free acquisition of an audio buffer slot in O(1) via CAS on bitmask.
    #[inline]
    pub fn acquire(&self) -> Option<usize> {
        let cap_mask = if self.capacity == 64 {
            !0u64
        } else {
            (1u64 << self.capacity) - 1
        };

        let mut current = self.allocated_mask.load(Ordering::Relaxed);
        loop {
            let available = (!current) & cap_mask;
            if available == 0 {
                return None; // Pool exhausted (Rule 11: Controlled limit behavior)
            }

            let slot = available.trailing_zeros() as usize;
            let next = current | (1u64 << slot);

            match self.allocated_mask.compare_exchange_weak(
                current,
                next,
                Ordering::AcqRel,
                Ordering::Relaxed,
            ) {
                Ok(_) => return Some(slot),
                Err(actual) => current = actual,
            }
        }
    }

    /// Lock-free release of an audio buffer slot back to the pool in O(1).
    #[inline]
    pub fn release(&self, slot: usize) {
        if slot < self.capacity {
            self.allocated_mask.fetch_and(!(1u64 << slot), Ordering::Release);
        }
    }

    #[inline]
    pub fn release_all(&self) {
        self.allocated_mask.store(0, Ordering::Release);
    }

    #[inline]
    pub fn is_allocated(&self, slot: usize) -> bool {
        if slot < self.capacity {
            (self.allocated_mask.load(Ordering::Relaxed) & (1u64 << slot)) != 0
        } else {
            false
        }
    }

    #[inline]
    pub fn available_count(&self) -> usize {
        let allocated = self.allocated_mask.load(Ordering::Relaxed).count_ones() as usize;
        self.capacity.saturating_sub(allocated)
    }

    #[inline]
    pub fn capacity(&self) -> usize {
        self.capacity
    }

    #[inline]
    pub fn get_slot_ptrs(&self, slot: usize) -> Option<(*mut f32, *mut f32)> {
        if slot < self.capacity {
            let ptr = self.buffers[slot].data.as_ptr() as *mut f32;
            Some((ptr, unsafe { ptr.add(self.max_block_size) }))
        } else {
            None
        }
    }

    #[inline]
    pub fn get_channel_ptrs_mut(&mut self, slot: usize) -> Option<(*mut f32, *mut f32)> {
        if slot < self.capacity {
            Some(self.buffers[slot].channel_ptrs_mut())
        } else {
            None
        }
    }

    #[inline]
    pub fn get_channel_ptrs(&self, slot: usize) -> Option<(*const f32, *const f32)> {
        if slot < self.capacity {
            Some(self.buffers[slot].channel_ptrs())
        } else {
            None
        }
    }

    #[inline]
    pub fn clear_buffer(&mut self, slot: usize, num_samples: usize) {
        if slot < self.capacity {
            self.buffers[slot].clear(num_samples);
        }
    }
}
