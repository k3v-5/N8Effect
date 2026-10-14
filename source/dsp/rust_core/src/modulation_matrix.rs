//! Universal Modulation Matrix & Macro Manager Core (Rules 7, 8, 25, 46, 47)
//! 64 preallocated modulation routes, 8 global macros + XY Pad with anti-click smoothing.

pub const MAX_ROUTES: usize = 64;
pub const MACRO_COUNT: usize = 8;
pub const MOD_SOURCE_COUNT: usize = 42;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct RustModulationRoute {
    pub source: u16,
    pub _pad: [u8; 2],
    pub target_node_id: u32,
    pub target_param_id: u32,
    pub amount: f32,
    pub bipolar: bool,
    pub is_active: bool,
    pub _pad2: [u8; 2],
}

impl Default for RustModulationRoute {
    fn default() -> Self {
        Self {
            source: 0,
            _pad: [0; 2],
            target_node_id: 0,
            target_param_id: 0,
            amount: 0.0,
            bipolar: true,
            is_active: false,
            _pad2: [0; 2],
        }
    }
}

pub struct RustMacroManager {
    macros: [f32; MACRO_COUNT],
    smoothed_macros: [f32; MACRO_COUNT],
    pad_x: f32,
    pad_y: f32,
    smoothed_pad_x: f32,
    smoothed_pad_y: f32,
}

impl RustMacroManager {
    pub fn new() -> Self {
        Self {
            macros: [0.5; MACRO_COUNT],
            smoothed_macros: [0.5; MACRO_COUNT],
            pad_x: 0.5,
            pad_y: 0.5,
            smoothed_pad_x: 0.5,
            smoothed_pad_y: 0.5,
        }
    }

    pub fn reset(&mut self) {
        for i in 0..MACRO_COUNT {
            self.smoothed_macros[i] = self.macros[i];
        }
        self.smoothed_pad_x = self.pad_x;
        self.smoothed_pad_y = self.pad_y;
    }

    #[inline]
    pub fn set_macro(&mut self, idx: usize, value: f32) {
        if idx < MACRO_COUNT {
            self.macros[idx] = value.clamp(0.0, 1.0);
        }
    }

    #[inline]
    pub fn get_macro(&self, idx: usize) -> f32 {
        if idx < MACRO_COUNT { self.macros[idx] } else { 0.0 }
    }

    #[inline]
    pub fn get_smoothed_macro(&self, idx: usize) -> f32 {
        if idx < MACRO_COUNT { self.smoothed_macros[idx] } else { 0.0 }
    }

    #[inline]
    pub fn set_pad(&mut self, x: f32, y: f32) {
        self.pad_x = x.clamp(0.0, 1.0);
        self.pad_y = y.clamp(0.0, 1.0);
    }

    #[inline]
    pub fn get_pad_x(&self) -> f32 { self.pad_x }
    #[inline]
    pub fn get_pad_y(&self) -> f32 { self.pad_y }
    #[inline]
    pub fn get_smoothed_pad_x(&self) -> f32 { self.smoothed_pad_x }
    #[inline]
    pub fn get_smoothed_pad_y(&self) -> f32 { self.smoothed_pad_y }

    #[inline]
    pub fn process_smoothing(&mut self) {
        const ALPHA: f32 = 0.01;
        for i in 0..MACRO_COUNT {
            self.smoothed_macros[i] += ALPHA * (self.macros[i] - self.smoothed_macros[i]);
        }
        self.smoothed_pad_x += ALPHA * (self.pad_x - self.smoothed_pad_x);
        self.smoothed_pad_y += ALPHA * (self.pad_y - self.smoothed_pad_y);
    }
}

pub struct RustModulationMatrix {
    routes: [RustModulationRoute; MAX_ROUTES],
}

impl RustModulationMatrix {
    pub fn new() -> Self {
        Self {
            routes: [RustModulationRoute::default(); MAX_ROUTES],
        }
    }

    pub fn reset(&mut self) {
        for r in &mut self.routes {
            r.is_active = false;
        }
    }

    #[inline]
    pub fn add_route(
        &mut self,
        source: u16,
        target_node: u32,
        target_param: u32,
        amount: f32,
        bipolar: bool,
    ) -> i32 {
        for (i, r) in self.routes.iter_mut().enumerate() {
            if !r.is_active {
                *r = RustModulationRoute {
                    source,
                    _pad: [0; 2],
                    target_node_id: target_node,
                    target_param_id: target_param,
                    amount: amount.clamp(-1.0, 1.0),
                    bipolar,
                    is_active: true,
                    _pad2: [0; 2],
                };
                return i as i32;
            }
        }
        -1
    }

    #[inline]
    pub fn remove_route(&mut self, index: usize) {
        if index < MAX_ROUTES {
            self.routes[index].is_active = false;
        }
    }

    #[inline]
    pub fn set_route_amount(&mut self, index: usize, amount: f32) {
        if index < MAX_ROUTES && self.routes[index].is_active {
            self.routes[index].amount = amount.clamp(-1.0, 1.0);
        }
    }

    #[inline]
    pub fn get_active_route_count(&self) -> usize {
        self.routes.iter().filter(|r| r.is_active).count()
    }

    #[inline]
    pub fn get_route(&self, index: usize) -> RustModulationRoute {
        let idx = index.min(MAX_ROUTES - 1);
        self.routes[idx]
    }

    #[inline]
    pub fn calculate_modulation_offset(
        &self,
        target_node: u32,
        target_param: u32,
        source_values: &[f32],
    ) -> f32 {
        let mut net_offset = 0.0f32;

        for r in &self.routes {
            if r.is_active && r.target_node_id == target_node && r.target_param_id == target_param {
                let src_idx = r.source as usize;
                if src_idx < source_values.len() {
                    let mut src_val = source_values[src_idx];
                    if !r.bipolar {
                        src_val = (src_val + 1.0) * 0.5;
                    }
                    net_offset += src_val * r.amount;
                }
            }
        }

        net_offset
    }
}
