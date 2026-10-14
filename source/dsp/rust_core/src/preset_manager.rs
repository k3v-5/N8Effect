//! Preset Manager & A/B Snapshot Morphing Engine (Rules 21, 22, 27, 46, 47)
//! High-performance Rust engine for A/B comparison slots, temporal macro morphing,
//! session timeline snapshots, and lock-free preset indexing.

pub const MAX_SNAPSHOTS: usize = 32;
pub const MAX_USER_PRESETS: usize = 128;
pub const MACRO_COUNT: usize = 8;

#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RustPresetSlot {
    SlotA = 0,
    SlotB = 1,
}

#[derive(Debug, Clone)]
pub struct RustPresetSnapshot {
    pub name: String,
    pub json_content: String,
    pub macros: [f32; MACRO_COUNT],
    pub timestamp_ms: u64,
}

impl Default for RustPresetSnapshot {
    fn default() -> Self {
        Self {
            name: String::new(),
            json_content: String::new(),
            macros: [0.0; MACRO_COUNT],
            timestamp_ms: 0,
        }
    }
}

pub struct RustPresetManager {
    // A/B Comparison Slots
    pub slot_a: RustPresetSnapshot,
    pub slot_b: RustPresetSnapshot,
    pub active_slot: RustPresetSlot,
    pub morph_factor: f32, // [0.0 = Slot A, 1.0 = Slot B]

    // Session Snapshots History (bounded capacity)
    pub snapshots: Vec<RustPresetSnapshot>,

    // In-memory User Presets Quick Storage
    pub user_presets: Vec<RustPresetSnapshot>,
}

impl RustPresetManager {
    pub fn new() -> Self {
        Self {
            slot_a: RustPresetSnapshot::default(),
            slot_b: RustPresetSnapshot::default(),
            active_slot: RustPresetSlot::SlotA,
            morph_factor: 0.0,
            snapshots: Vec::with_capacity(MAX_SNAPSHOTS),
            user_presets: Vec::with_capacity(MAX_USER_PRESETS),
        }
    }

    pub fn reset(&mut self) {
        self.slot_a = RustPresetSnapshot::default();
        self.slot_b = RustPresetSnapshot::default();
        self.active_slot = RustPresetSlot::SlotA;
        self.morph_factor = 0.0;
        self.snapshots.clear();
        self.user_presets.clear();
    }

    // =========================================================================
    // A/B Comparison & Morphing
    // =========================================================================

    pub fn store_slot_a(&mut self, name: &str, json_content: &str, macros: &[f32; MACRO_COUNT], timestamp: u64) {
        self.slot_a = RustPresetSnapshot {
            name: name.to_string(),
            json_content: json_content.to_string(),
            macros: *macros,
            timestamp_ms: timestamp,
        };
    }

    pub fn store_slot_b(&mut self, name: &str, json_content: &str, macros: &[f32; MACRO_COUNT], timestamp: u64) {
        self.slot_b = RustPresetSnapshot {
            name: name.to_string(),
            json_content: json_content.to_string(),
            macros: *macros,
            timestamp_ms: timestamp,
        };
    }

    pub fn set_active_slot(&mut self, slot: RustPresetSlot) {
        self.active_slot = slot;
        self.morph_factor = match slot {
            RustPresetSlot::SlotA => 0.0,
            RustPresetSlot::SlotB => 1.0,
        };
    }

    pub fn get_active_slot(&self) -> RustPresetSlot {
        self.active_slot
    }

    pub fn copy_a_to_b(&mut self) {
        self.slot_b = self.slot_a.clone();
    }

    pub fn copy_b_to_a(&mut self) {
        self.slot_a = self.slot_b.clone();
    }

    pub fn swap_slots(&mut self) {
        std::mem::swap(&mut self.slot_a, &mut self.slot_b);
        self.active_slot = match self.active_slot {
            RustPresetSlot::SlotA => RustPresetSlot::SlotB,
            RustPresetSlot::SlotB => RustPresetSlot::SlotA,
        };
        self.morph_factor = 1.0 - self.morph_factor;
    }

    pub fn set_morph_factor(&mut self, factor: f32) {
        self.morph_factor = factor.clamp(0.0, 1.0);
    }

    pub fn get_morph_factor(&self) -> f32 {
        self.morph_factor
    }

    pub fn get_interpolated_macros(&self, out_macros: &mut [f32; MACRO_COUNT]) {
        let t = self.morph_factor;
        // Smoothstep interpolation for natural tactile morphing
        let smooth_t = t * t * (3.0 - 2.0 * t);
        for i in 0..MACRO_COUNT {
            let a = self.slot_a.macros[i];
            let b = self.slot_b.macros[i];
            out_macros[i] = a + smooth_t * (b - a);
        }
    }

    // =========================================================================
    // Session Snapshots (Timeline History)
    // =========================================================================

    pub fn take_snapshot(&mut self, name: &str, json_content: &str, macros: &[f32; MACRO_COUNT], timestamp: u64) {
        if self.snapshots.len() >= MAX_SNAPSHOTS {
            self.snapshots.remove(0); // Evict oldest
        }
        self.snapshots.push(RustPresetSnapshot {
            name: name.to_string(),
            json_content: json_content.to_string(),
            macros: *macros,
            timestamp_ms: timestamp,
        });
    }

    pub fn get_snapshot_count(&self) -> usize {
        self.snapshots.len()
    }

    pub fn get_snapshot(&self, index: usize) -> Option<&RustPresetSnapshot> {
        self.snapshots.get(index)
    }

    // =========================================================================
    // User Presets Storage
    // =========================================================================

    pub fn save_user_preset(&mut self, name: &str, json_content: &str, macros: &[f32; MACRO_COUNT]) {
        if let Some(existing) = self.user_presets.iter_mut().find(|p| p.name == name) {
            existing.json_content = json_content.to_string();
            existing.macros = *macros;
            return;
        }

        if self.user_presets.len() >= MAX_USER_PRESETS {
            self.user_presets.remove(0);
        }

        self.user_presets.push(RustPresetSnapshot {
            name: name.to_string(),
            json_content: json_content.to_string(),
            macros: *macros,
            timestamp_ms: 0,
        });
    }

    pub fn load_user_preset(&self, name: &str) -> Option<&RustPresetSnapshot> {
        self.user_presets.iter().find(|p| p.name == name)
    }

    pub fn get_user_preset_count(&self) -> usize {
        self.user_presets.len()
    }

    pub fn get_user_preset_by_index(&self, index: usize) -> Option<&RustPresetSnapshot> {
        self.user_presets.get(index)
    }

    pub fn remove_user_preset(&mut self, name: &str) -> bool {
        let initial_len = self.user_presets.len();
        self.user_presets.retain(|p| p.name != name);
        self.user_presets.len() != initial_len
    }
}
