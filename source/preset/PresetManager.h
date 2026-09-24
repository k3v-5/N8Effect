#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "GraphSerializer.h"

namespace audio_graph {

/**
 * @brief Gestor central de Presets de Fábrica y de Usuario (Reglas 21 y 27).
 */
class PresetManager {
public:
    struct PresetEntry {
        std::string name;
        std::string category;
        std::string jsonContent;
    };

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
        names.reserve(userPresets_.size());
        for (const auto& [name, _] : userPresets_) {
            names.push_back(name);
        }
        return names;
    }

private:
    void populateFactoryPresets() {
        factoryPresets_.clear();

        // 1. Ethereal Shimmer Pad
        factoryPresets_.push_back({
            "Ethereal Shimmer Pad",
            "Atmosphere",
            R"({
  "schemaVersion": 1,
  "name": "Ethereal Shimmer Pad",
  "author": "N8Audio",
  "category": "Atmosphere",
  "description": "Shimmer etéreo con transposición de octava, reverberación FDN y congelamiento espectral",
  "dryLevel": 1.0,
  "wetLevel": 0.85,
  "macros": [0.7, 0.4, 0.8, 0.5, 0.2, 0.6, 0.7, 0.0],
  "nodes": [
    { "id": 1, "name": "Pitch Shift +12", "type": 11, "x": 60.0, "y": 80.0, "params": { "1": 12.0, "2": 0.0, "3": 1.0 } },
    { "id": 2, "name": "FDN Reverb", "type": 6, "x": 260.0, "y": 80.0, "params": { "1": 0.85, "2": 3.5, "3": 5000.0, "4": 20.0, "5": 0.8 } },
    { "id": 3, "name": "Spectral Freeze", "type": 12, "x": 460.0, "y": 80.0, "params": { "1": 0.0, "2": 0.65, "3": 0.2, "4": 0.5 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})"
        });

        // 2. Glitch Stutter Beat
        factoryPresets_.push_back({
            "Glitch Stutter Beat",
            "Glitch/Rhythm",
            R"({
  "schemaVersion": 1,
  "name": "Glitch Stutter Beat",
  "author": "N8Audio",
  "category": "Glitch/Rhythm",
  "description": "Rebanador rítmico glitch sincronizado con delay ping-pong y saturación analógica",
  "dryLevel": 0.9,
  "wetLevel": 0.95,
  "macros": [0.4, 0.8, 0.3, 0.6, 0.85, 0.5, 0.9, 0.0],
  "nodes": [
    { "id": 1, "name": "Glitch Slicer", "type": 14, "x": 60.0, "y": 80.0, "params": { "1": 2.0, "2": 0.65, "3": 0.3, "4": 0.9, "5": 0.8 } },
    { "id": 2, "name": "Ping-Pong Delay", "type": 5, "x": 260.0, "y": 80.0, "params": { "1": 180.0, "2": 360.0, "3": 0.55, "4": 8000.0, "5": 1.0, "6": 0.6 } },
    { "id": 3, "name": "Distortion", "type": 8, "x": 460.0, "y": 80.0, "params": { "1": 0.0, "2": 8.0, "3": 4000.0, "4": 0.4 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})"
        });

        // 3. Modal Vocal Resonator
        factoryPresets_.push_back({
            "Modal Vocal Resonator",
            "Resonance",
            R"({
  "schemaVersion": 1,
  "name": "Modal Vocal Resonator",
  "author": "N8Audio",
  "category": "Resonance",
  "description": "Banco de 6 resonadores afinados armónicos con ecualización de medios y difusión espacial",
  "dryLevel": 1.0,
  "wetLevel": 0.75,
  "macros": [0.5, 0.5, 0.6, 0.7, 0.1, 0.4, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Resonator Bank", "type": 13, "x": 60.0, "y": 80.0, "params": { "1": 330.0, "2": 2.0, "3": 0.1, "4": 0.15, "5": 0.75 } },
    { "id": 2, "name": "Parametric EQ", "type": 4, "x": 260.0, "y": 80.0, "params": { "1": 120.0, "2": 0.0, "3": 1200.0, "4": 3.0, "5": 1.5, "6": 8000.0, "7": 1.0 } },
    { "id": 3, "name": "FDN Reverb", "type": 6, "x": 460.0, "y": 80.0, "params": { "1": 0.6, "2": 2.0, "3": 6000.0, "4": 15.0, "5": 0.5 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})"
        });

        // 4. OTT Multiband Slam
        factoryPresets_.push_back({
            "OTT Multiband Slam",
            "Dynamics",
            R"({
  "schemaVersion": 1,
  "name": "OTT Multiband Slam",
  "author": "N8Audio",
  "category": "Dynamics",
  "description": "Procesamiento multibanda con compresión agresiva ascendente y descendente estilo OTT",
  "dryLevel": 1.0,
  "wetLevel": 1.0,
  "macros": [0.6, 0.2, 0.1, 0.5, 0.3, 0.8, 0.9, 0.0],
  "nodes": [
    { "id": 1, "name": "Multiband OTT", "type": 15, "x": 80.0, "y": 80.0, "params": { "1": 250.0, "2": 2500.0, "3": 1.2, "4": 1.2, "5": 1.2, "6": 0.8 } },
    { "id": 2, "name": "VCA Compressor", "type": 9, "x": 300.0, "y": 80.0, "params": { "1": -16.0, "2": 4.0, "3": 20.0, "4": 120.0, "5": 3.0, "6": 6.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 }
  ]
})"
        });

        // 5. Granular Ambient Cloud
        factoryPresets_.push_back({
            "Granular Ambient Cloud",
            "Granular",
            R"({
  "schemaVersion": 1,
  "name": "Granular Ambient Cloud",
  "author": "N8Audio",
  "category": "Granular",
  "description": "Nube estéreo densa de 128 micro-granos con jitter temporal, difusión y eco espacial",
  "dryLevel": 0.95,
  "wetLevel": 0.8,
  "macros": [0.8, 0.6, 0.9, 0.4, 0.4, 0.9, 0.6, 0.0],
  "nodes": [
    { "id": 1, "name": "Granular Cloud", "type": 7, "x": 60.0, "y": 80.0, "params": { "1": 90.0, "2": 25.0, "3": 35.0, "4": 0.0, "5": 3.0, "6": 0.7, "7": 0.8 } },
    { "id": 2, "name": "Stereo Delay", "type": 5, "x": 260.0, "y": 80.0, "params": { "1": 250.0, "2": 375.0, "3": 0.45, "4": 7000.0, "5": 0.0, "6": 0.5 } },
    { "id": 3, "name": "FDN Reverb", "type": 6, "x": 460.0, "y": 80.0, "params": { "1": 0.75, "2": 2.8, "3": 5500.0, "4": 15.0, "5": 0.6 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})"
        });

        // 6. Pure Sovereign Clean
        factoryPresets_.push_back({
            "Pure Sovereign Clean",
            "Utility",
            R"({
  "schemaVersion": 1,
  "name": "Pure Sovereign Clean",
  "author": "N8Audio",
  "category": "Utility",
  "description": "Dry 100% puro bit a bit sin alteración con cadena de análisis de eventos",
  "dryLevel": 1.0,
  "wetLevel": 0.0,
  "macros": [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Passthrough Node", "type": 3, "x": 100.0, "y": 80.0, "params": {} }
  ],
  "connections": []
})"
        });
    }

    std::vector<PresetEntry> factoryPresets_;
    std::unordered_map<std::string, std::string> userPresets_;
};

} // namespace audio_graph
