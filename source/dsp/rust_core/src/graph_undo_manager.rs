//! Graph Undo/Redo Timeline & Time-Travel Manager (Phase 7, Rules 21, 22, 47)
//! High-performance bounded history with bounded capacity, zero memory leaks,
//! and direct point-in-time state jumping.

#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RustActionCategory {
    NodeAdd = 0,
    NodeRemove = 1,
    Connection = 2,
    Disconnection = 3,
    ParameterChange = 4,
    PresetLoad = 5,
    Randomize = 6,
    General = 7,
}

#[derive(Debug, Clone)]
pub struct RustUndoCheckpoint {
    pub description: String,
    pub category: RustActionCategory,
    pub timestamp_ms: i64,
    pub serialized_state: String,
}

pub struct RustGraphUndoManager {
    max_history_steps: usize,
    timeline: Vec<RustUndoCheckpoint>,
    current_index: usize,
}

impl RustGraphUndoManager {
    pub fn new(max_history_steps: usize) -> Self {
        let capacity = if max_history_steps == 0 { 32 } else { max_history_steps };
        Self {
            max_history_steps: capacity,
            timeline: Vec::with_capacity(capacity),
            current_index: 0,
        }
    }

    pub fn push_action(&mut self, description: &str, category: u8, serialized_state: &str, timestamp_ms: i64) {
        let cat = match category {
            0 => RustActionCategory::NodeAdd,
            1 => RustActionCategory::NodeRemove,
            2 => RustActionCategory::Connection,
            3 => RustActionCategory::Disconnection,
            4 => RustActionCategory::ParameterChange,
            5 => RustActionCategory::PresetLoad,
            6 => RustActionCategory::Randomize,
            _ => RustActionCategory::General,
        };

        // If we were in the past and a new action is performed, truncate the future
        if !self.timeline.is_empty() && self.current_index + 1 < self.timeline.len() {
            self.timeline.truncate(self.current_index + 1);
        }

        self.timeline.push(RustUndoCheckpoint {
            description: description.to_string(),
            category: cat,
            timestamp_ms,
            serialized_state: serialized_state.to_string(),
        });

        // Bounded capacity protection (Rule 47)
        if self.timeline.len() > self.max_history_steps {
            self.timeline.remove(0);
        }

        self.current_index = self.timeline.len().saturating_sub(1);
    }

    pub fn can_undo(&self) -> bool {
        !self.timeline.is_empty() && self.current_index > 0
    }

    pub fn can_redo(&self) -> bool {
        !self.timeline.is_empty() && (self.current_index + 1 < self.timeline.len())
    }

    pub fn undo(&mut self) -> Option<&str> {
        if !self.can_undo() {
            return None;
        }
        self.current_index -= 1;
        Some(&self.timeline[self.current_index].serialized_state)
    }

    pub fn redo(&mut self) -> Option<&str> {
        if !self.can_redo() {
            return None;
        }
        self.current_index += 1;
        Some(&self.timeline[self.current_index].serialized_state)
    }

    pub fn jump_to_step(&mut self, target_index: usize) -> Option<&str> {
        if target_index >= self.timeline.len() || target_index == self.current_index {
            return None;
        }
        self.current_index = target_index;
        Some(&self.timeline[self.current_index].serialized_state)
    }

    pub fn get_current_index(&self) -> usize {
        self.current_index
    }

    pub fn get_history_size(&self) -> usize {
        self.timeline.len()
    }

    pub fn get_checkpoint(&self, index: usize) -> Option<&RustUndoCheckpoint> {
        self.timeline.get(index)
    }

    pub fn get_next_undo_description(&self) -> &str {
        if !self.can_undo() {
            ""
        } else {
            &self.timeline[self.current_index].description
        }
    }

    pub fn get_next_redo_description(&self) -> &str {
        if !self.can_redo() {
            ""
        } else {
            &self.timeline[self.current_index + 1].description
        }
    }

    pub fn get_state_len(&self, index: usize) -> usize {
        self.timeline.get(index).map(|c| c.serialized_state.len()).unwrap_or(0)
    }

    pub fn get_current_state_len(&self) -> usize {
        if self.timeline.is_empty() || self.current_index >= self.timeline.len() {
            0
        } else {
            self.timeline[self.current_index].serialized_state.len()
        }
    }

    pub fn clear(&mut self) {
        self.timeline.clear();
        self.current_index = 0;
    }
}
