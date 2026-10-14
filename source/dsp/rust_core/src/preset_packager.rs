//! Preset Packager & Bank Exchanger Engine (.n8pack & .n8bank) (Phase 7, Rules 21, 27, 47)
//! High-performance pack bundling, manifest validation, parsing, and duplicate resolution in Rust.

#[derive(Debug, Clone)]
pub struct RustPackManifest {
    pub pack_version: u32,
    pub pack_name: String,
    pub author: String,
    pub description: String,
    pub version: String,
    pub created_at: String,
    pub tags: Vec<String>,
    pub format: String,
    pub preset_count: usize,
}

impl Default for RustPackManifest {
    fn default() -> Self {
        Self {
            pack_version: 1,
            pack_name: "Untitled Pack".to_string(),
            author: "Community".to_string(),
            description: String::new(),
            version: "1.0.0".to_string(),
            created_at: String::new(),
            tags: Vec::new(),
            format: "N8Effect_PresetPack".to_string(),
            preset_count: 0,
        }
    }
}

#[derive(Debug, Clone)]
pub struct RustPackPresetEntry {
    pub name: String,
    pub category: String,
    pub author: String,
    pub content: String,
}

#[derive(Debug, Clone, Default)]
pub struct RustPackImportReport {
    pub total_found: usize,
    pub imported: usize,
    pub overwritten: usize,
    pub skipped: usize,
    pub imported_names: Vec<String>,
    pub errors: Vec<String>,
}

pub struct RustPresetPackager;

impl RustPresetPackager {
    pub fn escape_json(s: &str) -> String {
        let mut res = String::with_capacity(s.len() + 16);
        for c in s.chars() {
            match c {
                '"' => res.push_str("\\\""),
                '\\' => res.push_str("\\\\"),
                '\n' => res.push_str("\\n"),
                '\r' => res.push_str("\\r"),
                '\t' => res.push_str("\\t"),
                _ => res.push(c),
            }
        }
        res
    }

    pub fn unescape_json(s: &str) -> String {
        let mut res = String::with_capacity(s.len());
        let mut chars = s.chars().peekable();
        while let Some(c) = chars.next() {
            if c == '\\' {
                if let Some(&next) = chars.peek() {
                    match next {
                        '"' => { res.push('"'); chars.next(); },
                        '\\' => { res.push('\\'); chars.next(); },
                        'n' => { res.push('\n'); chars.next(); },
                        'r' => { res.push('\r'); chars.next(); },
                        't' => { res.push('\t'); chars.next(); },
                        _ => res.push(c),
                    }
                } else {
                    res.push(c);
                }
            } else {
                res.push(c);
            }
        }
        res
    }

    pub fn sanitize_filename(name: &str) -> String {
        let mut res = String::with_capacity(name.len());
        for c in name.chars() {
            match c {
                '/' | '\\' | ':' | '*' | '?' | '"' | '<' | '>' | '|' => res.push('_'),
                _ => res.push(c),
            }
        }
        if res.is_empty() {
            "Untitled".to_string()
        } else {
            res
        }
    }

    pub fn serialize_pack(
        presets: &[RustPackPresetEntry],
        manifest: &RustPackManifest,
        format: &str,
    ) -> String {
        let mut out = String::with_capacity(presets.len() * 1024 + 512);
        out.push_str("{\n");
        out.push_str(&format!("  \"format\": \"{}\",\n", Self::escape_json(format)));
        out.push_str(&format!("  \"packVersion\": {},\n", manifest.pack_version));
        out.push_str("  \"manifest\": {\n");
        out.push_str(&format!("    \"packName\": \"{}\",\n", Self::escape_json(&manifest.pack_name)));
        out.push_str(&format!("    \"author\": \"{}\",\n", Self::escape_json(&manifest.author)));
        out.push_str(&format!("    \"description\": \"{}\",\n", Self::escape_json(&manifest.description)));
        out.push_str(&format!("    \"version\": \"{}\",\n", Self::escape_json(&manifest.version)));
        out.push_str(&format!("    \"createdAt\": \"{}\",\n", Self::escape_json(&manifest.created_at)));
        out.push_str(&format!("    \"presetCount\": {},\n", presets.len()));
        out.push_str("    \"tags\": [");
        for (i, tag) in manifest.tags.iter().enumerate() {
            out.push('"');
            out.push_str(&Self::escape_json(tag));
            out.push('"');
            if i + 1 < manifest.tags.len() {
                out.push_str(", ");
            }
        }
        out.push_str("]\n  },\n");
        out.push_str("  \"presets\": [\n");
        for (i, p) in presets.iter().enumerate() {
            out.push_str("    {\n");
            out.push_str(&format!("      \"name\": \"{}\",\n", Self::escape_json(&p.name)));
            out.push_str(&format!("      \"category\": \"{}\",\n", Self::escape_json(&p.category)));
            out.push_str(&format!("      \"author\": \"{}\",\n", Self::escape_json(&p.author)));
            out.push_str(&format!("      \"content\": \"{}\"\n", Self::escape_json(&p.content)));
            out.push_str("    }");
            if i + 1 < presets.len() {
                out.push_str(",\n");
            } else {
                out.push('\n');
            }
        }
        out.push_str("  ]\n}\n");
        out
    }

