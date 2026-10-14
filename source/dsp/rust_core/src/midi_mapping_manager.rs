//! MIDI Mapping Manager & Hardware Profiles Engine (Rules 8, 9, 21, 46)
//! Non-linear CC response curves (Linear, Exponential, Logarithmic, SShaped),
//! MIDI Learn, profile presets, and lock-free real-time dispatch.

#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RustMidiCurve {
    Linear = 0,
    Exponential = 1,
    Logarithmic = 2,
    SShaped = 3,
}

#[derive(Debug, Clone)]
pub struct RustMidiMapping {
    pub cc_number: u8,
    pub channel: u8,
    pub param_id: u32,
    pub target_node_id: u32,
    pub min_val: f32,
    pub max_val: f32,
    pub inverted: bool,
    pub curve: RustMidiCurve,
    pub custom_label: String,
}

impl Default for RustMidiMapping {
    fn default() -> Self {
        Self {
            cc_number: 0,
            channel: 0,
            param_id: 0,
            target_node_id: u32::MAX, // InvalidNodeId
            min_val: 0.0,
            max_val: 1.0,
            inverted: false,
            curve: RustMidiCurve::Linear,
            custom_label: String::new(),
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub struct RustMidiDispatchItem {
    pub target_node_id: u32,
    pub param_id: u32,
    pub mapped_value: f32,
}

pub struct RustMidiMappingManager {
    pub mappings: Vec<RustMidiMapping>,
    pub is_learning: bool,
    pub pending_target: RustMidiMapping,
}

impl RustMidiMappingManager {
    pub const MAX_MAPPINGS: usize = 128;

    pub fn new() -> Self {
        Self {
            mappings: Vec::with_capacity(Self::MAX_MAPPINGS),
            is_learning: false,
            pending_target: RustMidiMapping::default(),
        }
    }

    pub fn reset(&mut self) {
        self.cancel_learning();
        self.mappings.clear();
    }

    pub fn start_learning(
        &mut self,
        param_id: u32,
        target_node_id: u32,
        min_val: f32,
        max_val: f32,
        curve: u8,
        inverted: bool,
    ) {
        let c = match curve {
            1 => RustMidiCurve::Exponential,
            2 => RustMidiCurve::Logarithmic,
            3 => RustMidiCurve::SShaped,
            _ => RustMidiCurve::Linear,
        };
        self.pending_target = RustMidiMapping {
            cc_number: 0,
            channel: 0,
            param_id,
            target_node_id,
            min_val,
            max_val,
            inverted,
            curve: c,
            custom_label: String::new(),
        };
        self.is_learning = true;
    }

    pub fn cancel_learning(&mut self) {
        self.is_learning = false;
    }

    pub fn is_learning(&self) -> bool {
        self.is_learning
    }

    pub fn add_mapping(&mut self, mapping: RustMidiMapping) -> bool {
        self.remove_mapping(mapping.param_id, mapping.target_node_id);
        if self.mappings.len() < Self::MAX_MAPPINGS {
            self.mappings.push(mapping);
            true
        } else {
            false
        }
    }

    pub fn remove_mapping(&mut self, param_id: u32, target_node_id: u32) -> bool {
        let initial_len = self.mappings.len();
        self.mappings.retain(|m| !(m.param_id == param_id && m.target_node_id == target_node_id));
        self.mappings.len() != initial_len
    }

    pub fn remove_mapping_by_cc(&mut self, cc_number: u8, channel: u8) {
        self.mappings.retain(|m| {
            !(m.cc_number == cc_number && (channel == 0 || m.channel == 0 || m.channel == channel))
        });
    }

    pub fn calculate_mapped_value(map: &RustMidiMapping, cc_value: u8) -> f32 {
        let mut norm = (cc_value as f32 / 127.0).clamp(0.0, 1.0);
        if map.inverted {
            norm = 1.0 - norm;
        }

        match map.curve {
            RustMidiCurve::Linear => {}
            RustMidiCurve::Exponential => {
                norm = norm * norm;
            }
            RustMidiCurve::Logarithmic => {
                norm = norm.sqrt();
            }
            RustMidiCurve::SShaped => {
                norm = norm * norm * (3.0 - 2.0 * norm);
            }
        }

        map.min_val + norm * (map.max_val - map.min_val)
    }

    pub fn process_control_change(
        &mut self,
        channel: u8,
        cc_number: u8,
        cc_value: u8,
        out_dispatches: &mut [RustMidiDispatchItem],
    ) -> usize {
        if self.is_learning {
            let mut new_map = self.pending_target.clone();
            new_map.cc_number = cc_number;
            new_map.channel = 0; // Omni

            let mut replaced = false;
            for m in &mut self.mappings {
                if m.param_id == new_map.param_id && m.target_node_id == new_map.target_node_id {
                    *m = new_map.clone();
                    replaced = true;
                    break;
                }
            }
            if !replaced && self.mappings.len() < Self::MAX_MAPPINGS {
                self.mappings.push(new_map);
            }
            self.is_learning = false;
            return 0;
        }

        let mut count = 0;
        for map in &self.mappings {
            if map.cc_number == cc_number && (map.channel == 0 || map.channel == channel) {
                if count < out_dispatches.len() {
                    let mapped_val = Self::calculate_mapped_value(map, cc_value);
                    out_dispatches[count] = RustMidiDispatchItem {
                        target_node_id: map.target_node_id,
                        param_id: map.param_id,
                        mapped_value: mapped_val,
                    };
                    count += 1;
                }
            }
        }
        count
    }

    pub fn load_factory_profile(&mut self, profile_name: &str) {
        self.mappings.clear();
        let (base_cc, label_prefix) = match profile_name {
            "Akai MPK Mini MK3" => (70u8, "Macro "),
            "Arturia KeyLab Essential" => (73u8, "Macro "),
            "Novation Launchkey" => (21u8, "Macro "),
            _ => (14u8, "Macro "),
        };

        for i in 0..8 {
            self.add_mapping(RustMidiMapping {
                cc_number: base_cc + i,
                channel: 0,
                param_id: (i + 1) as u32,
                target_node_id: u32::MAX,
                min_val: 0.0,
                max_val: 1.0,
                inverted: false,
                curve: RustMidiCurve::Linear,
                custom_label: format!("{}{}", label_prefix, i + 1),
            });
        }
    }

    pub fn export_json(&self, profile_name: &str) -> String {
        let mut ss = String::new();
        ss.push_str("{\n");
        ss.push_str("  \"schemaVersion\": 1,\n");
        ss.push_str(&format!("  \"profileName\": \"{}\",\n", profile_name));
        ss.push_str("  \"mappings\": [\n");

        for (i, m) in self.mappings.iter().enumerate() {
            ss.push_str("    {\n");
            ss.push_str(&format!("      \"cc\": {},\n", m.cc_number));
            ss.push_str(&format!("      \"channel\": {},\n", m.channel));
            ss.push_str(&format!("      \"paramId\": {},\n", m.param_id));
            ss.push_str(&format!("      \"targetNodeId\": {},\n", m.target_node_id));
            ss.push_str(&format!("      \"minVal\": {:.6},\n", m.min_val));
            ss.push_str(&format!("      \"maxVal\": {:.6},\n", m.max_val));
            ss.push_str(&format!("      \"inverted\": {},\n", if m.inverted { "true" } else { "false" }));
            ss.push_str(&format!("      \"curve\": {},\n", m.curve as u8));
            ss.push_str(&format!("      \"label\": \"{}\"\n", m.custom_label));
            if i + 1 < self.mappings.len() {
                ss.push_str("    },\n");
            } else {
                ss.push_str("    }\n");
            }
        }

        ss.push_str("  ]\n");
        ss.push_str("}\n");
        ss
    }

    pub fn import_json(&mut self, json_str: &str) -> bool {
        if json_str.is_empty() {
            return false;
        }

        let map_idx = match json_str.find("\"mappings\"") {
            Some(idx) => idx,
            None => return false,
        };

        let mut loaded = Vec::new();
        let mut pos = map_idx;

        while let Some(start_obj) = json_str[pos..].find('{') {
            let actual_start = pos + start_obj;
            let end_obj = match json_str[actual_start..].find('}') {
                Some(idx) => actual_start + idx,
                None => break,
            };

            let block = &json_str[actual_start..=end_obj];
            let extract_int = |key: &str, def: i64| -> i64 {
                if let Some(kpos) = block.find(key) {
                    if let Some(cpos) = block[kpos..].find(':') {
                        let val_str = &block[kpos + cpos + 1..];
                        let trimmed = val_str.trim_start_matches(|c: char| c.is_whitespace() || c == '"');
                        let token: String = trimmed.chars().take_while(|c| c.is_ascii_digit() || *c == '-').collect();
                        return token.parse::<i64>().unwrap_or(def);
                    }
                }
                def
            };

            let extract_float = |key: &str, def: f32| -> f32 {
                if let Some(kpos) = block.find(key) {
                    if let Some(cpos) = block[kpos..].find(':') {
                        let val_str = &block[kpos + cpos + 1..];
                        let trimmed = val_str.trim_start_matches(|c: char| c.is_whitespace() || c == '"');
                        let token: String = trimmed.chars().take_while(|c| c.is_ascii_digit() || *c == '.' || *c == '-').collect();
                        return token.parse::<f32>().unwrap_or(def);
                    }
                }
                def
            };

            let extract_bool = |key: &str, def: bool| -> bool {
                if let Some(kpos) = block.find(key) {
                    if let Some(cpos) = block[kpos..].find(':') {
                        let val_str = &block[kpos + cpos + 1..];
                        let trimmed = val_str.trim_start_matches(|c: char| c.is_whitespace());
                        return trimmed.starts_with("true");
                    }
                }
                def
            };

            let extract_str = |key: &str| -> String {
                if let Some(kpos) = block.find(key) {
                    if let Some(cpos) = block[kpos..].find(':') {
                        let rest = &block[kpos + cpos + 1..];
                        if let Some(q1) = rest.find('"') {
                            if let Some(q2) = rest[q1 + 1..].find('"') {
                                return rest[q1 + 1..q1 + 1 + q2].to_string();
                            }
                        }
                    }
                }
                String::new()
            };

            let cc = extract_int("\"cc\"", -1);
            if cc >= 0 {
                let channel = extract_int("\"channel\"", 0) as u8;
                let param_id = extract_int("\"paramId\"", 0) as u32;
                let target_node_id = extract_int("\"targetNodeId\"", u32::MAX as i64) as u32;
                let min_val = extract_float("\"minVal\"", 0.0);
                let max_val = extract_float("\"maxVal\"", 1.0);
                let inverted = extract_bool("\"inverted\"", false);
                let curve_val = extract_int("\"curve\"", 0) as u8;
                let curve = match curve_val {
                    1 => RustMidiCurve::Exponential,
                    2 => RustMidiCurve::Logarithmic,
                    3 => RustMidiCurve::SShaped,
                    _ => RustMidiCurve::Linear,
                };
                let custom_label = extract_str("\"label\"");

                loaded.push(RustMidiMapping {
                    cc_number: cc as u8,
                    channel,
                    param_id,
                    target_node_id,
                    min_val,
                    max_val,
                    inverted,
                    curve,
                    custom_label,
                });
            }

            pos = end_obj + 1;
        }

        if !loaded.is_empty() {
            self.mappings = loaded;
            true
        } else {
            false
        }
    }
}
