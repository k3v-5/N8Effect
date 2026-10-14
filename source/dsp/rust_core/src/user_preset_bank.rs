//! User Preset Bank & Reactive Indexing Engine (Phase 7, Rules 21, 27, 47)
//! High-performance in-memory indexing, tag/category aggregation,
//! and case-insensitive multi-attribute filtering.

#[derive(Debug, Clone)]
pub struct RustUserPresetRecord {
    pub file_path: String,
    pub name: String,
    pub category: String,
    pub author: String,
    pub description: String,
    pub version: String,
    pub tags: Vec<String>,
    pub favorite: bool,
    pub json_content: String,
}

pub struct RustUserPresetBank {
    directory: String,
    presets: Vec<RustUserPresetRecord>,
}

impl RustUserPresetBank {
    pub fn new() -> Self {
        Self {
            directory: String::new(),
            presets: Vec::with_capacity(64),
        }
    }

    pub fn set_directory(&mut self, dir_path: &str) {
        self.directory = dir_path.to_string();
    }

    pub fn get_directory(&self) -> &str {
        &self.directory
    }

    pub fn clear(&mut self) {
        self.presets.clear();
    }

    pub fn add_preset(
        &mut self,
        file_path: &str,
        name: &str,
        category: &str,
        author: &str,
        desc: &str,
        version: &str,
        tags_csv: &str,
        favorite: bool,
        json_content: &str,
    ) {
        let tags: Vec<String> = if tags_csv.is_empty() {
            Vec::new()
        } else {
            tags_csv
                .split(',')
                .map(|s| s.trim().to_string())
                .filter(|s| !s.is_empty())
                .collect()
        };

        self.presets.push(RustUserPresetRecord {
            file_path: file_path.to_string(),
            name: name.to_string(),
            category: category.to_string(),
            author: author.to_string(),
            description: desc.to_string(),
            version: version.to_string(),
            tags,
            favorite,
            json_content: json_content.to_string(),
        });
    }

    pub fn sort_alphabetical(&mut self) {
        self.presets.sort_by(|a, b| {
            a.name.to_lowercase().cmp(&b.name.to_lowercase())
        });
    }

    pub fn get_preset_count(&self) -> usize {
        self.presets.len()
    }

    pub fn get_preset(&self, index: usize) -> Option<&RustUserPresetRecord> {
        self.presets.get(index)
    }

    pub fn get_preset_mut(&mut self, index: usize) -> Option<&mut RustUserPresetRecord> {
        self.presets.get_mut(index)
    }

    pub fn delete_preset(&mut self, index: usize) -> bool {
        if index < self.presets.len() {
            self.presets.remove(index);
            true
        } else {
            false
        }
    }

    pub fn toggle_favorite(&mut self, index: usize) -> bool {
        if let Some(p) = self.presets.get_mut(index) {
            p.favorite = !p.favorite;
            true
        } else {
            false
        }
    }

    pub fn filter(
        &self,
        query: &str,
        category: &str,
        tags_csv: &str,
        favorites_only: bool,
    ) -> Vec<usize> {
        let q_lower = query.to_lowercase();
        let cat_filter = if category.is_empty() || category.eq_ignore_ascii_case("All") {
            None
        } else {
            Some(category.to_lowercase())
        };

        let req_tags: Vec<String> = if tags_csv.is_empty() {
            Vec::new()
        } else {
            tags_csv
                .split(',')
                .map(|s| s.trim().to_lowercase())
                .filter(|s| !s.is_empty())
                .collect()
        };

        let mut matches = Vec::new();

        for (i, p) in self.presets.iter().enumerate() {
            if favorites_only && !p.favorite {
                continue;
            }

            if let Some(ref cf) = cat_filter {
                if !p.category.to_lowercase().eq(cf) {
                    continue;
                }
            }

            // Check required tags
            let mut tags_match = true;
            for req in &req_tags {
                let has_tag = p.tags.iter().any(|t| t.to_lowercase() == *req);
                if !has_tag {
                    tags_match = false;
                    break;
                }
            }
            if !tags_match {
                continue;
            }

            // Check query search in name, author, description
            if !q_lower.is_empty() {
                let n_lower = p.name.to_lowercase();
                let a_lower = p.author.to_lowercase();
                let d_lower = p.description.to_lowercase();

                if !n_lower.contains(&q_lower)
                    && !a_lower.contains(&q_lower)
                    && !d_lower.contains(&q_lower)
                {
                    continue;
                }
            }

            matches.push(i);
        }

        matches
    }

    pub fn get_all_tags(&self) -> Vec<String> {
        let mut tags = Vec::new();
        for p in &self.presets {
            for t in &p.tags {
                if !tags.iter().any(|existing: &String| existing.eq_ignore_ascii_case(t)) {
                    tags.push(t.clone());
                }
            }
        }
        tags.sort_by(|a, b| a.to_lowercase().cmp(&b.to_lowercase()));
        tags
    }

    pub fn get_all_categories(&self) -> Vec<String> {
        let mut cats = Vec::new();
        for p in &self.presets {
            if !p.category.is_empty()
                && !cats.iter().any(|existing: &String| existing.eq_ignore_ascii_case(&p.category))
            {
                cats.push(p.category.clone());
            }
        }
        cats.sort_by(|a, b| a.to_lowercase().cmp(&b.to_lowercase()));
        cats
    }

    pub fn get_all_tags_csv(&self) -> String {
        self.get_all_tags().join(",")
    }

    pub fn get_all_categories_csv(&self) -> String {
        self.get_all_categories().join(",")
    }

    pub fn get_preset_tags_csv(&self, index: usize) -> String {
        self.presets.get(index).map(|p| p.tags.join(",")).unwrap_or_default()
    }

    pub fn get_preset_json_len(&self, index: usize) -> usize {
        self.presets.get(index).map(|p| p.json_content.len()).unwrap_or(0)
    }

    pub fn update_preset_json(&mut self, index: usize, json_content: &str) -> bool {
        if let Some(p) = self.presets.get_mut(index) {
            p.json_content = json_content.to_string();
            true
        } else {
            false
        }
    }
}