    pub fn inspect_pack_manifest(
        content: &str,
    ) -> Result<RustPackManifest, String> {
        let format = Self::extract_string(content, "format")
            .ok_or_else(|| "Invalid pack/bank format: missing or unrecognized 'format' header".to_string())?;

        if format != "N8Effect_PresetPack" && format != "N8Effect_Bank" {
            return Err("Invalid pack/bank format: missing or unrecognized 'format' header".to_string());
        }

        let manifest_sec = Self::extract_object_section(content, "manifest")
            .ok_or_else(|| "Corrupted pack: missing 'manifest' section".to_string())?;

        let pack_version = Self::extract_uint(content, "packVersion").unwrap_or(1);
        let pack_name = Self::extract_string(&manifest_sec, "packName").unwrap_or_else(|| "Untitled Pack".to_string());
        let author = Self::extract_string(&manifest_sec, "author").unwrap_or_else(|| "Unknown".to_string());
        let description = Self::extract_string(&manifest_sec, "description").unwrap_or_default();
        let version = Self::extract_string(&manifest_sec, "version").unwrap_or_else(|| "1.0.0".to_string());
        let created_at = Self::extract_string(&manifest_sec, "createdAt").unwrap_or_default();
        let preset_count = Self::extract_uint(&manifest_sec, "presetCount").unwrap_or(0) as usize;

        let tags = Self::extract_string_array(&manifest_sec, "tags");

        Ok(RustPackManifest {
            pack_version,
            pack_name,
            author,
            description,
            version,
            created_at,
            tags,
            format,
            preset_count,
        })
    }

    pub fn extract_presets(content: &str) -> Vec<RustPackPresetEntry> {
        let mut entries = Vec::new();
        if let Some(presets_arr) = Self::extract_array_section(content, "presets") {
            let objs = Self::split_objects(&presets_arr);
            for obj in objs {
                let name = Self::extract_string(&obj, "name").unwrap_or_else(|| "Imported Preset".to_string());
                let category = Self::extract_string(&obj, "category").unwrap_or_else(|| "General".to_string());
                let author = Self::extract_string(&obj, "author").unwrap_or_else(|| "Unknown".to_string());
                let raw_content = Self::extract_string(&obj, "content").unwrap_or_default();
                let unescaped_content = Self::unescape_json(&raw_content);
                entries.push(RustPackPresetEntry {
                    name,
                    category,
                    author,
                    content: unescaped_content,
                });
            }
        }
        entries
    }

    // Helper functions for lightweight JSON parsing
    fn extract_string(json: &str, key: &str) -> Option<String> {
        let pattern = format!("\"{}\"", key);
        let key_pos = json.find(&pattern)?;
        let rest = &json[key_pos + pattern.len()..];
        let colon_pos = rest.find(':')?;
        let after_colon = rest[colon_pos + 1..].trim_start();
        if after_colon.starts_with('"') {
            let str_val = &after_colon[1..];
            let mut res = String::new();
            let mut escape = false;
            for c in str_val.chars() {
                if escape {
                    match c {
                        '"' => res.push('"'),
                        '\\' => res.push('\\'),
                        'n' => res.push('\n'),
                        'r' => res.push('\r'),
                        't' => res.push('\t'),
                        _ => { res.push('\\'); res.push(c); }
                    }
                    escape = false;
                } else if c == '\\' {
                    escape = true;
                } else if c == '"' {
                    return Some(res);
                } else {
                    res.push(c);
                }
            }
        }
        None
    }

    fn extract_uint(json: &str, key: &str) -> Option<u32> {
        let pattern = format!("\"{}\"", key);
        let key_pos = json.find(&pattern)?;
        let rest = &json[key_pos + pattern.len()..];
        let colon_pos = rest.find(':')?;
        let after_colon = rest[colon_pos + 1..].trim_start();
        let end_pos = after_colon.find(|c: char| !c.is_ascii_digit()).unwrap_or(after_colon.len());
        after_colon[..end_pos].parse::<u32>().ok()
    }

