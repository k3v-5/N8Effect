//! RustDualWorldEngine - Sovereign Hybrid Dual World Audio Engine Core (Roadmap Phase 4)
//! Developed for the N8Effect Audio Engine Architecture (Rules 1, 2, 3, 4, 7, 9, 14, 15, 17, 34, 35, 46, 47).
//! Unifies sovereign Dry World isolation, Live Transient/Pitch/Spectral Analysis,
//! Event World lifecycle synthesis, Lock-free DAG Graph execution with buffer reuse,
//! and True Peak anti-clip Master mixing in a single high-performance Rust processing pass.
//! ZERO heap allocations in the audio processing thread.

use std::sync::atomic::{AtomicBool, Ordering};
use crate::buffer_pool::RustAudioBufferPool;
use crate::graph_engine::{rust_validate_and_topological_sort, RustGraphConnection, RustGraphPlanExchanger, MAX_GRAPH_NODES};
use crate::node_instance::{RustNodeInstance, RustProcessorRegistry};
use crate::event_manager::RustEventManager;
use crate::analysis_engine::RustAnalysisEngine;
use crate::modulation_engine::RustModulationEngine;
use crate::adaptive_noise_floor_estimator::RustAdaptiveNoiseFloorEstimator;
use crate::common::{denormal_guard, fast_tanh};

pub struct RustDualWorldEngine {
    sample_rate: f64,
    max_block_size: usize,

    dry_buffer_l: Vec<f32>,
    dry_buffer_r: Vec<f32>,
    wet_buffer_l: Vec<f32>,
    wet_buffer_r: Vec<f32>,
    event_buffer_l: Vec<f32>,
    event_buffer_r: Vec<f32>,

    pub buffer_pool: RustAudioBufferPool,
    pub plan_exchanger: RustGraphPlanExchanger,
    pub registry: RustProcessorRegistry,
    pub nodes: Vec<RustNodeInstance>,

    pub event_manager: RustEventManager,
    pub analysis_engine: RustAnalysisEngine,
    pub modulation_engine: RustModulationEngine,
    pub noise_floor_estimator: RustAdaptiveNoiseFloorEstimator,

    target_dry_level: f32,
    current_dry_level: f32,
    target_wet_level: f32,
    current_wet_level: f32,

    editor_active: AtomicBool,
}

unsafe impl Send for RustDualWorldEngine {}

impl RustDualWorldEngine {
    pub const DEFAULT_POOL_CAPACITY: usize = 64;
    pub const KNEE_START: f32 = 0.85;              // -1.4 dBFS: Transparent bit-exact pass
    pub const CEILING: f32 = 0.98855;              // -0.1 dBFS True Peak ceiling (Rule 34)
    pub const HEADROOM: f32 = Self::CEILING - Self::KNEE_START; // 0.13855
    pub const INV_HEADROOM: f32 = 1.0 / Self::HEADROOM;
    pub const GAIN_SMOOTHING_ALPHA: f32 = 0.005;   // Anti-click volume smoothing (Rule 35)

    pub fn new(sample_rate: f64, max_block_size: usize) -> Self {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let bs = max_block_size.max(512);

        let mut engine = Self {
            sample_rate: sr,
            max_block_size: bs,
            dry_buffer_l: vec![0.0f32; bs],
            dry_buffer_r: vec![0.0f32; bs],
            wet_buffer_l: vec![0.0f32; bs],
            wet_buffer_r: vec![0.0f32; bs],
            event_buffer_l: vec![0.0f32; bs],
            event_buffer_r: vec![0.0f32; bs],
            buffer_pool: RustAudioBufferPool::new(Self::DEFAULT_POOL_CAPACITY, bs),
            plan_exchanger: RustGraphPlanExchanger::new(),
            registry: RustProcessorRegistry::new(),
            nodes: Vec::with_capacity(MAX_GRAPH_NODES),
            event_manager: RustEventManager::new(),
            analysis_engine: RustAnalysisEngine::new(sr, bs),
            modulation_engine: RustModulationEngine::new(),
            noise_floor_estimator: RustAdaptiveNoiseFloorEstimator::new(),
            target_dry_level: 1.0,
            current_dry_level: 1.0,
            target_wet_level: 1.0,
            current_wet_level: 1.0,
            editor_active: AtomicBool::new(false),
        };

        engine.prepare(sr, bs);
        engine
    }

