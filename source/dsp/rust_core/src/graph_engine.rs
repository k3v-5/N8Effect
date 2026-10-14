//! Graph Engine & DAG Topological Compiler (Roadmap Phase 6, Rules 4, 15, 28, 29, 30, 46, 47)
//! High-performance graph validation, predictive cycle detection, Kahn's topological sort,
//! auto-layout depth computation, and atomic double-buffered plan swapping.

use std::sync::atomic::{AtomicPtr, Ordering};

pub const MAX_GRAPH_NODES: usize = 256;
pub const MAX_GRAPH_CONNECTIONS: usize = 1024;
pub const SIDECHAIN_PIN_ID: u32 = 100;
pub const AUDIO_RATE_MOD_PIN_ID: u32 = 200;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct RustGraphConnection {
    pub id: u32,
    pub source_node_id: u32,
    pub source_pin_id: u32,
    pub dest_node_id: u32,
    pub dest_pin_id: u32,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct RustNodeLayout {
    pub node_id: u32,
    pub depth: u32,
    pub pos_x: f32,
    pub pos_y: f32,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct RustExecutionStepDesc {
    pub node_id: u32,
    pub is_bypassed: bool,
    pub is_root: bool,
    pub is_leaf: bool,
    pub sidechain_step_index: i32,
    pub audio_rate_mod_step_index: i32,
    pub num_predecessors: u32,
    pub num_successors: u32,
}

impl Default for RustExecutionStepDesc {
    fn default() -> Self {
        Self {
            node_id: 0,
            is_bypassed: false,
            is_root: true,
            is_leaf: true,
            sidechain_step_index: -1,
            audio_rate_mod_step_index: -1,
            num_predecessors: 0,
            num_successors: 0,
        }
    }
}

/// Predictive cycle detection: checks if adding a directed edge (src -> dest) creates a cycle in the DAG (Rules 4, 15, 28, 29).
/// Fast BFS from dest looking for src. Zero heap allocations when node count <= MAX_GRAPH_NODES.
pub fn rust_would_create_cycle(
    src_node: u32,
    dest_node: u32,
    node_ids: &[u32],
    connections: &[RustGraphConnection],
) -> bool {
    if src_node == dest_node || src_node == 0 || dest_node == 0 {
        return true;
    }

    let mut has_src = false;
    let mut has_dest = false;
    for &id in node_ids {
        if id == src_node { has_src = true; }
        if id == dest_node { has_dest = true; }
    }
    if !has_src || !has_dest {
        return true;
    }

    let mut queue = [0u32; MAX_GRAPH_NODES];
    let mut visited = [0u32; MAX_GRAPH_NODES];
    let mut head = 0;
    let mut tail = 0;
    let mut visited_count = 0;

    queue[tail] = dest_node;
    tail += 1;
    visited[visited_count] = dest_node;
    visited_count += 1;

    while head < tail {
        let current = queue[head];
        head += 1;

        if current == src_node {
            return true;
        }

        for conn in connections {
            if conn.source_node_id == current {
                let nxt = conn.dest_node_id;
                let mut already_visited = false;
                for i in 0..visited_count {
                    if visited[i] == nxt {
                        already_visited = true;
                        break;
                    }
                }

                if !already_visited && visited_count < MAX_GRAPH_NODES && tail < MAX_GRAPH_NODES {
                    visited[visited_count] = nxt;
                    visited_count += 1;
                    queue[tail] = nxt;
                    tail += 1;
                }
            }
        }
    }

    false
}

/// Kahn's algorithm for acyclic topological sort (Rule 29).
/// Writes deterministic execution order into out_sorted and returns true if valid DAG.
pub fn rust_validate_and_topological_sort(
    node_ids: &[u32],
    connections: &[RustGraphConnection],
    out_sorted: &mut [u32],
) -> bool {
    let n = node_ids.len();
    if n == 0 {
        return true;
    }
    if out_sorted.len() < n {
        return false;
    }

    let mut in_degrees = [0i32; MAX_GRAPH_NODES];
    let limit = n.min(MAX_GRAPH_NODES);

    // Validate that all connection endpoints exist in node_ids
    for conn in connections {
        let mut src_found = false;
        let mut dest_found = false;
        let mut dest_idx = 0;

        for (idx, &id) in node_ids.iter().take(limit).enumerate() {
            if id == conn.source_node_id { src_found = true; }
            if id == conn.dest_node_id { dest_found = true; dest_idx = idx; }
        }

        if !src_found || !dest_found {
            return false;
        }

        in_degrees[dest_idx] += 1;
    }

    // Queue of nodes with zero in-degree
    let mut queue = [0usize; MAX_GRAPH_NODES];
    let mut head = 0;
    let mut tail = 0;

    for i in 0..limit {
        if in_degrees[i] == 0 {
            queue[tail] = i;
            tail += 1;
        }
    }

    let mut sorted_count = 0;
    while head < tail {
        let u_idx = queue[head];
        head += 1;

        let u_id = node_ids[u_idx];
        out_sorted[sorted_count] = u_id;
        sorted_count += 1;

        // Decrease in-degree of all successors
        for conn in connections {
            if conn.source_node_id == u_id {
                let v_id = conn.dest_node_id;
                for (v_idx, &id) in node_ids.iter().take(limit).enumerate() {
                    if id == v_id {
                        in_degrees[v_idx] -= 1;
                        if in_degrees[v_idx] == 0 {
                            queue[tail] = v_idx;
                            tail += 1;
                        }
                        break;
                    }
                }
            }
        }
    }

    sorted_count == limit
}

/// Computes topological depth levels (longest distance from roots) for auto-layout and rendering.
pub fn rust_compute_topological_depths(
    node_ids: &[u32],
    connections: &[RustGraphConnection],
    out_depths: &mut [u32],
) {
    let n = node_ids.len().min(out_depths.len()).min(MAX_GRAPH_NODES);
    for i in 0..n {
        out_depths[i] = 0;
    }

    let max_iter = n;
    for _ in 0..max_iter {
        let mut changed = false;
        for conn in connections {
            if conn.source_node_id == conn.dest_node_id {
                continue;
            }

            let mut src_idx = None;
            let mut dest_idx = None;
            for (idx, &id) in node_ids.iter().take(n).enumerate() {
                if id == conn.source_node_id { src_idx = Some(idx); }
                if id == conn.dest_node_id { dest_idx = Some(idx); }
            }

            if let (Some(s), Some(d)) = (src_idx, dest_idx) {
                let target_d = out_depths[s] + 1;
                if out_depths[d] < target_d {
                    out_depths[d] = target_d;
                    changed = true;
                }
            }
        }
        if !changed {
            break;
        }
    }
}

/// Auto-layout computation: aligns nodes in columns based on topological depth and rows by index.
pub fn rust_compute_auto_layout(
    node_ids: &[u32],
    connections: &[RustGraphConnection],
    start_x: f32,
    start_y: f32,
    step_x: f32,
    step_y: f32,
    out_layouts: &mut [RustNodeLayout],
) {
    let n = node_ids.len().min(out_layouts.len()).min(MAX_GRAPH_NODES);
    let mut depths = [0u32; MAX_GRAPH_NODES];
    rust_compute_topological_depths(node_ids, connections, &mut depths[..n]);

    let mut row_in_column = [0u32; MAX_GRAPH_NODES];
    let mut max_depth = 0u32;
    for &d in &depths[..n] {
        if d > max_depth { max_depth = d; }
    }

    let col_count = (max_depth as usize + 1).min(MAX_GRAPH_NODES);
    let mut nodes_in_col = [0u32; MAX_GRAPH_NODES];

    for i in 0..n {
        let col = depths[i] as usize;
        if col < col_count {
            row_in_column[i] = nodes_in_col[col];
            nodes_in_col[col] += 1;
        }
    }

    for i in 0..n {
        let col = depths[i];
        let row = row_in_column[i];

        out_layouts[i] = RustNodeLayout {
            node_id: node_ids[i],
            depth: col,
            pos_x: start_x + (col as f32) * step_x,
            pos_y: start_y + (row as f32) * step_y,
        };
    }
}

/// Compiled runtime graph plan container with cacheline-friendly structure.
pub struct RustGraphPlan {
    pub steps: [RustExecutionStepDesc; MAX_GRAPH_NODES],
    pub step_count: usize,
    pub has_explicit_connections: bool,
}

impl RustGraphPlan {
    pub fn new() -> Self {
        Self {
            steps: [RustExecutionStepDesc::default(); MAX_GRAPH_NODES],
            step_count: 0,
            has_explicit_connections: false,
        }
    }

    pub fn compile_from(
        &mut self,
        topological_order: &[u32],
        connections: &[RustGraphConnection],
        bypassed_flags: &[bool],
    ) {
        let n = topological_order.len().min(MAX_GRAPH_NODES);
        self.step_count = n;
        self.has_explicit_connections = !connections.is_empty();

        for i in 0..n {
            let nid = topological_order[i];
            let is_bypassed = if i < bypassed_flags.len() { bypassed_flags[i] } else { false };

            self.steps[i] = RustExecutionStepDesc {
                node_id: nid,
                is_bypassed,
                is_root: true,
                is_leaf: true,
                sidechain_step_index: -1,
                audio_rate_mod_step_index: -1,
                num_predecessors: 0,
                num_successors: 0,
            };
        }

        for conn in connections {
            let mut src_idx = None;
            let mut dest_idx = None;

            for (idx, &id) in topological_order.iter().take(n).enumerate() {
                if id == conn.source_node_id { src_idx = Some(idx); }
                if id == conn.dest_node_id { dest_idx = Some(idx); }
            }

            if let (Some(s), Some(d)) = (src_idx, dest_idx) {
                self.steps[s].num_successors += 1;
                self.steps[s].is_leaf = false;

                if conn.dest_pin_id == SIDECHAIN_PIN_ID {
                    self.steps[d].sidechain_step_index = s as i32;
                } else if conn.dest_pin_id == AUDIO_RATE_MOD_PIN_ID {
                    self.steps[d].audio_rate_mod_step_index = s as i32;
                } else {
                    self.steps[d].num_predecessors += 1;
                    self.steps[d].is_root = false;
                }
            }
        }

        if self.has_explicit_connections {
            for i in 0..n {
                let st = &mut self.steps[i];
                if st.num_predecessors == 0 && st.num_successors == 0
                    && st.sidechain_step_index < 0 && st.audio_rate_mod_step_index < 0
                {
                    st.is_root = false;
                    st.is_leaf = false;
                }
            }
        }
    }
}

/// Lock-free Double-Buffered Graph Plan Exchanger (Rule 30: Never mutate running graph in audio thread).
pub struct RustGraphPlanExchanger {
    plan_a: Box<RustGraphPlan>,
    plan_b: Box<RustGraphPlan>,
    active_ptr: AtomicPtr<RustGraphPlan>,
}

unsafe impl Send for RustGraphPlanExchanger {}
unsafe impl Sync for RustGraphPlanExchanger {}

impl RustGraphPlanExchanger {
    pub fn new() -> Self {
        let mut pa = Box::new(RustGraphPlan::new());
        let pb = Box::new(RustGraphPlan::new());
        let a_raw = pa.as_mut() as *mut RustGraphPlan;

        Self {
            plan_a: pa,
            plan_b: pb,
            active_ptr: AtomicPtr::new(a_raw),
        }
    }

    /// Prepares standby plan with new topological compilation without touching audio thread.
    pub fn compile_standby_plan(
        &mut self,
        topological_order: &[u32],
        connections: &[RustGraphConnection],
        bypassed_flags: &[bool],
    ) {
        let current = self.active_ptr.load(Ordering::Relaxed);
        let standby: &mut RustGraphPlan = if current == (self.plan_a.as_mut() as *mut RustGraphPlan) {
            &mut self.plan_b
        } else {
            &mut self.plan_a
        };

        standby.compile_from(topological_order, connections, bypassed_flags);
    }

    /// Atomically swaps running plan with compiled standby plan in O(1) lock-free.
    pub fn commit_standby_plan(&self) {
        let current = self.active_ptr.load(Ordering::Relaxed);
        let next = if current == (self.plan_a.as_ref() as *const RustGraphPlan as *mut RustGraphPlan) {
            self.plan_b.as_ref() as *const RustGraphPlan as *mut RustGraphPlan
        } else {
            self.plan_a.as_ref() as *const RustGraphPlan as *mut RustGraphPlan
        };

        self.active_ptr.store(next, Ordering::Release);
    }

    pub fn get_active_plan(&self) -> &RustGraphPlan {
        let ptr = self.active_ptr.load(Ordering::Acquire);
        unsafe { &*ptr }
    }

    pub fn get_active_step_count(&self) -> usize {
        self.get_active_plan().step_count
    }
}