    fn extract_object_section(json: &str, key: &str) -> Option<String> {
        let pattern = format!("\"{}\"", key);
        let key_pos = json.find(&pattern)?;
        let rest = &json[key_pos + pattern.len()..];
        let brace_start = rest.find('{')?;
        let mut depth = 0;
        let mut in_str = false;
        let mut escape = false;
        for (i, c) in rest[brace_start..].char_indices() {
            if escape {
                escape = false;
                continue;
            }
            if c == '\\' && in_str {
                escape = true;
                continue;
            }
            if c == '"' {
                in_str = !in_str;
                continue;
            }
            if !in_str {
                if c == '{' {
                    depth += 1;
                } else if c == '}' {
                    depth -= 1;
                    if depth == 0 {
                        return Some(rest[brace_start..=brace_start + i].to_string());
                    }
                }
            }
        }
        None
    }

    fn extract_array_section(json: &str, key: &str) -> Option<String> {
        let pattern = format!("\"{}\"", key);
        let key_pos = json.find(&pattern)?;
        let rest = &json[key_pos + pattern.len()..];
        let bracket_start = rest.find('[')?;
        let mut depth = 0;
        let mut in_str = false;
        let mut escape = false;
        for (i, c) in rest[bracket_start..].char_indices() {
            if escape {
                escape = false;
                continue;
            }
            if c == '\\' && in_str {
                escape = true;
                continue;
            }
            if c == '"' {
                in_str = !in_str;
                continue;
            }
            if !in_str {
                if c == '[' {
                    depth += 1;
                } else if c == ']' {
                    depth -= 1;
                    if depth == 0 {
                        return Some(rest[bracket_start..=bracket_start + i].to_string());
                    }
                }
            }
        }
        None
    }

    fn extract_string_array(json: &str, key: &str) -> Vec<String> {
        let mut items = Vec::new();
        if let Some(arr_sec) = Self::extract_array_section(json, key) {
            let mut in_str = false;
            let mut escape = false;
            let mut cur = String::new();
            for c in arr_sec.chars() {
                if escape {
                    cur.push(c);
                    escape = false;
                    continue;
                }
                if c == '\\' && in_str {
                    escape = true;
                    continue;
                }
                if c == '"' {
                    if in_str {
                        items.push(cur.clone());
                        cur.clear();
                        in_str = false;
                    } else {
                        in_str = true;
                    }
                    continue;
                }
                if in_str {
                    cur.push(c);
                }
            }
        }
        items
    }

    fn split_objects(array_json: &str) -> Vec<String> {
        let mut objs = Vec::new();
        let mut depth = 0;
        let mut in_str = false;
        let mut escape = false;
        let mut start_idx = None;

        for (i, c) in array_json.char_indices() {
            if escape {
                escape = false;
                continue;
            }
            if c == '\\' && in_str {
                escape = true;
                continue;
            }
            if c == '"' {
                in_str = !in_str;
                continue;
            }
            if !in_str {
                if c == '{' {
                    if depth == 0 {
                        start_idx = Some(i);
                    }
                    depth += 1;
                } else if c == '}' {
                    depth -= 1;
                    if depth == 0 {
                        if let Some(start) = start_idx {
                            objs.push(array_json[start..=i].to_string());
                            start_idx = None;
                        }
                    }
                }
            }
        }
        objs
    }
}

pub struct RustPackBuilder {
    pub manifest: RustPackManifest,
    pub presets: Vec<RustPackPresetEntry>,
}

impl RustPackBuilder {
    pub fn new(
        format: &str,
        pack_name: &str,
        author: &str,
        description: &str,
        version: &str,
        created_at: &str,
        tags_csv: &str,
    ) -> Self {
        let tags: Vec<String> = if tags_csv.is_empty() {
            Vec::new()
        } else {
            tags_csv.split(',').map(|s| s.trim().to_string()).filter(|s| !s.is_empty()).collect()
        };

        Self {
            manifest: RustPackManifest {
                pack_version: 1,
                pack_name: pack_name.to_string(),
                author: author.to_string(),
                description: description.to_string(),
                version: version.to_string(),
                created_at: created_at.to_string(),
                tags,
                format: format.to_string(),
                preset_count: 0,
            },
            presets: Vec::new(),
        }
    }

    pub fn add_preset(&mut self, name: &str, category: &str, author: &str, content: &str) {
        self.presets.push(RustPackPresetEntry {
            name: name.to_string(),
            category: category.to_string(),
            author: author.to_string(),
            content: content.to_string(),
        });
    }

    pub fn serialize(&self) -> String {
        RustPresetPackager::serialize_pack(&self.presets, &self.manifest, &self.manifest.format)
    }
}

pub struct RustPackExtractor {
    pub presets: Vec<RustPackPresetEntry>,
}

impl RustPackExtractor {
    pub fn new(content: &str) -> Self {
        let presets = RustPresetPackager::extract_presets(content);
        Self { presets }
    }
}