    pub fn prepare(&mut self, sample_rate: f64, max_block_size: usize) {
        let sr = if sample_rate > 0.0 { sample_rate } else { 44100.0 };
        let bs = max_block_size.max(512);

        self.sample_rate = sr;
        self.max_block_size = bs;

        if self.dry_buffer_l.len() < bs {
            self.dry_buffer_l.resize(bs, 0.0);
            self.dry_buffer_r.resize(bs, 0.0);
            self.wet_buffer_l.resize(bs, 0.0);
            self.wet_buffer_r.resize(bs, 0.0);
            self.event_buffer_l.resize(bs, 0.0);
            self.event_buffer_r.resize(bs, 0.0);
        }

        self.buffer_pool.prepare(bs);
        self.analysis_engine.prepare(sr, bs);
        self.modulation_engine.prepare(sr);
        self.event_manager.prepare(sr, bs, 64);
        self.noise_floor_estimator.prepare(sr);

        for node in &mut self.nodes {
            node.processor.prepare(sr, bs);
            node.processor.reset();
        }

        self.current_dry_level = self.target_dry_level;
        self.current_wet_level = self.target_wet_level;
    }

    pub fn reset(&mut self) {
        self.dry_buffer_l.fill(0.0);
        self.dry_buffer_r.fill(0.0);
        self.wet_buffer_l.fill(0.0);
        self.wet_buffer_r.fill(0.0);
        self.event_buffer_l.fill(0.0);
        self.event_buffer_r.fill(0.0);

        self.buffer_pool.reset();
        self.analysis_engine.reset();
        self.modulation_engine.reset();
        self.event_manager.reset();
        self.noise_floor_estimator.reset();

        for node in &mut self.nodes {
            node.processor.reset();
        }

        self.current_dry_level = self.target_dry_level;
        self.current_wet_level = self.target_wet_level;
    }

    // Dry / Wet Level Control (Rules 1, 17, 35)
    #[inline(always)]
    pub fn set_dry_level(&mut self, level: f32) {
        self.target_dry_level = level.clamp(0.0, 2.0);
    }

    #[inline(always)]
    pub fn get_dry_level(&self) -> f32 {
        self.target_dry_level
    }

    #[inline(always)]
    pub fn set_wet_level(&mut self, level: f32) {
        self.target_wet_level = level.clamp(0.0, 2.0);
    }

    #[inline(always)]
    pub fn get_wet_level(&self) -> f32 {
        self.target_wet_level
    }

    #[inline(always)]
    pub fn set_editor_active(&self, active: bool) {
        self.editor_active.store(active, Ordering::Relaxed);
        self.event_manager.set_editor_active(active);
    }

    #[inline(always)]
    pub fn is_editor_active(&self) -> bool {
        self.editor_active.load(Ordering::Relaxed)
    }

    // Node graph manipulation (Rules 5, 19, 20)
    pub fn add_node(&mut self, node_id: u32, type_id: u32) -> bool {
        if self.nodes.iter().any(|n| n.node_id == node_id) {
            return false;
        }

        let mut proc = match self.registry.create_node(type_id) {
            Some(p) => p,
            None => return false,
        };

        proc.prepare(self.sample_rate, self.max_block_size);
        proc.reset();

        self.nodes.push(RustNodeInstance::new(node_id, type_id, proc));
        true
    }

    pub fn remove_node(&mut self, node_id: u32) -> bool {
        if let Some(pos) = self.nodes.iter().position(|n| n.node_id == node_id) {
            self.nodes.remove(pos);
            true
        } else {
            false
        }
    }

    pub fn set_node_bypassed(&mut self, node_id: u32, bypassed: bool) {
        if let Some(node) = self.nodes.iter_mut().find(|n| n.node_id == node_id) {
            node.is_bypassed = bypassed;
        }
    }

    pub fn set_node_parameter(&mut self, node_id: u32, param_id: u32, value: f32) {
        if let Some(node) = self.nodes.iter_mut().find(|n| n.node_id == node_id) {
            node.processor.set_parameter(param_id, value);
        }
    }

    pub fn get_node_parameter(&self, node_id: u32, param_id: u32) -> f32 {
        if let Some(node) = self.nodes.iter().find(|n| n.node_id == node_id) {
            node.processor.get_parameter(param_id)
        } else {
            0.0
        }
    }

