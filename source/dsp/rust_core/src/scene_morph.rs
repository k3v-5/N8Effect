//! Scene Snapshot, Continuous Anti-Click Morphing & Smart Acoustic Randomizer (Phase 7, Rules 7, 11, 12, 21, 35, 46, 47)
//! High-performance real-time parameter interpolation, 4-corner XY vector morphing,
//! and procedural acoustically-guarded patch mutation with zero heap allocations.

pub const MAX_SCENE_NODES: usize = 256;
pub const MAX_NODE_PARAMS: usize = 32;
pub const NUM_MACROS: usize = 8;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct RustParamEntry {
    pub param_id: u32,
    pub value: f32,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct RustNodeParamEntry {
    pub node_id: u32,
    pub param_count: u32,
    pub params: [RustParamEntry; MAX_NODE_PARAMS],
}

impl Default for RustNodeParamEntry {
    fn default() -> Self {
        Self {
            node_id: 0,
            param_count: 0,
            params: [RustParamEntry { param_id: 0, value: 0.0 }; MAX_NODE_PARAMS],
        }
    }
}

#[repr(C)]
pub struct RustSceneSnapshot {
    pub is_captured: bool,
    pub node_count: u32,
    pub nodes: [RustNodeParamEntry; MAX_SCENE_NODES],
    pub macros: [f32; NUM_MACROS],
}

impl RustSceneSnapshot {
    pub fn new() -> Self {
        Self {
            is_captured: false,
            node_count: 0,
            nodes: [RustNodeParamEntry::default(); MAX_SCENE_NODES],
            macros: [0.5; NUM_MACROS],
        }
    }

    pub fn clear(&mut self) {
        self.is_captured = false;
        self.node_count = 0;
        self.macros.fill(0.5);
    }

    pub fn set_node_params(&mut self, node_id: u32, params: &[RustParamEntry]) {
        for i in 0..(self.node_count as usize) {
            if self.nodes[i].node_id == node_id {
                let count = params.len().min(MAX_NODE_PARAMS);
                self.nodes[i].param_count = count as u32;
                self.nodes[i].params[..count].copy_from_slice(&params[..count]);
                return;
            }
        }
        if (self.node_count as usize) < MAX_SCENE_NODES {
            let idx = self.node_count as usize;
            let count = params.len().min(MAX_NODE_PARAMS);
            self.nodes[idx].node_id = node_id;
            self.nodes[idx].param_count = count as u32;
            self.nodes[idx].params[..count].copy_from_slice(&params[..count]);
            self.node_count += 1;
        }
    }

    pub fn get_param(&self, node_id: u32, param_id: u32) -> Option<f32> {
        for i in 0..(self.node_count as usize) {
            if self.nodes[i].node_id == node_id {
                let n = self.nodes[i].param_count as usize;
                for j in 0..n {
                    if self.nodes[i].params[j].param_id == param_id {
                        return Some(self.nodes[i].params[j].value);
                    }
                }
                return None;
            }
        }
        None
    }
}

/// Continuous Anti-Click Morphing Engine (Rule 7, 21, 35).
/// Supports 1D linear morphing between Scene A and Scene B, as well as 4-corner XY vector morphing (A, B, C, D).
pub struct RustSceneMorphEngine {
    pub scene_a: Box<RustSceneSnapshot>,
    pub scene_b: Box<RustSceneSnapshot>,
    pub scene_c: Box<RustSceneSnapshot>,
    pub scene_d: Box<RustSceneSnapshot>,

    // 1D Linear Morphing
    target_morph_1d: f32,
    current_morph_1d: f32,

    // 2D XY Vector Morphing
    target_morph_x: f32,
    target_morph_y: f32,
    current_morph_x: f32,
    current_morph_y: f32,
}

impl RustSceneMorphEngine {
    pub fn new() -> Self {
        Self {
            scene_a: Box::new(RustSceneSnapshot::new()),
            scene_b: Box::new(RustSceneSnapshot::new()),
            scene_c: Box::new(RustSceneSnapshot::new()),
            scene_d: Box::new(RustSceneSnapshot::new()),
            target_morph_1d: 0.0,
            current_morph_1d: 0.0,
            target_morph_x: 0.0,
            target_morph_y: 0.0,
            current_morph_x: 0.0,
            current_morph_y: 0.0,
        }
    }

    pub fn set_morph_factor_1d(&mut self, t: f32) {
        self.target_morph_1d = t.clamp(0.0, 1.0);
    }

    pub fn get_target_morph_1d(&self) -> f32 {
        self.target_morph_1d
    }

    pub fn get_current_morph_1d(&self) -> f32 {
        self.current_morph_1d
    }

    pub fn set_morph_xy(&mut self, x: f32, y: f32) {
        self.target_morph_x = x.clamp(0.0, 1.0);
        self.target_morph_y = y.clamp(0.0, 1.0);
    }

    pub fn get_target_morph_x(&self) -> f32 {
        self.target_morph_x
    }

    pub fn get_target_morph_y(&self) -> f32 {
        self.target_morph_y
    }

    pub fn get_current_morph_x(&self) -> f32 {
        self.current_morph_x
    }

    pub fn get_current_morph_y(&self) -> f32 {
        self.current_morph_y
    }

    /// Advances exponential anti-click smoothing for 1D morphing (Rule 35).
    pub fn step_smoothing_1d(&mut self, smoothing_speed: f32) -> f32 {
        let alpha = smoothing_speed.clamp(0.001, 1.0);
        self.current_morph_1d += alpha * (self.target_morph_1d - self.current_morph_1d);
        self.current_morph_1d
    }

    /// Advances exponential anti-click smoothing for 2D XY vector morphing.
    pub fn step_smoothing_xy(&mut self, smoothing_speed: f32) -> (f32, f32) {
        let alpha = smoothing_speed.clamp(0.001, 1.0);
        self.current_morph_x += alpha * (self.target_morph_x - self.current_morph_x);
        self.current_morph_y += alpha * (self.target_morph_y - self.current_morph_y);
        (self.current_morph_x, self.current_morph_y)
    }

    /// Computes interpolated parameter value for a specific node and parameter under 1D morphing.
    pub fn interpolate_param_1d(&self, node_id: u32, param_id: u32, fallback_val: f32) -> f32 {
        let has_a = self.scene_a.is_captured;
        let has_b = self.scene_b.is_captured;

        if !has_a && !has_b {
            return fallback_val;
        }

        let val_a = self.scene_a.get_param(node_id, param_id);
        let val_b = self.scene_b.get_param(node_id, param_id);

        match (val_a, val_b) {
            (Some(a), Some(b)) => {
                let t = self.current_morph_1d;
                (1.0 - t) * a + t * b
            }
            (Some(a), None) => a,
            (None, Some(b)) => b,
            (None, None) => fallback_val,
        }
    }

    /// Computes interpolated macros for 1D morphing.
    pub fn interpolate_macros_1d(&self, out_macros: &mut [f32; NUM_MACROS]) {
        let has_a = self.scene_a.is_captured;
        let has_b = self.scene_b.is_captured;

        if has_a && has_b {
            let t = self.current_morph_1d;
            for i in 0..NUM_MACROS {
                out_macros[i] = (1.0 - t) * self.scene_a.macros[i] + t * self.scene_b.macros[i];
            }
        } else if has_a {
            out_macros.copy_from_slice(&self.scene_a.macros);
        } else if has_b {
            out_macros.copy_from_slice(&self.scene_b.macros);
        }
    }

    /// Computes 4-corner XY bilinear interpolation for a node parameter.
    /// A: (0, 0), B: (1, 0), C: (0, 1), D: (1, 1).
    /// Dynamically normalizes weights across captured scenes if not all 4 are captured.
    pub fn interpolate_param_xy(&self, node_id: u32, param_id: u32, fallback_val: f32) -> f32 {
        let x = self.current_morph_x;
        let y = self.current_morph_y;

        let has_a = self.scene_a.is_captured;
        let has_b = self.scene_b.is_captured;
        let has_c = self.scene_c.is_captured;
        let has_d = self.scene_d.is_captured;

        let mut w_a = if has_a { (1.0 - x) * (1.0 - y) } else { 0.0 };
        let mut w_b = if has_b { x * (1.0 - y) } else { 0.0 };
        let mut w_c = if has_c { (1.0 - x) * y } else { 0.0 };
        let mut w_d = if has_d { x * y } else { 0.0 };

        let total_w = w_a + w_b + w_c + w_d;
        if total_w < 1e-6 {
            return fallback_val;
        }
        w_a /= total_w;
        w_b /= total_w;
        w_c /= total_w;
        w_d /= total_w;

        let val_a = if has_a { self.scene_a.get_param(node_id, param_id).unwrap_or(fallback_val) } else { 0.0 };
        let val_b = if has_b { self.scene_b.get_param(node_id, param_id).unwrap_or(fallback_val) } else { 0.0 };
        let val_c = if has_c { self.scene_c.get_param(node_id, param_id).unwrap_or(fallback_val) } else { 0.0 };
        let val_d = if has_d { self.scene_d.get_param(node_id, param_id).unwrap_or(fallback_val) } else { 0.0 };

        w_a * val_a + w_b * val_b + w_c * val_c + w_d * val_d
    }

    /// Computes 4-corner XY bilinear interpolation for all 8 macros.
    pub fn interpolate_macros_xy(&self, out_macros: &mut [f32; NUM_MACROS]) {
        let x = self.current_morph_x;
        let y = self.current_morph_y;

        let has_a = self.scene_a.is_captured;
        let has_b = self.scene_b.is_captured;
        let has_c = self.scene_c.is_captured;
        let has_d = self.scene_d.is_captured;

        let mut w_a = if has_a { (1.0 - x) * (1.0 - y) } else { 0.0 };
        let mut w_b = if has_b { x * (1.0 - y) } else { 0.0 };
        let mut w_c = if has_c { (1.0 - x) * y } else { 0.0 };
        let mut w_d = if has_d { x * y } else { 0.0 };

        let total_w = w_a + w_b + w_c + w_d;
        if total_w < 1e-6 {
            return;
        }
        w_a /= total_w;
        w_b /= total_w;
        w_c /= total_w;
        w_d /= total_w;

        for i in 0..NUM_MACROS {
            let ma = if has_a { self.scene_a.macros[i] } else { 0.0 };
            let mb = if has_b { self.scene_b.macros[i] } else { 0.0 };
            let mc = if has_c { self.scene_c.macros[i] } else { 0.0 };
            let md = if has_d { self.scene_d.macros[i] } else { 0.0 };
            out_macros[i] = w_a * ma + w_b * mb + w_c * mc + w_d * md;
        }
    }
}

/// High-Performance Smart Procedural Acoustic Randomizer (Rules 11, 12, 34, 47).
pub struct RustSmartRandomizer {
    seed: u32,
}

impl RustSmartRandomizer {
    pub fn new(seed: u32) -> Self {
        let s = if seed != 0 { seed } else { 0x87654321 };
        Self { seed: s }
    }

    #[inline(always)]
    pub fn next_u32(&mut self) -> u32 {
        self.seed ^= self.seed << 13;
        self.seed ^= self.seed >> 17;
        self.seed ^= self.seed << 5;
        self.seed
    }

    #[inline(always)]
    pub fn uniform_f32(&mut self, min_val: f32, max_val: f32) -> f32 {
        let norm = (self.next_u32() & 0x7FFFFFFF) as f32 / 2147483648.0;
        min_val + norm * (max_val - min_val)
    }

    /// Perturbs a continuous parameter with intensity and acoustic protection clamps (Rules 11, 12, 34, 47).
    pub fn perturb_parameter(
        &mut self,
        current_val: f32,
        min_val: f32,
        max_val: f32,
        intensity: f32,
        is_feedback: bool,
        is_resonance: bool,
        is_drive: bool,
    ) -> f32 {
        let range = max_val - min_val;
        let delta = self.uniform_f32(-1.0, 1.0) * intensity * range;
        let mut candidate = current_val + delta;

        if is_feedback {
            // Guard against runaway feedback explosion (Rule 12)
            let safe_limit = 0.82f32.min(max_val);
            candidate = candidate.clamp(min_val, safe_limit);
        } else if is_resonance {
            // Guard against screeching auto-oscillation resonance peaks (Rule 34)
            let safe_limit = 14.0f32.min(max_val);
            candidate = candidate.clamp(min_val, safe_limit);
        } else if is_drive {
            // Guard against deafening saturation drive
            let safe_limit = (min_val + range * 0.85).min(max_val);
            candidate = candidate.clamp(min_val, safe_limit);
        } else {
            candidate = candidate.clamp(min_val, max_val);
        }

        candidate
    }

    /// Mutates an array of macros within [0.0, 1.0].
    pub fn mutate_macros(&mut self, macros: &mut [f32; NUM_MACROS], intensity: f32) {
        for m in macros.iter_mut() {
            let delta = self.uniform_f32(-intensity, intensity);
            *m = (*m + delta).clamp(0.0, 1.0);
        }
    }
}
