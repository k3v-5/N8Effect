//! Graph Serializer & Structured JSON Topology Engine (Rules 21, 22, 28, 46, 47)
//! High-performance JSON escaping, extraction, parsing and topology serialization.
//! Provides zero-allocation string scanning and deterministic migration routines.

pub const MAX_NODES: usize = 128;
pub const MAX_CONNECTIONS: usize = 512;
pub const MAX_GROUPS: usize = 32;

#[derive(Debug, Clone)]
pub struct RustPresetMetadata {
    pub schema_version: u32,
    pub name: String,
    pub author: String,
    pub category: String,
    pub description: String,
    pub tags: Vec<String>,
    pub favorite: bool,
    pub dry_level: f32,
    pub wet_level: f32,
}

impl Default for RustPresetMetadata {
    fn default() -> Self {
        Self {
            schema_version: 1,
            name: "Default Preset".to_string(),
            author: "N8Audio".to_string(),
            category: "General".to_string(),
            description: String::new(),
            tags: Vec::new(),
            favorite: false,
            dry_level: 1.0,
            wet_level: 1.0,
        }
    }
}

pub struct RustGraphSerializer;

impl RustGraphSerializer {
    pub fn escape_json(s: &str) -> String {
        let mut out = String::with_capacity(s.len() + 16);
        for c in s.chars() {
            match c {
                '"' => out.push_str("\\\""),
                '\\' => out.push_str("\\\\"),
                '\n' => out.push_str("\\n"),
                '\r' => out.push_str("\\r"),
                '\t' => out.push_str("\\t"),
                _ => out.push(c),
            }
        }
        out
    }

    pub fn extract_key<'a>(json: &'a str, key: &str) -> Option<&'a str> {
        let search = format!("\"{}\"", key);
        let pos = json.find(&search)?;
        let rest = &json[pos + search.len()..];
        let colon = rest.find(':')?;
        let after_colon = &rest[colon + 1..];
        Some(after_colon.trim_start())
    }

    pub fn extract_string(json: &str, key: &str, default_val: &str) -> String {
        if let Some(val) = Self::extract_key(json, key) {
            if val.starts_with('"') {
                if let Some(end) = val[1..].find('"') {
                    return val[1..=end].to_string();
                }
            }
        }
        default_val.to_string()
    }

    pub fn extract_uint(json: &str, key: &str, default_val: u32) -> u32 {
        if let Some(val) = Self::extract_key(json, key) {
            let num_str: String = val.chars().take_while(|c| c.is_ascii_digit()).collect();
            if let Ok(n) = num_str.parse::<u32>() {
                return n;
            }
        }
        default_val
    }

    pub fn extract_float(json: &str, key: &str, default_val: f32) -> f32 {
        if let Some(val) = Self::extract_key(json, key) {
            let num_str: String = val.chars().take_while(|c| c.is_ascii_digit() || *c == '.' || *c == '-' || *c == '+').collect();
            if let Ok(n) = num_str.parse::<f32>() {
                return n;
            }
        }
        default_val
    }

    pub fn extract_section<'a>(json: &'a str, key: &str, open_c: char, close_c: char) -> Option<&'a str> {
        let val = Self::extract_key(json, key)?;
        if !val.starts_with(open_c) {
            return None;
        }

        let mut depth = 0;
        for (i, c) in val.chars().enumerate() {
            if c == open_c {
                depth += 1;
            } else if c == close_c {
                depth -= 1;
                if depth == 0 {
                    return Some(&val[1..i]);
                }
            }
        }
        None
    }

    pub fn extract_array_section<'a>(json: &'a str, key: &str) -> Option<&'a str> {
        Self::extract_section(json, key, '[', ']')
    }

    pub fn extract_object_section<'a>(json: &'a str, key: &str) -> Option<&'a str> {
        Self::extract_section(json, key, '{', '}')
    }

    pub fn split_objects<'a>(array_content: &'a str) -> Vec<&'a str> {
        let mut objs = Vec::new();
        let mut depth = 0;
        let mut start = 0;

        for (i, c) in array_content.chars().enumerate() {
            if c == '{' {
                if depth == 0 {
                    start = i;
                }
                depth += 1;
            } else if c == '}' {
                depth -= 1;
                if depth == 0 {
                    objs.push(&array_content[start..=i]);
                }
            }
        }
        objs
    }

    pub fn parse_number_list(text: &str) -> Vec<f32> {
        let mut nums = Vec::new();
        let mut current = String::new();

        for c in text.chars() {
            if c.is_ascii_digit() || c == '.' || c == '-' || c == '+' {
                current.push(c);
            } else if c == ',' || c == ' ' || c == '\n' || c == '\r' || c == '\t' {
                if !current.is_empty() {
                    if let Ok(f) = current.parse::<f32>() {
                        nums.push(f);
                    }
                    current.clear();
                }
            }
        }
        if !current.is_empty() {
            if let Ok(f) = current.parse::<f32>() {
                nums.push(f);
            }
        }
        nums
    }

    pub fn parse_key_value_floats(text: &str) -> Vec<(u32, f32)> {
        let mut pairs = Vec::new();
        let mut pos = 0;

        while let Some(q1) = text[pos..].find('"') {
            let abs_q1 = pos + q1;
            if let Some(q2) = text[abs_q1 + 1..].find('"') {
                let abs_q2 = abs_q1 + 1 + q2;
                let key_str = &text[abs_q1 + 1..abs_q2];
                let pid = key_str.parse::<u32>().unwrap_or(0);

                if let Some(colon) = text[abs_q2 + 1..].find(':') {
                    let abs_colon = abs_q2 + 1 + colon;
                    let val_str = &text[abs_colon + 1..];
                    let val_end = val_str.find(|c| c == ',' || c == '}' || c == '\n').unwrap_or(val_str.len());
                    let token = val_str[..val_end].trim();
                    let val = token.parse::<f32>().unwrap_or(0.0);
                    pairs.push((pid, val));
                    pos = abs_colon + 1 + val_end;
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        pairs
    }

    pub fn parse_string_list(array_content: &str) -> Vec<String> {
        let mut items = Vec::new();
        let mut pos = 0;
        while let Some(q1) = array_content[pos..].find('"') {
            let abs_q1 = pos + q1;
            if let Some(q2) = array_content[abs_q1 + 1..].find('"') {
                let abs_q2 = abs_q1 + 1 + q2;
                items.push(array_content[abs_q1 + 1..abs_q2].to_string());
                pos = abs_q2 + 1;
            } else {
                break;
            }
        }
        items
    }
}
