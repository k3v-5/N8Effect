#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <array>
#include <chrono>
#include "../dsp/AllDspNodes.h"
#include "GraphSerializer.h"
#include "FactoryPresetCatalog.h"
#include "UserPresetBank.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Gestor central de Presets de Fábrica y de Usuario, Ranuras A/B y Snapshots (Reglas 21, 22, 27, 46, 47).
 * Acelerado por el motor nativo Rust n8_rust_core para comparaciones A/B fluidas y morphing temporal sin locks.
 */
class PresetManager {
public:
    using PresetEntry = FactoryPresetEntry;

    enum class Slot : uint8_t {
        SlotA = 0,
        SlotB = 1
    };

    PresetManager() {
        rustCore_ = n8_rust_preset_manager_create();
        populateFactoryPresets();
    }

    ~PresetManager() {
        if (rustCore_ != nullptr) {
            n8_rust_preset_manager_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    PresetManager(const PresetManager& other) {
        rustCore_ = n8_rust_preset_manager_create();
        copyFrom(other);
    }

    PresetManager& operator=(const PresetManager& other) {
        if (this != &other) {
            copyFrom(other);
        }
        return *this;
    }

    PresetManager(PresetManager&& other) noexcept {
        rustCore_ = other.rustCore_;
        other.rustCore_ = nullptr;
        factoryPresets_ = std::move(other.factoryPresets_);
        userPresets_ = std::move(other.userPresets_);
        userBank_ = std::move(other.userBank_);
    }

    PresetManager& operator=(PresetManager&& other) noexcept {
        if (this != &other) {
            if (rustCore_ != nullptr) {
                n8_rust_preset_manager_destroy(rustCore_);
            }
            rustCore_ = other.rustCore_;
            other.rustCore_ = nullptr;
            factoryPresets_ = std::move(other.factoryPresets_);
            userPresets_ = std::move(other.userPresets_);
            userBank_ = std::move(other.userBank_);
        }
        return *this;
    }

    // =========================================================================
    // Catálogo de Fábrica (Rules 19, 20, 21)
    // =========================================================================

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

    // =========================================================================
    // Gestión de Presets de Usuario
    // =========================================================================

    void saveUserPreset(std::string_view name, const Graph& graph, const PresetMetadata& meta, const std::array<float, 8>& macros) {
        PresetMetadata userMeta = meta;
        userMeta.name = std::string(name);
        userMeta.category = "User";

        std::string json = GraphSerializer::serialize(graph, userMeta, macros);
        userPresets_[std::string(name)] = json;

        if (rustCore_ != nullptr) {
            n8_rust_preset_manager_save_user_preset(rustCore_, userMeta.name.c_str(), json.c_str(), macros.data());
        }
    }

    bool loadUserPreset(std::string_view name, Graph& outGraph, PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
        std::string json;
        if (rustCore_ != nullptr) {
            std::vector<char> buf(65536, 0);
            std::array<float, 8> macs{};
            std::string n(name);
            if (n8_rust_preset_manager_load_user_preset(rustCore_, n.c_str(), buf.data(), buf.size(), macs.data())) {
                json = std::string(buf.data());
                outMacros = macs;
            }
        }

        if (json.empty()) {
            auto it = userPresets_.find(std::string(name));
            if (it == userPresets_.end()) return false;
            json = it->second;
        }

        std::string err;
        return GraphSerializer::deserialize(json, outGraph, outMeta, outMacros, err);
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

    // =========================================================================
    // A/B Comparison & Temporal Morphing (Roadmap Phase 4, Rules 21, 27)
    // =========================================================================

    void storeSlotA(std::string_view name, const Graph& graph, const PresetMetadata& meta, const std::array<float, 8>& macros) {
        if (rustCore_ == nullptr) return;
        std::string json = GraphSerializer::serialize(graph, meta, macros);
        std::string n(name);
        uint64_t ts = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        n8_rust_preset_manager_store_slot_a(rustCore_, n.c_str(), json.c_str(), macros.data(), ts);
    }

    void storeSlotB(std::string_view name, const Graph& graph, const PresetMetadata& meta, const std::array<float, 8>& macros) {
        if (rustCore_ == nullptr) return;
        std::string json = GraphSerializer::serialize(graph, meta, macros);
        std::string n(name);
        uint64_t ts = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        n8_rust_preset_manager_store_slot_b(rustCore_, n.c_str(), json.c_str(), macros.data(), ts);
    }

    bool recallSlotA(Graph& outGraph, PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
        if (rustCore_ == nullptr) return false;
        char nameBuf[128] = { 0 };
        std::vector<char> jsonBuf(65536, 0);
        if (n8_rust_preset_manager_get_slot_a(rustCore_, nameBuf, sizeof(nameBuf), jsonBuf.data(), jsonBuf.size(), outMacros.data())) {
            std::string err;
            return GraphSerializer::deserialize(jsonBuf.data(), outGraph, outMeta, outMacros, err);
        }
        return false;
    }

    bool recallSlotB(Graph& outGraph, PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
        if (rustCore_ == nullptr) return false;
        char nameBuf[128] = { 0 };
        std::vector<char> jsonBuf(65536, 0);
        if (n8_rust_preset_manager_get_slot_b(rustCore_, nameBuf, sizeof(nameBuf), jsonBuf.data(), jsonBuf.size(), outMacros.data())) {
            std::string err;
            return GraphSerializer::deserialize(jsonBuf.data(), outGraph, outMeta, outMacros, err);
        }
        return false;
    }

    void copyAToB() noexcept {
        if (rustCore_ != nullptr) n8_rust_preset_manager_copy_a_to_b(rustCore_);
    }

    void copyBToA() noexcept {
        if (rustCore_ != nullptr) n8_rust_preset_manager_copy_b_to_a(rustCore_);
    }

    void swapSlots() noexcept {
        if (rustCore_ != nullptr) n8_rust_preset_manager_swap_slots(rustCore_);
    }

    void setMorphFactor(float factor) noexcept {
        if (rustCore_ != nullptr) n8_rust_preset_manager_set_morph_factor(rustCore_, factor);
    }

    float getMorphFactor() const noexcept {
        if (rustCore_ != nullptr) return n8_rust_preset_manager_get_morph_factor(rustCore_);
        return 0.0f;
    }

    std::array<float, 8> getInterpolatedMacros() const noexcept {
        std::array<float, 8> macs{};
        if (rustCore_ != nullptr) {
            n8_rust_preset_manager_get_interpolated_macros(rustCore_, macs.data());
        }
        return macs;
    }

    // =========================================================================
    // Session Snapshots History
    // =========================================================================

    void takeSnapshot(std::string_view name, const Graph& graph, const PresetMetadata& meta, const std::array<float, 8>& macros) {
        if (rustCore_ == nullptr) return;
        std::string json = GraphSerializer::serialize(graph, meta, macros);
        std::string n(name);
        uint64_t ts = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        n8_rust_preset_manager_take_snapshot(rustCore_, n.c_str(), json.c_str(), macros.data(), ts);
    }

    size_t getSnapshotCount() const noexcept {
        if (rustCore_ != nullptr) return n8_rust_preset_manager_get_snapshot_count(rustCore_);
        return 0;
    }

    bool restoreSnapshot(size_t index, Graph& outGraph, PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
        if (rustCore_ == nullptr) return false;
        char nameBuf[128] = { 0 };
        std::vector<char> jsonBuf(65536, 0);
        if (n8_rust_preset_manager_get_snapshot(rustCore_, index, nameBuf, sizeof(nameBuf), jsonBuf.data(), jsonBuf.size(), outMacros.data())) {
            std::string err;
            return GraphSerializer::deserialize(jsonBuf.data(), outGraph, outMeta, outMacros, err);
        }
        return false;
    }

private:
    void populateFactoryPresets() {
        factoryPresets_ = createFactoryPresetCatalog();
    }

    void copyFrom(const PresetManager& other) {
        factoryPresets_ = other.factoryPresets_;
        userPresets_ = other.userPresets_;
        userBank_.setDirectory(other.userBank_.getDirectory());
    }

    RustPresetManager* rustCore_{ nullptr };
    std::vector<PresetEntry> factoryPresets_;
    std::unordered_map<std::string, std::string> userPresets_;
    UserPresetBank userBank_;
};

} // namespace audio_graph