    /// Compiles and atomically swaps DAG execution plan without audio thread locks (Rules 28, 29, 30).
    pub fn update_graph_topology(
        &mut self,
        node_ids: &[u32],
        connections: &[RustGraphConnection],
        bypassed_flags: &[bool],
    ) -> bool {
        let n = node_ids.len();
        if n == 0 {
            self.plan_exchanger.compile_standby_plan(&[], &[], &[]);
            self.plan_exchanger.commit_standby_plan();
            return true;
        }

        let mut sorted = vec![0u32; n];
        if !rust_validate_and_topological_sort(node_ids, connections, &mut sorted) {
            return false; // Cycle detected or invalid connections (Rule 29)
        }

        self.plan_exchanger.compile_standby_plan(&sorted, connections, bypassed_flags);
        self.plan_exchanger.commit_standby_plan();
        true
    }

    /// Master True Peak Anti-Clip Padé Tanh Knee Limiter (Rules 17, 34, 47)
    #[inline(always)]
    fn master_anti_clip(x: f32) -> f32 {
        if x > Self::KNEE_START {
            Self::KNEE_START + Self::HEADROOM * fast_tanh((x - Self::KNEE_START) * Self::INV_HEADROOM)
        } else if x < -Self::KNEE_START {
            -Self::KNEE_START - Self::HEADROOM * fast_tanh((-x - Self::KNEE_START) * Self::INV_HEADROOM)
        } else {
            x
        }
    }

