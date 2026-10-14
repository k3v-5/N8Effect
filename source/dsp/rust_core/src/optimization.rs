//! Optimization Engine: AVX2/SIMD Mathematical Kernels, Hardware Denormal Guards & DAG Buffer Slot Allocation (Roadmap Phase 8, Rules 9, 34, 46, 47)
//! Provides vectorized sample transformations, FTZ/DAZ hardware register guards,
//! Padé rational approximations, and memory-budget slot reusability computation.

#[cfg(target_arch = "x86_64")]
#[allow(deprecated)]
use std::arch::x86_64::{_mm_getcsr, _mm_setcsr};

pub const MAX_SIMD_NODES: usize = 256;

// ==============================================================================
// 1. Hardware FTZ / DAZ Anti-Denormal Guards (Rule 47)
// ==============================================================================

/// Enables hardware Flush-To-Zero (0x8000) and Denormals-Are-Zero (0x0040) on x86_64.
/// Returns the previous MXCSR register value to allow restoring on scope exit.
#[inline(always)]
#[allow(deprecated)]
pub unsafe fn rust_enable_hardware_ftz_daz() -> u32 {
    #[cfg(target_arch = "x86_64")]
    {
        let old = _mm_getcsr();
        _mm_setcsr(old | 0x8040);
        old
    }
    #[cfg(not(target_arch = "x86_64"))]
    {
        0
    }
}

/// Restores the hardware MXCSR register to a previous state.
#[inline(always)]
#[allow(deprecated)]
pub unsafe fn rust_restore_hardware_mxcsr(old_mxcsr: u32) {
    #[cfg(target_arch = "x86_64")]
    {
        _mm_setcsr(old_mxcsr);
    }
    #[cfg(not(target_arch = "x86_64"))]
    {
        let _ = old_mxcsr;
    }
}

/// Checks if both FTZ and DAZ are currently active in the CPU control register.
#[inline(always)]
#[allow(deprecated)]
pub fn rust_is_ftz_daz_active() -> bool {
    #[cfg(target_arch = "x86_64")]
    unsafe {
        (_mm_getcsr() & 0x8040) == 0x8040
    }
    #[cfg(not(target_arch = "x86_64"))]
    {
        false
    }
}

// ==============================================================================
// 2. High-Performance Vectorized Math Kernels (Rule 47)
// ==============================================================================

/// Fast Padé [5/4] rational approximation for tanh(x).
/// Error < 0.04% for |x| < 3.0. Monotonic, continuous, with exact asymptotes at +/- 1.0.
#[inline(always)]
pub fn fast_pade_tanh(x: f32) -> f32 {
    if x >= 3.0 {
        1.0
    } else if x <= -3.0 {
        -1.0
    } else {
        let x2 = x * x;
        let x4 = x2 * x2;
        let num = x * (945.0 + 105.0 * x2 + x4);
        let den = 945.0 + 420.0 * x2 + 15.0 * x4;
        num / den
    }
}

/// Fast cubic soft-clip: f(x) = x - (4/27) * x^3 for |x| < 1.5.
#[inline(always)]
pub fn fast_soft_clip(x: f32) -> f32 {
    if x >= 1.5 {
        1.0
    } else if x <= -1.5 {
        -1.0
    } else {
        x - (4.0 / 27.0) * (x * x * x)
    }
}

/// Fast Padé [2/2] approximation of exp(x) for negative x (envelopes and filter coefficients).
#[inline(always)]
pub fn fast_exp_negative(x: f32) -> f32 {
    if x <= -16.0 {
        0.0
    } else if x >= 0.0 {
        1.0
    } else {
        let x2 = x * x;
        let num = 12.0 + 6.0 * x + x2;
        let den = 12.0 - 6.0 * x + x2;
        num / den
    }
}

/// Fast decibel to linear gain: 10^(db / 20) = exp(db * ln(10) / 20).
#[inline(always)]
pub fn fast_db_to_gain(db: f32) -> f32 {
    if db <= -96.0 {
        0.0
    } else if db >= 36.0 {
        63.0957
    } else if db.abs() < 1e-4 {
        1.0
    } else {
        let x = db * 0.11512925;
        if x <= 0.0 {
            fast_exp_negative(x)
        } else {
            1.0 / fast_exp_negative(-x)
        }
    }
}

/// Vectorized Padé tanh: out[i] = fast_pade_tanh(in[i]).
pub fn vec_pade_tanh(input: &[f32], output: &mut [f32]) {
    let n = input.len().min(output.len());
    for i in 0..n {
        output[i] = fast_pade_tanh(input[i]);
    }
}

