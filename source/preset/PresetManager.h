#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include "../dsp/AllDspNodes.h"
#include "GraphSerializer.h"
#include "FactoryPresetCatalog.h"
#include "UserPresetBank.h"

namespace audio_graph {

/**
 * @brief Gestor central de Presets de Fábrica y de Usuario (Reglas 21 y 27).
 */
class PresetManager {
public:
    using PresetEntry = FactoryPresetEntry;

    PresetManager() {
        populateFactoryPresets();
    }

    const std::vector<PresetEntry>& getFactoryPresets() const noexcept {
        return factoryPresets_;
    }

    std::vector<std::string> getFactoryPresetNames() const {
        std::vector<std::string> names;
        names.reserve(factoryPresets_.size());
        for (const auto& p : factoryPresets_) {
            names.push_back(p.name);
        }
        return names;
    }

    std::vector<std::string> getCategories() const {
        std::vector<std::string> cats;
        for (const auto& p : factoryPresets_) {
            if (std::find(cats.begin(), cats.end(), p.category) == cats.end()) {
                cats.push_back(p.category);
            }
        }
        return cats;
    }

    bool loadFactoryPreset(size_t index, Graph& outGraph, PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
        if (index >= factoryPresets_.size()) return false;
        std::string err;
        return GraphSerializer::deserialize(factoryPresets_[index].jsonContent, outGraph, outMeta, outMacros, err);
    }

    void saveUserPreset(std::string_view name, const Graph& graph, const PresetMetadata& meta, const std::array<float, 8>& macros) {
        PresetMetadata userMeta = meta;
        userMeta.name = std::string(name);
        userMeta.category = "User";

        std::string json = GraphSerializer::serialize(graph, userMeta, macros);
        userPresets_[std::string(name)] = std::move(json);
    }

    bool loadUserPreset(std::string_view name, Graph& outGraph, PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
        auto it = userPresets_.find(std::string(name));
        if (it == userPresets_.end()) return false;

        std::string err;
        return GraphSerializer::deserialize(it->second, outGraph, outMeta, outMacros, err);
    }

    std::vector<std::string> getUserPresetNames() const {
        std::vector<std::string> names;
        names.reserve(userBank_.getPresetCount() + userPresets_.size());
        for (const auto& p : userBank_.getPresets()) {
            names.push_back(p.metadata.name);
        }
        for (const auto& [name, _] : userPresets_) {
            if (std::find(names.begin(), names.end(), name) == names.end()) {
                names.push_back(name);
            }
        }
        return names;
    }

    UserPresetBank& getUserBank() noexcept { return userBank_; }
    const UserPresetBank& getUserBank() const noexcept { return userBank_; }

private:
    void populateFactoryPresets() {
        factoryPresets_ = createFactoryPresetCatalog();
    }

    std::vector<PresetEntry> factoryPresets_;
    std::unordered_map<std::string, std::string> userPresets_;
    UserPresetBank userBank_;
};

} // namespace audio_graph