    /// Unified Real-Time Audio Block Processing (Rules 1, 2, 4, 9, 17, 34, 35, 47).
    /// Executes Analysis, Dry Isolation, Event Synthesis, Graph Execution, and Master Mixing.
    pub fn process(
        &mut self,
        in_l: *const f32,
        in_r: *const f32,
        out_l: *mut f32,
        out_r: *mut f32,
        num_samples: usize,
    ) {
        if in_l.is_null() || out_l.is_null() || num_samples == 0 {
            return;
        }

        let limit = num_samples.min(self.max_block_size);
        let in_r_ptr = if in_r.is_null() { in_l } else { in_r };
        let out_r_ptr = if out_r.is_null() { out_l } else { out_r };

        // 0. ANALYSIS STAGE (Rules 1, 7)
        let in_channels = [in_l, in_r_ptr];
        self.analysis_engine.process(in_channels.as_ptr(), 2, limit as u32);
        let snap = self.analysis_engine.get_snapshot();

        // Feed modulation matrix analysis sources
        self.modulation_engine.update_analysis_sources(
            snap.onset_strength,
            snap.pitch_normalized,
            snap.spectral_centroid,
            snap.spectral_flux,
        );

        // 1. DRY WORLD: Pure 100% bit-exact copy of input signal (Rule 1 & 17)
        unsafe {
            std::ptr::copy_nonoverlapping(in_l, self.dry_buffer_l.as_mut_ptr(), limit);
            std::ptr::copy_nonoverlapping(in_r_ptr, self.dry_buffer_r.as_mut_ptr(), limit);
        }

        // 2. EVENT WORLD:
        let in_l_slice = unsafe { std::slice::from_raw_parts(in_l, limit) };
        let in_r_slice = unsafe { std::slice::from_raw_parts(in_r_ptr, limit) };
        self.event_manager.capture_input_audio(in_l_slice, in_r_slice, limit);

        // Source energy calculation & Adaptive Noise Floor Tracking (Rules 3, 27)
        let mut energy_l = 0.0f32;
        let mut energy_r = 0.0f32;
        for s in 0..limit {
            energy_l += denormal_guard(in_l_slice[s]).abs();
            energy_r += denormal_guard(in_r_slice[s]).abs();
        }
        let source_energy = (energy_l + energy_r) / (limit as f32 * 2.0);
        self.noise_floor_estimator.update_block(source_energy, limit as u32);
        let source_silent = self.noise_floor_estimator.is_source_silent();

        // Render active events into eventBuffer
        let mut event_in_l = in_l;
        let mut event_in_r = in_r_ptr;

        if self.event_manager.get_active_event_count() > 0 {
            unsafe {
                std::ptr::copy_nonoverlapping(in_l, self.event_buffer_l.as_mut_ptr(), limit);
                std::ptr::copy_nonoverlapping(in_r_ptr, self.event_buffer_r.as_mut_ptr(), limit);
            }

            self.event_manager.render(
                &mut self.event_buffer_l[..limit],
                &mut self.event_buffer_r[..limit],
                limit,
                source_energy,
                if source_silent { 1 } else { 0 },
            );

            event_in_l = self.event_buffer_l.as_ptr();
            event_in_r = self.event_buffer_r.as_ptr();
        }

        // 3. GRAPH EXECUTION (Wet World): Process DAG in wet buffers without touching Dry (Rules 4, 9, 47)
        self.wet_buffer_l[..limit].fill(0.0);
        self.wet_buffer_r[..limit].fill(0.0);
        self.buffer_pool.release_all();

        let plan = self.plan_exchanger.get_active_plan();
        let total_steps = plan.step_count;

        if total_steps > 0 {
            if !plan.has_explicit_connections {
                // Sequential ping-pong execution chain
                if let (Some(slot_a), Some(slot_b)) = (self.buffer_pool.acquire(), self.buffer_pool.acquire()) {
                    let (a_l, a_r) = self.buffer_pool.get_slot_ptrs(slot_a).unwrap();
                    unsafe {
                        std::ptr::copy_nonoverlapping(event_in_l, a_l, limit);
                        std::ptr::copy_nonoverlapping(event_in_r, a_r, limit);
                    }

                    let mut curr_in = slot_a;
                    let mut curr_out = slot_b;

                    for s in 0..total_steps {
                        let step = &plan.steps[s];
                        if let Some(node) = self.nodes.iter_mut().find(|n| n.node_id == step.node_id) {
                            let (in_ptrs_l, in_ptrs_r) = self.buffer_pool.get_slot_ptrs(curr_in).unwrap();
                            let (out_ptrs_l, out_ptrs_r) = self.buffer_pool.get_slot_ptrs(curr_out).unwrap();

                            if step.is_bypassed || node.is_bypassed {
                                unsafe {
                                    std::ptr::copy_nonoverlapping(in_ptrs_l, out_ptrs_l, limit);
                                    std::ptr::copy_nonoverlapping(in_ptrs_r, out_ptrs_r, limit);
                                }
                            } else {
                                node.processor.process(
                                    in_ptrs_l, in_ptrs_r,
                                    out_ptrs_l, out_ptrs_r,
                                    limit,
                                    std::ptr::null(), std::ptr::null(),
                                    std::ptr::null(), std::ptr::null(),
                                );
                            }
                            std::mem::swap(&mut curr_in, &mut curr_out);
                        }
                    }

                    let (final_l, final_r) = self.buffer_pool.get_slot_ptrs(curr_in).unwrap();
                    unsafe {
                        std::ptr::copy_nonoverlapping(final_l, self.wet_buffer_l.as_mut_ptr(), limit);
                        std::ptr::copy_nonoverlapping(final_r, self.wet_buffer_r.as_mut_ptr(), limit);
                    }
                }
            } else {
                // Full DAG modular graph dispatch with parallel fan-in and sidechains
                let max_steps = total_steps.min(64);
                let mut step_slots: [Option<usize>; 64] = [None; 64];
                for i in 0..max_steps {
                    step_slots[i] = self.buffer_pool.acquire();
                }

                for i in 0..max_steps {
                    let step = &plan.steps[i];
                    let out_slot = match step_slots[i] { Some(s) => s, None => break };
                    let (out_buf_l, out_buf_r) = self.buffer_pool.get_slot_ptrs(out_slot).unwrap();

                    let mut in_l_ptr = std::ptr::null();
                    let mut in_r_ptr = std::ptr::null();

                    if step.is_root {
                        in_l_ptr = event_in_l;
                        in_r_ptr = event_in_r;
                    } else if step.num_predecessors == 1 {
                        let pred_idx = step.predecessor_step_indices[0];
                        if pred_idx >= 0 && (pred_idx as usize) < max_steps {
                            if let Some(p_slot) = step_slots[pred_idx as usize] {
                                let (pl, pr) = self.buffer_pool.get_slot_ptrs(p_slot).unwrap();
                                in_l_ptr = pl;
                                in_r_ptr = pr;
                            }
                        }
                    } else if step.num_predecessors > 1 {
                        // Fan-in / Merge: acquire accumulator buffer from pool
                        if let Some(mix_slot) = self.buffer_pool.acquire() {
                            let (mix_l, mix_r) = self.buffer_pool.get_slot_ptrs(mix_slot).unwrap();
                            unsafe {
                                std::slice::from_raw_parts_mut(mix_l, limit).fill(0.0);
                                std::slice::from_raw_parts_mut(mix_r, limit).fill(0.0);

                                for p_num in 0..(step.num_predecessors.min(8) as usize) {
                                    let pred_idx = step.predecessor_step_indices[p_num];
                                    if pred_idx >= 0 && (pred_idx as usize) < max_steps {
                                        if let Some(p_slot) = step_slots[pred_idx as usize] {
                                            let (pl, pr) = self.buffer_pool.get_slot_ptrs(p_slot).unwrap();
                                            for s in 0..limit {
                                                *mix_l.add(s) += *pl.add(s);
                                                *mix_r.add(s) += *pr.add(s);
                                            }
                                        }
                                    }
                                }
                            }
                            in_l_ptr = mix_l;
                            in_r_ptr = mix_r;
                        }
                    }

                    // Sidechain & Audio-rate mod resolution
                    let mut sc_l_ptr = std::ptr::null();
                    let mut sc_r_ptr = std::ptr::null();
                    if step.sidechain_step_index >= 0 && (step.sidechain_step_index as usize) < max_steps {
                        if let Some(sc_slot) = step_slots[step.sidechain_step_index as usize] {
                            let (scl, scr) = self.buffer_pool.get_slot_ptrs(sc_slot).unwrap();
                            sc_l_ptr = scl;
                            sc_r_ptr = scr;
                        }
                    }

                    let mut arm_l_ptr = std::ptr::null();
                    let mut arm_r_ptr = std::ptr::null();
                    if step.audio_rate_mod_step_index >= 0 && (step.audio_rate_mod_step_index as usize) < max_steps {
                        if let Some(arm_slot) = step_slots[step.audio_rate_mod_step_index as usize] {
                            let (arml, armr) = self.buffer_pool.get_slot_ptrs(arm_slot).unwrap();
                            arm_l_ptr = arml;
                            arm_r_ptr = armr;
                        }
                    }

                    if let Some(node) = self.nodes.iter_mut().find(|n| n.node_id == step.node_id) {
                        if step.is_bypassed || node.is_bypassed {
                            unsafe {
                                if !in_l_ptr.is_null() {
                                    std::ptr::copy_nonoverlapping(in_l_ptr, out_buf_l, limit);
                                } else {
                                    std::slice::from_raw_parts_mut(out_buf_l, limit).fill(0.0);
                                }
                                if !in_r_ptr.is_null() {
                                    std::ptr::copy_nonoverlapping(in_r_ptr, out_buf_r, limit);
                                } else {
                                    std::slice::from_raw_parts_mut(out_buf_r, limit).fill(0.0);
                                }
                            }
                        } else {
                            node.processor.process(
                                in_l_ptr, in_r_ptr,
                                out_buf_l, out_buf_r,
                                limit,
                                sc_l_ptr, sc_r_ptr,
                                arm_l_ptr, arm_r_ptr,
                            );
                        }
                    }
                }

                // Accumulate leaf node outputs into wet_buffer
                for i in 0..max_steps {
                    let step = &plan.steps[i];
                    if step.is_leaf {
                        if let Some(out_slot) = step_slots[i] {
                            let (leaf_l, leaf_r) = self.buffer_pool.get_slot_ptrs(out_slot).unwrap();
                            unsafe {
                                for s in 0..limit {
                                    self.wet_buffer_l[s] += *leaf_l.add(s);
                                    self.wet_buffer_r[s] += *leaf_r.add(s);
                                }
                            }
                        }
                    }
                }
            }
        }

        self.buffer_pool.release_all();

        // 4. MASTER STAGE: Clean Dry/Wet mix with anti-click smoothing & Padé True Peak Anti-Clip (Rules 17, 34, 35)
        let alpha = Self::GAIN_SMOOTHING_ALPHA;
        for s in 0..limit {
            self.current_dry_level += alpha * (self.target_dry_level - self.current_dry_level);
            self.current_wet_level += alpha * (self.target_wet_level - self.current_wet_level);

            let d_l = denormal_guard(self.dry_buffer_l[s]) * self.current_dry_level;
            let d_r = denormal_guard(self.dry_buffer_r[s]) * self.current_dry_level;
            let w_l = denormal_guard(self.wet_buffer_l[s]) * self.current_wet_level;
            let w_r = denormal_guard(self.wet_buffer_r[s]) * self.current_wet_level;

            let sum_l = d_l + w_l;
            let sum_r = d_r + w_r;

            unsafe {
                *out_l.add(s) = Self::master_anti_clip(sum_l);
                *out_r_ptr.add(s) = Self::master_anti_clip(sum_r);
            }
        }
    }
}
