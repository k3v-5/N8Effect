#pragma once

#include <vector>
#include <string>
#include <string_view>
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

struct FactoryPresetEntry {
    std::string name;
    std::string category;
    std::string jsonContent;
};

/**
 * @brief Indexador y buscador nativo acelerado en Rust para el Catálogo Maestro de Fábrica (Reglas 4, 19, 20, 21, 46)
 */
class FactoryPresetCatalogIndex {
public:
    FactoryPresetCatalogIndex() {
        rustCore_ = n8_rust_factory_preset_catalog_create();
    }

    ~FactoryPresetCatalogIndex() {
        if (rustCore_ != nullptr) {
            n8_rust_factory_preset_catalog_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    void indexCatalog(const std::vector<FactoryPresetEntry>& catalog) {
        if (rustCore_ == nullptr) return;
        n8_rust_factory_preset_catalog_reset(rustCore_);
        for (const auto& entry : catalog) {
            n8_rust_factory_preset_catalog_register_preset(
                rustCore_, entry.name.c_str(), entry.category.c_str(), entry.jsonContent.c_str());
        }
    }

    size_t getCount() const noexcept {
        if (rustCore_ != nullptr) return n8_rust_factory_preset_catalog_get_count(rustCore_);
        return 0;
    }

    size_t getCategoryCount() const noexcept {
        if (rustCore_ != nullptr) return n8_rust_factory_preset_catalog_get_category_count(rustCore_);
        return 0;
    }

    std::vector<size_t> search(std::string_view query) const {
        std::vector<size_t> results;
        if (rustCore_ == nullptr) return results;
        std::string q(query);
        std::vector<size_t> buf(128, 0);
        size_t n = n8_rust_factory_preset_catalog_search(rustCore_, q.c_str(), buf.data(), buf.size());
        results.assign(buf.begin(), buf.begin() + std::min(n, buf.size()));
        return results;
    }

    std::vector<size_t> filterByCategory(std::string_view category) const {
        std::vector<size_t> results;
        if (rustCore_ == nullptr) return results;
        std::string c(category);
        std::vector<size_t> buf(128, 0);
        size_t n = n8_rust_factory_preset_catalog_filter_by_category(rustCore_, c.c_str(), buf.data(), buf.size());
        results.assign(buf.begin(), buf.begin() + std::min(n, buf.size()));
        return results;
    }

private:
    RustFactoryPresetCatalog* rustCore_{ nullptr };
};


/**
 * @brief Catálogo Maestro de Presets DAG de Alta Complejidad (Reglas 4, 21, 22, 44 y 46).
 * 60 Presets Flagship organizados en 15 categorías temáticas especializadas.
 * Todos los presets implementan topologías no triviales de Grafo Acíclico Dirigido (DAG):
 * bifurcaciones paralelas (fan-out/fan-in), decodificación Mid/Side, filtros mutantes no lineales (ZDF),
 * dispersión de fase allpass y resonadores fractales basados en la proporción áurea.
 */
inline std::vector<FactoryPresetEntry> createFactoryPresetCatalog() {
    std::vector<FactoryPresetEntry> catalog;
    catalog.reserve(68);

    // -------------------------------------------------------------------------
    // Dispersión & Texturas Cuánticas -> Quantum Chirp Diffusion Rig
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Quantum Chirp Diffusion Rig",
        "Dispersión & Texturas Cuánticas",
        R"json({
  "schemaVersion": 1,
  "name": "Quantum Chirp Diffusion Rig",
  "author": "N8Audio",
  "category": "Dispersi\u00f3n & Texturas Cu\u00e1nticas",
  "description": "Cascada de 20 etapas de dispersi\u00f3n allpass con bifurcaci\u00f3n paralela a pitch delay y tr\u00e9molo din\u00e1mico",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [
    0.7,
    0.5,
    0.8,
    0.4,
    0.6,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Transient Snap",
      "type": 33,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.7,
        "2": -0.2,
        "3": 1.2,
        "4": 1.0,
        "5": 2.0,
        "6": 1.0
      }
    },
    {
      "id": 2,
      "name": "Phase Dispersion",
      "type": 65,
      "x": 220.0,
      "y": 80.0,
      "params": {
        "1": 20.0,
        "2": 1800.0,
        "3": 0.85,
        "4": 12000.0,
        "5": 0.6,
        "6": 1.0
      }
    },
    {
      "id": 3,
      "name": "Pitch Delay",
      "type": 64,
      "x": 420.0,
      "y": 40.0,
      "params": {
        "1": 180.0,
        "2": 270.0,
        "3": 12.0,
        "4": 12.0,
        "5": 0.55,
        "6": 0.0,
        "7": 8000.0,
        "8": 0.7
      }
    },
    {
      "id": 4,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 420.0,
      "y": 140.0,
      "params": {
        "1": 4.5,
        "2": 0.8,
        "3": 1.0,
        "4": 1.0,
        "5": 0.4,
        "6": 0.6,
        "7": 0.85
      }
    },
    {
      "id": 5,
      "name": "Spectral Smear",
      "type": 52,
      "x": 640.0,
      "y": 80.0,
      "params": {
        "1": 4.0,
        "2": 0.85,
        "3": 0.9,
        "4": 0.0,
        "5": 0.8
      }
    },
    {
      "id": 6,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 840.0,
      "y": 80.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Dispersión & Texturas Cuánticas -> Acoustic Ice Cracker
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Acoustic Ice Cracker",
        "Dispersión & Texturas Cuánticas",
        R"json({
  "schemaVersion": 1,
  "name": "Acoustic Ice Cracker",
  "author": "N8Audio",
  "category": "Dispersi\u00f3n & Texturas Cu\u00e1nticas",
  "description": "Dispersi\u00f3n de chirps no lineales sobre fracturas de decimation y resonador comb con saturaci\u00f3n",
  "dryLevel": 0.2,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.6,
    0.5,
    0.7,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Bitcrusher Decimator",
      "type": 40,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 12.0,
        "2": 2.5,
        "3": 0.02,
        "4": 0.3,
        "5": 0.0,
        "6": 0.6
      }
    },
    {
      "id": 2,
      "name": "Phase Dispersion",
      "type": 65,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 24.0,
        "2": 2400.0,
        "3": 0.92,
        "4": 14000.0,
        "5": 0.75,
        "6": 1.0
      }
    },
    {
      "id": 3,
      "name": "Comb Resonator",
      "type": 63,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 440.0,
        "2": 0.88,
        "3": 9000.0,
        "4": 15.0,
        "5": 0.6
      }
    },
    {
      "id": 4,
      "name": "Multiband Saturator",
      "type": 62,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 1.8,
        "2": 1.0,
        "3": 2.2,
        "4": 2.0,
        "5": 1.6,
        "6": 1.0,
        "7": 200.0,
        "8": 3500.0,
        "9": 0.8
      }
    },
    {
      "id": 5,
      "name": "Space Delay",
      "type": 28,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 220.0,
        "2": 330.0,
        "3": 0.5,
        "4": 7500.0,
        "5": 1.0,
        "6": 0.6
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.8,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Dispersión & Texturas Cuánticas -> Laser Droplet Phantasm
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Laser Droplet Phantasm",
        "Dispersión & Texturas Cuánticas",
        R"json({
  "schemaVersion": 1,
  "name": "Laser Droplet Phantasm",
  "author": "N8Audio",
  "category": "Dispersi\u00f3n & Texturas Cu\u00e1nticas",
  "description": "Slicing r\u00edtmico convertido en gotas de lluvia ac\u00fastica y chirps l\u00e1ser con transposici\u00f3n espectral",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [
    0.75,
    0.4,
    0.85,
    0.5,
    0.6,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Audio Slicer",
      "type": 49,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.0,
        "2": 8.0,
        "3": -1.0,
        "4": 1.0,
        "5": 0.0,
        "6": 50.0,
        "7": 4.0,
        "8": 0.35,
        "9": 0.0,
        "10": 14000.0,
        "11": 0.95
      }
    },
    {
      "id": 2,
      "name": "Laser Dispersion",
      "type": 65,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 18.0,
        "2": 3200.0,
        "3": 0.88,
        "4": 15000.0,
        "5": 0.7,
        "6": 1.0
      }
    },
    {
      "id": 3,
      "name": "Pitch Shifter +7st",
      "type": 11,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 7.0,
        "2": 0.0,
        "3": 35.0,
        "4": 0.8
      }
    },
    {
      "id": 4,
      "name": "Reverse Reverb Swell",
      "type": 38,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 1.5,
        "2": 0.8,
        "3": 6500.0,
        "4": 0.85
      }
    },
    {
      "id": 5,
      "name": "Mid/Side Decoder",
      "type": 25,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.5,
        "2": 120.0,
        "3": 0.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 25.0,
        "4": 1.5,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Dispersión & Texturas Cuánticas -> Subatomic Smear Continuum
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Subatomic Smear Continuum",
        "Dispersión & Texturas Cuánticas",
        R"json({
  "schemaVersion": 1,
  "name": "Subatomic Smear Continuum",
  "author": "N8Audio",
  "category": "Dispersi\u00f3n & Texturas Cu\u00e1nticas",
  "description": "Cadena difusora de desfasamiento espectral FFT y dispersi\u00f3n allpass en peine hologr\u00e1fico",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [
    0.6,
    0.7,
    0.9,
    0.5,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Spectral Smear",
      "type": 52,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 5.0,
        "2": 0.9,
        "3": 0.92,
        "4": -0.1,
        "5": 0.85
      }
    },
    {
      "id": 2,
      "name": "Phase Dispersion",
      "type": 65,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 22.0,
        "2": 950.0,
        "3": 0.75,
        "4": 10000.0,
        "5": 0.55,
        "6": 0.9
      }
    },
    {
      "id": 3,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 2.0,
        "2": 0.5,
        "3": 0.8
      }
    },
    {
      "id": 4,
      "name": "Bipolar Flanger",
      "type": 18,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 0.25,
        "2": 0.8,
        "3": 2.0,
        "4": -0.8,
        "5": 0.75
      }
    },
    {
      "id": 5,
      "name": "Space Delay",
      "type": 28,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 350.0,
        "2": 525.0,
        "3": 0.6,
        "4": 6500.0,
        "5": 1.0,
        "6": 0.55
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 35.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Filtros Mutantes & Topología No Lineal -> Acid Vowel Metamorphosis
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Acid Vowel Metamorphosis",
        "Filtros Mutantes & Topología No Lineal",
        R"json({
  "schemaVersion": 1,
  "name": "Acid Vowel Metamorphosis",
  "author": "N8Audio",
  "category": "Filtros Mutantes & Topolog\u00eda No Lineal",
  "description": "SVF no lineal ZDF con barrido continuo de topolog\u00eda, modulaci\u00f3n FM por modulador en anillo y saturador multibanda",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.7,
    0.5,
    0.8,
    0.6,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Ring Modulator FM",
      "type": 19,
      "x": 40.0,
      "y": 140.0,
      "params": {
        "1": 180.0,
        "2": 0.0,
        "3": 0.0,
        "4": 0.9
      }
    },
    {
      "id": 2,
      "name": "Morphing SVF Acid",
      "type": 66,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 850.0,
        "2": 18.0,
        "3": 0.35,
        "4": 4.5,
        "5": 0.7
      }
    },
    {
      "id": 3,
      "name": "Multiband Saturator",
      "type": 62,
      "x": 440.0,
      "y": 40.0,
      "params": {
        "1": 2.2,
        "2": 1.0,
        "3": 3.0,
        "4": 2.0,
        "5": 1.5,
        "6": 1.0,
        "7": 220.0,
        "8": 3200.0,
        "9": 0.85
      }
    },
    {
      "id": 4,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 6.0,
        "2": 0.75,
        "3": 1.0,
        "4": 1.0,
        "5": 0.5,
        "6": 0.7,
        "7": 0.9
      }
    },
    {
      "id": 5,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 60.0,
        "2": 20.0,
        "3": 2.2,
        "4": 1.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 4
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Filtros Mutantes & Topología No Lineal -> Screaming Topology Wormhole
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Screaming Topology Wormhole",
        "Filtros Mutantes & Topología No Lineal",
        R"json({
  "schemaVersion": 1,
  "name": "Screaming Topology Wormhole",
  "author": "N8Audio",
  "category": "Filtros Mutantes & Topolog\u00eda No Lineal",
  "description": "Filtro auto-oscilante en lazo de retroalimentaci\u00f3n con corrimiento de frecuencia Hilbert y eco ping-pong",
  "dryLevel": 0.25,
  "wetLevel": 0.95,
  "macros": [
    0.85,
    0.75,
    0.6,
    0.7,
    0.8,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Morphing SVF Screamer",
      "type": 66,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 1400.0,
        "2": 24.0,
        "3": 0.55,
        "4": 5.0,
        "5": 0.0
      }
    },
    {
      "id": 2,
      "name": "Freq Shifter",
      "type": 20,
      "x": 240.0,
      "y": 30.0,
      "params": {
        "1": 7.0,
        "2": 0.3,
        "3": 0.8
      }
    },
    {
      "id": 3,
      "name": "Space Ping-Pong",
      "type": 28,
      "x": 240.0,
      "y": 140.0,
      "params": {
        "1": 240.0,
        "2": 360.0,
        "3": 0.6,
        "4": 8000.0,
        "5": 1.0,
        "6": 0.65
      }
    },
    {
      "id": 4,
      "name": "Comb Resonator",
      "type": 63,
      "x": 480.0,
      "y": 80.0,
      "params": {
        "1": 330.0,
        "2": 0.88,
        "3": 6000.0,
        "4": 15.0,
        "5": 0.7
      }
    },
    {
      "id": 5,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 680.0,
      "y": 80.0,
      "params": {
        "1": 2.0,
        "2": 0.6,
        "3": 0.8
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 880.0,
      "y": 80.0,
      "params": {
        "1": -0.8,
        "2": -0.1,
        "3": 25.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Filtros Mutantes & Topología No Lineal -> Cybernetic Formant Biome
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Cybernetic Formant Biome",
        "Filtros Mutantes & Topología No Lineal",
        R"json({
  "schemaVersion": 1,
  "name": "Cybernetic Formant Biome",
  "author": "N8Audio",
  "category": "Filtros Mutantes & Topolog\u00eda No Lineal",
  "description": "Bifurcaci\u00f3n en paralelo de formantes vocales humanos y resonancia mutante con compresi\u00f3n din\u00e1mica reactiva",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.6,
    0.7,
    0.75,
    0.5,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Transient Snap",
      "type": 33,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.6,
        "2": -0.2,
        "3": 1.0,
        "4": 1.0,
        "5": 2.0,
        "6": 1.0
      }
    },
    {
      "id": 2,
      "name": "Formant Vowel",
      "type": 31,
      "x": 240.0,
      "y": 30.0,
      "params": {
        "1": 0.35,
        "2": 5.0,
        "3": 1.2,
        "4": 0.5,
        "5": 0.85
      }
    },
    {
      "id": 3,
      "name": "Morphing SVF Peak",
      "type": 66,
      "x": 240.0,
      "y": 140.0,
      "params": {
        "1": 1800.0,
        "2": 8.0,
        "3": 0.75,
        "4": 2.5,
        "5": 0.0
      }
    },
    {
      "id": 4,
      "name": "Dynamic EQ",
      "type": 54,
      "x": 460.0,
      "y": 80.0,
      "params": {
        "1": 2500.0,
        "2": 2.0,
        "3": 4.0,
        "4": -18.0,
        "5": 10.0,
        "6": 100.0,
        "7": 0.8
      }
    },
    {
      "id": 5,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 3.0,
        "2": 0.5,
        "3": 0.8
      }
    },
    {
      "id": 6,
      "name": "Space Reverb",
      "type": 6,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": 0.85,
        "2": 4.0,
        "3": 6000.0,
        "4": 25.0,
        "5": 0.5
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Filtros Mutantes & Topología No Lineal -> Nonlinear Diode Quake
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Buchla 292 Vactrol Resonant Strike",
        "Filtros Mutantes & Topología No Lineal",
        R"json({
  "schemaVersion": 1,
  "name": "Buchla 292 Vactrol Resonant Strike",
  "author": "N8Audio",
  "category": "Filtros Mutantes & Topolog\u00eda No Lineal",
  "description": "Optoacoplador Buchla 292 Vactrol LPG con decaimiento l\u00e1nguido bifurcado a filtro mutante ZDF y resonador en peine",
  "dryLevel": 0.25,
  "wetLevel": 0.95,
  "macros": [
    0.85,
    0.6,
    0.7,
    0.8,
    0.6,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Multiband Saturator",
      "type": 62,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 2.5,
        "2": 1.0,
        "3": 2.0,
        "4": 2.0,
        "5": 1.8,
        "6": 1.0,
        "7": 180.0,
        "8": 3000.0,
        "9": 0.85
      }
    },
    {
      "id": 2,
      "name": "Buchla Vactrol LPG",
      "type": 68,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 0.18,
        "2": 0.65,
        "3": 220.0,
        "4": 0.9,
        "5": 0.5,
        "6": 2.2,
        "7": 1.0
      }
    },
    {
      "id": 3,
      "name": "Morphing SVF Drive",
      "type": 66,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 750.0,
        "2": 8.0,
        "3": 0.3,
        "4": 4.0,
        "5": 0.0
      }
    },
    {
      "id": 4,
      "name": "Comb Resonator",
      "type": 63,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 220.0,
        "2": 0.82,
        "3": 7000.0,
        "4": 12.0,
        "5": 0.7
      }
    },
    {
      "id": 5,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 5.0,
        "2": 0.7,
        "3": 1.0,
        "4": 1.0,
        "5": 0.4,
        "6": 0.6,
        "7": 0.8
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Placas Fractales & Resonadores Áureos -> Hyperbolic Crystal Cathedral
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Hyperbolic Crystal Cathedral",
        "Placas Fractales & Resonadores Áureos",
        R"json({
  "schemaVersion": 1,
  "name": "Hyperbolic Crystal Cathedral",
  "author": "N8Audio",
  "category": "Placas Fractales & Resonadores \u00c1ureos",
  "description": "Matriz f\u00edsica de 6 l\u00edneas con retardo \u00e1ureo (phi^k) dispersada en prisma de refracci\u00f3n y shimmer coral",
  "dryLevel": 0.2,
  "wetLevel": 0.95,
  "macros": [
    0.85,
    0.5,
    0.9,
    0.6,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Fractal Resonator",
      "type": 67,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 329.63,
        "2": 0.618,
        "3": 0.92,
        "4": 8500.0,
        "5": 0.9,
        "6": 0.85
      }
    },
    {
      "id": 2,
      "name": "Shimmer Reverb",
      "type": 50,
      "x": 260.0,
      "y": 30.0,
      "params": {
        "1": 7.0,
        "2": 7500.0,
        "3": 0.7,
        "4": 0.0,
        "5": 1.5,
        "6": 0.75
      }
    },
    {
      "id": 3,
      "name": "8-Voice Refraction",
      "type": 51,
      "x": 260.0,
      "y": 140.0,
      "params": {
        "1": 8.0,
        "2": 24.0,
        "3": 25.0,
        "4": 0.9,
        "5": 0.8,
        "6": 2.5,
        "7": 0.85
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 500.0,
      "y": 80.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 5,
      "name": "Mid/Side Decoder",
      "type": 25,
      "x": 700.0,
      "y": 80.0,
      "params": {
        "1": 1.6,
        "2": 110.0,
        "3": 0.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 900.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 35.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Placas Fractales & Resonadores Áureos -> Tibetan Singing Bowl Continuum
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Tibetan Singing Bowl Continuum",
        "Placas Fractales & Resonadores Áureos",
        R"json({
  "schemaVersion": 1,
  "name": "Tibetan Singing Bowl Continuum",
  "author": "N8Audio",
  "category": "Placas Fractales & Resonadores \u00c1ureos",
  "description": "Excitaci\u00f3n arm\u00f3nica impulsando resonador fractal \u00e1ureo con peine sintonizado y tr\u00e9molo ac\u00fastico",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.7,
    0.6,
    0.8,
    0.4,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Harmonic Exciter",
      "type": 35,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 3500.0,
        "2": 0.7,
        "3": 0.5,
        "4": 0.85
      }
    },
    {
      "id": 2,
      "name": "Fractal Resonator",
      "type": 67,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 196.0,
        "2": 0.5,
        "3": 0.96,
        "4": 6000.0,
        "5": 0.85,
        "6": 0.9
      }
    },
    {
      "id": 3,
      "name": "Comb Resonator",
      "type": 63,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 392.0,
        "2": 0.82,
        "3": 8000.0,
        "4": 10.0,
        "5": 0.6
      }
    },
    {
      "id": 4,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 2.5,
        "2": 0.65,
        "3": 1.0,
        "4": 1.0,
        "5": 0.3,
        "6": 0.5,
        "7": 0.8
      }
    },
    {
      "id": 5,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 45.0,
        "2": 15.0,
        "3": 1.8,
        "4": 1.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Placas Fractales & Resonadores Áureos -> Microtonal Glass Monochord
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Microtonal Glass Monochord",
        "Placas Fractales & Resonadores Áureos",
        R"json({
  "schemaVersion": 1,
  "name": "Microtonal Glass Monochord",
  "author": "N8Audio",
  "category": "Placas Fractales & Resonadores \u00c1ureos",
  "description": "Cuerda f\u00edsica pulsada de Karplus-Strong excitando matriz fractal con retardos de cristal de cuarzo",
  "dryLevel": 0.25,
  "wetLevel": 0.95,
  "macros": [
    0.8,
    0.5,
    0.85,
    0.6,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Karplus Plucked",
      "type": 37,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 220.0,
        "2": 0.98,
        "3": 0.3,
        "4": 0.8,
        "5": 0.0,
        "6": 0.9
      }
    },
    {
      "id": 2,
      "name": "Fractal Resonator",
      "type": 67,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 440.0,
        "2": 0.618,
        "3": 0.91,
        "4": 10000.0,
        "5": 0.8,
        "6": 0.85
      }
    },
    {
      "id": 3,
      "name": "Pitch Delay Crystals",
      "type": 64,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 250.0,
        "2": 375.0,
        "3": 7.0,
        "4": 10.0,
        "5": 0.6,
        "6": 0.0,
        "7": 9000.0,
        "8": 0.75
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 5,
      "name": "Stereo Imager",
      "type": 55,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.5,
        "2": 15.0,
        "3": 120.0,
        "4": 0.8
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 25.0,
        "4": 1.5,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Placas Fractales & Resonadores Áureos -> Sympathetic String Matrix
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Sympathetic String Matrix",
        "Placas Fractales & Resonadores Áureos",
        R"json({
  "schemaVersion": 1,
  "name": "Sympathetic String Matrix",
  "author": "N8Audio",
  "category": "Placas Fractales & Resonadores \u00c1ureos",
  "description": "Cadena acoplada de peines bipolares y matriz fractal \u00e1urea con apertura est\u00e9reo y reverberaci\u00f3n",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.75,
    0.5,
    0.8,
    0.5,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Transient Snap",
      "type": 33,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.7,
        "2": -0.3,
        "3": 1.2,
        "4": 1.0,
        "5": 2.0,
        "6": 1.0
      }
    },
    {
      "id": 2,
      "name": "Comb Resonator",
      "type": 63,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 130.81,
        "2": 0.9,
        "3": 7500.0,
        "4": 14.0,
        "5": 0.75
      }
    },
    {
      "id": 3,
      "name": "Fractal Resonator",
      "type": 67,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 261.63,
        "2": 0.7,
        "3": 0.89,
        "4": 8000.0,
        "5": 0.9,
        "6": 0.8
      }
    },
    {
      "id": 4,
      "name": "Tape Saturation",
      "type": 21,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 2.8,
        "2": 1.0,
        "3": 0.35,
        "4": 0.8,
        "5": 0.85
      }
    },
    {
      "id": 5,
      "name": "Space Reverb",
      "type": 6,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 0.9,
        "2": 5.5,
        "3": 5500.0,
        "4": 30.0,
        "5": 0.55
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Desfase Espectral & Halos Líquidos -> Binaural Spectral Smear Halo
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Binaural Spectral Smear Halo",
        "Desfase Espectral & Halos Líquidos",
        R"json({
  "schemaVersion": 1,
  "name": "Binaural Spectral Smear Halo",
  "author": "N8Audio",
  "category": "Desfase Espectral & Halos L\u00edquidos",
  "description": "Procesamiento independiente Mid/Side: Centro saturado anal\u00f3gico con laterales en difusi\u00f3n espectral y chirps allpass",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [
    0.7,
    0.8,
    0.9,
    0.5,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "M/S Encoder",
      "type": 24,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 1.0
      }
    },
    {
      "id": 2,
      "name": "Mid Saturator",
      "type": 62,
      "x": 240.0,
      "y": 30.0,
      "params": {
        "1": 2.5,
        "2": 1.0,
        "3": 2.0,
        "4": 1.5,
        "5": 1.2,
        "6": 1.0,
        "7": 250.0,
        "8": 3000.0,
        "9": 0.85
      }
    },
    {
      "id": 3,
      "name": "Side Spectral Smear",
      "type": 52,
      "x": 240.0,
      "y": 140.0,
      "params": {
        "1": 6.0,
        "2": 0.92,
        "3": 0.95,
        "4": -0.1,
        "5": 0.9
      }
    },
    {
      "id": 4,
      "name": "Side Phase Dispersion",
      "type": 65,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 18.0,
        "2": 1600.0,
        "3": 0.8,
        "4": 12000.0,
        "5": 0.5,
        "6": 0.95
      }
    },
    {
      "id": 5,
      "name": "M/S Decoder",
      "type": 25,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.6,
        "2": 100.0,
        "3": 0.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Desfase Espectral & Halos Líquidos -> Infinite Frozen Aurora
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Infinite Frozen Aurora",
        "Desfase Espectral & Halos Líquidos",
        R"json({
  "schemaVersion": 1,
  "name": "Infinite Frozen Aurora",
  "author": "N8Audio",
  "category": "Desfase Espectral & Halos L\u00edquidos",
  "description": "Congelamiento espectral con barrido de filtro mutante SVF, transposici\u00f3n a la quinta y retardo est\u00e9reo",
  "dryLevel": 0.2,
  "wetLevel": 0.95,
  "macros": [
    0.9,
    0.4,
    0.85,
    0.6,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Spectral Freeze",
      "type": 12,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 1.0,
        "2": 0.9,
        "3": 0.95,
        "4": 0.85
      }
    },
    {
      "id": 2,
      "name": "Morphing SVF Sweep",
      "type": 66,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 1100.0,
        "2": 6.0,
        "3": 0.4,
        "4": 2.0,
        "5": 0.0
      }
    },
    {
      "id": 3,
      "name": "Pitch Shift +7st",
      "type": 11,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 7.0,
        "2": 0.0,
        "3": 35.0,
        "4": 0.75
      }
    },
    {
      "id": 4,
      "name": "Space Delay",
      "type": 28,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 320.0,
        "2": 480.0,
        "3": 0.55,
        "4": 6500.0,
        "5": 1.0,
        "6": 0.6
      }
    },
    {
      "id": 5,
      "name": "Stereo Imager",
      "type": 55,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.6,
        "2": 18.0,
        "3": 140.0,
        "4": 0.85
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 25.0,
        "4": 1.5,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Desfase Espectral & Halos Líquidos -> Liquid Phase Refractor
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Liquid Phase Refractor",
        "Desfase Espectral & Halos Líquidos",
        R"json({
  "schemaVersion": 1,
  "name": "Liquid Phase Refractor",
  "author": "N8Audio",
  "category": "Desfase Espectral & Halos L\u00edquidos",
  "description": "Prisma de refracci\u00f3n en 8 voces acoplado a difusi\u00f3n espectral y reverberaci\u00f3n inversa 3D",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.7,
    0.85,
    0.5,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Refraction Prism",
      "type": 51,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 8.0,
        "2": 22.0,
        "3": 25.0,
        "4": 0.9,
        "5": 0.75,
        "6": 2.2,
        "7": 0.85
      }
    },
    {
      "id": 2,
      "name": "Spectral Smear",
      "type": 52,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 4.5,
        "2": 0.88,
        "3": 0.9,
        "4": 0.0,
        "5": 0.8
      }
    },
    {
      "id": 3,
      "name": "Reverse Reverb Swell",
      "type": 38,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 1.8,
        "2": 0.85,
        "3": 7000.0,
        "4": 0.8
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 2.0,
        "2": 0.5,
        "3": 0.8
      }
    },
    {
      "id": 5,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": -45.0,
        "2": 20.0,
        "3": 2.0,
        "4": 1.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Desfase Espectral & Halos Líquidos -> Spectral Crystal Shards
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Spectral Crystal Shards",
        "Desfase Espectral & Halos Líquidos",
        R"json({
  "schemaVersion": 1,
  "name": "Spectral Crystal Shards",
  "author": "N8Audio",
  "category": "Desfase Espectral & Halos L\u00edquidos",
  "description": "Filtrado espectral FFT con transposici\u00f3n arm\u00f3nica a la 19na y retardos reflectantes",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.85,
    0.6,
    0.8,
    0.7,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Spectral Processor",
      "type": 29,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 12.0,
        "4": 0.85
      }
    },
    {
      "id": 2,
      "name": "Pitch Shift +19st",
      "type": 11,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 19.0,
        "2": 0.0,
        "3": 40.0,
        "4": 0.7
      }
    },
    {
      "id": 3,
      "name": "Shimmer Reverb",
      "type": 50,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 6.5,
        "2": 8000.0,
        "3": 0.65,
        "4": 0.0,
        "5": 1.4,
        "6": 0.7
      }
    },
    {
      "id": 4,
      "name": "Advanced Delay",
      "type": 28,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 180.0,
        "2": 270.0,
        "3": 0.5,
        "4": 7000.0,
        "5": 1.0,
        "6": 0.6
      }
    },
    {
      "id": 5,
      "name": "Phase Dispersion",
      "type": 65,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 16.0,
        "2": 2200.0,
        "3": 0.75,
        "4": 12000.0,
        "5": 0.45,
        "6": 0.85
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Micro-Muestreo Granular -> Stochastic Granular Prism
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Stochastic Granular Prism",
        "Micro-Muestreo Granular",
        R"json({
  "schemaVersion": 1,
  "name": "Stochastic Granular Prism",
  "author": "N8Audio",
  "category": "Micro-Muestreo Granular",
  "description": "Nube de granos estoc\u00e1sticos dividida en prisma de 8 voces y retardos de micro-pitch crystals",
  "dryLevel": 0.25,
  "wetLevel": 0.95,
  "macros": [
    0.85,
    0.6,
    0.8,
    0.5,
    0.6,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Granular Cloud",
      "type": 7,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 45.0,
        "2": 35.0,
        "3": 30.0,
        "4": 0.0,
        "5": 0.5,
        "6": 0.85,
        "7": 0.9,
        "10": 1.0
      }
    },
    {
      "id": 2,
      "name": "Refraction Prism",
      "type": 51,
      "x": 260.0,
      "y": 30.0,
      "params": {
        "1": 8.0,
        "2": 18.0,
        "3": 20.0,
        "4": 0.9,
        "5": 0.7,
        "6": 2.0,
        "7": 0.8
      }
    },
    {
      "id": 3,
      "name": "Pitch Delay Crystals",
      "type": 64,
      "x": 260.0,
      "y": 140.0,
      "params": {
        "1": 150.0,
        "2": 225.0,
        "3": 12.0,
        "4": 15.0,
        "5": 0.5,
        "6": 0.0,
        "7": 7500.0,
        "8": 0.7
      }
    },
    {
      "id": 4,
      "name": "Comb Resonator",
      "type": 63,
      "x": 500.0,
      "y": 80.0,
      "params": {
        "1": 220.0,
        "2": 0.82,
        "3": 8500.0,
        "4": 12.0,
        "5": 0.65
      }
    },
    {
      "id": 5,
      "name": "Mid/Side Decoder",
      "type": 25,
      "x": 720.0,
      "y": 80.0,
      "params": {
        "1": 1.5,
        "2": 110.0,
        "3": 0.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 920.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Micro-Muestreo Granular -> Reverse Grain Vortex
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Reverse Grain Vortex",
        "Micro-Muestreo Granular",
        R"json({
  "schemaVersion": 1,
  "name": "Reverse Grain Vortex",
  "author": "N8Audio",
  "category": "Micro-Muestreo Granular",
  "description": "Granulaci\u00f3n en tiempo inverso con lazo acoplado a saturaci\u00f3n multibanda y chorus dimensional",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.5,
    0.85,
    0.7,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Reverse Reverb",
      "type": 38,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 1.4,
        "2": 0.8,
        "3": 6500.0,
        "4": 0.85
      }
    },
    {
      "id": 2,
      "name": "Reverse Granular",
      "type": 7,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 80.0,
        "2": 25.0,
        "3": 50.0,
        "4": -12.0,
        "5": 0.0,
        "6": 0.8,
        "7": 0.85,
        "10": 1.0
      }
    },
    {
      "id": 3,
      "name": "Multiband Saturator",
      "type": 62,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 2.0,
        "2": 1.0,
        "3": 2.5,
        "4": 2.0,
        "5": 1.6,
        "6": 1.0,
        "7": 220.0,
        "8": 3400.0,
        "9": 0.8
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 5,
      "name": "Space Delay",
      "type": 28,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 240.0,
        "2": 360.0,
        "3": 0.5,
        "4": 6500.0,
        "5": 1.0,
        "6": 0.6
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Micro-Muestreo Granular -> Crystalline Shimmer Swarm
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Crystalline Shimmer Swarm",
        "Micro-Muestreo Granular",
        R"json({
  "schemaVersion": 1,
  "name": "Crystalline Shimmer Swarm",
  "author": "N8Audio",
  "category": "Micro-Muestreo Granular",
  "description": "Micro-part\u00edculas granulares dispersadas en cascada allpass y shimmer de octava ascendente",
  "dryLevel": 0.2,
  "wetLevel": 0.95,
  "macros": [
    0.85,
    0.6,
    0.9,
    0.5,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Granular Cloud",
      "type": 7,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 20.0,
        "2": 60.0,
        "3": 35.0,
        "4": 0.0,
        "5": 1.2,
        "6": 0.9,
        "7": 0.9,
        "10": 2.0
      }
    },
    {
      "id": 2,
      "name": "Shimmer Reverb",
      "type": 50,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 8.0,
        "2": 8000.0,
        "3": 0.7,
        "4": 0.0,
        "5": 1.6,
        "6": 0.75
      }
    },
    {
      "id": 3,
      "name": "Phase Dispersion",
      "type": 65,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 18.0,
        "2": 2000.0,
        "3": 0.8,
        "4": 12000.0,
        "5": 0.5,
        "6": 0.9
      }
    },
    {
      "id": 4,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 3.5,
        "2": 0.7,
        "3": 1.0,
        "4": 1.0,
        "5": 0.35,
        "6": 0.6,
        "7": 0.85
      }
    },
    {
      "id": 5,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 45.0,
        "2": 15.0,
        "3": 2.0,
        "4": 1.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Micro-Muestreo Granular -> Sub-Atomic Texture Cloud
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Sub-Atomic Texture Cloud",
        "Micro-Muestreo Granular",
        R"json({
  "schemaVersion": 1,
  "name": "Sub-Atomic Texture Cloud",
  "author": "N8Audio",
  "category": "Micro-Muestreo Granular",
  "description": "Textura micro-granular filtrada por formantes vocales con difusi\u00f3n espectral l\u00edquida",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.75,
    0.5,
    0.85,
    0.6,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Granular Texture",
      "type": 7,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 15.0,
        "2": 55.0,
        "3": 40.0,
        "4": 0.0,
        "5": 1.0,
        "6": 0.85,
        "7": 0.9,
        "10": 2.0
      }
    },
    {
      "id": 2,
      "name": "Formant Filter",
      "type": 31,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 0.45,
        "2": 4.5,
        "3": 1.1,
        "4": 0.4,
        "5": 0.8
      }
    },
    {
      "id": 3,
      "name": "Spectral Smear",
      "type": 52,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 4.0,
        "2": 0.88,
        "3": 0.9,
        "4": -0.15,
        "5": 0.85
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 3.0,
        "2": 0.55,
        "3": 0.8
      }
    },
    {
      "id": 5,
      "name": "Space Delay",
      "type": 28,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 260.0,
        "2": 390.0,
        "3": 0.5,
        "4": 6500.0,
        "5": 1.0,
        "6": 0.6
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Glitch & Deconstrucción Rítmica -> Neural Buffer Fracture
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Neural Buffer Fracture",
        "Glitch & Deconstrucción Rítmica",
        R"json({
  "schemaVersion": 1,
  "name": "Neural Buffer Fracture",
  "author": "N8Audio",
  "category": "Glitch & Deconstrucci\u00f3n R\u00edtmica",
  "description": "Slicing r\u00edtmico estoc\u00e1stico con divisi\u00f3n a bitcrusher y frenado de cinta procesado por OTT",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.7,
    0.5,
    0.8,
    0.7,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Audio Slicer",
      "type": 49,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.0,
        "2": 16.0,
        "3": -1.0,
        "4": 1.0,
        "5": 0.0,
        "6": 40.0,
        "7": 6.0,
        "8": 0.4,
        "9": 0.0,
        "10": 16000.0,
        "11": 1.0
      }
    },
    {
      "id": 2,
      "name": "Glitch Slicer",
      "type": 14,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 2.0,
        "2": 0.5,
        "3": 0.4,
        "4": 0.8,
        "5": 0.85
      }
    },
    {
      "id": 3,
      "name": "Bitcrusher Drive",
      "type": 40,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 10.0,
        "2": 3.0,
        "3": 0.05,
        "4": 0.4,
        "5": 0.0,
        "6": 0.75
      }
    },
    {
      "id": 4,
      "name": "Tape Stop Brake",
      "type": 30,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 0.0,
        "2": 350.0,
        "3": 0.75,
        "4": 0.6
      }
    },
    {
      "id": 5,
      "name": "Multiband OTT",
      "type": 15,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 180.0,
        "2": 3000.0,
        "3": 1.4,
        "4": 1.2,
        "5": 1.3,
        "6": 0.9
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.8,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Glitch & Deconstrucción Rítmica -> Polyrhythmic Glitch Chopper
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Polyrhythmic Glitch Chopper",
        "Glitch & Deconstrucción Rítmica",
        R"json({
  "schemaVersion": 1,
  "name": "Polyrhythmic Glitch Chopper",
  "author": "N8Audio",
  "category": "Glitch & Deconstrucci\u00f3n R\u00edtmica",
  "description": "Slicer sincopado bifurcado a eco est\u00e9reo y modulaci\u00f3n en anillo con filtro morphing SVF",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.6,
    0.7,
    0.7,
    0.6,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Audio Slicer",
      "type": 49,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.0,
        "2": 8.0,
        "3": -1.0,
        "4": 1.0,
        "5": 0.0,
        "6": 60.0,
        "7": 4.0,
        "8": 0.3,
        "9": 0.0,
        "10": 12000.0,
        "11": 0.9
      }
    },
    {
      "id": 2,
      "name": "Ping-Pong Delay",
      "type": 28,
      "x": 240.0,
      "y": 30.0,
      "params": {
        "1": 180.0,
        "2": 270.0,
        "3": 0.5,
        "4": 6000.0,
        "5": 1.0,
        "6": 0.6
      }
    },
    {
      "id": 3,
      "name": "Ring Modulator",
      "type": 19,
      "x": 240.0,
      "y": 140.0,
      "params": {
        "1": 220.0,
        "2": 0.0,
        "3": 0.0,
        "4": 0.75
      }
    },
    {
      "id": 4,
      "name": "Morphing SVF Band",
      "type": 66,
      "x": 460.0,
      "y": 80.0,
      "params": {
        "1": 1500.0,
        "2": 6.0,
        "3": 0.2,
        "4": 2.5,
        "5": 0.0
      }
    },
    {
      "id": 5,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 6.0,
        "2": 0.8,
        "3": 1.0,
        "4": 1.0,
        "5": 0.4,
        "6": 0.6,
        "7": 0.85
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Glitch & Deconstrucción Rítmica -> Cybernetic Stutter Matrix
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Cybernetic Stutter Matrix",
        "Glitch & Deconstrucción Rítmica",
        R"json({
  "schemaVersion": 1,
  "name": "Cybernetic Stutter Matrix",
  "author": "N8Audio",
  "category": "Glitch & Deconstrucci\u00f3n R\u00edtmica",
  "description": "Deconstrucci\u00f3n glitch estoc\u00e1stica en paralelo a corrimiento de frecuencia Hilbert y dispersi\u00f3n allpass",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.85,
    0.7,
    0.6,
    0.8,
    0.6,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Glitch Machine",
      "type": 14,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 1.0,
        "2": 0.7,
        "3": 0.35,
        "4": 0.4,
        "5": 0.9
      }
    },
    {
      "id": 2,
      "name": "Freq Shifter",
      "type": 20,
      "x": 240.0,
      "y": 30.0,
      "params": {
        "1": 14.0,
        "2": 0.3,
        "3": 0.8
      }
    },
    {
      "id": 3,
      "name": "Phase Dispersion",
      "type": 65,
      "x": 240.0,
      "y": 140.0,
      "params": {
        "1": 18.0,
        "2": 2200.0,
        "3": 0.85,
        "4": 12000.0,
        "5": 0.6,
        "6": 0.9
      }
    },
    {
      "id": 4,
      "name": "Multiband Saturator",
      "type": 62,
      "x": 460.0,
      "y": 80.0,
      "params": {
        "1": 2.2,
        "2": 1.0,
        "3": 2.8,
        "4": 2.0,
        "5": 1.5,
        "6": 1.0,
        "7": 200.0,
        "8": 3200.0,
        "9": 0.85
      }
    },
    {
      "id": 5,
      "name": "Space Delay",
      "type": 28,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 200.0,
        "2": 300.0,
        "3": 0.5,
        "4": 7000.0,
        "5": 1.0,
        "6": 0.55
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.8,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Glitch & Deconstrucción Rítmica -> Quantum Beat Decimator
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Quantum Beat Decimator",
        "Glitch & Deconstrucción Rítmica",
        R"json({
  "schemaVersion": 1,
  "name": "Quantum Beat Decimator",
  "author": "N8Audio",
  "category": "Glitch & Deconstrucci\u00f3n R\u00edtmica",
  "description": "Fragmentador r\u00edtmico con pitch shift a la quinta y decimaci\u00f3n no lineal excitando un optoacoplador Buchla Vactrol LPG con tr\u00e9molo din\u00e1mico",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.6,
    0.6,
    0.7,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Audio Slicer",
      "type": 49,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.0,
        "2": 8.0,
        "3": -1.0,
        "4": 1.0,
        "5": 0.0,
        "6": 45.0,
        "7": 5.0,
        "8": 0.35,
        "9": 0.0,
        "10": 13000.0,
        "11": 0.95
      }
    },
    {
      "id": 2,
      "name": "Pitch Shift +7st",
      "type": 11,
      "x": 240.0,
      "y": 30.0,
      "params": {
        "1": 7.0,
        "2": 0.0,
        "3": 35.0,
        "4": 0.8
      }
    },
    {
      "id": 3,
      "name": "Bitcrusher Decimator",
      "type": 40,
      "x": 240.0,
      "y": 140.0,
      "params": {
        "1": 10.0,
        "2": 2.2,
        "3": 0.03,
        "4": 0.3,
        "5": 0.0,
        "6": 0.7
      }
    },
    {
      "id": 4,
      "name": "Buchla Vactrol Ping",
      "type": 68,
      "x": 460.0,
      "y": 80.0,
      "params": {
        "1": 0.14,
        "2": 0.60,
        "3": 140.0,
        "4": 0.95,
        "5": 0.5,
        "6": 2.2,
        "7": 0.9
      }
    },
    {
      "id": 5,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 5.5,
        "2": 0.75,
        "3": 1.0,
        "4": 1.0,
        "5": 0.4,
        "6": 0.65,
        "7": 0.85
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Cintas Analógicas & Lo-Fi -> Melted Tape Memory Machine
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Melted Tape Memory Machine",
        "Cintas Analógicas & Lo-Fi",
        R"json({
  "schemaVersion": 1,
  "name": "Melted Tape Memory Machine",
  "author": "N8Audio",
  "category": "Cintas Anal\u00f3gicas & Lo-Fi",
  "description": "Emulaci\u00f3n de cassette vintage desgastado con saturador tri-banda, eco oscuro y chorus BBD espacial",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [
    0.75,
    0.6,
    0.8,
    0.5,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Lo-Fi Cassette",
      "type": 56,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.7,
        "2": 0.5,
        "3": 0.4,
        "4": 0.35,
        "5": 4500.0,
        "6": 0.6,
        "7": 0.85
      }
    },
    {
      "id": 2,
      "name": "Multiband Saturator",
      "type": 62,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 2.5,
        "2": 1.0,
        "3": 2.2,
        "4": 1.5,
        "5": 1.4,
        "6": 1.0,
        "7": 220.0,
        "8": 3200.0,
        "9": 0.85
      }
    },
    {
      "id": 3,
      "name": "Advanced Delay",
      "type": 28,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 280.0,
        "2": 420.0,
        "3": 0.55,
        "4": 4500.0,
        "5": 1.0,
        "6": 0.6
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 5,
      "name": "Stereo Imager",
      "type": 55,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.4,
        "2": 12.0,
        "3": 110.0,
        "4": 0.8
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 25.0,
        "4": 1.5,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Cintas Analógicas & Lo-Fi -> 1978 Magnetic Decay Spiral
    // -------------------------------------------------------------------------
    catalog.push_back({
        "1978 Magnetic Decay Spiral",
        "Cintas Analógicas & Lo-Fi",
        R"json({
  "schemaVersion": 1,
  "name": "1978 Magnetic Decay Spiral",
  "author": "N8Audio",
  "category": "Cintas Anal\u00f3gicas & Lo-Fi",
  "description": "Saturaci\u00f3n de cinta con hist\u00e9resis magn\u00e9tica dividida a resonador comb y tr\u00e9molo \u00f3ptico anal\u00f3gico",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.7,
    0.7,
    0.75,
    0.6,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Tape Saturation",
      "type": 21,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 3.8,
        "2": 1.0,
        "3": 0.45,
        "4": 0.8,
        "5": 0.85
      }
    },
    {
      "id": 2,
      "name": "Comb Resonator",
      "type": 63,
      "x": 240.0,
      "y": 30.0,
      "params": {
        "1": 180.0,
        "2": 0.85,
        "3": 6500.0,
        "4": 10.0,
        "5": 0.65
      }
    },
    {
      "id": 3,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 240.0,
      "y": 140.0,
      "params": {
        "1": 4.0,
        "2": 0.75,
        "3": 1.0,
        "4": 1.0,
        "5": 0.4,
        "6": 0.6,
        "7": 0.85
      }
    },
    {
      "id": 4,
      "name": "Space Reverb",
      "type": 6,
      "x": 460.0,
      "y": 80.0,
      "params": {
        "1": 0.85,
        "2": 4.5,
        "3": 5000.0,
        "4": 25.0,
        "5": 0.55
      }
    },
    {
      "id": 5,
      "name": "Tape Stop",
      "type": 30,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 0.0,
        "2": 450.0,
        "3": 0.8,
        "4": 0.7
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Cintas Analógicas & Lo-Fi -> Radioactive Cassette Degradation
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Radioactive Cassette Degradation",
        "Cintas Analógicas & Lo-Fi",
        R"json({
  "schemaVersion": 1,
  "name": "Radioactive Cassette Degradation",
  "author": "N8Audio",
  "category": "Cintas Anal\u00f3gicas & Lo-Fi",
  "description": "Saturaci\u00f3n de cinta anal\u00f3gica cruzada en paralelo con decimador lo-fi de 10 bits, filtro telef\u00f3nico morphing y eco degradado",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [
    0.8,
    0.5,
    0.7,
    0.6,
    0.6,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Tape Saturation",
      "type": 21,
      "x": 40.0,
      "y": 30.0,
      "params": {
        "1": 3.5,
        "2": 1.2,
        "3": 0.35,
        "4": 0.6,
        "5": 0.85
      }
    },
    {
      "id": 2,
      "name": "Bitcrusher Decimator",
      "type": 40,
      "x": 40.0,
      "y": 140.0,
      "params": {
        "1": 10.0,
        "2": 2.0,
        "3": 0.04,
        "4": 0.3,
        "5": 0.0,
        "6": 0.65
      }
    },
    {
      "id": 3,
      "name": "Morphing SVF Tele",
      "type": 66,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 1600.0,
        "2": 4.5,
        "3": 0.25,
        "4": 3.0,
        "5": 0.0
      }
    },
    {
      "id": 4,
      "name": "Lo-Fi Cassette",
      "type": 56,
      "x": 460.0,
      "y": 80.0,
      "params": {
        "1": 0.8,
        "2": 0.6,
        "3": 0.5,
        "4": 0.45,
        "5": 3800.0,
        "6": 0.7,
        "7": 0.9
      }
    },
    {
      "id": 5,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 2.0,
        "2": 0.5,
        "3": 0.8
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Cintas Analógicas & Lo-Fi -> Sub-Zero Drone Machine
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Sub-Zero Drone Machine",
        "Cintas Analógicas & Lo-Fi",
        R"json({
  "schemaVersion": 1,
  "name": "Sub-Zero Drone Machine",
  "author": "N8Audio",
  "category": "Cintas Anal\u00f3gicas & Lo-Fi",
  "description": "Plataforma de zumbidos y texturas oscuras con ecualizaci\u00f3n quir\u00fargica, delay de cinta y chorus anal\u00f3gico",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [
    0.75,
    0.6,
    0.85,
    0.5,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Lo-Fi Cassette",
      "type": 56,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.6,
        "2": 0.4,
        "3": 0.3,
        "4": 0.3,
        "5": 5000.0,
        "6": 0.5,
        "7": 0.8
      }
    },
    {
      "id": 2,
      "name": "Parametric EQ",
      "type": 27,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 80.0,
        "2": 3.0,
        "3": 850.0,
        "4": 1.5,
        "5": -4.0,
        "6": 7000.0,
        "7": -3.0
      }
    },
    {
      "id": 3,
      "name": "Advanced Delay",
      "type": 28,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 350.0,
        "2": 525.0,
        "3": 0.6,
        "4": 5500.0,
        "5": 1.0,
        "6": 0.6
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 3.0,
        "2": 0.65,
        "3": 0.85
      }
    },
    {
      "id": 5,
      "name": "Space Reverb",
      "type": 6,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 0.95,
        "2": 6.5,
        "3": 4500.0,
        "4": 35.0,
        "5": 0.6
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Espacios Infinitos & Shimmer -> Abyssal Shimmer Wormhole
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Abyssal Shimmer Wormhole",
        "Espacios Infinitos & Shimmer",
        R"json({
  "schemaVersion": 1,
  "name": "Abyssal Shimmer Wormhole",
  "author": "N8Audio",
  "category": "Espacios Infinitos & Shimmer",
  "description": "Reverberaci\u00f3n shimmer de colas infinitas dividida en difusi\u00f3n espectral y dispersi\u00f3n allpass con decodificador Mid/Side",
  "dryLevel": 0.2,
  "wetLevel": 0.95,
  "macros": [
    0.9,
    0.6,
    0.95,
    0.6,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Shimmer Reverb",
      "type": 50,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 12.0,
        "2": 7000.0,
        "3": 0.7,
        "4": 0.0,
        "5": 1.6,
        "6": 0.75
      }
    },
    {
      "id": 2,
      "name": "Spectral Smear",
      "type": 52,
      "x": 260.0,
      "y": 30.0,
      "params": {
        "1": 5.0,
        "2": 0.9,
        "3": 0.92,
        "4": -0.1,
        "5": 0.85
      }
    },
    {
      "id": 3,
      "name": "Phase Dispersion",
      "type": 65,
      "x": 260.0,
      "y": 140.0,
      "params": {
        "1": 20.0,
        "2": 1500.0,
        "3": 0.85,
        "4": 11000.0,
        "5": 0.55,
        "6": 0.9
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 500.0,
      "y": 80.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 5,
      "name": "Mid/Side Decoder",
      "type": 25,
      "x": 720.0,
      "y": 80.0,
      "params": {
        "1": 1.6,
        "2": 130.0,
        "3": 0.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 920.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 35.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Espacios Infinitos & Shimmer -> Celestial Catacomb Reflection
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Celestial Catacomb Reflection",
        "Espacios Infinitos & Shimmer",
        R"json({
  "schemaVersion": 1,
  "name": "Celestial Catacomb Reflection",
  "author": "N8Audio",
  "category": "Espacios Infinitos & Shimmer",
  "description": "Reflexiones de catedral inmensa refractadas en prisma arm\u00f3nico con paneo orbital y tr\u00e9molo",
  "dryLevel": 0.25,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.7,
    0.9,
    0.5,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Space Reverb",
      "type": 6,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.96,
        "2": 9.0,
        "3": 5500.0,
        "4": 40.0,
        "5": 0.65
      }
    },
    {
      "id": 2,
      "name": "Refraction Prism",
      "type": 51,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 8.0,
        "2": 20.0,
        "3": 22.0,
        "4": 0.9,
        "5": 0.75,
        "6": 2.0,
        "7": 0.8
      }
    },
    {
      "id": 3,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 3.0,
        "2": 0.7,
        "3": 1.0,
        "4": 1.0,
        "5": 0.35,
        "6": 0.55,
        "7": 0.8
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 2.0,
        "2": 0.5,
        "3": 0.8
      }
    },
    {
      "id": 5,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 30.0,
        "2": 25.0,
        "3": 2.2,
        "4": 1.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Espacios Infinitos & Shimmer -> Reverse Bloom Nebula
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Reverse Bloom Nebula",
        "Espacios Infinitos & Shimmer",
        R"json({
  "schemaVersion": 1,
  "name": "Reverse Bloom Nebula",
  "author": "N8Audio",
  "category": "Espacios Infinitos & Shimmer",
  "description": "Hinchamiento de reverberaci\u00f3n invertida retroalimentada a eco espacial y resonador \u00e1ureo",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [
    0.85,
    0.5,
    0.85,
    0.6,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Reverse Reverb",
      "type": 38,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 2.0,
        "2": 0.85,
        "3": 7000.0,
        "4": 0.85
      }
    },
    {
      "id": 2,
      "name": "Advanced Delay",
      "type": 28,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 300.0,
        "2": 450.0,
        "3": 0.6,
        "4": 6500.0,
        "5": 1.0,
        "6": 0.65
      }
    },
    {
      "id": 3,
      "name": "Fractal Resonator",
      "type": 67,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 220.0,
        "2": 0.618,
        "3": 0.9,
        "4": 8000.0,
        "5": 0.85,
        "6": 0.8
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 5,
      "name": "Stereo Imager",
      "type": 55,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.5,
        "2": 15.0,
        "3": 120.0,
        "4": 0.8
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Espacios Infinitos & Shimmer -> Prismatic Ambient Halo
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Prismatic Ambient Halo",
        "Espacios Infinitos & Shimmer",
        R"json({
  "schemaVersion": 1,
  "name": "Prismatic Ambient Halo",
  "author": "N8Audio",
  "category": "Espacios Infinitos & Shimmer",
  "description": "Dispersi\u00f3n prism\u00e1tica con calor anal\u00f3gico de cinta, shimmer espacial y paneo tridimensional",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.85,
    0.6,
    0.9,
    0.5,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Refraction Prism",
      "type": 51,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 6.0,
        "2": 16.0,
        "3": 18.0,
        "4": 0.85,
        "5": 0.5,
        "6": 1.8,
        "7": 0.8
      }
    },
    {
      "id": 2,
      "name": "Tape Flutter",
      "type": 21,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 2.8,
        "2": 0.0,
        "3": 0.65,
        "4": 0.75,
        "5": 0.75
      }
    },
    {
      "id": 3,
      "name": "Shimmer Reverb",
      "type": 50,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 9.0,
        "2": 7000.0,
        "3": 0.6,
        "4": 1.0,
        "5": 1.4,
        "6": 0.65
      }
    },
    {
      "id": 4,
      "name": "Phase Dispersion",
      "type": 65,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 16.0,
        "2": 1800.0,
        "3": 0.75,
        "4": 12000.0,
        "5": 0.5,
        "6": 0.85
      }
    },
    {
      "id": 5,
      "name": "Spatializer 3D",
      "type": 26,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": -30.0,
        "2": 20.0,
        "3": 2.5,
        "4": 1.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Modulación Psicoacústica -> Vortex Dimension 3D Orbital
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Shepard-Risset Barberpole Vortex",
        "Modulación Psicoacústica",
        R"json({
  "schemaVersion": 1,
  "name": "Shepard-Risset Barberpole Vortex",
  "author": "N8Audio",
  "category": "Modulaci\u00f3n Psicoac\u00fastica",
  "description": "Chorus SDD-320 de 4 voces cruzado con filtro Barberpole de ascenso infinito (Shepard-Risset), tr\u00e9molo arm\u00f3nico y paneo esf\u00e9rico 3D",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.7,
    0.8,
    0.6,
    0.5,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 4.0,
        "2": 0.7,
        "3": 0.9
      }
    },
    {
      "id": 2,
      "name": "Barberpole Filter",
      "type": 69,
      "x": 240.0,
      "y": 30.0,
      "params": {
        "1": 0.45,
        "2": 0.82,
        "3": 0.65,
        "4": 90.0,
        "5": 14000.0,
        "6": 0.85
      }
    },
    {
      "id": 3,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 240.0,
      "y": 140.0,
      "params": {
        "1": 5.0,
        "2": 0.8,
        "3": 1.0,
        "4": 1.0,
        "5": 0.4,
        "6": 0.65,
        "7": 0.85
      }
    },
    {
      "id": 4,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 460.0,
      "y": 80.0,
      "params": {
        "1": 55.0,
        "2": 25.0,
        "3": 2.2,
        "4": 1.0
      }
    },
    {
      "id": 5,
      "name": "Stereo Imager",
      "type": 55,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.5,
        "2": 15.0,
        "3": 120.0,
        "4": 0.8
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Modulación Psicoacústica -> Bipolar Comb Weaver Matrix
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Bipolar Comb Weaver Matrix",
        "Modulación Psicoacústica",
        R"json({
  "schemaVersion": 1,
  "name": "Bipolar Comb Weaver Matrix",
  "author": "N8Audio",
  "category": "Modulaci\u00f3n Psicoac\u00fastica",
  "description": "Resonador de peine bipolar con codificaci\u00f3n M/S: ecualizaci\u00f3n central y dispersi\u00f3n allpass en laterales",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.65,
    0.75,
    0.5,
    0.7,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Comb Resonator",
      "type": 63,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 330.0,
        "2": -0.88,
        "3": 8500.0,
        "4": 16.0,
        "5": 0.8
      }
    },
    {
      "id": 2,
      "name": "M/S Encoder",
      "type": 24,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 1.0
      }
    },
    {
      "id": 3,
      "name": "Mid Tone EQ",
      "type": 27,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 100.0,
        "2": 2.0,
        "3": 2500.0,
        "4": 2.0,
        "5": 3.0,
        "6": 8000.0,
        "7": -1.0
      }
    },
    {
      "id": 4,
      "name": "Side Phase Dispersion",
      "type": 65,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 16.0,
        "2": 2000.0,
        "3": 0.8,
        "4": 12000.0,
        "5": 0.5,
        "6": 0.9
      }
    },
    {
      "id": 5,
      "name": "M/S Decoder",
      "type": 25,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.6,
        "2": 110.0,
        "3": 0.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Modulación Psicoacústica -> Haas Micro-Pitch Hologram
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Haas Micro-Pitch Hologram",
        "Modulación Psicoacústica",
        R"json({
  "schemaVersion": 1,
  "name": "Haas Micro-Pitch Hologram",
  "author": "N8Audio",
  "category": "Modulaci\u00f3n Psicoac\u00fastica",
  "description": "Dual detuning micro-tonal de Cristales con retardo Haas est\u00e9reo y filtrado mutante en barrido 3D",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.7,
    0.6,
    0.5,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Pitch Delay Micro",
      "type": 64,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 40.0,
        "2": 60.0,
        "3": 0.0,
        "4": 12.0,
        "5": 0.35,
        "6": 0.0,
        "7": 9000.0,
        "8": 0.8
      }
    },
    {
      "id": 2,
      "name": "Stereo Imager Haas",
      "type": 55,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 1.8,
        "2": 16.0,
        "3": 130.0,
        "4": 0.9
      }
    },
    {
      "id": 3,
      "name": "Morphing SVF Tone",
      "type": 66,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 1800.0,
        "2": 3.5,
        "3": 0.45,
        "4": 2.0,
        "5": 0.0
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 2.0,
        "2": 0.5,
        "3": 0.8
      }
    },
    {
      "id": 5,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 40.0,
        "2": 10.0,
        "3": 2.0,
        "4": 1.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Modulación Psicoacústica -> Orbital Euclidean Pulsar
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Orbital Euclidean Pulsar",
        "Modulación Psicoacústica",
        R"json({
  "schemaVersion": 1,
  "name": "Orbital Euclidean Pulsar",
  "author": "N8Audio",
  "category": "Modulaci\u00f3n Psicoac\u00fastica",
  "description": "Paneo orbital 3D continuo alimentando chorus dimensional bifurcado a filtro Barberpole descendente infinito y tr\u00e9molo din\u00e1mico con calidez de cinta",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.75,
    0.85,
    0.7,
    0.5,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 45.0,
        "2": 15.0,
        "3": 2.0,
        "4": 1.0
      }
    },
    {
      "id": 2,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 3,
      "name": "Barberpole Descent",
      "type": 69,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": -0.65,
        "2": 0.78,
        "3": 0.6,
        "4": 100.0,
        "5": 12000.0,
        "6": 0.8
      }
    },
    {
      "id": 4,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 4.0,
        "2": 0.7,
        "3": 1.0,
        "4": 1.0,
        "5": 0.4,
        "6": 0.6,
        "7": 0.8
      }
    },
    {
      "id": 5,
      "name": "Lo-Fi Cassette",
      "type": 56,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 0.5,
        "2": 0.35,
        "3": 0.25,
        "4": 0.2,
        "5": 6000.0,
        "6": 0.4,
        "7": 0.75
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Lazos de Realimentación Inestable -> Supercritical Feedback Reactor
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Supercritical Feedback Reactor",
        "Lazos de Realimentación Inestable",
        R"json({
  "schemaVersion": 1,
  "name": "Supercritical Feedback Reactor",
  "author": "N8Audio",
  "category": "Lazos de Realimentaci\u00f3n Inestable",
  "description": "Reactor de realimentaci\u00f3n acoplado con corrimiento arm\u00f3nico Hilbert, saturaci\u00f3n y dispersi\u00f3n en peine",
  "dryLevel": 0.2,
  "wetLevel": 0.95,
  "macros": [
    0.85,
    0.75,
    0.7,
    0.8,
    0.7,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Freq Shifter",
      "type": 20,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 8.0,
        "2": 0.3,
        "3": 0.85
      }
    },
    {
      "id": 2,
      "name": "Wavefold Distortion",
      "type": 8,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 4.5,
        "2": 2.0,
        "3": 5500.0,
        "4": 0.8
      }
    },
    {
      "id": 3,
      "name": "Comb Resonator",
      "type": 63,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 220.0,
        "2": 0.85,
        "3": 7000.0,
        "4": 12.0,
        "5": 0.7
      }
    },
    {
      "id": 4,
      "name": "Phase Dispersion",
      "type": 65,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 18.0,
        "2": 2000.0,
        "3": 0.8,
        "4": 11000.0,
        "5": 0.6,
        "6": 0.9
      }
    },
    {
      "id": 5,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.8,
        "2": -0.1,
        "3": 25.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Lazos de Realimentación Inestable -> Chaotic Tape Resonance Loop
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Chaotic Tape Resonance Loop",
        "Lazos de Realimentación Inestable",
        R"json({
  "schemaVersion": 1,
  "name": "Chaotic Tape Resonance Loop",
  "author": "N8Audio",
  "category": "Lazos de Realimentaci\u00f3n Inestable",
  "description": "Saturador anal\u00f3gico alimentando filtro mutante SVF en auto-oscilaci\u00f3n con delay espacial",
  "dryLevel": 0.25,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.7,
    0.6,
    0.8,
    0.6,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Tape Saturation",
      "type": 21,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 4.0,
        "2": 1.0,
        "3": 0.4,
        "4": 0.8,
        "5": 0.85
      }
    },
    {
      "id": 2,
      "name": "Morphing SVF Res",
      "type": 66,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 950.0,
        "2": 15.0,
        "3": 0.35,
        "4": 4.5,
        "5": 0.0
      }
    },
    {
      "id": 3,
      "name": "Space Delay",
      "type": 28,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 240.0,
        "2": 360.0,
        "3": 0.6,
        "4": 6000.0,
        "5": 1.0,
        "6": 0.65
      }
    },
    {
      "id": 4,
      "name": "Multiband Saturator",
      "type": 62,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 2.0,
        "2": 1.0,
        "3": 2.5,
        "4": 2.0,
        "5": 1.6,
        "6": 1.0,
        "7": 200.0,
        "8": 3200.0,
        "9": 0.8
      }
    },
    {
      "id": 5,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 50.0,
        "2": 15.0,
        "3": 2.0,
        "4": 1.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Lazos de Realimentación Inestable -> Self-Oscillating Resonant Spiral
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Self-Oscillating Resonant Spiral",
        "Lazos de Realimentación Inestable",
        R"json({
  "schemaVersion": 1,
  "name": "Self-Oscillating Resonant Spiral",
  "author": "N8Audio",
  "category": "Lazos de Realimentaci\u00f3n Inestable",
  "description": "Banco de resonadores modales excitando matriz fractal con control autom\u00e1tico de ganancia y paneo 3D",
  "dryLevel": 0.2,
  "wetLevel": 0.95,
  "macros": [
    0.85,
    0.6,
    0.8,
    0.5,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Resonator Bank",
      "type": 13,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 220.0,
        "2": 2.5,
        "3": 0.45,
        "4": 0.25,
        "5": 0.8
      }
    },
    {
      "id": 2,
      "name": "Fractal Resonator",
      "type": 67,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 440.0,
        "2": 0.618,
        "3": 0.94,
        "4": 9000.0,
        "5": 0.85,
        "6": 0.9
      }
    },
    {
      "id": 3,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 3.0,
        "2": 0.75,
        "3": 1.0,
        "4": 1.0,
        "5": 0.4,
        "6": 0.6,
        "7": 0.85
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 5,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 60.0,
        "2": 20.0,
        "3": 2.2,
        "4": 1.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.8,
        "2": -0.1,
        "3": 25.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Lazos de Realimentación Inestable -> Vocoded Spectral Mist Loop
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Vocoded Spectral Mist Loop",
        "Lazos de Realimentación Inestable",
        R"json({
  "schemaVersion": 1,
  "name": "Vocoded Spectral Mist Loop",
  "author": "N8Audio",
  "category": "Lazos de Realimentaci\u00f3n Inestable",
  "description": "Vocoder modular acoplado a difusi\u00f3n espectral y retardo con realimentaci\u00f3n densa",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.5,
    0.8,
    0.6,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Vocoder Carrier",
      "type": 36,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 16.0,
        "2": 0.5,
        "3": 0.8,
        "4": 0.85
      }
    },
    {
      "id": 2,
      "name": "Spectral Smear",
      "type": 52,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 5.0,
        "2": 0.9,
        "3": 0.92,
        "4": -0.1,
        "5": 0.85
      }
    },
    {
      "id": 3,
      "name": "Space Delay",
      "type": 28,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 250.0,
        "2": 375.0,
        "3": 0.6,
        "4": 6500.0,
        "5": 1.0,
        "6": 0.65
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 2.0,
        "2": 0.5,
        "3": 0.8
      }
    },
    {
      "id": 5,
      "name": "Stereo Imager",
      "type": 55,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.5,
        "2": 15.0,
        "3": 120.0,
        "4": 0.8
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Sintetizador de Voces & Formantes -> Alien Throat Morphosphere
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Alien Throat Morphosphere",
        "Sintetizador de Voces & Formantes",
        R"json({
  "schemaVersion": 1,
  "name": "Alien Throat Morphosphere",
  "author": "N8Audio",
  "category": "Sintetizador de Voces & Formantes",
  "description": "Sintetizador de tracto vocal alien\u00edgena con vocoder, filtro de formantes, SVF mutante y chorus",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.7,
    0.7,
    0.6,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Vocoder Engine",
      "type": 36,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 16.0,
        "2": 0.6,
        "3": 0.85,
        "4": 0.9
      }
    },
    {
      "id": 2,
      "name": "Formant Vowel",
      "type": 31,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 0.5,
        "2": 5.0,
        "3": 1.2,
        "4": 0.45,
        "5": 0.85
      }
    },
    {
      "id": 3,
      "name": "Morphing SVF Peak",
      "type": 66,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 1400.0,
        "2": 6.0,
        "3": 0.7,
        "4": 3.0,
        "5": 0.0
      }
    },
    {
      "id": 4,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 5.0,
        "2": 0.8,
        "3": 1.0,
        "4": 1.0,
        "5": 0.4,
        "6": 0.65,
        "7": 0.85
      }
    },
    {
      "id": 5,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Sintetizador de Voces & Formantes -> Android Talking Synthesizer
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Android Talking Synthesizer",
        "Sintetizador de Voces & Formantes",
        R"json({
  "schemaVersion": 1,
  "name": "Android Talking Synthesizer",
  "author": "N8Audio",
  "category": "Sintetizador de Voces & Formantes",
  "description": "Modulador en anillo acoplado a formantes vocales con decimador 8-bit y retardo espacial",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.85,
    0.6,
    0.6,
    0.7,
    0.6,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Ring Modulator",
      "type": 19,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 150.0,
        "2": 0.0,
        "3": 0.0,
        "4": 0.85
      }
    },
    {
      "id": 2,
      "name": "Formant Filter",
      "type": 31,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 0.4,
        "2": 4.0,
        "3": 1.0,
        "4": 0.4,
        "5": 0.8
      }
    },
    {
      "id": 3,
      "name": "Bitcrusher Decimator",
      "type": 40,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 8.0,
        "2": 2.5,
        "3": 0.03,
        "4": 0.3,
        "5": 0.0,
        "6": 0.7
      }
    },
    {
      "id": 4,
      "name": "Space Delay",
      "type": 28,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 220.0,
        "2": 330.0,
        "3": 0.5,
        "4": 6500.0,
        "5": 1.0,
        "6": 0.6
      }
    },
    {
      "id": 5,
      "name": "Stereo Imager",
      "type": 55,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.5,
        "2": 15.0,
        "3": 120.0,
        "4": 0.8
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Sintetizador de Voces & Formantes -> Spectral Robot Choir
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Spectral Robot Choir",
        "Sintetizador de Voces & Formantes",
        R"json({
  "schemaVersion": 1,
  "name": "Spectral Robot Choir",
  "author": "N8Audio",
  "category": "Sintetizador de Voces & Formantes",
  "description": "Coro rob\u00f3tico extraterrestre con transposici\u00f3n a la octava, matriz fractal \u00e1urea y reverb",
  "dryLevel": 0.25,
  "wetLevel": 0.95,
  "macros": [
    0.8,
    0.5,
    0.85,
    0.6,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Vocoder Engine",
      "type": 36,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 16.0,
        "2": 0.7,
        "3": 0.85,
        "4": 0.9
      }
    },
    {
      "id": 2,
      "name": "Pitch Shift +12st",
      "type": 11,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 12.0,
        "2": 0.0,
        "3": 35.0,
        "4": 0.75
      }
    },
    {
      "id": 3,
      "name": "Fractal Resonator",
      "type": 67,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 440.0,
        "2": 0.618,
        "3": 0.92,
        "4": 9000.0,
        "5": 0.85,
        "6": 0.85
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 5,
      "name": "Space Reverb",
      "type": 6,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 0.9,
        "2": 5.5,
        "3": 6000.0,
        "4": 30.0,
        "5": 0.6
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Sintetizador de Voces & Formantes -> Vocal Talkbox Resonator
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Vocal Talkbox Resonator",
        "Sintetizador de Voces & Formantes",
        R"json({
  "schemaVersion": 1,
  "name": "Vocal Talkbox Resonator",
  "author": "N8Audio",
  "category": "Sintetizador de Voces & Formantes",
  "description": "Filtro auto-wah con seguimiento envolvente impulsando formantes y saturaci\u00f3n no lineal",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.75,
    0.6,
    0.6,
    0.7,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Auto-Wah Filter",
      "type": 57,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 600.0,
        "2": 3.5,
        "3": 0.7,
        "4": 15.0,
        "5": 120.0,
        "6": 0.8
      }
    },
    {
      "id": 2,
      "name": "Formant Filter",
      "type": 31,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 0.35,
        "2": 4.5,
        "3": 1.1,
        "4": 0.45,
        "5": 0.85
      }
    },
    {
      "id": 3,
      "name": "Multiband Saturator",
      "type": 62,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 2.0,
        "2": 1.0,
        "3": 2.5,
        "4": 2.0,
        "5": 1.5,
        "6": 1.0,
        "7": 200.0,
        "8": 3200.0,
        "9": 0.8
      }
    },
    {
      "id": 4,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 4.5,
        "2": 0.7,
        "3": 1.0,
        "4": 1.0,
        "5": 0.4,
        "6": 0.6,
        "7": 0.8
      }
    },
    {
      "id": 5,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 2.0,
        "2": 0.5,
        "3": 0.8
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Dinámica Agresiva & OTT -> Master Wall Multiband Crusher
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Master Wall Multiband Crusher",
        "Dinámica Agresiva & OTT",
        R"json({
  "schemaVersion": 1,
  "name": "Master Wall Multiband Crusher",
  "author": "N8Audio",
  "category": "Din\u00e1mica Agresiva & OTT",
  "description": "Saturador multibanda tri-frecuencia acoplado a din\u00e1mica OTT agresiva, transient punch y limitador brickwall",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [
    0.8,
    0.7,
    0.5,
    0.85,
    0.6,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Multiband Saturator",
      "type": 62,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 3.5,
        "2": 1.0,
        "3": 2.8,
        "4": 2.0,
        "5": 2.2,
        "6": 1.0,
        "7": 180.0,
        "8": 3000.0,
        "9": 0.9
      }
    },
    {
      "id": 2,
      "name": "Multiband OTT",
      "type": 15,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 180.0,
        "2": 3000.0,
        "3": 1.6,
        "4": 1.3,
        "5": 1.4,
        "6": 0.95
      }
    },
    {
      "id": 3,
      "name": "Transient Snap",
      "type": 33,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 0.8,
        "2": -0.3,
        "3": 1.2,
        "4": 1.0,
        "5": 2.0,
        "6": 1.0
      }
    },
    {
      "id": 4,
      "name": "Tape Saturation",
      "type": 21,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 3.0,
        "2": 1.0,
        "3": 0.3,
        "4": 0.8,
        "5": 0.85
      }
    },
    {
      "id": 5,
      "name": "Stereo Imager",
      "type": 55,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.4,
        "2": 12.0,
        "3": 120.0,
        "4": 0.85
      }
    },
    {
      "id": 6,
      "name": "Brickwall Peak Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.8,
        "2": -0.1,
        "3": 25.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Dinámica Agresiva & OTT -> Industrial Cybernetic Wavefolder
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Industrial Cybernetic Wavefolder",
        "Dinámica Agresiva & OTT",
        R"json({
  "schemaVersion": 1,
  "name": "Industrial Cybernetic Wavefolder",
  "author": "N8Audio",
  "category": "Din\u00e1mica Agresiva & OTT",
  "description": "Pliegue de onda no lineal con filtro mutante SVF saturado y compresi\u00f3n hacia arriba",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.85,
    0.6,
    0.5,
    0.9,
    0.7,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Wavefold Distortion",
      "type": 8,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 5.5,
        "2": 2.0,
        "3": 6500.0,
        "4": 0.85
      }
    },
    {
      "id": 2,
      "name": "Morphing SVF Peak",
      "type": 66,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 1200.0,
        "2": 8.0,
        "3": 0.65,
        "4": 4.5,
        "5": 0.0
      }
    },
    {
      "id": 3,
      "name": "Multiband OTT",
      "type": 15,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 200.0,
        "2": 3200.0,
        "3": 1.4,
        "4": 1.2,
        "5": 1.3,
        "6": 0.9
      }
    },
    {
      "id": 4,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 6.0,
        "2": 0.75,
        "3": 1.0,
        "4": 1.0,
        "5": 0.45,
        "6": 0.65,
        "7": 0.85
      }
    },
    {
      "id": 5,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 45.0,
        "2": 15.0,
        "3": 2.0,
        "4": 1.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.8,
        "2": -0.1,
        "3": 25.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Dinámica Agresiva & OTT -> Transient Demolisher Rig
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Transient Demolisher Rig",
        "Dinámica Agresiva & OTT",
        R"json({
  "schemaVersion": 1,
  "name": "Transient Demolisher Rig",
  "author": "N8Audio",
  "category": "Din\u00e1mica Agresiva & OTT",
  "description": "Modelador de transientes con decimaci\u00f3n bitcrush y saturaci\u00f3n anal\u00f3gica en tres bandas",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [
    0.8,
    0.5,
    0.7,
    0.8,
    0.6,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Transient Snap",
      "type": 33,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.9,
        "2": -0.4,
        "3": 1.4,
        "4": 1.0,
        "5": 2.0,
        "6": 1.0
      }
    },
    {
      "id": 2,
      "name": "Bitcrusher Decimator",
      "type": 40,
      "x": 240.0,
      "y": 30.0,
      "params": {
        "1": 10.0,
        "2": 2.5,
        "3": 0.03,
        "4": 0.35,
        "5": 0.0,
        "6": 0.75
      }
    },
    {
      "id": 3,
      "name": "Multiband Saturator",
      "type": 62,
      "x": 240.0,
      "y": 140.0,
      "params": {
        "1": 2.8,
        "2": 1.0,
        "3": 2.6,
        "4": 2.0,
        "5": 1.8,
        "6": 1.0,
        "7": 200.0,
        "8": 3000.0,
        "9": 0.85
      }
    },
    {
      "id": 4,
      "name": "Comb Resonator",
      "type": 63,
      "x": 480.0,
      "y": 80.0,
      "params": {
        "1": 220.0,
        "2": 0.8,
        "3": 8000.0,
        "4": 12.0,
        "5": 0.65
      }
    },
    {
      "id": 5,
      "name": "Stereo Imager",
      "type": 55,
      "x": 680.0,
      "y": 80.0,
      "params": {
        "1": 1.5,
        "2": 15.0,
        "3": 120.0,
        "4": 0.8
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 880.0,
      "y": 80.0,
      "params": {
        "1": -0.8,
        "2": -0.1,
        "3": 25.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Dinámica Agresiva & OTT -> Hypersonic Cybernetic Slicer
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Hypersonic Cybernetic Slicer",
        "Dinámica Agresiva & OTT",
        R"json({
  "schemaVersion": 1,
  "name": "Hypersonic Cybernetic Slicer",
  "author": "N8Audio",
  "category": "Din\u00e1mica Agresiva & OTT",
  "description": "Fragmentaci\u00f3n a 16vas con wavefolding no lineal, compresi\u00f3n multinivel y limitaci\u00f3n de pico",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.85,
    0.7,
    0.6,
    0.85,
    0.7,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Hyper Slicer",
      "type": 49,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.0,
        "2": 16.0,
        "3": -1.0,
        "4": 1.0,
        "5": 0.0,
        "6": 40.0,
        "7": 6.0,
        "8": 0.4,
        "9": 0.0,
        "10": 16000.0,
        "11": 1.0
      }
    },
    {
      "id": 2,
      "name": "Wavefold Drive",
      "type": 8,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 5.5,
        "2": 2.0,
        "3": 6500.0,
        "4": 0.8
      }
    },
    {
      "id": 3,
      "name": "Multiband OTT",
      "type": 15,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 180.0,
        "2": 3000.0,
        "3": 1.3,
        "4": 1.1,
        "5": 1.2,
        "6": 0.9
      }
    },
    {
      "id": 4,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 7.0,
        "2": 0.8,
        "3": 1.0,
        "4": 1.0,
        "5": 0.5,
        "6": 0.7,
        "7": 0.9
      }
    },
    {
      "id": 5,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 2.0,
        "2": 0.5,
        "3": 0.8
      }
    },
    {
      "id": 6,
      "name": "Brickwall Peak",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -1.0,
        "2": -0.2,
        "3": 40.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Subarmónicos & Graves -> Sub-Octave Earthquake Growl
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Sub-Octave Earthquake Growl",
        "Subarmónicos & Graves",
        R"json({
  "schemaVersion": 1,
  "name": "Sub-Octave Earthquake Growl",
  "author": "N8Audio",
  "category": "Subarm\u00f3nicos & Graves",
  "description": "Generador de sub-octava con saturaci\u00f3n de bajos gruesos, EQ din\u00e1mica y limitador anti-clip",
  "dryLevel": 0.35,
  "wetLevel": 0.95,
  "macros": [
    0.8,
    0.5,
    0.6,
    0.75,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Pitch Shift -12st",
      "type": 11,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": -12.0,
        "2": 0.0,
        "3": 50.0,
        "4": 0.9
      }
    },
    {
      "id": 2,
      "name": "Multiband Saturator Low",
      "type": 62,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 4.0,
        "2": 1.0,
        "3": 1.8,
        "4": 1.2,
        "5": 1.0,
        "6": 1.0,
        "7": 150.0,
        "8": 2500.0,
        "9": 0.9
      }
    },
    {
      "id": 3,
      "name": "Dynamic EQ Low",
      "type": 54,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 60.0,
        "2": 1.5,
        "3": 6.0,
        "4": -12.0,
        "5": 20.0,
        "6": 120.0,
        "7": 0.85
      }
    },
    {
      "id": 4,
      "name": "Tape Saturation",
      "type": 21,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 2.5,
        "2": 1.0,
        "3": 0.3,
        "4": 0.8,
        "5": 0.8
      }
    },
    {
      "id": 5,
      "name": "Stereo Imager Mono-Bass",
      "type": 55,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.2,
        "2": 0.0,
        "3": 180.0,
        "4": 0.9
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 35.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Subarmónicos & Graves -> Deep Space Resonant Drone
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Deep Space Resonant Drone",
        "Subarmónicos & Graves",
        R"json({
  "schemaVersion": 1,
  "name": "Deep Space Resonant Drone",
  "author": "N8Audio",
  "category": "Subarm\u00f3nicos & Graves",
  "description": "Resonador modal a 55Hz (La fundamental) acoplado a peine sintonizado y matriz fractal en paneo 3D",
  "dryLevel": 0.25,
  "wetLevel": 0.95,
  "macros": [
    0.85,
    0.6,
    0.85,
    0.6,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Modal 55Hz",
      "type": 58,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 55.0,
        "2": 0.92,
        "3": 0.3,
        "4": 0.5,
        "5": 0.0,
        "6": 0.9
      }
    },
    {
      "id": 2,
      "name": "Comb Resonator",
      "type": 63,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 110.0,
        "2": 0.85,
        "3": 6000.0,
        "4": 12.0,
        "5": 0.7
      }
    },
    {
      "id": 3,
      "name": "Fractal Resonator",
      "type": 67,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 165.0,
        "2": 0.618,
        "3": 0.91,
        "4": 7000.0,
        "5": 0.85,
        "6": 0.8
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 2.0,
        "2": 0.5,
        "3": 0.8
      }
    },
    {
      "id": 5,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 40.0,
        "2": 15.0,
        "3": 2.0,
        "4": 1.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Subarmónicos & Graves -> Sub-Harmonic Earth Shaker
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Sub-Harmonic Earth Shaker",
        "Subarmónicos & Graves",
        R"json({
  "schemaVersion": 1,
  "name": "Sub-Harmonic Earth Shaker",
  "author": "N8Audio",
  "category": "Subarm\u00f3nicos & Graves",
  "description": "Cuerda f\u00edsica Karplus de graves profundos con realce param\u00e9trico y compresi\u00f3n multibanda",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.5,
    0.6,
    0.8,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Karplus Bass",
      "type": 37,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 65.41,
        "2": 0.98,
        "3": 0.25,
        "4": 0.85,
        "5": 0.0,
        "6": 0.9
      }
    },
    {
      "id": 2,
      "name": "Parametric Sub EQ",
      "type": 27,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 50.0,
        "2": 4.5,
        "3": 200.0,
        "4": 1.8,
        "5": -3.0,
        "6": 6000.0,
        "7": -4.0
      }
    },
    {
      "id": 3,
      "name": "Multiband Saturator",
      "type": 62,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 3.0,
        "2": 1.0,
        "3": 1.6,
        "4": 1.2,
        "5": 1.0,
        "6": 1.0,
        "7": 160.0,
        "8": 2800.0,
        "9": 0.85
      }
    },
    {
      "id": 4,
      "name": "Multiband OTT",
      "type": 15,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 150.0,
        "2": 2500.0,
        "3": 1.4,
        "4": 1.1,
        "5": 1.2,
        "6": 0.9
      }
    },
    {
      "id": 5,
      "name": "Stereo Imager",
      "type": 55,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.2,
        "2": 0.0,
        "3": 160.0,
        "4": 0.9
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.8,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Subarmónicos & Graves -> Seismic Sub Reinforcer
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Seismic Sub Reinforcer",
        "Subarmónicos & Graves",
        R"json({
  "schemaVersion": 1,
  "name": "Seismic Sub Reinforcer",
  "author": "N8Audio",
  "category": "Subarm\u00f3nicos & Graves",
  "description": "Transientes contundentes reforzados con subarm\u00f3nico a -12st y saturador de 3 bandas",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.75,
    0.5,
    0.5,
    0.8,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Transient Snap",
      "type": 33,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.8,
        "2": -0.3,
        "3": 1.3,
        "4": 1.0,
        "5": 2.0,
        "6": 1.0
      }
    },
    {
      "id": 2,
      "name": "Pitch Shift Sub",
      "type": 11,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": -12.0,
        "2": 0.0,
        "3": 45.0,
        "4": 0.85
      }
    },
    {
      "id": 3,
      "name": "Multiband Saturator",
      "type": 62,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 3.2,
        "2": 1.0,
        "3": 2.0,
        "4": 1.5,
        "5": 1.2,
        "6": 1.0,
        "7": 180.0,
        "8": 3000.0,
        "9": 0.85
      }
    },
    {
      "id": 4,
      "name": "Dynamic EQ Low",
      "type": 54,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 80.0,
        "2": 2.0,
        "3": 4.0,
        "4": -14.0,
        "5": 15.0,
        "6": 100.0,
        "7": 0.8
      }
    },
    {
      "id": 5,
      "name": "Stereo Imager",
      "type": 55,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.2,
        "2": 0.0,
        "3": 160.0,
        "4": 0.85
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Experimental & Avant-Garde -> Extraterrestrial Bio-Resonator
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Extraterrestrial Bio-Resonator",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Extraterrestrial Bio-Resonator",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Resonador fractal acoplado a dispersi\u00f3n allpass y filtro SVF mutante con audio-rate FM",
  "dryLevel": 0.2,
  "wetLevel": 0.95,
  "macros": [
    0.9,
    0.7,
    0.75,
    0.8,
    0.7,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Fractal Resonator",
      "type": 67,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 220.0,
        "2": 0.618,
        "3": 0.93,
        "4": 8500.0,
        "5": 0.85,
        "6": 0.9
      }
    },
    {
      "id": 2,
      "name": "Ring Modulator FM",
      "type": 19,
      "x": 240.0,
      "y": 30.0,
      "params": {
        "1": 180.0,
        "2": 0.0,
        "3": 0.0,
        "4": 0.85
      }
    },
    {
      "id": 3,
      "name": "Phase Dispersion",
      "type": 65,
      "x": 240.0,
      "y": 140.0,
      "params": {
        "1": 20.0,
        "2": 2400.0,
        "3": 0.88,
        "4": 12000.0,
        "5": 0.6,
        "6": 0.9
      }
    },
    {
      "id": 4,
      "name": "Morphing SVF Acid",
      "type": 66,
      "x": 460.0,
      "y": 80.0,
      "params": {
        "1": 950.0,
        "2": 14.0,
        "3": 0.45,
        "4": 4.0,
        "5": 0.6
      }
    },
    {
      "id": 5,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.8,
        "2": -0.1,
        "3": 25.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 4
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Experimental & Avant-Garde -> Quantum Entangled Particle Wave
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Quantum Entangled Particle Wave",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Quantum Entangled Particle Wave",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Part\u00edculas segmentadas divididas en refracci\u00f3n prism\u00e1tica, pitch delay y difusi\u00f3n espectral",
  "dryLevel": 0.25,
  "wetLevel": 0.95,
  "macros": [
    0.85,
    0.6,
    0.85,
    0.6,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Audio Slicer",
      "type": 49,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.0,
        "2": 16.0,
        "3": -1.0,
        "4": 1.0,
        "5": 0.0,
        "6": 40.0,
        "7": 5.0,
        "8": 0.35,
        "9": 0.0,
        "10": 14000.0,
        "11": 0.95
      }
    },
    {
      "id": 2,
      "name": "Refraction Prism",
      "type": 51,
      "x": 240.0,
      "y": 30.0,
      "params": {
        "1": 8.0,
        "2": 22.0,
        "3": 24.0,
        "4": 0.9,
        "5": 0.8,
        "6": 2.2,
        "7": 0.85
      }
    },
    {
      "id": 3,
      "name": "Pitch Delay Crystals",
      "type": 64,
      "x": 240.0,
      "y": 140.0,
      "params": {
        "1": 200.0,
        "2": 300.0,
        "3": 7.0,
        "4": 14.0,
        "5": 0.55,
        "6": 0.0,
        "7": 8500.0,
        "8": 0.75
      }
    },
    {
      "id": 4,
      "name": "Spectral Smear",
      "type": 52,
      "x": 460.0,
      "y": 80.0,
      "params": {
        "1": 4.5,
        "2": 0.88,
        "3": 0.9,
        "4": -0.1,
        "5": 0.85
      }
    },
    {
      "id": 5,
      "name": "Mid/Side Decoder",
      "type": 25,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.6,
        "2": 120.0,
        "3": 0.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Experimental & Avant-Garde -> Cosmic Laser Phantasm
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Cosmic Laser Phantasm",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Cosmic Laser Phantasm",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Transientes convertidos en estallidos de chirps l\u00e1ser con retardo cristalino y paneo esf\u00e9rico",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [
    0.8,
    0.7,
    0.85,
    0.6,
    0.6,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Transient Snap",
      "type": 33,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 0.85,
        "2": -0.3,
        "3": 1.3,
        "4": 1.0,
        "5": 2.0,
        "6": 1.0
      }
    },
    {
      "id": 2,
      "name": "Laser Dispersion",
      "type": 65,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 24.0,
        "2": 2800.0,
        "3": 0.92,
        "4": 15000.0,
        "5": 0.7,
        "6": 1.0
      }
    },
    {
      "id": 3,
      "name": "Pitch Delay Crystals",
      "type": 64,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 160.0,
        "2": 240.0,
        "3": 12.0,
        "4": 15.0,
        "5": 0.6,
        "6": 0.0,
        "7": 9000.0,
        "8": 0.75
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 5,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 65.0,
        "2": 20.0,
        "3": 2.2,
        "4": 1.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 25.0,
        "4": 1.5,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Experimental & Avant-Garde -> Bioluminescent Dark Drone
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Bioluminescent Dark Drone",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Bioluminescent Dark Drone",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Resonancia f\u00edsica modal en baja frecuencia con filtro mutante SVF, retardo de tono y congelador espectral",
  "dryLevel": 0.2,
  "wetLevel": 0.95,
  "macros": [
    0.85,
    0.5,
    0.9,
    0.6,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Modal 110Hz",
      "type": 58,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 110.0,
        "2": 0.9,
        "3": 0.35,
        "4": 0.5,
        "5": 0.0,
        "6": 0.85
      }
    },
    {
      "id": 2,
      "name": "Morphing SVF Tone",
      "type": 66,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 750.0,
        "2": 5.0,
        "3": 0.3,
        "4": 2.5,
        "5": 0.0
      }
    },
    {
      "id": 3,
      "name": "Pitch Delay",
      "type": 64,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 250.0,
        "2": 375.0,
        "3": -7.0,
        "4": 10.0,
        "5": 0.55,
        "6": 0.0,
        "7": 7500.0,
        "8": 0.7
      }
    },
    {
      "id": 4,
      "name": "Spectral Freeze",
      "type": 12,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 0.85,
        "2": 0.8,
        "3": 0.9,
        "4": 0.8
      }
    },
    {
      "id": 5,
      "name": "Stereo Imager",
      "type": 55,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 1.6,
        "2": 15.0,
        "3": 130.0,
        "4": 0.85
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Ecos Cinematográficos & Paisajes -> Solaris Deep Space Echoplex
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Solaris Deep Space Echoplex",
        "Ecos Cinematográficos & Paisajes",
        R"json({
  "schemaVersion": 1,
  "name": "Solaris Deep Space Echoplex",
  "author": "N8Audio",
  "category": "Ecos Cinematogr\u00e1ficos & Paisajes",
  "description": "Eco de cinta anal\u00f3gico profundo con retardos de cristal de cuarzo y matriz fractal en shimmer infinito",
  "dryLevel": 0.25,
  "wetLevel": 0.95,
  "macros": [
    0.85,
    0.6,
    0.9,
    0.5,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Advanced Tape Delay",
      "type": 28,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 350.0,
        "2": 525.0,
        "3": 0.6,
        "4": 6500.0,
        "5": 1.0,
        "6": 0.65
      }
    },
    {
      "id": 2,
      "name": "Pitch Delay Crystals",
      "type": 64,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 180.0,
        "2": 270.0,
        "3": 12.0,
        "4": 15.0,
        "5": 0.6,
        "6": 0.0,
        "7": 8500.0,
        "8": 0.75
      }
    },
    {
      "id": 3,
      "name": "Fractal Resonator",
      "type": 67,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 261.63,
        "2": 0.618,
        "3": 0.92,
        "4": 9000.0,
        "5": 0.85,
        "6": 0.85
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 5,
      "name": "Shimmer Reverb",
      "type": 50,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 9.0,
        "2": 7500.0,
        "3": 0.65,
        "4": 0.0,
        "5": 1.5,
        "6": 0.7
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Ecos Cinematográficos & Paisajes -> Glacial Canyon Horizon
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Glacial Canyon Horizon",
        "Ecos Cinematográficos & Paisajes",
        R"json({
  "schemaVersion": 1,
  "name": "Glacial Canyon Horizon",
  "author": "N8Audio",
  "category": "Ecos Cinematogr\u00e1ficos & Paisajes",
  "description": "Dispersi\u00f3n allpass l\u00edquida con ecos reflectantes, difusi\u00f3n espectral y reverberaci\u00f3n de ca\u00f1\u00f3n \u00e1rtico",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [
    0.8,
    0.7,
    0.9,
    0.5,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Phase Dispersion",
      "type": 65,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 20.0,
        "2": 1400.0,
        "3": 0.8,
        "4": 11000.0,
        "5": 0.5,
        "6": 0.9
      }
    },
    {
      "id": 2,
      "name": "Space Delay",
      "type": 28,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 400.0,
        "2": 600.0,
        "3": 0.65,
        "4": 6000.0,
        "5": 1.0,
        "6": 0.65
      }
    },
    {
      "id": 3,
      "name": "Spectral Smear",
      "type": 52,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 5.0,
        "2": 0.9,
        "3": 0.92,
        "4": -0.1,
        "5": 0.85
      }
    },
    {
      "id": 4,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 3.0,
        "2": 0.6,
        "3": 0.85
      }
    },
    {
      "id": 5,
      "name": "Space Reverb",
      "type": 6,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 0.95,
        "2": 8.0,
        "3": 4500.0,
        "4": 35.0,
        "5": 0.6
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Ecos Cinematográficos & Paisajes -> Sub-Quantum Wormhole Drift
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Sub-Quantum Wormhole Drift",
        "Ecos Cinematográficos & Paisajes",
        R"json({
  "schemaVersion": 1,
  "name": "Sub-Quantum Wormhole Drift",
  "author": "N8Audio",
  "category": "Ecos Cinematogr\u00e1ficos & Paisajes",
  "description": "Corrimiento de frecuencia Hilbert continuo con reverberaci\u00f3n inversa, saturaci\u00f3n y paneo esf\u00e9rico",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [
    0.8,
    0.7,
    0.85,
    0.6,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Freq Shifter",
      "type": 20,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 6.0,
        "2": 0.35,
        "3": 0.85
      }
    },
    {
      "id": 2,
      "name": "Reverse Reverb",
      "type": 38,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 1.8,
        "2": 0.85,
        "3": 6500.0,
        "4": 0.85
      }
    },
    {
      "id": 3,
      "name": "Multiband Saturator",
      "type": 62,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 2.2,
        "2": 1.0,
        "3": 2.6,
        "4": 2.0,
        "5": 1.6,
        "6": 1.0,
        "7": 220.0,
        "8": 3200.0,
        "9": 0.85
      }
    },
    {
      "id": 4,
      "name": "Dynamic Tremolo",
      "type": 61,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 4.5,
        "2": 0.75,
        "3": 1.0,
        "4": 1.0,
        "5": 0.4,
        "6": 0.6,
        "7": 0.85
      }
    },
    {
      "id": 5,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": 50.0,
        "2": 20.0,
        "3": 2.2,
        "4": 1.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Ecos Cinematográficos & Paisajes -> Alien Ring Transmission Matrix
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Alien Ring Transmission Matrix",
        "Ecos Cinematográficos & Paisajes",
        R"json({
  "schemaVersion": 1,
  "name": "Alien Ring Transmission Matrix",
  "author": "N8Audio",
  "category": "Ecos Cinematogr\u00e1ficos & Paisajes",
  "description": "Modulador en anillo acoplado a transposici\u00f3n arm\u00f3nica a la 19na con pliegue de onda no lineal y retardo espacial",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.85,
    0.6,
    0.75,
    0.7,
    0.6,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Ring Modulator",
      "type": 19,
      "x": 40.0,
      "y": 80.0,
      "params": {
        "1": 280.0,
        "2": 0.0,
        "3": 0.0,
        "4": 0.85
      }
    },
    {
      "id": 2,
      "name": "Pitch Shift +19st",
      "type": 11,
      "x": 240.0,
      "y": 80.0,
      "params": {
        "1": 19.0,
        "2": 0.0,
        "3": 40.0,
        "4": 0.75
      }
    },
    {
      "id": 3,
      "name": "Wavefold Distortion",
      "type": 8,
      "x": 440.0,
      "y": 30.0,
      "params": {
        "1": 4.5,
        "2": 2.0,
        "3": 5500.0,
        "4": 0.8
      }
    },
    {
      "id": 4,
      "name": "Space Delay",
      "type": 28,
      "x": 440.0,
      "y": 140.0,
      "params": {
        "1": 300.0,
        "2": 450.0,
        "3": 0.55,
        "4": 6500.0,
        "5": 1.0,
        "6": 0.6
      }
    },
    {
      "id": 5,
      "name": "Spatial Panner 3D",
      "type": 26,
      "x": 660.0,
      "y": 80.0,
      "params": {
        "1": -40.0,
        "2": 20.0,
        "3": 2.2,
        "4": 1.0
      }
    },
    {
      "id": 6,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 860.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 30.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    },
    {
      "id": 4,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 5,
      "srcNode": 4,
      "srcPin": 2,
      "destNode": 5,
      "destPin": 1
    },
    {
      "id": 6,
      "srcNode": 5,
      "srcPin": 2,
      "destNode": 6,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Mastering & Dinámica Compleja -> French Touch Multiband Pumping Rig
    // -------------------------------------------------------------------------
    catalog.push_back({
        "French Touch Multiband Pumping Rig",
        "Mastering & Dinámica Compleja",
        R"json({
  "schemaVersion": 1,
  "name": "French Touch Multiband Pumping Rig",
  "author": "N8Audio",
  "category": "Mastering & Din\u00e1mica Compleja",
  "description": "Bombeo r\u00edtmico Kickstart sobre frecuencias subgraves con saturaci\u00f3n multibanda y expansi\u00f3n est\u00e9reo",
  "dryLevel": 0.1,
  "wetLevel": 0.95,
  "macros": [
    0.8,
    0.5,
    0.7,
    0.4,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Multiband Drive",
      "type": 62,
      "x": 60.0,
      "y": 80.0,
      "params": {
        "1": 180.0,
        "2": 2600.0,
        "3": 1.4,
        "4": 2.2,
        "5": 1.6,
        "6": 0.85
      }
    },
    {
      "id": 2,
      "name": "Dynamic Pumper",
      "type": 70,
      "x": 260.0,
      "y": 80.0,
      "params": {
        "1": 2.0,
        "2": 0.92,
        "3": 0.0,
        "4": 1.1,
        "5": 0.0,
        "6": 1.0,
        "7": 220.0,
        "8": 0.0,
        "9": 1.0
      }
    },
    {
      "id": 3,
      "name": "Stereo Imager",
      "type": 55,
      "x": 480.0,
      "y": 80.0,
      "params": {
        "1": 1.35,
        "2": 110.0,
        "3": 0.25,
        "4": 0.0,
        "5": 0.9
      }
    },
    {
      "id": 4,
      "name": "Master Limiter",
      "type": 39,
      "x": 700.0,
      "y": 80.0,
      "params": {
        "1": -0.8,
        "2": -0.1,
        "3": 40.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Espacialización & Psicoacústica 3D -> Bipolar Spectral Mirage 3D
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Bipolar Spectral Mirage 3D",
        "Espacialización & Psicoacústica 3D",
        R"json({
  "schemaVersion": 1,
  "name": "Bipolar Spectral Mirage 3D",
  "author": "N8Audio",
  "category": "Espacializaci\u00f3n & Psicoac\u00fastica 3D",
  "description": "Separaci\u00f3n espectral din\u00e1mica con microretardo Haas, proyecci\u00f3n de agudos y graves contrapuestos",
  "dryLevel": 0.2,
  "wetLevel": 0.9,
  "macros": [
    0.75,
    0.6,
    0.8,
    0.5,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Spectral Tilt Panner",
      "type": 71,
      "x": 60.0,
      "y": 80.0,
      "params": {
        "1": 700.0,
        "2": -0.85,
        "3": 0.85,
        "4": 0.7,
        "5": 20.0,
        "6": 0.5,
        "7": 95.0,
        "8": 1.0
      }
    },
    {
      "id": 2,
      "name": "Morphing Filter",
      "type": 66,
      "x": 280.0,
      "y": 80.0,
      "params": {
        "1": 2.5,
        "2": 1800.0,
        "3": 2.2,
        "4": 0.8,
        "5": 0.75
      }
    },
    {
      "id": 3,
      "name": "Dimension Chorus",
      "type": 60,
      "x": 500.0,
      "y": 80.0,
      "params": {
        "1": 0.4,
        "2": 0.65,
        "3": 0.8,
        "4": 0.7
      }
    },
    {
      "id": 4,
      "name": "True Peak Limiter",
      "type": 39,
      "x": 720.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 35.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Mastering & Dinámica Compleja -> Infrasonic Sub-Bass Titan
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Infrasonic Sub-Bass Titan",
        "Mastering & Dinámica Compleja",
        R"json({
  "schemaVersion": 1,
  "name": "Infrasonic Sub-Bass Titan",
  "author": "N8Audio",
  "category": "Mastering & Din\u00e1mica Compleja",
  "description": "S\u00edntesis infras\u00f3nica anal\u00f3gica dbx de suboctavas combinada con moldeado arm\u00f3nico Chebyshev y limitaci\u00f3n brickwall",
  "dryLevel": 0.15,
  "wetLevel": 0.95,
  "macros": [
    0.8,
    0.6,
    0.7,
    0.4,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Subharmonic Generator",
      "type": 73,
      "x": 60.0,
      "y": 80.0,
      "params": {
        "1": 85.0,
        "2": 0.85,
        "3": 0.5,
        "4": 1.4,
        "5": 1.1,
        "6": 1.0
      }
    },
    {
      "id": 2,
      "name": "Chebyshev Shaper",
      "type": 72,
      "x": 280.0,
      "y": 80.0,
      "params": {
        "1": 1.2,
        "2": 0.45,
        "3": 0.25,
        "4": 0.15,
        "5": 0.05,
        "6": 0.0,
        "7": 0.0,
        "8": 0.0,
        "9": 0.85
      }
    },
    {
      "id": 3,
      "name": "Multiband Dynamics",
      "type": 30,
      "x": 500.0,
      "y": 80.0,
      "params": {
        "1": 150.0,
        "2": 3000.0,
        "3": -12.0,
        "4": 3.0,
        "5": 1.0
      }
    },
    {
      "id": 4,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 720.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 35.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Espacialización & Psicoacústica 3D -> Doppler Holographic Holodeck 3D
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Doppler Holographic Holodeck 3D",
        "Espacialización & Psicoacústica 3D",
        R"json({
  "schemaVersion": 1,
  "name": "Doppler Holographic Holodeck 3D",
  "author": "N8Audio",
  "category": "Espacializaci\u00f3n & Psicoac\u00fastica 3D",
  "description": "\u00d3rbita f\u00edsica 3D Doppler continua alrededor de la cabeza con expansi\u00f3n hologr\u00e1fica RACE fuera de los altavoces",
  "dryLevel": 0.2,
  "wetLevel": 0.9,
  "macros": [
    0.7,
    0.65,
    0.8,
    0.5,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Orbital Doppler",
      "type": 75,
      "x": 60.0,
      "y": 80.0,
      "params": {
        "1": 0.45,
        "2": 4.0,
        "3": 12.0,
        "4": 0.75,
        "5": 0.6,
        "6": 0.24,
        "7": 1.0
      }
    },
    {
      "id": 2,
      "name": "Transaural Expander",
      "type": 74,
      "x": 280.0,
      "y": 80.0,
      "params": {
        "1": 185.0,
        "2": 0.72,
        "3": 6200.0,
        "4": 0.65,
        "5": 0.9
      }
    },
    {
      "id": 3,
      "name": "Shimmer Reverb",
      "type": 50,
      "x": 500.0,
      "y": 80.0,
      "params": {
        "1": 0.45,
        "2": 0.7,
        "3": 0.5,
        "4": 0.4,
        "5": 0.35
      }
    },
    {
      "id": 4,
      "name": "True Peak Limiter",
      "type": 39,
      "x": 720.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 35.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Granular & Degradación Vintage -> Tape Decay Memory Shuffler
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Tape Decay Memory Shuffler",
        "Granular & Degradación Vintage",
        R"json({
  "schemaVersion": 1,
  "name": "Tape Decay Memory Shuffler",
  "author": "N8Audio",
  "category": "Granular & Degradaci\u00f3n Vintage",
  "description": "Fragmentaci\u00f3n granular estoc\u00e1stica en reversa con degradaci\u00f3n continua de cinta magn\u00e9tica de bucle infinito",
  "dryLevel": 0.2,
  "wetLevel": 0.9,
  "macros": [
    0.75,
    0.6,
    0.7,
    0.5,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Granular Shuffler",
      "type": 76,
      "x": 60.0,
      "y": 80.0,
      "params": {
        "1": 1400.0,
        "2": 150.0,
        "3": 14.0,
        "4": 0.4,
        "5": 4.0,
        "6": 0.8,
        "7": 0.0,
        "8": 0.85
      }
    },
    {
      "id": 2,
      "name": "Disintegration Looper",
      "type": 77,
      "x": 280.0,
      "y": 80.0,
      "params": {
        "1": 1200.0,
        "2": 0.78,
        "3": 0.55,
        "4": 0.35,
        "5": 0.3,
        "6": 0.6,
        "7": 0.75
      }
    },
    {
      "id": 3,
      "name": "Comb Resonator",
      "type": 63,
      "x": 500.0,
      "y": 80.0,
      "params": {
        "1": 320.0,
        "2": 0.45,
        "3": 0.5,
        "4": 0.6
      }
    },
    {
      "id": 4,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 720.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 35.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Pitch Shifting & Resonancias Especiales -> Bode Spiral Spring Chamber
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Bode Spiral Spring Chamber",
        "Pitch Shifting & Resonancias Especiales",
        R"json({
  "schemaVersion": 1,
  "name": "Bode Spiral Spring Chamber",
  "author": "N8Audio",
  "category": "Pitch Shifting & Resonancias Especiales",
  "description": "Desplazamiento arm\u00f3nico en espiral Moog/Bode alimentando c\u00e1mara electromagn\u00e9tica de muelles con dispersi\u00f3n chirpeada",
  "dryLevel": 0.2,
  "wetLevel": 0.9,
  "macros": [
    0.7,
    0.65,
    0.8,
    0.5,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Bode Shifter",
      "type": 79,
      "x": 60.0,
      "y": 80.0,
      "params": {
        "1": 18.0,
        "2": 12.0,
        "3": 0.35,
        "4": 6500.0,
        "5": 1.0,
        "6": 0.9
      }
    },
    {
      "id": 2,
      "name": "Spring Tank",
      "type": 78,
      "x": 280.0,
      "y": 80.0,
      "params": {
        "1": 0.75,
        "2": 3.2,
        "3": 0.8,
        "4": 1.4,
        "5": 4200.0,
        "6": 0.65
      }
    },
    {
      "id": 3,
      "name": "Morphing SVF",
      "type": 66,
      "x": 500.0,
      "y": 80.0,
      "params": {
        "1": 3800.0,
        "2": 0.4,
        "3": 0.0,
        "4": 1.0
      }
    },
    {
      "id": 4,
      "name": "True Peak Limiter",
      "type": 39,
      "x": 720.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 35.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Pitch Shifting & Resonancias Especiales -> Microtonal Vocal Formant Chime
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Microtonal Vocal Formant Chime",
        "Pitch Shifting & Resonancias Especiales",
        R"json({
  "schemaVersion": 1,
  "name": "Microtonal Vocal Formant Chime",
  "author": "N8Audio",
  "category": "Pitch Shifting & Resonancias Especiales",
  "description": "Armonizaci\u00f3n microtonal 31-EDO con desacoplamiento de formantes vocales anat\u00f3micos y resonancia de cuerda sintonizada",
  "dryLevel": 0.2,
  "wetLevel": 0.9,
  "macros": [
    0.7,
    0.6,
    0.8,
    0.5,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Microtonal Harmonizer",
      "type": 80,
      "x": 60.0,
      "y": 80.0,
      "params": {
        "1": 0.0,
        "2": 31.0,
        "3": -0.65,
        "4": 7.0,
        "5": -14.0,
        "6": 0.65,
        "7": 3.0,
        "8": 0.25,
        "9": 0.85
      }
    },
    {
      "id": 2,
      "name": "Formant Shifter",
      "type": 81,
      "x": 280.0,
      "y": 80.0,
      "params": {
        "1": 0.0,
        "2": 4.0,
        "3": 0.35,
        "4": 0.8,
        "5": 0.75
      }
    },
    {
      "id": 3,
      "name": "Comb Resonator",
      "type": 63,
      "x": 500.0,
      "y": 80.0,
      "params": {
        "1": 440.0,
        "2": 0.4,
        "3": 0.5,
        "4": 0.5
      }
    },
    {
      "id": 4,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 720.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 35.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    }
  ]
})json"
    });

    // -------------------------------------------------------------------------
    // Espacialización & Psicoacústica 3D -> Spectral Resonance Tamer Cathedral
    // -------------------------------------------------------------------------
    catalog.push_back({
        "Spectral Resonance Tamer Cathedral",
        "Espacialización & Psicoacústica 3D",
        R"json({
  "schemaVersion": 1,
  "name": "Spectral Resonance Tamer Cathedral",
  "author": "N8Audio",
  "category": "Espacializaci\u00f3n & Psicoac\u00fastica 3D",
  "description": "Espacio ac\u00fastico mutante de catedral c\u00f3smica con supresi\u00f3n din\u00e1mica Soothe de resonancias estridentes e imagen est\u00e9reo 3D",
  "dryLevel": 0.2,
  "wetLevel": 0.9,
  "macros": [
    0.75,
    0.65,
    0.8,
    0.5,
    0.4,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Morphing Convolver",
      "type": 83,
      "x": 60.0,
      "y": 80.0,
      "params": {
        "1": 0.65,
        "2": 4.5,
        "3": 0.5,
        "4": 20.0,
        "5": 0.85,
        "6": 0.75
      }
    },
    {
      "id": 2,
      "name": "Resonance Tamer",
      "type": 82,
      "x": 280.0,
      "y": 80.0,
      "params": {
        "1": 0.75,
        "2": 0.8,
        "3": 5.0,
        "4": 8.0,
        "5": 60.0,
        "6": 100.0,
        "7": 12000.0,
        "8": 0.0,
        "9": 1.0
      }
    },
    {
      "id": 3,
      "name": "Stereo Imager",
      "type": 55,
      "x": 500.0,
      "y": 80.0,
      "params": {
        "1": 1.35,
        "2": 0.0,
        "3": 120.0,
        "4": 1.0
      }
    },
    {
      "id": 4,
      "name": "True Peak Limiter",
      "type": 39,
      "x": 720.0,
      "y": 80.0,
      "params": {
        "1": -0.5,
        "2": -0.1,
        "3": 35.0,
        "4": 2.0,
        "5": 1.0
      }
    }
  ],
  "connections": [
    {
      "id": 1,
      "srcNode": 1,
      "srcPin": 2,
      "destNode": 2,
      "destPin": 1
    },
    {
      "id": 2,
      "srcNode": 2,
      "srcPin": 2,
      "destNode": 3,
      "destPin": 1
    },
    {
      "id": 3,
      "srcNode": 3,
      "srcPin": 2,
      "destNode": 4,
      "destPin": 1
    }
  ]
})json"
    });

    return catalog;
}

} // namespace audio_graph


