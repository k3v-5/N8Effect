//! RustContainerCore - SubGraph Container Engine (NodeType::Container / Rules 5, 6, 9, 28, 46, 47)
//! Developed for the N8Effect Hybrid C++ / Rust Audio Engine Architecture.
//! Encapsulates sub-graph execution routing, anti-click parameter smoothing,
//! dry/wet mixing, multi-channel passthrough, denormal eradication,
//! and topological sub-graph validation.
//! Zero allocations in audio thread, cacheline aligned, AVX2 SIMD friendly.

use crate::common::{clamp, denormal_guard};
use crate::graph_engine::{rust_validate_and_topological_sort, RustGraphConnection, MAX_GRAPH_NODES};

pub const CONTAINER_PARAM_MIX: u32 = 1;
pub const CONTAINER_PARAM_OUTPUT_GAIN: u32 = 2;

/// Cacheline-aligned SubGraph Container DSP Core
#[repr(align(64))]
pub struct RustContainerCore {
    sample_rate: f32,
    max_block_size: usize,

    target_mix: f32,
    current_mix: f32,
    target_gain: f32,
    current_gain: f32,
    alpha: f32,
}

unsafe impl Send for RustContainerCore {}
unsafe impl Sync for RustContainerCore {}

impl RustContainerCore {
    /// Smoothing filter coefficient (anti-click smoothing, Rule 35)
    pub const DEFAULT_SMOOTHING_ALPHA: f32 = 0.005;