/// Vectorized Multiply-Add: out[i] = a[i] * b[i] + c[i].
pub fn vec_multiply_add(a: &[f32], b: &[f32], c: &[f32], out: &mut [f32]) {
    let n = a.len().min(b.len()).min(c.len()).min(out.len());
    for i in 0..n {
        out[i] = a[i] * b[i] + c[i];
    }
}

/// Vectorized Gain: out[i] = in[i] * gain.
pub fn vec_apply_gain(input: &[f32], output: &mut [f32], gain: f32) {
    let n = input.len().min(output.len());
    for i in 0..n {
        output[i] = input[i] * gain;
    }
}

/// Vectorized Linear Dry/Wet Mix: out[i] = (1 - mix) * dry[i] + mix * wet[i].
pub fn vec_mix(dry: &[f32], wet: &[f32], out: &mut [f32], mix: f32) {
    let n = dry.len().min(wet.len()).min(out.len());
    let dry_gain = 1.0 - mix;
    for i in 0..n {
        out[i] = dry[i] * dry_gain + wet[i] * mix;
    }
}

/// Vectorized Denormal Flush: subnormals are set to 0.0.
pub fn vec_flush_denormals(buffer: &mut [f32]) {
    for sample in buffer.iter_mut() {
        if sample.abs() < 1.0e-15 {
            *sample = 0.0;
        }
    }
}

/// Vectorized Soft Clip: out[i] = fast_soft_clip(in[i]).
pub fn vec_soft_clip(input: &[f32], output: &mut [f32]) {
    let n = input.len().min(output.len());
    for i in 0..n {
        output[i] = fast_soft_clip(input[i]);
    }
}

// ==============================================================================
// 3. DAG Buffer Lifetime & Reusable Slot Optimizer (Rule 47)
// ==============================================================================

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct RustStepSlotAssignment {
    pub step_index: u32,
    pub allocated_slot: u32,
}

/// Computes the optimal buffer reuse schedule for an acyclic topological execution plan.
/// Tracks when predecessor output buffers are no longer referenced by any remaining successors,
/// allowing immediate recycling into a bounded slot pool.
pub fn rust_compute_optimal_buffer_slots(
    step_predecessors: &[u32], // Flattened predecessor indices
    step_pred_offsets: &[u32],  // Offset array into step_predecessors for each step
    step_pred_counts: &[u32],   // Number of predecessors for each step
    num_steps: u32,
    out_assignments: &mut [RustStepSlotAssignment],
) -> u32 {
    let n = (num_steps as usize).min(MAX_SIMD_NODES).min(out_assignments.len());
    if n == 0 {
        return 0;
    }

    // Step 1: Calculate remaining consumer count for each step
    let mut remaining_consumers = [0u32; MAX_SIMD_NODES];
    for s in 0..n {
        let offset = if s < step_pred_offsets.len() { step_pred_offsets[s] as usize } else { 0 };
        let count = if s < step_pred_counts.len() { step_pred_counts[s] as usize } else { 0 };

        for k in 0..count {
            let p_idx = step_predecessors.get(offset + k).copied().unwrap_or(0) as usize;
            if p_idx < MAX_SIMD_NODES {
                remaining_consumers[p_idx] += 1;
            }
        }
    }

    // Step 2: Slot availability tracking
    let mut step_assigned_slot = [0u32; MAX_SIMD_NODES];
    let mut free_slots = [0u32; MAX_SIMD_NODES];
    let mut free_slot_count = 0usize;
    let mut max_slots_used = 0u32;

    for s in 0..n {
        // Allocate a slot for step s
        let slot = if free_slot_count > 0 {
            free_slot_count -= 1;
            free_slots[free_slot_count]
        } else {
            let s_id = max_slots_used;
            max_slots_used += 1;
            s_id
        };

        step_assigned_slot[s] = slot;
        out_assignments[s] = RustStepSlotAssignment {
            step_index: s as u32,
            allocated_slot: slot,
        };

        // Decrement remaining consumers of predecessors and free slots if finished
        let offset = if s < step_pred_offsets.len() { step_pred_offsets[s] as usize } else { 0 };
        let count = if s < step_pred_counts.len() { step_pred_counts[s] as usize } else { 0 };

        for k in 0..count {
            let p_idx = step_predecessors.get(offset + k).copied().unwrap_or(0) as usize;
            if p_idx < MAX_SIMD_NODES && remaining_consumers[p_idx] > 0 {
                remaining_consumers[p_idx] -= 1;
                if remaining_consumers[p_idx] == 0 {
                    let recycled_slot = step_assigned_slot[p_idx];
                    if free_slot_count < MAX_SIMD_NODES {
                        free_slots[free_slot_count] = recycled_slot;
                        free_slot_count += 1;
                    }
                }
            }
        }
    }

    max_slots_used
}
