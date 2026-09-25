#pragma once

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include "GraphSerializer.h"

namespace audio_graph {

/**
 * @brief Registro indexado de un preset de usuario en disco con metadatos y tags (Reglas 21, 27).
 */
struct UserPresetRecord {
    std::string filePath;
    PresetMetadata metadata;
    std::string jsonContent;
};

/**
 * @brief Gestor de persistencia física en disco e indexación reactiva por tags/categorías (Reglas 21, 27).
 * Opera en hilo de background o GUI, nunca en el audio thread.
 */
class UserPresetBank {
public:
    UserPresetBank() = default;

    void setDirectory(const std::string& dirPath) {
        directory_ = dirPath;
        std::error_code ec;
        if (!std::filesystem::exists(directory_, ec)) {
            std::filesystem::create_directories(directory_, ec);
        }
        rescan();
    }

    const std::string& getDirectory() const noexcept {
        return directory_;
    }

    void rescan() {
        presets_.clear();
        if (directory_.empty()) return;

        std::error_code ec;
        if (!std::filesystem::exists(directory_, ec) || !std::filesystem::is_directory(directory_, ec)) {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(directory_, ec)) {
            if (entry.is_regular_file(ec) && entry.path().extension() == ".n8preset") {
                std::ifstream inFile(entry.path(), std::ios::in | std::ios::binary);
                if (inFile.is_open()) {
                    std::string content((std::istreambuf_iterator<char>(inFile)),
                                        std::istreambuf_iterator<char>());
                    inFile.close();

                    Graph dummyGraph;
                    PresetMetadata meta;
                    std::array<float, 8> dummyMacros{};
                    std::string err;

                    if (GraphSerializer::deserialize(content, dummyGraph, meta, dummyMacros, err)) {
                        UserPresetRecord rec;
                        rec.filePath = entry.path().string();
                        rec.metadata = meta;
                        rec.jsonContent = std::move(content);
                        presets_.push_back(std::move(rec));
                    }
                }
            }
        }

        // Ordenar alfabéticamente por nombre
        std::sort(presets_.begin(), presets_.end(), [](const UserPresetRecord& a, const UserPresetRecord& b) {
            return toLower(a.metadata.name) < toLower(b.metadata.name);
        });
    }

    bool savePreset(const Graph& graph,
                    const PresetMetadata& meta,
                    const std::array<float, 8>& macros)
    {
        if (directory_.empty()) return false;

        std::error_code ec;
        if (!std::filesystem::exists(directory_, ec)) {
            std::filesystem::create_directories(directory_, ec);
        }

        std::string filename = sanitizeFilename(meta.name) + ".n8preset";
        auto fullPath = (std::filesystem::path(directory_) / filename).string();

        std::string json = GraphSerializer::serialize(graph, meta, macros);
        std::ofstream outFile(fullPath, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!outFile.is_open()) return false;

        outFile.write(json.data(), json.size());
        outFile.close();

        rescan();
        return true;
    }

    bool deletePreset(size_t index) {
        if (index >= presets_.size()) return false;

        std::error_code ec;
        std::filesystem::remove(presets_[index].filePath, ec);
        presets_.erase(presets_.begin() + static_cast<ptrdiff_t>(index));
        return !ec;
    }

    bool toggleFavorite(size_t index) {
        if (index >= presets_.size()) return false;

        auto& rec = presets_[index];
        rec.metadata.favorite = !rec.metadata.favorite;

        Graph dummyGraph;
        PresetMetadata dummyMeta;
        std::array<float, 8> dummyMacros{};
        std::string err;

        if (GraphSerializer::deserialize(rec.jsonContent, dummyGraph, dummyMeta, dummyMacros, err)) {
            dummyMeta.favorite = rec.metadata.favorite;
            rec.jsonContent = GraphSerializer::serialize(dummyGraph, dummyMeta, dummyMacros);

            std::ofstream outFile(rec.filePath, std::ios::out | std::ios::trunc | std::ios::binary);
            if (outFile.is_open()) {
                outFile.write(rec.jsonContent.data(), rec.jsonContent.size());
                outFile.close();
            }
        }
        return true;
    }

    std::vector<size_t> filter(const std::string& query,
                               const std::string& category,
                               const std::vector<std::string>& requiredTags,
                               bool favoritesOnly) const
    {
        std::vector<size_t> matches;
        const std::string qLower = toLower(query);

        for (size_t i = 0; i < presets_.size(); ++i) {
            const auto& meta = presets_[i].metadata;

            if (favoritesOnly && !meta.favorite) {
                continue;
            }

            if (!category.empty() && category != "All" && meta.category != category) {
                continue;
            }

            bool tagsMatch = true;
            for (const auto& reqTag : requiredTags) {
                if (reqTag.empty()) continue;
                bool hasThisTag = false;
                for (const auto& tag : meta.tags) {
                    if (toLower(tag) == toLower(reqTag)) {
                        hasThisTag = true;
                        break;
                    }
                }
                if (!hasThisTag) {
                    tagsMatch = false;
                    break;
                }
            }
            if (!tagsMatch) continue;

            if (!qLower.empty()) {
                const std::string nameLower = toLower(meta.name);
                const std::string authorLower = toLower(meta.author);
                const std::string descLower = toLower(meta.description);

                if (nameLower.find(qLower) == std::string::npos &&
                    authorLower.find(qLower) == std::string::npos &&
                    descLower.find(qLower) == std::string::npos) {
                    continue;
                }
            }

            matches.push_back(i);
        }

        return matches;
    }

    std::vector<std::string> getAllTags() const {
        std::vector<std::string> tags;
        for (const auto& p : presets_) {
            for (const auto& t : p.metadata.tags) {
                if (std::find(tags.begin(), tags.end(), t) == tags.end()) {
                    tags.push_back(t);
                }
            }
        }
        std::sort(tags.begin(), tags.end());
        return tags;
    }

    std::vector<std::string> getAllCategories() const {
        std::vector<std::string> cats;
        for (const auto& p : presets_) {
            if (!p.metadata.category.empty() && std::find(cats.begin(), cats.end(), p.metadata.category) == cats.end()) {
                cats.push_back(p.metadata.category);
            }
        }
        std::sort(cats.begin(), cats.end());
        return cats;
    }

    const std::vector<UserPresetRecord>& getPresets() const noexcept {
        return presets_;
    }

    const UserPresetRecord* getPreset(size_t index) const noexcept {
        if (index < presets_.size()) {
            return &presets_[index];
        }
        return nullptr;
    }

    size_t getPresetCount() const noexcept {
        return presets_.size();
    }

private:
    static std::string toLower(std::string_view s) {
        std::string res;
        res.reserve(s.size());
        for (char c : s) {
            res += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return res;
    }

    static std::string sanitizeFilename(std::string_view name) {
        std::string res;
        for (char c : name) {
            if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                res += '_';
            } else {
                res += c;
            }
        }
        if (res.empty()) res = "Untitled";
        return res;
    }

    std::string directory_;
    std::vector<UserPresetRecord> presets_;
};

} // namespace audio_graph