    pub fn new(sample_rate: f32) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        Self {
            sample_rate: sr,
            max_block_size: 512,
            target_mix: 1.0,
            current_mix: 1.0,
            target_gain: 1.0,
            current_gain: 1.0,
            alpha: Self::DEFAULT_SMOOTHING_ALPHA,
        }
    }

    pub fn prepare(&mut self, sample_rate: f32, max_block_size: usize) {
        self.sample_rate = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        self.max_block_size = if max_block_size > 0 { max_block_size } else { 512 };
        self.current_mix = self.target_mix;
        self.current_gain = self.target_gain;
    }

    pub fn reset(&mut self) {
        self.current_mix = self.target_mix;
        self.current_gain = self.target_gain;
    }

    #[inline(always)]
    pub fn set_parameter(&mut self, param_id: u32, value: f32) {
        match param_id {
            CONTAINER_PARAM_MIX => {
                self.target_mix = clamp(value, 0.0, 1.0);
            }
            CONTAINER_PARAM_OUTPUT_GAIN => {
                self.target_gain = clamp(value, 0.0, 2.0);
            }
            _ => {}
        }
    }

    #[inline(always)]
    pub fn get_parameter(&self, param_id: u32) -> f32 {
        match param_id {
            CONTAINER_PARAM_MIX => self.target_mix,
            CONTAINER_PARAM_OUTPUT_GAIN => self.target_gain,
            _ => 0.0,
        }
    }

    /// Stereo passthrough processing when sub-graph is empty
    pub fn process_passthrough(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: usize,
    ) {
        if num_samples == 0 {
            return;
        }

        for s in 0..num_samples {
            self.current_gain += self.alpha * (self.target_gain - self.current_gain);

            let inl = if !in_l.is_null() {
                unsafe { *in_l.add(s) }
            } else {
                0.0
            };
            let inr = if !in_r.is_null() {
                unsafe { *in_r.add(s) }
            } else {
                inl
            };

            if !out_l.is_null() {
                unsafe { *out_l.add(s) = denormal_guard(inl * self.current_gain); }
            }
            if !out_r.is_null() {
                unsafe { *out_r.add(s) = denormal_guard(inr * self.current_gain); }
            }
        }
    }

    /// Stereo Dry/Wet and Gain Mix processing after sub-graph execution
    pub fn process_mix(
        &mut self,
        dry_l: *const f32,
        dry_r: *const f32,
        wet_l: *const f32,
        wet_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: usize,
    ) {
        if num_samples == 0 {
            return;
        }

        for s in 0..num_samples {
            self.current_mix += self.alpha * (self.target_mix - self.current_mix);
            self.current_gain += self.alpha * (self.target_gain - self.current_gain);

            let dl = if !dry_l.is_null() {
                unsafe { *dry_l.add(s) }
            } else {
                0.0
            };
            let dr = if !dry_r.is_null() {
                unsafe { *dry_r.add(s) }
            } else {
                dl
            };

            let wl = if !wet_l.is_null() {
                unsafe { *wet_l.add(s) }
            } else {
                0.0
            };
            let wr = if !wet_r.is_null() {
                unsafe { *wet_r.add(s) }
            } else {
                wl
            };

            let mixed_l = dl + self.current_mix * (wl - dl);
            let mixed_r = dr + self.current_mix * (wr - dr);

            if !out_l.is_null() {
                unsafe { *out_l.add(s) = denormal_guard(mixed_l * self.current_gain); }
            }
            if !out_r.is_null() {
                unsafe { *out_r.add(s) = denormal_guard(mixed_r * self.current_gain); }
            }
        }
    }

    /// Multi-channel passthrough processing when sub-graph is empty (ProcessContext compliant)
    pub fn process_channels_passthrough(
        &mut self,
        in_ptrs: *const *const f32,
        num_in: usize,
        out_ptrs: *mut *mut f32,
        num_out: usize,
        num_samples: usize,
    ) {
        if num_samples == 0 || out_ptrs.is_null() || num_out == 0 {
            return;
        }

        for s in 0..num_samples {
            self.current_gain += self.alpha * (self.target_gain - self.current_gain);

            for ch in 0..num_out {
                let out_ch_ptr = unsafe { *out_ptrs.add(ch) };
                if out_ch_ptr.is_null() {
                    continue;
                }

                let in_sample = if ch < num_in && !in_ptrs.is_null() {
                    let in_ch_ptr = unsafe { *in_ptrs.add(ch) };
                    if !in_ch_ptr.is_null() {
                        unsafe { *in_ch_ptr.add(s) }
                    } else {
                        0.0
                    }
                } else {
                    0.0
                };

                unsafe {
                    *out_ch_ptr.add(s) = denormal_guard(in_sample * self.current_gain);
                }
            }
        }
    }

    /// Multi-channel Dry/Wet and Gain Mix processing after sub-graph execution (ProcessContext compliant)
    pub fn process_channels_mix(
        &mut self,
        dry_ptrs: *const *const f32,
        num_dry: usize,
        wet_ptrs: *const *const f32,
        num_wet: usize,
        out_ptrs: *mut *mut f32,
        num_out: usize,
        num_samples: usize,
    ) {
        if num_samples == 0 || out_ptrs.is_null() || num_out == 0 {
            return;
        }

        for s in 0..num_samples {
            self.current_mix += self.alpha * (self.target_mix - self.current_mix);
            self.current_gain += self.alpha * (self.target_gain - self.current_gain);

            for ch in 0..num_out {
                let out_ch_ptr = unsafe { *out_ptrs.add(ch) };
                if out_ch_ptr.is_null() {
                    continue;
                }

                let dry = if ch < num_dry && !dry_ptrs.is_null() {
                    let dry_ch_ptr = unsafe { *dry_ptrs.add(ch) };
                    if !dry_ch_ptr.is_null() {
                        unsafe { *dry_ch_ptr.add(s) }
                    } else {
                        0.0
                    }
                } else {
                    0.0
                };

                let wet = if ch < num_wet && !wet_ptrs.is_null() {
                    let wet_ch_ptr = unsafe { *wet_ptrs.add(ch) };
                    if !wet_ch_ptr.is_null() {
                        unsafe { *wet_ch_ptr.add(s) }
                    } else {
                        0.0
                    }
                } else {
                    0.0
                };

                let mixed = dry + self.current_mix * (wet - dry);
                unsafe {
                    *out_ch_ptr.add(s) = denormal_guard(mixed * self.current_gain);
                }
            }
        }
    }

    /// Validates sub-graph topological order without allocations using Kahn's algorithm
    pub fn validate_subgraph(node_ids: &[u32], connections: &[RustGraphConnection]) -> bool {
        let mut sorted = [0u32; MAX_GRAPH_NODES];
        rust_validate_and_topological_sort(node_ids, connections, &mut sorted[..node_ids.len()])
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_container_core_lifecycle() {
        let mut container = RustContainerCore::new(44100.0);
        assert_eq!(container.get_parameter(CONTAINER_PARAM_MIX), 1.0);
        assert_eq!(container.get_parameter(CONTAINER_PARAM_OUTPUT_GAIN), 1.0);

        container.set_parameter(CONTAINER_PARAM_MIX, 0.75);
        container.set_parameter(CONTAINER_PARAM_OUTPUT_GAIN, 1.4);
        assert_eq!(container.get_parameter(CONTAINER_PARAM_MIX), 0.75);
        assert_eq!(container.get_parameter(CONTAINER_PARAM_OUTPUT_GAIN), 1.4);

        container.prepare(48000.0, 256);
        assert_eq!(container.current_mix, 0.75);
        assert_eq!(container.current_gain, 1.4);

        container.reset();
        assert_eq!(container.current_mix, 0.75);
        assert_eq!(container.current_gain, 1.4);
    }

    #[test]
    fn test_container_core_passthrough() {
        let mut container = RustContainerCore::new(44100.0);
        container.prepare(44100.0, 64);
        container.set_parameter(CONTAINER_PARAM_OUTPUT_GAIN, 1.5);
        container.reset();

        let in_l = vec![0.5f32; 64];
        let in_r = vec![0.25f32; 64];
        let mut out_l = vec![0.0f32; 64];
        let mut out_r = vec![0.0f32; 64];

        container.process_passthrough(
            in_l.as_ptr(),
            in_r.as_ptr(),
            out_l.as_mut_ptr(),
            out_r.as_mut_ptr(),
            64,
        );

        for i in 0..64 {
            assert!(!out_l[i].is_nan() && !out_l[i].is_infinite());
            assert!(!out_r[i].is_nan() && !out_r[i].is_infinite());
        }
        assert!((out_l[63] - 0.75).abs() < 1e-3);
        assert!((out_r[63] - 0.375).abs() < 1e-3);
    }

    #[test]
    fn test_container_core_dry_wet_mix() {
        let mut container = RustContainerCore::new(44100.0);
        container.prepare(44100.0, 128);
        container.set_parameter(CONTAINER_PARAM_MIX, 0.5);
        container.set_parameter(CONTAINER_PARAM_OUTPUT_GAIN, 1.0);
        container.reset();

        let dry_l = vec![1.0f32; 128];
        let dry_r = vec![1.0f32; 128];
        let wet_l = vec![0.0f32; 128];
        let wet_r = vec![0.0f32; 128];
        let mut out_l = vec![0.0f32; 128];
        let mut out_r = vec![0.0f32; 128];

        container.process_mix(
            dry_l.as_ptr(),
            dry_r.as_ptr(),
            wet_l.as_ptr(),
            wet_r.as_ptr(),
            out_l.as_mut_ptr(),
            out_r.as_mut_ptr(),
            128,
        );

        for i in 0..128 {
            assert!(!out_l[i].is_nan() && !out_l[i].is_infinite());
        }
        // At 50% mix, 1.0 dry and 0.0 wet should be ~0.5
        assert!((out_l[127] - 0.5).abs() < 1e-3);
        assert!((out_r[127] - 0.5).abs() < 1e-3);
    }

    #[test]
    fn test_container_core_channels_processing() {
        let mut container = RustContainerCore::new(44100.0);
        container.prepare(44100.0, 32);
        container.set_parameter(CONTAINER_PARAM_MIX, 1.0);
        container.set_parameter(CONTAINER_PARAM_OUTPUT_GAIN, 1.2);
        container.reset();

        let d0 = vec![0.5f32; 32];
        let d1 = vec![0.5f32; 32];
        let w0 = vec![0.8f32; 32];
        let w1 = vec![0.8f32; 32];
        let mut o0 = vec![0.0f32; 32];
        let mut o1 = vec![0.0f32; 32];

        let dry_ptrs = [d0.as_ptr(), d1.as_ptr()];
        let wet_ptrs = [w0.as_ptr(), w1.as_ptr()];
        let mut out_ptrs = [o0.as_mut_ptr(), o1.as_mut_ptr()];

        container.process_channels_mix(
            dry_ptrs.as_ptr(),
            2,
            wet_ptrs.as_ptr(),
            2,
            out_ptrs.as_mut_ptr(),
            2,
            32,
        );

        for s in 0..32 {
            assert!(!o0[s].is_nan() && !o0[s].is_infinite());
            assert!(!o1[s].is_nan() && !o1[s].is_infinite());
        }
        assert!((o0[31] - (0.8 * 1.2)).abs() < 1e-3);
        assert!((o1[31] - (0.8 * 1.2)).abs() < 1e-3);
    }

    #[test]
    fn test_container_validate_subgraph() {
        let node_ids = [1, 2, 3];
        let connections = [
            RustGraphConnection { id: 1, source_node_id: 1, source_pin_id: 2, dest_node_id: 2, dest_pin_id: 1 },
            RustGraphConnection { id: 2, source_node_id: 2, source_pin_id: 2, dest_node_id: 3, dest_pin_id: 1 },
        ];

        let valid = RustContainerCore::validate_subgraph(&node_ids, &connections);
        assert!(valid);

        let cycle_connections = [
            RustGraphConnection { id: 1, source_node_id: 1, source_pin_id: 2, dest_node_id: 2, dest_pin_id: 1 },
            RustGraphConnection { id: 2, source_node_id: 2, source_pin_id: 2, dest_node_id: 3, dest_pin_id: 1 },
            RustGraphConnection { id: 3, source_node_id: 3, source_pin_id: 2, dest_node_id: 1, dest_pin_id: 1 },
        ];
        let cycle_valid = RustContainerCore::validate_subgraph(&node_ids, &cycle_connections);
        assert!(!cycle_valid);
    }
}
