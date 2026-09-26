#pragma once

#include <vector>
#include <string>

namespace audio_graph {

struct FactoryPresetEntry {
    std::string name;
    std::string category;
    std::string jsonContent;
};

/**
 * @brief Catálogo Maestro de 20 Categorías Temáticas y 70 Presets Modulares (Reglas 21, 22, 44 y 46).
 * Incluye todos los tipos de efectos DSP y contenedores del motor en diversas configuraciones
 * (serie, paralelo, fan-out, fan-in, lazos de feedback y racks de eventos).
 */
inline std::vector<FactoryPresetEntry> createFactoryPresetCatalog() {
    std::vector<FactoryPresetEntry> catalog;
    catalog.reserve(80);

    // =========================================================================
    // CATEGORÍA 1: Modulación Psicoacústica
    // =========================================================================
    catalog.push_back({
        "Vortex Dimension Spread",
        "Modulación Psicoacústica",
        R"json({
  "schemaVersion": 1,
  "name": "Vortex Dimension Spread",
  "author": "N8Audio",
  "category": "Modulación Psicoacústica",
  "description": "Paneo orbital 3D con chorus multifásico y barrido de fase en 6 etapas",
  "dryLevel": 0.5,
  "wetLevel": 0.9,
  "macros": [0.6, 0.5, 0.7, 0.4, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Chorus 4-Voice", "type": 17, "x": 40.0, "y": 60.0, "params": { "1": 1.5, "2": 0.7, "3": 0.3, "4": 4.0, "5": 0.8 } },
    { "id": 2, "name": "Phaser 6-Stage", "type": 16, "x": 260.0, "y": 60.0, "params": { "1": 0.4, "2": 0.8, "3": 0.6, "4": 1200.0, "5": 0.85 } },
    { "id": 3, "name": "Spatial Panner 3D", "type": 26, "x": 480.0, "y": 60.0, "params": { "1": 45.0, "2": 15.0, "3": 1.8, "4": 1.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Bipolar Comb Weaver",
        "Modulación Psicoacústica",
        R"json({
  "schemaVersion": 1,
  "name": "Bipolar Comb Weaver",
  "author": "N8Audio",
  "category": "Modulación Psicoacústica",
  "description": "Modulación en peine con inversión de polaridad, corrimiento espectral y apertura Mid/Side",
  "dryLevel": 0.6,
  "wetLevel": 0.85,
  "macros": [0.5, 0.7, 0.4, 0.6, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Bipolar Flanger", "type": 18, "x": 40.0, "y": 60.0, "params": { "1": 0.35, "2": 0.85, "3": 2.2, "4": -0.85, "5": 0.9 } },
    { "id": 2, "name": "M/S Encoder", "type": 24, "x": 240.0, "y": 60.0, "params": { "1": 1.0 } },
    { "id": 3, "name": "Freq Shifter", "type": 20, "x": 440.0, "y": 60.0, "params": { "1": 3.0, "2": 0.25, "3": 0.75 } },
    { "id": 4, "name": "M/S Decoder", "type": 25, "x": 640.0, "y": 60.0, "params": { "1": 1.6, "2": 100.0, "3": 0.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 2: Experimental & Avant-Garde
    // =========================================================================
    catalog.push_back({
        "Alien Ring Transmission",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Alien Ring Transmission",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Modulación en anillo en 4 cuadrantes, transposición a la 19na y pliegue de onda no lineal",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [0.8, 0.6, 0.5, 0.7, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Ring Modulator", "type": 19, "x": 40.0, "y": 60.0, "params": { "1": 320.0, "2": 0.0, "3": 0.15, "4": 0.85 } },
    { "id": 2, "name": "Pitch Shift +19", "type": 11, "x": 250.0, "y": 60.0, "params": { "1": 19.0, "2": 0.0, "3": 40.0, "4": 0.7 } },
    { "id": 3, "name": "Wavefold Distortion", "type": 8, "x": 460.0, "y": 60.0, "params": { "1": 4.5, "2": 2.0, "3": 4500.0, "4": 0.8 } },
    { "id": 4, "name": "Space Delay", "type": 28, "x": 670.0, "y": 60.0, "params": { "1": 300.0, "2": 450.0, "3": 0.55, "4": 5000.0, "5": 1.0, "6": 0.5 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Chaotic Modal Resonator",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Chaotic Modal Resonator",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Banco resonador armónico desfasado por frecuencia negativa y degradación analógica",
  "dryLevel": 0.5,
  "wetLevel": 0.85,
  "macros": [0.7, 0.4, 0.6, 0.8, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Resonator Bank", "type": 13, "x": 40.0, "y": 60.0, "params": { "1": 175.0, "2": 2.0, "3": 1.2, "4": 0.6, "5": 0.9 } },
    { "id": 2, "name": "Freq Shifter -85Hz", "type": 20, "x": 260.0, "y": 60.0, "params": { "1": -85.0, "2": 0.35, "3": 0.8 } },
    { "id": 3, "name": "Tape Wow & Flutter", "type": 21, "x": 480.0, "y": 60.0, "params": { "1": 3.0, "2": 0.0, "3": 0.75, "4": 0.65, "5": 0.85 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 3: Glitch & Deconstrucción Rítmica
    // =========================================================================
    catalog.push_back({
        "Quantum Beat Slicer",
        "Glitch & Deconstrucción Rítmica",
        R"json({
  "schemaVersion": 1,
  "name": "Quantum Beat Slicer",
  "author": "N8Audio",
  "category": "Glitch & Deconstrucción Rítmica",
  "description": "Fragmentación rítmica en semicorcheas con repetición e inversión estocástica",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [0.9, 0.7, 0.6, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Glitch Slicer 1/16", "type": 14, "x": 40.0, "y": 60.0, "params": { "1": 2.0, "2": 0.65, "3": 0.45, "4": 0.3, "5": 0.9 } },
    { "id": 2, "name": "Parametric EQ", "type": 27, "x": 250.0, "y": 60.0, "params": { "1": 100.0, "2": -3.0, "3": 2500.0, "4": 2.0, "5": 3.0, "6": 8000.0, "7": 1.0 } },
    { "id": 3, "name": "Ping-Pong Delay", "type": 28, "x": 460.0, "y": 60.0, "params": { "1": 180.0, "2": 270.0, "3": 0.4, "4": 6500.0, "5": 1.0, "6": 0.55 } },
    { "id": 4, "name": "Compressor VCA", "type": 9, "x": 670.0, "y": 60.0, "params": { "1": -16.0, "2": 4.0, "3": 5.0, "4": 80.0, "5": 3.0, "6": 1.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Stutter Granular Freeze",
        "Glitch & Deconstrucción Rítmica",
        R"json({
  "schemaVersion": 1,
  "name": "Stutter Granular Freeze",
  "author": "N8Audio",
  "category": "Glitch & Deconstrucción Rítmica",
  "description": "Nube granular densa segmentada rítmicamente mediante compuerta estroboscópica y reverb",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [0.8, 0.6, 0.7, 0.4, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Granular Cloud", "type": 7, "x": 40.0, "y": 60.0, "params": { "1": 45.0, "2": 40.0, "3": 25.0, "4": 0.0, "5": 0.5, "6": 0.8, "7": 0.9 } },
    { "id": 2, "name": "Glitch Gate", "type": 14, "x": 260.0, "y": 60.0, "params": { "1": 3.0, "2": 0.35, "3": 0.2, "4": 0.1, "5": 0.95 } },
    { "id": 3, "name": "FDN Reverb", "type": 6, "x": 480.0, "y": 60.0, "params": { "1": 0.8, "2": 3.0, "3": 6000.0, "4": 15.0, "5": 0.45 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 4: Espacios Infinitos & Shimmer
    // =========================================================================
    catalog.push_back({
        "Celestial Shimmer Halo",
        "Espacios Infinitos & Shimmer",
        R"json({
  "schemaVersion": 1,
  "name": "Celestial Shimmer Halo",
  "author": "N8Audio",
  "category": "Espacios Infinitos & Shimmer",
  "description": "Reverberación FDN de 8 segundos con halo de octava superior y ensanchamiento Mid/Side",
  "dryLevel": 0.65,
  "wetLevel": 0.9,
  "macros": [0.8, 0.8, 0.6, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "FDN Reverb 8s", "type": 6, "x": 40.0, "y": 60.0, "params": { "1": 0.9, "2": 8.0, "3": 6500.0, "4": 30.0, "5": 0.75 } },
    { "id": 2, "name": "Pitch Shifter +12", "type": 11, "x": 250.0, "y": 60.0, "params": { "1": 12.0, "2": 0.0, "3": 45.0, "4": 0.8 } },
    { "id": 3, "name": "Ping-Pong Delay", "type": 28, "x": 460.0, "y": 60.0, "params": { "1": 350.0, "2": 525.0, "3": 0.5, "4": 8000.0, "5": 1.0, "6": 0.45 } },
    { "id": 4, "name": "M/S Stereo Spread", "type": 25, "x": 670.0, "y": 60.0, "params": { "1": 1.7, "2": 120.0, "3": 0.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Abyssal Catacomb Drone",
        "Espacios Infinitos & Shimmer",
        R"json({
  "schemaVersion": 1,
  "name": "Abyssal Catacomb Drone",
  "author": "N8Audio",
  "category": "Espacios Infinitos & Shimmer",
  "description": "Espacio subterráneo masivo con subarmónicos descendentes comprimidos por OTT multibanda",
  "dryLevel": 0.5,
  "wetLevel": 0.9,
  "macros": [0.7, 0.9, 0.4, 0.8, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Pitch Shifter -12", "type": 11, "x": 40.0, "y": 60.0, "params": { "1": -12.0, "2": 0.0, "3": 60.0, "4": 0.75 } },
    { "id": 2, "name": "FDN Deep Space", "type": 6, "x": 260.0, "y": 60.0, "params": { "1": 1.0, "2": 12.0, "3": 3500.0, "4": 50.0, "5": 0.85 } },
    { "id": 3, "name": "Multiband OTT", "type": 15, "x": 480.0, "y": 60.0, "params": { "1": 160.0, "2": 2800.0, "3": 1.2, "4": 0.8, "5": 1.0, "6": 0.9 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 5: Espectral & Drones Congelados
    // =========================================================================
    catalog.push_back({
        "Sub-Zero Spectral Drone",
        "Espectral & Drones Congelados",
        R"json({
  "schemaVersion": 1,
  "name": "Sub-Zero Spectral Drone",
  "author": "N8Audio",
  "category": "Espectral & Drones Congelados",
  "description": "Congelamiento instantáneo FFT transformado en drone armónico con difusión y reverb",
  "dryLevel": 0.3,
  "wetLevel": 1.0,
  "macros": [0.9, 0.7, 0.8, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Spectral Freeze", "type": 12, "x": 40.0, "y": 60.0, "params": { "1": 1.0, "2": 0.85, "3": 5.0, "4": 0.95 } },
    { "id": 2, "name": "Resonator Harmonic", "type": 13, "x": 250.0, "y": 60.0, "params": { "1": 220.0, "2": 2.0, "3": 1.8, "4": 0.5, "5": 0.8 } },
    { "id": 3, "name": "Chorus Ensemble", "type": 17, "x": 460.0, "y": 60.0, "params": { "1": 0.8, "2": 0.6, "3": 0.2, "4": 4.0, "5": 0.75 } },
    { "id": 4, "name": "FDN Reverb", "type": 6, "x": 670.0, "y": 60.0, "params": { "1": 0.85, "2": 4.5, "3": 5500.0, "4": 20.0, "5": 0.5 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Spectral Smear Mirage",
        "Espectral & Drones Congelados",
        R"json({
  "schemaVersion": 1,
  "name": "Spectral Smear Mirage",
  "author": "N8Audio",
  "category": "Espectral & Drones Congelados",
  "description": "Desvanecimiento tímbrico espectral con calidez analógica de cinta y barrido de fase",
  "dryLevel": 0.5,
  "wetLevel": 0.85,
  "macros": [0.6, 0.7, 0.5, 0.8, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Spectral Gate/Tilt", "type": 29, "x": 40.0, "y": 60.0, "params": { "1": -45.0, "2": 2.5, "3": 0.85 } },
    { "id": 2, "name": "Tape Saturation", "type": 21, "x": 260.0, "y": 60.0, "params": { "1": 3.2, "2": 1.0, "3": 0.45, "4": 0.8, "5": 0.9 } },
    { "id": 3, "name": "Phaser Sweep", "type": 16, "x": 480.0, "y": 60.0, "params": { "1": 0.25, "2": 0.8, "3": 0.5, "4": 900.0, "5": 0.7 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 6: Distorsión Industrial & Caos
    // =========================================================================
    catalog.push_back({
        "Cybernetic Wavefolder",
        "Distorsión Industrial & Caos",
        R"json({
  "schemaVersion": 1,
  "name": "Cybernetic Wavefolder",
  "author": "N8Audio",
  "category": "Distorsión Industrial & Caos",
  "description": "Plegado de onda cibernético no lineal con ecualización de medios y compresión",
  "dryLevel": 0.25,
  "wetLevel": 0.95,
  "macros": [0.9, 0.8, 0.6, 0.7, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Distortion Wavefold", "type": 8, "x": 40.0, "y": 60.0, "params": { "1": 7.5, "2": 2.0, "3": 7000.0, "4": 0.9 } },
    { "id": 2, "name": "Parametric EQ", "type": 27, "x": 250.0, "y": 60.0, "params": { "1": 120.0, "2": -4.0, "3": 1800.0, "4": 2.5, "5": 5.0, "6": 6500.0, "7": -2.0 } },
    { "id": 3, "name": "Compressor", "type": 9, "x": 460.0, "y": 60.0, "params": { "1": -18.0, "2": 6.0, "3": 3.0, "4": 60.0, "5": 4.0, "6": 1.0 } },
    { "id": 4, "name": "Tape Warmth", "type": 21, "x": 670.0, "y": 60.0, "params": { "1": 2.5, "2": 1.0, "3": 0.2, "4": 0.9, "5": 0.85 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Bit-Crushed Industrial Grime",
        "Distorsión Industrial & Caos",
        R"json({
  "schemaVersion": 1,
  "name": "Bit-Crushed Industrial Grime",
  "author": "N8Audio",
  "category": "Distorsión Industrial & Caos",
  "description": "Trituración digital de bits, modulación cuadrada de carrier y compresión OTT salvaje",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [0.85, 0.7, 0.9, 0.6, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Bit Crusher", "type": 8, "x": 40.0, "y": 60.0, "params": { "1": 12.0, "2": 4.0, "3": 3800.0, "4": 0.85 } },
    { "id": 2, "name": "Square Ring Mod", "type": 19, "x": 260.0, "y": 60.0, "params": { "1": 180.0, "2": 3.0, "3": 0.1, "4": 0.65 } },
    { "id": 3, "name": "Multiband OTT", "type": 15, "x": 480.0, "y": 60.0, "params": { "1": 220.0, "2": 2600.0, "3": 1.4, "4": 1.2, "5": 1.1, "6": 0.9 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 7: Cintas Analógicas & Lo-Fi
    // =========================================================================
    catalog.push_back({
        "Vintage Worn Cassette",
        "Cintas Analógicas & Lo-Fi",
        R"json({
  "schemaVersion": 1,
  "name": "Vintage Worn Cassette",
  "author": "N8Audio",
  "category": "Cintas Analógicas & Lo-Fi",
  "description": "Emulación de cassette vintage con lloro mecánico, corte de agudos y calidez magnética",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [0.7, 0.5, 0.8, 0.6, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Tape Cassette 7.5", "type": 21, "x": 40.0, "y": 60.0, "params": { "1": 4.0, "2": 0.0, "3": 0.65, "4": 0.8, "5": 0.95 } },
    { "id": 2, "name": "Bandpass EQ", "type": 27, "x": 250.0, "y": 60.0, "params": { "1": 150.0, "2": -4.0, "3": 1400.0, "4": 1.2, "5": 1.5, "6": 5500.0, "7": -7.0 } },
    { "id": 3, "name": "Subtle Chorus", "type": 17, "x": 460.0, "y": 60.0, "params": { "1": 0.6, "2": 0.35, "3": 0.1, "4": 2.0, "5": 0.4 } },
    { "id": 4, "name": "Glue Compressor", "type": 9, "x": 670.0, "y": 60.0, "params": { "1": -14.0, "2": 3.0, "3": 12.0, "4": 120.0, "5": 2.0, "6": 1.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Melted Vinyl Nostalgia",
        "Cintas Analógicas & Lo-Fi",
        R"json({
  "schemaVersion": 1,
  "name": "Melted Vinyl Nostalgia",
  "author": "N8Audio",
  "category": "Cintas Analógicas & Lo-Fi",
  "description": "Disco de vinilo desgastado con micro-detune continuo, saturación y eco slapback",
  "dryLevel": 0.4,
  "wetLevel": 0.85,
  "macros": [0.65, 0.7, 0.4, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Micro Detune", "type": 11, "x": 40.0, "y": 60.0, "params": { "1": 0.0, "2": 14.0, "3": 45.0, "4": 0.55 } },
    { "id": 2, "name": "Vinyl Saturation", "type": 21, "x": 260.0, "y": 60.0, "params": { "1": 2.8, "2": 1.0, "3": 0.5, "4": 0.7, "5": 0.85 } },
    { "id": 3, "name": "Tape Slapback", "type": 28, "x": 480.0, "y": 60.0, "params": { "1": 110.0, "2": 145.0, "3": 0.3, "4": 4500.0, "5": 0.0, "6": 0.5 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 8: Racks Autónomos de Eventos
    // =========================================================================
    catalog.push_back({
        "Particulate Event Cloud CUT",
        "Racks Autónomos de Eventos",
        R"json({
  "schemaVersion": 1,
  "name": "Particulate Event Cloud CUT",
  "author": "N8Audio",
  "category": "Racks Autónomos de Eventos",
  "description": "Partículas reactivas que se apagan instantáneamente (CUT) al liberar la tecla",
  "dryLevel": 0.5,
  "wetLevel": 0.9,
  "macros": [0.8, 0.7, 0.6, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Event Rack CUT", "type": 23, "x": 40.0, "y": 60.0, "params": { "1": 0.0, "2": 0.85, "3": 1.0, "4": 140.0, "5": 1.0, "6": 0.9 } },
    { "id": 2, "name": "Stereo Delay", "type": 28, "x": 260.0, "y": 60.0, "params": { "1": 220.0, "2": 330.0, "3": 0.45, "4": 7000.0, "5": 1.0, "6": 0.5 } },
    { "id": 3, "name": "FDN Reverb", "type": 6, "x": 480.0, "y": 60.0, "params": { "1": 0.75, "2": 2.5, "3": 6000.0, "4": 15.0, "5": 0.4 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Ghost Particle Hold FREEZE",
        "Racks Autónomos de Eventos",
        R"json({
  "schemaVersion": 1,
  "name": "Ghost Particle Hold FREEZE",
  "author": "N8Audio",
  "category": "Racks Autónomos de Eventos",
  "description": "Partículas congeladas que continúan en bucle flotando independientemente en 3D",
  "dryLevel": 0.6,
  "wetLevel": 0.85,
  "macros": [0.75, 0.5, 0.8, 0.6, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Event Rack FREEZE", "type": 23, "x": 40.0, "y": 60.0, "params": { "1": 0.0, "2": 0.6, "3": 1.0, "4": 280.0, "5": 0.0, "6": 0.85 } },
    { "id": 2, "name": "Pitch Shift +7", "type": 11, "x": 260.0, "y": 60.0, "params": { "1": 7.0, "2": 0.0, "3": 35.0, "4": 0.6 } },
    { "id": 3, "name": "Spatial Panner", "type": 26, "x": 480.0, "y": 60.0, "params": { "1": -60.0, "2": 20.0, "3": 2.5, "4": 0.95 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 9: Lazos de Realimentación Inestable
    // =========================================================================
    catalog.push_back({
        "Self-Oscillating Damped Loop",
        "Lazos de Realimentación Inestable",
        R"json({
  "schemaVersion": 1,
  "name": "Self-Oscillating Damped Loop",
  "author": "N8Audio",
  "category": "Lazos de Realimentación Inestable",
  "description": "Lazo con realimentación super-unitaria 1.15x contenida con saturación sigmoidal y phaser",
  "dryLevel": 0.5,
  "wetLevel": 0.9,
  "macros": [0.85, 0.6, 0.7, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Feedback Container", "type": 22, "x": 40.0, "y": 60.0, "params": { "1": 1.15, "2": 180.0, "3": 4000.0, "4": 1.0, "5": 0.85 } },
    { "id": 2, "name": "Phaser Sweep", "type": 16, "x": 260.0, "y": 60.0, "params": { "1": 0.3, "2": 0.75, "3": 0.45, "4": 1500.0, "5": 0.7 } },
    { "id": 3, "name": "Safety EQ", "type": 27, "x": 480.0, "y": 60.0, "params": { "1": 90.0, "2": -2.0, "3": 2200.0, "4": 1.5, "5": 0.0, "6": 7500.0, "7": -3.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Shifting Feedback Vortex",
        "Lazos de Realimentación Inestable",
        R"json({
  "schemaVersion": 1,
  "name": "Shifting Feedback Vortex",
  "author": "N8Audio",
  "category": "Lazos de Realimentación Inestable",
  "description": "Cascada infinita donde cada repetición se eleva 5 Hz en espiral dentro de una reverb FDN",
  "dryLevel": 0.6,
  "wetLevel": 0.85,
  "macros": [0.7, 0.75, 0.6, 0.8, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Feedback Loop", "type": 22, "x": 40.0, "y": 60.0, "params": { "1": 0.92, "2": 240.0, "3": 6000.0, "4": 1.0, "5": 0.8 } },
    { "id": 2, "name": "Freq Shifter +5Hz", "type": 20, "x": 260.0, "y": 60.0, "params": { "1": 5.0, "2": 0.2, "3": 0.8 } },
    { "id": 3, "name": "FDN Reverb", "type": 6, "x": 480.0, "y": 60.0, "params": { "1": 0.8, "2": 3.5, "3": 5000.0, "4": 25.0, "5": 0.5 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 10: Micro-Muestreo Granular
    // =========================================================================
    catalog.push_back({
        "Stochastic Grain Swarm",
        "Micro-Muestreo Granular",
        R"json({
  "schemaVersion": 1,
  "name": "Stochastic Grain Swarm",
  "author": "N8Audio",
  "category": "Micro-Muestreo Granular",
  "description": "Enjambre de 60 granos/s con fluctuación temporal estocástica y paneo orbital 3D",
  "dryLevel": 0.45,
  "wetLevel": 0.9,
  "macros": [0.8, 0.6, 0.7, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Granular Engine", "type": 7, "x": 40.0, "y": 60.0, "params": { "1": 60.0, "2": 45.0, "3": 30.0, "4": 0.0, "5": 0.7, "6": 0.9, "7": 0.9 } },
    { "id": 2, "name": "Spatial Panner", "type": 26, "x": 260.0, "y": 60.0, "params": { "1": 30.0, "2": 10.0, "3": 1.5, "4": 1.0 } },
    { "id": 3, "name": "FDN Reverb", "type": 6, "x": 480.0, "y": 60.0, "params": { "1": 0.8, "2": 3.2, "3": 6500.0, "4": 20.0, "5": 0.45 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Granular Pitch Morph",
        "Micro-Muestreo Granular",
        R"json({
  "schemaVersion": 1,
  "name": "Granular Pitch Morph",
  "author": "N8Audio",
  "category": "Micro-Muestreo Granular",
  "description": "Granulación con dispersión tonal polifónica, chorus estéreo y retardo ping-pong",
  "dryLevel": 0.5,
  "wetLevel": 0.85,
  "macros": [0.75, 0.8, 0.5, 0.6, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Pitch Granulator", "type": 7, "x": 40.0, "y": 60.0, "params": { "1": 35.0, "2": 50.0, "3": 20.0, "4": 12.0, "5": 2.0, "6": 0.7, "7": 0.85 } },
    { "id": 2, "name": "Chorus Ensemble", "type": 17, "x": 260.0, "y": 60.0, "params": { "1": 1.1, "2": 0.6, "3": 0.25, "4": 3.0, "5": 0.7 } },
    { "id": 3, "name": "Ping-Pong Delay", "type": 28, "x": 480.0, "y": 60.0, "params": { "1": 250.0, "2": 375.0, "3": 0.45, "4": 7500.0, "5": 1.0, "6": 0.5 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 11: Resonadores Modales & Afinación
    // =========================================================================
    catalog.push_back({
        "Metallic Chime Cathedral",
        "Resonadores Modales & Afinación",
        R"json({
  "schemaVersion": 1,
  "name": "Metallic Chime Cathedral",
  "author": "N8Audio",
  "category": "Resonadores Modales & Afinación",
  "description": "Campanas metálicas afinadas modalmente en quintas con reverberación de catedral",
  "dryLevel": 0.45,
  "wetLevel": 0.9,
  "macros": [0.7, 0.9, 0.5, 0.6, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Modal Chimes", "type": 13, "x": 40.0, "y": 60.0, "params": { "1": 330.0, "2": 1.0, "3": 2.2, "4": 0.7, "5": 0.9 } },
    { "id": 2, "name": "Cathedral FDN", "type": 6, "x": 260.0, "y": 60.0, "params": { "1": 0.9, "2": 6.5, "3": 7000.0, "4": 25.0, "5": 0.6 } },
    { "id": 3, "name": "M/S Width 1.4x", "type": 25, "x": 480.0, "y": 60.0, "params": { "1": 1.4, "2": 100.0, "3": 0.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Sub-Harmonic Earth Shaker",
        "Resonadores Modales & Afinación",
        R"json({
  "schemaVersion": 1,
  "name": "Sub-Harmonic Earth Shaker",
  "author": "N8Audio",
  "category": "Resonadores Modales & Afinación",
  "description": "Generación sísmica a -24 semitonos con saturación de válvulas y graves mono centrados",
  "dryLevel": 0.6,
  "wetLevel": 0.85,
  "macros": [0.85, 0.6, 0.7, 0.9, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Sub Pitch -24", "type": 11, "x": 40.0, "y": 60.0, "params": { "1": -24.0, "2": 0.0, "3": 65.0, "4": 0.8 } },
    { "id": 2, "name": "Sub Resonator", "type": 13, "x": 250.0, "y": 60.0, "params": { "1": 55.0, "2": 0.0, "3": 1.5, "4": 0.2, "5": 0.75 } },
    { "id": 3, "name": "Tube Distortion", "type": 8, "x": 460.0, "y": 60.0, "params": { "1": 3.5, "2": 3.0, "3": 3000.0, "4": 0.7 } },
    { "id": 4, "name": "Mono Bass Maker", "type": 25, "x": 670.0, "y": 60.0, "params": { "1": 1.0, "2": 160.0, "3": 0.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 12: Paneo 3D & Espacialización
    // =========================================================================
    catalog.push_back({
        "Binaural Orbiting Echo",
        "Paneo 3D & Espacialización",
        R"json({
  "schemaVersion": 1,
  "name": "Binaural Orbiting Echo",
  "author": "N8Audio",
  "category": "Paneo 3D & Espacialización",
  "description": "Ecos orbitando binauralmente con modelo Woodworth ITD/ILD y decodificación Mid/Side",
  "dryLevel": 0.5,
  "wetLevel": 0.9,
  "macros": [0.65, 0.5, 0.7, 0.8, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Spatial Panner", "type": 26, "x": 40.0, "y": 60.0, "params": { "1": 65.0, "2": 25.0, "3": 2.0, "4": 1.0 } },
    { "id": 2, "name": "Ping-Pong Echo", "type": 28, "x": 260.0, "y": 60.0, "params": { "1": 280.0, "2": 420.0, "3": 0.45, "4": 6500.0, "5": 1.0, "6": 0.6 } },
    { "id": 3, "name": "M/S Decoder", "type": 25, "x": 480.0, "y": 60.0, "params": { "1": 1.5, "2": 90.0, "3": 0.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Psychoacoustic Sphere Dive",
        "Paneo 3D & Espacialización",
        R"json({
  "schemaVersion": 1,
  "name": "Psychoacoustic Sphere Dive",
  "author": "N8Audio",
  "category": "Paneo 3D & Espacialización",
  "description": "Inmersión esférica tridimensional con barrido de fase y difusión espacial en reverb",
  "dryLevel": 0.55,
  "wetLevel": 0.85,
  "macros": [0.7, 0.6, 0.8, 0.4, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "3D Panner Sphere", "type": 26, "x": 40.0, "y": 60.0, "params": { "1": -45.0, "2": -30.0, "3": 3.0, "4": 0.9 } },
    { "id": 2, "name": "Phaser Sweep", "type": 16, "x": 260.0, "y": 60.0, "params": { "1": 0.2, "2": 0.7, "3": 0.5, "4": 1100.0, "5": 0.65 } },
    { "id": 3, "name": "FDN Reverb", "type": 6, "x": 480.0, "y": 60.0, "params": { "1": 0.85, "2": 4.0, "3": 5000.0, "4": 20.0, "5": 0.5 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 13: Dinámica Agresiva & OTT
    // =========================================================================
    catalog.push_back({
        "Master Wall Hypersonic OTT",
        "Dinámica Agresiva & OTT",
        R"json({
  "schemaVersion": 1,
  "name": "Master Wall Hypersonic OTT",
  "author": "N8Audio",
  "category": "Dinámica Agresiva & OTT",
  "description": "Pared sonora con compresión ascendente/descendente multibanda, saturación y ecualización",
  "dryLevel": 0.2,
  "wetLevel": 1.0,
  "macros": [0.95, 0.8, 0.7, 0.6, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Parametric EQ", "type": 27, "x": 40.0, "y": 60.0, "params": { "1": 80.0, "2": 2.0, "3": 3000.0, "4": 1.5, "5": 1.0, "6": 10000.0, "7": 2.0 } },
    { "id": 2, "name": "Multiband OTT", "type": 15, "x": 250.0, "y": 60.0, "params": { "1": 180.0, "2": 3200.0, "3": 1.3, "4": 1.1, "5": 1.2, "6": 0.95 } },
    { "id": 3, "name": "Tape Head Bump", "type": 21, "x": 460.0, "y": 60.0, "params": { "1": 2.0, "2": 2.0, "3": 0.1, "4": 0.9, "5": 0.9 } },
    { "id": 4, "name": "Master Compressor", "type": 9, "x": 670.0, "y": 60.0, "params": { "1": -12.0, "2": 3.5, "3": 25.0, "4": 150.0, "5": 2.0, "6": 1.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Punchy Transient Smasher",
        "Dinámica Agresiva & OTT",
        R"json({
  "schemaVersion": 1,
  "name": "Punchy Transient Smasher",
  "author": "N8Audio",
  "category": "Dinámica Agresiva & OTT",
  "description": "Pegada masiva para percusión y sintes con soft clipping y maximización OTT",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [0.85, 0.75, 0.6, 0.8, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "VCA Compressor", "type": 9, "x": 40.0, "y": 60.0, "params": { "1": -20.0, "2": 5.0, "3": 8.0, "4": 60.0, "5": 4.5, "6": 1.0 } },
    { "id": 2, "name": "Soft Clip Distortion", "type": 8, "x": 260.0, "y": 60.0, "params": { "1": 3.0, "2": 0.0, "3": 8500.0, "4": 0.7 } },
    { "id": 3, "name": "Multiband OTT", "type": 15, "x": 480.0, "y": 60.0, "params": { "1": 150.0, "2": 2500.0, "3": 1.2, "4": 1.0, "5": 1.0, "6": 0.85 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 14: Sintetizador de Voces & Formantes
    // =========================================================================
    catalog.push_back({
        "Vocal Talkbox Resonator",
        "Sintetizador de Voces & Formantes",
        R"json({
  "schemaVersion": 1,
  "name": "Vocal Talkbox Resonator",
  "author": "N8Audio",
  "category": "Sintetizador de Voces & Formantes",
  "description": "Emulación de tracto vocal talkbox mediante formantes armónicos y saturación",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [0.75, 0.6, 0.5, 0.7, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Formant Bank", "type": 13, "x": 40.0, "y": 60.0, "params": { "1": 440.0, "2": 4.0, "3": 0.8, "4": 0.45, "5": 0.9 } },
    { "id": 2, "name": "Tube Drive", "type": 8, "x": 260.0, "y": 60.0, "params": { "1": 2.5, "2": 3.0, "3": 5000.0, "4": 0.6 } },
    { "id": 3, "name": "Warm Filter", "type": 27, "x": 480.0, "y": 60.0, "params": { "1": 350.0, "2": 1.0, "3": 2800.0, "4": 1.8, "5": 2.5, "6": 7000.0, "7": -4.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Robotic Formant Modulator",
        "Sintetizador de Voces & Formantes",
        R"json({
  "schemaVersion": 1,
  "name": "Robotic Formant Modulator",
  "author": "N8Audio",
  "category": "Sintetizador de Voces & Formantes",
  "description": "Timbres de robot extraterrestre mediante modulación carrier y garganta resonante",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [0.8, 0.65, 0.7, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Carrier Ring Mod", "type": 19, "x": 40.0, "y": 60.0, "params": { "1": 150.0, "2": 1.0, "3": 0.2, "4": 0.75 } },
    { "id": 2, "name": "Formant Throat", "type": 13, "x": 260.0, "y": 60.0, "params": { "1": 520.0, "2": 3.0, "3": 1.0, "4": 0.6, "5": 0.85 } },
    { "id": 3, "name": "Chorus Width", "type": 17, "x": 480.0, "y": 60.0, "params": { "1": 1.2, "2": 0.5, "3": 0.2, "4": 3.0, "5": 0.65 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 15: Ecos Cinematográficos & Paisajes
    // =========================================================================
    catalog.push_back({
        "Solaris Deep Space Tape",
        "Ecos Cinematográficos & Paisajes",
        R"json({
  "schemaVersion": 1,
  "name": "Solaris Deep Space Tape",
  "author": "N8Audio",
  "category": "Ecos Cinematográficos & Paisajes",
  "description": "Retardo de cinta cinematográfica de ciencia ficción con ecos cálidos y reverberación profunda",
  "dryLevel": 0.6,
  "wetLevel": 0.85,
  "macros": [0.75, 0.85, 0.6, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Tape Warmth", "type": 21, "x": 40.0, "y": 60.0, "params": { "1": 3.0, "2": 1.0, "3": 0.4, "4": 0.85, "5": 0.9 } },
    { "id": 2, "name": "Deep Space Delay", "type": 28, "x": 260.0, "y": 60.0, "params": { "1": 450.0, "2": 675.0, "3": 0.55, "4": 4500.0, "5": 1.0, "6": 0.6 } },
    { "id": 3, "name": "FDN Void Reverb", "type": 6, "x": 480.0, "y": 60.0, "params": { "1": 0.9, "2": 6.0, "3": 4800.0, "4": 40.0, "5": 0.55 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Glacial Canyon Wall",
        "Ecos Cinematográficos & Paisajes",
        R"json({
  "schemaVersion": 1,
  "name": "Glacial Canyon Wall",
  "author": "N8Audio",
  "category": "Ecos Cinematográficos & Paisajes",
  "description": "Ecos glaciares con reverberación FDN y congelamiento espectral",
  "dryLevel": 0.5,
  "wetLevel": 0.9,
  "macros": [0.8, 0.7, 0.9, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Canyon Delay", "type": 28, "x": 40.0, "y": 60.0, "params": { "1": 500.0, "2": 750.0, "3": 0.6, "4": 7000.0, "5": 1.0, "6": 0.7 } },
    { "id": 2, "name": "Bipolar Flanger", "type": 18, "x": 250.0, "y": 60.0, "params": { "1": 0.15, "2": 0.7, "3": 4.0, "4": -0.7, "5": 0.5 } },
    { "id": 3, "name": "Glacial FDN", "type": 6, "x": 460.0, "y": 60.0, "params": { "1": 0.9, "2": 7.0, "3": 6000.0, "4": 30.0, "5": 0.6 } },
    { "id": 4, "name": "Spectral Smear", "type": 12, "x": 670.0, "y": 60.0, "params": { "1": 0.0, "2": 0.6, "3": 3.0, "4": 0.65 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 16: Subarmónicos & Graves Psicoacústicos
    // =========================================================================
    catalog.push_back({
        "Sub-Octave Growl Engine",
        "Subarmónicos & Graves",
        R"json({
  "schemaVersion": 1,
  "name": "Sub-Octave Growl Engine",
  "author": "N8Audio",
  "category": "Subarmónicos & Graves",
  "description": "Octava inferior con saturación analógica de válvulas y graves mono para subwoofer",
  "dryLevel": 0.65,
  "wetLevel": 0.85,
  "macros": [0.8, 0.7, 0.6, 0.9, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Pitch Shifter -12", "type": 11, "x": 40.0, "y": 60.0, "params": { "1": -12.0, "2": 0.0, "3": 55.0, "4": 0.85 } },
    { "id": 2, "name": "Tube Distortion", "type": 8, "x": 250.0, "y": 60.0, "params": { "1": 4.0, "2": 3.0, "3": 4000.0, "4": 0.75 } },
    { "id": 3, "name": "Punch Compressor", "type": 9, "x": 460.0, "y": 60.0, "params": { "1": -15.0, "2": 4.0, "3": 10.0, "4": 90.0, "5": 3.0, "6": 1.0 } },
    { "id": 4, "name": "Mono Bass Maker", "type": 25, "x": 670.0, "y": 60.0, "params": { "1": 1.0, "2": 150.0, "3": 0.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Sub-Bass Reinforcer & Saturator",
        "Subarmónicos & Graves",
        R"json({
  "schemaVersion": 1,
  "name": "Sub-Bass Reinforcer & Saturator",
  "author": "N8Audio",
  "category": "Subarmónicos & Graves",
  "description": "Refuerzo orgánico de subgraves con saturación de cinta a 15 ips sin embarrar la imagen estéreo",
  "dryLevel": 0.7,
  "wetLevel": 0.8,
  "macros": [0.75, 0.6, 0.8, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Low Shelf EQ", "type": 27, "x": 40.0, "y": 60.0, "params": { "1": 60.0, "2": 4.5, "3": 500.0, "4": 1.0, "5": -2.0, "6": 5000.0, "7": 0.0 } },
    { "id": 2, "name": "Tape 15 ips", "type": 21, "x": 260.0, "y": 60.0, "params": { "1": 2.5, "2": 1.0, "3": 0.15, "4": 0.9, "5": 0.85 } },
    { "id": 3, "name": "Mono Bass Decoder", "type": 25, "x": 480.0, "y": 60.0, "params": { "1": 1.1, "2": 130.0, "3": 0.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 17: Desfase Espectral & Transmutación
    // =========================================================================
    catalog.push_back({
        "Bode Frequency Shifter Delay",
        "Desfase Espectral",
        R"json({
  "schemaVersion": 1,
  "name": "Bode Frequency Shifter Delay",
  "author": "N8Audio",
  "category": "Desfase Espectral",
  "description": "Efecto clásico Bode con desplazamiento no armónico de +15 Hz y ecos desfasados",
  "dryLevel": 0.5,
  "wetLevel": 0.85,
  "macros": [0.65, 0.7, 0.6, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Freq Shifter +15Hz", "type": 20, "x": 40.0, "y": 60.0, "params": { "1": 15.0, "2": 0.3, "3": 0.85 } },
    { "id": 2, "name": "Ping-Pong Delay", "type": 28, "x": 260.0, "y": 60.0, "params": { "1": 260.0, "2": 390.0, "3": 0.5, "4": 6500.0, "5": 1.0, "6": 0.6 } },
    { "id": 3, "name": "Phaser Sweep", "type": 16, "x": 480.0, "y": 60.0, "params": { "1": 0.35, "2": 0.8, "3": 0.65, "4": 1400.0, "5": 0.75 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Barberpole Frequency Mirage",
        "Desfase Espectral",
        R"json({
  "schemaVersion": 1,
  "name": "Barberpole Frequency Mirage",
  "author": "N8Audio",
  "category": "Desfase Espectral",
  "description": "Sensación de caída tonal perpetua mediante desplazamiento negativo, flanger y chorus",
  "dryLevel": 0.55,
  "wetLevel": 0.85,
  "macros": [0.7, 0.6, 0.8, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Freq Shifter -40Hz", "type": 20, "x": 40.0, "y": 60.0, "params": { "1": -40.0, "2": 0.35, "3": 0.8 } },
    { "id": 2, "name": "Flanger Comb", "type": 18, "x": 260.0, "y": 60.0, "params": { "1": 0.2, "2": 0.75, "3": 3.0, "4": 0.6, "5": 0.65 } },
    { "id": 3, "name": "Chorus 4-Voice", "type": 17, "x": 480.0, "y": 60.0, "params": { "1": 1.0, "2": 0.55, "3": 0.2, "4": 4.0, "5": 0.7 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 18: Contenedores Anidados Multi-Capa
    // =========================================================================
    catalog.push_back({
        "Sub-Graph Micro-Universe",
        "Contenedores Anidados",
        R"json({
  "schemaVersion": 1,
  "name": "Sub-Graph Micro-Universe",
  "author": "N8Audio",
  "category": "Contenedores Anidados",
  "description": "Subgrafo encapsulado en un contenedor modular que alimenta el motor de reverberación y 3D",
  "dryLevel": 0.5,
  "wetLevel": 0.85,
  "macros": [0.75, 0.6, 0.7, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Sub-Graph Container", "type": 10, "x": 40.0, "y": 60.0, "params": { "1": 0.85, "2": 1.1 } },
    { "id": 2, "name": "FDN Reverb", "type": 6, "x": 260.0, "y": 60.0, "params": { "1": 0.8, "2": 3.5, "3": 6000.0, "4": 20.0, "5": 0.5 } },
    { "id": 3, "name": "Spatial Panner 3D", "type": 26, "x": 480.0, "y": 60.0, "params": { "1": 50.0, "2": 20.0, "3": 1.8, "4": 0.9 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Nested Event Matrix",
        "Contenedores Anidados",
        R"json({
  "schemaVersion": 1,
  "name": "Nested Event Matrix",
  "author": "N8Audio",
  "category": "Contenedores Anidados",
  "description": "Contenedor modular anidado en serie con rack autónomo de eventos y compresión OTT",
  "dryLevel": 0.45,
  "wetLevel": 0.9,
  "macros": [0.8, 0.7, 0.6, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Audio Container", "type": 10, "x": 40.0, "y": 60.0, "params": { "1": 0.9, "2": 1.0 } },
    { "id": 2, "name": "Event Rack", "type": 23, "x": 260.0, "y": 60.0, "params": { "1": 0.0, "2": 0.75, "3": 1.0, "4": 180.0, "5": 0.8, "6": 0.85 } },
    { "id": 3, "name": "Multiband OTT", "type": 15, "x": 480.0, "y": 60.0, "params": { "1": 200.0, "2": 2600.0, "3": 1.2, "4": 1.0, "5": 1.1, "6": 0.9 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 19: Secuenciación Sincronizada al Tempo
    // =========================================================================
    catalog.push_back({
        "Motorik Rhythmic Chopper",
        "Secuenciación Sincronizada",
        R"json({
  "schemaVersion": 1,
  "name": "Motorik Rhythmic Chopper",
  "author": "N8Audio",
  "category": "Secuenciación Sincronizada",
  "description": "Troceador rítmico krautrock en corcheas con filtro sintonizado y retardo estéreo",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [0.85, 0.6, 0.7, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Glitch Chopper 1/8", "type": 14, "x": 40.0, "y": 60.0, "params": { "1": 1.0, "2": 0.55, "3": 0.3, "4": 0.15, "5": 0.95 } },
    { "id": 2, "name": "Parametric Filter", "type": 27, "x": 260.0, "y": 60.0, "params": { "1": 120.0, "2": 0.0, "3": 2400.0, "4": 2.2, "5": 3.0, "6": 8000.0, "7": -1.0 } },
    { "id": 3, "name": "Stereo Delay", "type": 28, "x": 480.0, "y": 60.0, "params": { "1": 250.0, "2": 375.0, "3": 0.5, "4": 6500.0, "5": 1.0, "6": 0.55 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Polyrhythmic Dub Echoplex",
        "Secuenciación Sincronizada",
        R"json({
  "schemaVersion": 1,
  "name": "Polyrhythmic Dub Echoplex",
  "author": "N8Audio",
  "category": "Secuenciación Sincronizada",
  "description": "Retardos polirrítmicos cruzados con saturación analógica y cámara de reverberación",
  "dryLevel": 0.5,
  "wetLevel": 0.85,
  "macros": [0.75, 0.8, 0.65, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Polyrhythmic Delay", "type": 28, "x": 40.0, "y": 60.0, "params": { "1": 375.0, "2": 500.0, "3": 0.65, "4": 5000.0, "5": 1.0, "6": 0.75 } },
    { "id": 2, "name": "Tape Saturation", "type": 21, "x": 260.0, "y": 60.0, "params": { "1": 3.5, "2": 1.0, "3": 0.35, "4": 0.8, "5": 0.85 } },
    { "id": 3, "name": "Dub Chamber Reverb", "type": 6, "x": 480.0, "y": 60.0, "params": { "1": 0.85, "2": 4.5, "3": 4500.0, "4": 30.0, "5": 0.55 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // =========================================================================
    // CATEGORÍA 20: Caos Reactivo & Detección Acústica
    // =========================================================================
    catalog.push_back({
        "Transient Exploder Reactive Rig",
        "Caos Reactivo",
        R"json({
  "schemaVersion": 1,
  "name": "Transient Exploder Reactive Rig",
  "author": "N8Audio",
  "category": "Caos Reactivo",
  "description": "Ráfagas de distorsión y troceado detonadas orgánicamente por transitorios acústicos",
  "dryLevel": 0.45,
  "wetLevel": 0.9,
  "macros": [0.85, 0.7, 0.9, 0.6, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Drive Saturation", "type": 8, "x": 40.0, "y": 60.0, "params": { "1": 5.0, "2": 0.0, "3": 6000.0, "4": 0.8 } },
    { "id": 2, "name": "Reactive Slicer", "type": 14, "x": 260.0, "y": 60.0, "params": { "1": 2.0, "2": 0.7, "3": 0.4, "4": 0.25, "5": 0.85 } },
    { "id": 3, "name": "Explosive Reverb", "type": 6, "x": 480.0, "y": 60.0, "params": { "1": 0.9, "2": 5.0, "3": 5500.0, "4": 10.0, "5": 0.6 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    catalog.push_back({
        "Pitch-Tracking Harmonic Shifter",
        "Caos Reactivo",
        R"json({
  "schemaVersion": 1,
  "name": "Pitch-Tracking Harmonic Shifter",
  "author": "N8Audio",
  "category": "Caos Reactivo",
  "description": "Seguimiento de fundamental en tiempo real acoplado a resonadores modales y paneo 3D",
  "dryLevel": 0.5,
  "wetLevel": 0.85,
  "macros": [0.75, 0.8, 0.6, 0.7, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Pitch Shifter +7", "type": 11, "x": 40.0, "y": 60.0, "params": { "1": 7.0, "2": 0.0, "3": 35.0, "4": 0.7 } },
    { "id": 2, "name": "Harmonic Resonator", "type": 13, "x": 260.0, "y": 60.0, "params": { "1": 220.0, "2": 2.0, "3": 1.4, "4": 0.5, "5": 0.8 } },
    { "id": 3, "name": "Spatial Panner", "type": 26, "x": 480.0, "y": 60.0, "params": { "1": -40.0, "2": 15.0, "3": 2.2, "4": 0.95 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });



    // =========================================================================
    // NUEVOS PRESETS MODULARES AVANZADOS Y EXPERIMENTALES (30 PRESETS ADICIONALES)
    // =========================================================================
    catalog.push_back({
        "Sub-Quantum Wormhole",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Sub-Quantum Wormhole",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Translocacion temporal inversa con corrimiento continuo, cuerda Karplus y diezmador cuantico",
  "dryLevel": 0.4,
  "wetLevel": 0.95,
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
      "name": "Reverse Swell",
      "type": 38,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 0.8,
        "2": 0.85,
        "3": 7500.0,
        "4": 0.4,
        "5": 0.9
      }
    },
    {
      "id": 2,
      "name": "Bode Frequency Shift",
      "type": 20,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": -65.0,
        "2": 0.3,
        "3": 0.85
      }
    },
    {
      "id": 3,
      "name": "Karplus Resonator",
      "type": 37,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 164.8,
        "2": 0.4,
        "3": 2.2,
        "4": 0.35,
        "5": 1.0,
        "6": 0.8
      }
    },
    {
      "id": 4,
      "name": "Quantum Decimator",
      "type": 40,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": 10.0,
        "2": 3.0,
        "3": 0.1,
        "4": 0.2,
        "5": 3.0,
        "6": 0.85
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

    catalog.push_back({
        "Bioluminescent Dark Drone",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Bioluminescent Dark Drone",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Textura de ruido de lluvia con modelado vocal de formantes, congelacion espectral y FDN Reverb",
  "dryLevel": 0.3,
  "wetLevel": 0.9,
  "macros": [
    0.6,
    0.8,
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
      "name": "Rain Texture Gen",
      "type": 32,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 4.0,
        "2": 0.7,
        "3": 6500.0,
        "4": 0.3,
        "5": 0.6,
        "6": 0.85
      }
    },
    {
      "id": 2,
      "name": "Vocal Formant O-U",
      "type": 31,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 3.5,
        "2": 12.0,
        "3": 0.85,
        "4": 0.6,
        "5": 0.9
      }
    },
    {
      "id": 3,
      "name": "Spectral Freeze Cloud",
      "type": 12,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 0.85,
        "2": 0.45,
        "3": 0.2,
        "4": 0.8
      }
    },
    {
      "id": 4,
      "name": "Cosmic Reverb",
      "type": 6,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": 0.92,
        "2": 5.0,
        "3": 6000.0,
        "4": 25.0,
        "5": 0.7
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

    catalog.push_back({
        "Alien Throat Resonator",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Alien Throat Resonator",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Vocoder robotico de 16 bandas modulado por filtro de formantes, multiplicador AM y reverb temporal inverso",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [
    0.75,
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
      "name": "16-Band Vocoder",
      "type": 36,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 0.0,
        "2": 120.0,
        "3": 1.25,
        "4": 8.0,
        "5": 0.04,
        "6": 0.9
      }
    },
    {
      "id": 2,
      "name": "Formant Vowel A-I",
      "type": 31,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 1.8,
        "2": 10.0,
        "3": 1.1,
        "4": 0.5,
        "5": 0.85
      }
    },
    {
      "id": 3,
      "name": "Metallic Ring Mod",
      "type": 19,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 280.0,
        "2": 0.0,
        "3": 0.2,
        "4": 0.65
      }
    },
    {
      "id": 4,
      "name": "Reverse Swell Bloom",
      "type": 38,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": 0.6,
        "2": 0.75,
        "3": 9000.0,
        "4": 0.35,
        "5": 0.8
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

    catalog.push_back({
        "Hilbert Phase Deconstruction",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Hilbert Phase Deconstruction",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Separacion analitica de Hilbert con dispersion en peine de flanger y decodificacion Mid/Side estereo ultra-ancha",
  "dryLevel": 0.5,
  "wetLevel": 0.85,
  "macros": [
    0.7,
    0.5,
    0.6,
    0.4,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Hilbert Shift +12Hz",
      "type": 20,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 12.0,
        "2": 0.4,
        "3": 0.9
      }
    },
    {
      "id": 2,
      "name": "Dual Comb Flanger",
      "type": 18,
      "x": 260.0,
      "y": 60.0,
      "params": {
        "1": 0.3,
        "2": 0.85,
        "3": 3.5,
        "4": -0.75,
        "5": 0.8
      }
    },
    {
      "id": 3,
      "name": "M/S Spatial Widener",
      "type": 25,
      "x": 480.0,
      "y": 60.0,
      "params": {
        "1": 1.75,
        "2": 130.0,
        "3": 1.0,
        "4": 1.0
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
    }
  ]
})json"
    });

    catalog.push_back({
        "Supercritical Reactor Core",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Supercritical Reactor Core",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Lazo de feedback no lineal en saturacion simetrica, contencion por limitador brickwall y filtro analogico",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [
    0.85,
    0.6,
    0.7,
    0.9,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Feedback Container",
      "type": 22,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 0.82,
        "2": 45.0,
        "3": 6500.0,
        "4": 0.95,
        "5": 0.8
      }
    },
    {
      "id": 2,
      "name": "Tube Overdrive",
      "type": 8,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 8.5,
        "2": 3.0,
        "3": 7000.0,
        "4": 0.9
      }
    },
    {
      "id": 3,
      "name": "Brickwall Limiter",
      "type": 39,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": -4.0,
        "2": -0.3,
        "3": 25.0,
        "4": 1.5,
        "5": 1.0
      }
    },
    {
      "id": 4,
      "name": "Post Cutoff Filter",
      "type": 4,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": 3500.0,
        "2": 1.2,
        "3": 0.0
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

    catalog.push_back({
        "Ghostly AM Radio Static",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Ghostly AM Radio Static",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Inyeccion de hissing analogico de cinta, modulador AM con batimiento y saturacion magnetica de cabezal",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.65,
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
      "name": "Tape Hiss Noise",
      "type": 32,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 2.0,
        "2": 0.5,
        "3": 5500.0,
        "4": 0.45,
        "5": 0.55,
        "6": 0.8
      }
    },
    {
      "id": 2,
      "name": "AM Heterodyne",
      "type": 19,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 650.0,
        "2": 0.0,
        "3": 0.35,
        "4": 0.7
      }
    },
    {
      "id": 3,
      "name": "Bandpass Radio",
      "type": 4,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 1800.0,
        "2": 3.5,
        "3": 2.0
      }
    },
    {
      "id": 4,
      "name": "Tape Saturation 7.5ips",
      "type": 21,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": 4.5,
        "2": 0.0,
        "3": 0.65,
        "4": 0.7,
        "5": 0.9
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

    catalog.push_back({
        "Cybernetic Cyborg Chants",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Cybernetic Cyborg Chants",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Confluencia de formantes morfologicos, generador de portadora vocoder con octavacion y chorus multi-voz",
  "dryLevel": 0.45,
  "wetLevel": 0.85,
  "macros": [
    0.7,
    0.8,
    0.5,
    0.6,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Vowel Synthesizer",
      "type": 31,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 2.2,
        "2": 9.0,
        "3": 0.9,
        "4": 0.45,
        "5": 0.9
      }
    },
    {
      "id": 2,
      "name": "16-Band Speech Vocoder",
      "type": 36,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 1.0,
        "2": 95.0,
        "3": 1.15,
        "4": 7.0,
        "5": 0.06,
        "6": 0.85
      }
    },
    {
      "id": 3,
      "name": "Pitch Octaver +12",
      "type": 11,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 12.0,
        "2": 0.0,
        "3": 35.0,
        "4": 0.7
      }
    },
    {
      "id": 4,
      "name": "Stereo Spread Chorus",
      "type": 17,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": 0.8,
        "2": 0.65,
        "3": 0.35,
        "4": 4.0,
        "5": 0.75
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

    catalog.push_back({
        "Alien Formant Morphosphere",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Alien Formant Morphosphere",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Mapeo vocal multidimensional acoplado a resonador modal con decaimiento extendido y swell inverso",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [
    0.65,
    0.75,
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
      "name": "Formant Morph A-E-I",
      "type": 31,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 1.2,
        "2": 14.0,
        "3": 1.3,
        "4": 0.5,
        "5": 0.95
      }
    },
    {
      "id": 2,
      "name": "Modal Bell Resonator",
      "type": 13,
      "x": 260.0,
      "y": 60.0,
      "params": {
        "1": 330.0,
        "2": 3.0,
        "3": 0.6,
        "4": 0.3,
        "5": 0.85
      }
    },
    {
      "id": 3,
      "name": "Reverse Reverb Swell",
      "type": 38,
      "x": 480.0,
      "y": 60.0,
      "params": {
        "1": 1.1,
        "2": 0.8,
        "3": 6500.0,
        "4": 0.4,
        "5": 0.8
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
    }
  ]
})json"
    });

    catalog.push_back({
        "Radioactive Vinyl Decay",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Radioactive Vinyl Decay",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Friccion de surco de vinilo ruidoso, wow-flutter de cinta en 7.5 ips, trituracion de 6 bits y ecualizacion",
  "dryLevel": 0.3,
  "wetLevel": 0.95,
  "macros": [
    0.8,
    0.65,
    0.55,
    0.7,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Vinyl Crackle Generator",
      "type": 32,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 3.0,
        "2": 0.65,
        "3": 7500.0,
        "4": 0.5,
        "5": 0.5,
        "6": 0.9
      }
    },
    {
      "id": 2,
      "name": "Vintage Reel Wow",
      "type": 21,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 5.0,
        "2": 0.0,
        "3": 0.85,
        "4": 0.8,
        "5": 0.9
      }
    },
    {
      "id": 3,
      "name": "6-Bit Lo-Fi Crusher",
      "type": 40,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 6.0,
        "2": 5.0,
        "3": 0.15,
        "4": 0.35,
        "5": 4.0,
        "6": 0.85
      }
    },
    {
      "id": 4,
      "name": "Vintage Tone Sculpt",
      "type": 27,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": 150.0,
        "2": 4.0,
        "3": 3200.0,
        "4": 1.5,
        "5": -6.0,
        "6": 6000.0,
        "7": -8.0
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

    catalog.push_back({
        "Broken Tube Satellite",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Broken Tube Satellite",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Excitador de aire extremo acoplado a distorsion tipo valvula con de-esser de aspereza y limitador brickwall",
  "dryLevel": 0.5,
  "wetLevel": 0.8,
  "macros": [
    0.8,
    0.7,
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
      "name": "Air & Sub Exciter",
      "type": 35,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 8000.0,
        "2": 4.0,
        "3": 0.8,
        "4": 65.0,
        "5": 2.5,
        "6": 0.5,
        "7": 0.9
      }
    },
    {
      "id": 2,
      "name": "Tube Overdrive",
      "type": 8,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 6.0,
        "2": 3.0,
        "3": 9000.0,
        "4": 0.85
      }
    },
    {
      "id": 3,
      "name": "De-Esser Harshness",
      "type": 42,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 5500.0,
        "2": 2.0,
        "3": -18.0,
        "4": 16.0,
        "5": 0.0,
        "6": 0.0
      }
    },
    {
      "id": 4,
      "name": "Peak Brickwall Limiter",
      "type": 39,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": -2.0,
        "2": -0.2,
        "3": 15.0,
        "4": 1.2,
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

    catalog.push_back({
        "Rotary Android Vowel Box",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Rotary Android Vowel Box",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Altavoz giratorio Leslie acoplado a filtros formantes morfologicos y compresion de cinta vintage",
  "dryLevel": 0.45,
  "wetLevel": 0.85,
  "macros": [
    0.6,
    0.7,
    0.5,
    0.6,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Formant Vowel E",
      "type": 31,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 1.0,
        "2": 8.0,
        "3": 1.05,
        "4": 0.4,
        "5": 0.95
      }
    },
    {
      "id": 2,
      "name": "Leslie Rotary Speaker",
      "type": 34,
      "x": 260.0,
      "y": 60.0,
      "params": {
        "1": 1.0,
        "2": 1.2,
        "3": 0.9,
        "4": 2.5,
        "5": 0.9,
        "6": 0.9
      }
    },
    {
      "id": 3,
      "name": "Tape Saturation Drive",
      "type": 21,
      "x": 480.0,
      "y": 60.0,
      "params": {
        "1": 3.0,
        "2": 1.0,
        "3": 0.4,
        "4": 0.6,
        "5": 0.85
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
    }
  ]
})json"
    });

    catalog.push_back({
        "Self-Oscillating Tape Spiral",
        "Experimental & Avant-Garde",
        R"json({
  "schemaVersion": 1,
  "name": "Self-Oscillating Tape Spiral",
  "author": "N8Audio",
  "category": "Experimental & Avant-Garde",
  "description": "Calidez de cinta magnetica realimentada continuamente en bucle no lineal con modelado vocal",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.6,
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
      "name": "Tape Drive 15ips",
      "type": 21,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 4.0,
        "2": 1.0,
        "3": 0.5,
        "4": 0.7,
        "5": 0.9
      }
    },
    {
      "id": 2,
      "name": "Feedback Container",
      "type": 22,
      "x": 260.0,
      "y": 60.0,
      "params": {
        "1": 0.78,
        "2": 65.0,
        "3": 5500.0,
        "4": 0.9,
        "5": 0.85
      }
    },
    {
      "id": 3,
      "name": "Formant Vowel U",
      "type": 31,
      "x": 480.0,
      "y": 60.0,
      "params": {
        "1": 4.0,
        "2": 10.0,
        "3": 0.85,
        "4": 0.5,
        "5": 0.8
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
    }
  ]
})json"
    });

    catalog.push_back({
        "Glitch Polyphony Slicer",
        "Glitch & Deconstrucción Rítmica",
        R"json({
  "schemaVersion": 1,
  "name": "Glitch Polyphony Slicer",
  "author": "N8Audio",
  "category": "Glitch & Deconstrucci\u00f3n R\u00edtmica",
  "description": "Fragmentacion ritmica estocastica con transpocision cromatica de quinta, modulador de anillo y paneo 3D",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.85,
    0.75,
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
      "name": "Glitch Slicer 1/8",
      "type": 14,
      "x": 40.0,
      "y": 60.0,
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
      "name": "Pitch Shift +7st",
      "type": 11,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 7.0,
        "2": 0.0,
        "3": 35.0,
        "4": 0.8
      }
    },
    {
      "id": 3,
      "name": "Metallic Ring Mod",
      "type": 19,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 220.0,
        "2": 0.0,
        "3": 0.2,
        "4": 0.7
      }
    },
    {
      "id": 4,
      "name": "Spatial Orbit Panner",
      "type": 26,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": 65.0,
        "2": 20.0,
        "3": 2.0,
        "4": 0.95
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

    catalog.push_back({
        "Neural Stutter Machine",
        "Glitch & Deconstrucción Rítmica",
        R"json({
  "schemaVersion": 1,
  "name": "Neural Stutter Machine",
  "author": "N8Audio",
  "category": "Glitch & Deconstrucci\u00f3n R\u00edtmica",
  "description": "Frenado de cinta de inercia acoplado a delay en rebote ping-pong, diezmado y modelador transiente agresivo",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.7,
    0.65,
    0.6,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Tape Stop Brake",
      "type": 30,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 0.0,
        "2": 0.5,
        "3": 0.25,
        "4": 1.0,
        "5": 0.4,
        "6": 0.85
      }
    },
    {
      "id": 2,
      "name": "Ping-Pong Echo",
      "type": 28,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 150.0,
        "2": 225.0,
        "3": 0.5,
        "4": 6500.0,
        "5": 1.0,
        "6": 0.7
      }
    },
    {
      "id": 3,
      "name": "Bitcrusher 8-Bit",
      "type": 40,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 8.0,
        "2": 2.0,
        "3": 0.1,
        "4": 0.1,
        "5": 2.0,
        "6": 0.8
      }
    },
    {
      "id": 4,
      "name": "Transient Punch",
      "type": 33,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": 0.5,
        "2": -0.3,
        "3": 1.2,
        "4": 1.0,
        "5": 2.0,
        "6": 0.9
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

    catalog.push_back({
        "Cybernetic Buffer Fracture",
        "Glitch & Deconstrucción Rítmica",
        R"json({
  "schemaVersion": 1,
  "name": "Cybernetic Buffer Fracture",
  "author": "N8Audio",
  "category": "Glitch & Deconstrucci\u00f3n R\u00edtmica",
  "description": "Repeticion de buffer con compuerta de tartamudeo, triturador agresivo, compresor multibanda y delay",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.9,
    0.7,
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
      "name": "Stutter Glitch Slicer",
      "type": 14,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 2.0,
        "2": 0.8,
        "3": 0.4,
        "4": 0.5,
        "5": 0.9
      }
    },
    {
      "id": 2,
      "name": "Crusher Decimator",
      "type": 40,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 7.0,
        "2": 4.0,
        "3": 0.08,
        "4": 0.3,
        "5": 4.0,
        "6": 0.85
      }
    },
    {
      "id": 3,
      "name": "Multiband Slam",
      "type": 15,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 280.0,
        "2": 2800.0,
        "3": 1.4,
        "4": 1.2,
        "5": 1.5,
        "6": 0.9
      }
    },
    {
      "id": 4,
      "name": "Space Delay Rebound",
      "type": 28,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": 180.0,
        "2": 360.0,
        "3": 0.45,
        "4": 5500.0,
        "5": 1.0,
        "6": 0.65
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

    catalog.push_back({
        "Spectral Crystal Shards",
        "Espectral & Drones Congelados",
        R"json({
  "schemaVersion": 1,
  "name": "Spectral Crystal Shards",
  "author": "N8Audio",
  "category": "Espectral & Drones Congelados",
  "description": "Filtrado espectral FFT con compuerta armonica, nube granular, resonadores y espacializacion 3D",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [
    0.75,
    0.7,
    0.6,
    0.65,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Spectral Tilt & Gate",
      "type": 29,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": -45.0,
        "2": 3.0,
        "3": 0.9
      }
    },
    {
      "id": 2,
      "name": "Granular Cloud Mist",
      "type": 7,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 60.0,
        "2": 25.0,
        "3": 35.0,
        "4": 12.0,
        "5": 2.0,
        "6": 0.8,
        "7": 0.8
      }
    },
    {
      "id": 3,
      "name": "Crystalline Resonator",
      "type": 13,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 523.25,
        "2": 2.5,
        "3": 0.5,
        "4": 0.4,
        "5": 0.75
      }
    },
    {
      "id": 4,
      "name": "Spatial Panner",
      "type": 26,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": -50.0,
        "2": 25.0,
        "3": 2.2,
        "4": 0.95
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

    catalog.push_back({
        "Infinite Frozen Aurora",
        "Espectral & Drones Congelados",
        R"json({
  "schemaVersion": 1,
  "name": "Infinite Frozen Aurora",
  "author": "N8Audio",
  "category": "Espectral & Drones Congelados",
  "description": "Congelacion espectral con borrado temporal (smear), chorus estereo, swell inverso y paneo orbital",
  "dryLevel": 0.35,
  "wetLevel": 0.95,
  "macros": [
    0.8,
    0.85,
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
      "name": "Spectral Freeze Hold",
      "type": 12,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 0.9,
        "2": 0.75,
        "3": 0.25,
        "4": 0.9
      }
    },
    {
      "id": 2,
      "name": "4-Voice Chorus",
      "type": 17,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 1.2,
        "2": 0.75,
        "3": 0.3,
        "4": 4.0,
        "5": 0.8
      }
    },
    {
      "id": 3,
      "name": "Reverse Bloom Reverb",
      "type": 38,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 0.9,
        "2": 0.85,
        "3": 8500.0,
        "4": 0.45,
        "5": 0.85
      }
    },
    {
      "id": 4,
      "name": "Azimuth 3D Panner",
      "type": 26,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": 80.0,
        "2": 15.0,
        "3": 2.5,
        "4": 1.0
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

    catalog.push_back({
        "Spectral Smear Horizon",
        "Espectral & Drones Congelados",
        R"json({
  "schemaVersion": 1,
  "name": "Spectral Smear Horizon",
  "author": "N8Audio",
  "category": "Espectral & Drones Congelados",
  "description": "Borroneado espectral FFT con corrimiento microtonal de frecuencia y cola de reverberacion infinita",
  "dryLevel": 0.4,
  "wetLevel": 0.85,
  "macros": [
    0.7,
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
      "name": "Spectral Smear FFT",
      "type": 12,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 0.4,
        "2": 0.9,
        "3": 0.1,
        "4": 0.85
      }
    },
    {
      "id": 2,
      "name": "Micro Freq Shift +7Hz",
      "type": 20,
      "x": 260.0,
      "y": 60.0,
      "params": {
        "1": 7.5,
        "2": 0.25,
        "3": 0.8
      }
    },
    {
      "id": 3,
      "name": "Ambient FDN Reverb",
      "type": 6,
      "x": 480.0,
      "y": 60.0,
      "params": {
        "1": 0.88,
        "2": 4.5,
        "3": 7500.0,
        "4": 30.0,
        "5": 0.75
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
    }
  ]
})json"
    });

    catalog.push_back({
        "Micro-Grain Granular Shatter",
        "Micro-Muestreo Granular",
        R"json({
  "schemaVersion": 1,
  "name": "Micro-Grain Granular Shatter",
  "author": "N8Audio",
  "category": "Micro-Muestreo Granular",
  "description": "Nube de granos microscopicos de 25ms de alta densidad con perfilado transiente y techo de limiter brickwall",
  "dryLevel": 0.45,
  "wetLevel": 0.9,
  "macros": [
    0.85,
    0.7,
    0.5,
    0.6,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Micro Granular Cloud",
      "type": 7,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 25.0,
        "2": 45.0,
        "3": 50.0,
        "4": 0.0,
        "5": 5.0,
        "6": 0.9,
        "7": 0.9
      }
    },
    {
      "id": 2,
      "name": "Transient Sculptor",
      "type": 33,
      "x": 260.0,
      "y": 60.0,
      "params": {
        "1": 0.6,
        "2": -0.4,
        "3": 1.4,
        "4": 1.0,
        "5": 3.0,
        "6": 0.9
      }
    },
    {
      "id": 3,
      "name": "Brickwall Peak Limiter",
      "type": 39,
      "x": 480.0,
      "y": 60.0,
      "params": {
        "1": -3.0,
        "2": -0.1,
        "3": 20.0,
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
    }
  ]
})json"
    });

    catalog.push_back({
        "Granular Shimmer Mirage",
        "Micro-Muestreo Granular",
        R"json({
  "schemaVersion": 1,
  "name": "Granular Shimmer Mirage",
  "author": "N8Audio",
  "category": "Micro-Muestreo Granular",
  "description": "Enjambre granular con spray tonal transpuesto a la octava superior acoplado a reverberacion inversa con diffusion",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [
    0.75,
    0.8,
    0.65,
    0.7,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Granular Particle Cloud",
      "type": 7,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 90.0,
        "2": 28.0,
        "3": 40.0,
        "4": 0.0,
        "5": 3.0,
        "6": 0.85,
        "7": 0.85
      }
    },
    {
      "id": 2,
      "name": "Pitch Octaver +12st",
      "type": 11,
      "x": 260.0,
      "y": 60.0,
      "params": {
        "1": 12.0,
        "2": 0.0,
        "3": 40.0,
        "4": 0.85
      }
    },
    {
      "id": 3,
      "name": "Reverse Reverb Swell",
      "type": 38,
      "x": 480.0,
      "y": 60.0,
      "params": {
        "1": 0.7,
        "2": 0.85,
        "3": 8000.0,
        "4": 0.35,
        "5": 0.8
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
    }
  ]
})json"
    });

    catalog.push_back({
        "Stochastic Cloud Swarm",
        "Micro-Muestreo Granular",
        R"json({
  "schemaVersion": 1,
  "name": "Stochastic Cloud Swarm",
  "author": "N8Audio",
  "category": "Micro-Muestreo Granular",
  "description": "Dispersion granular estocastica sincronizada alimentando resonadores modales y orbita 3D",
  "dryLevel": 0.45,
  "wetLevel": 0.85,
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
      "name": "Granular Swarm",
      "type": 7,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 70.0,
        "2": 32.0,
        "3": 60.0,
        "4": -5.0,
        "5": 4.0,
        "6": 0.95,
        "7": 0.85
      }
    },
    {
      "id": 2,
      "name": "Modal Bell Bank",
      "type": 13,
      "x": 260.0,
      "y": 60.0,
      "params": {
        "1": 440.0,
        "2": 2.0,
        "3": 0.4,
        "4": 0.2,
        "5": 0.8
      }
    },
    {
      "id": 3,
      "name": "3D Dynamic Panner",
      "type": 26,
      "x": 480.0,
      "y": 60.0,
      "params": {
        "1": -60.0,
        "2": 30.0,
        "3": 1.8,
        "4": 0.95
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
    }
  ]
})json"
    });

    catalog.push_back({
        "Strummed Obsidian Harp",
        "Resonadores Modales & Afinación",
        R"json({
  "schemaVersion": 1,
  "name": "Strummed Obsidian Harp",
  "author": "N8Audio",
  "category": "Resonadores Modales & Afinaci\u00f3n",
  "description": "Simulacion de cuerda punteada Karplus-Strong acoplada a linea de retardo estereo ping-pong y reverb calida",
  "dryLevel": 0.5,
  "wetLevel": 0.85,
  "macros": [
    0.7,
    0.6,
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
      "name": "Karplus Plucked String",
      "type": 37,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 130.8,
        "2": 0.35,
        "3": 3.0,
        "4": 0.2,
        "5": 1.0,
        "6": 0.9
      }
    },
    {
      "id": 2,
      "name": "Stereo Delay Ping-Pong",
      "type": 28,
      "x": 260.0,
      "y": 60.0,
      "params": {
        "1": 240.0,
        "2": 360.0,
        "3": 0.45,
        "4": 7000.0,
        "5": 1.0,
        "6": 0.6
      }
    },
    {
      "id": 3,
      "name": "Natural Room Reverb",
      "type": 6,
      "x": 480.0,
      "y": 60.0,
      "params": {
        "1": 0.75,
        "2": 2.8,
        "3": 6500.0,
        "4": 15.0,
        "5": 0.6
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
    }
  ]
})json"
    });

    catalog.push_back({
        "Excited Resonant Monochord",
        "Resonadores Modales & Afinación",
        R"json({
  "schemaVersion": 1,
  "name": "Excited Resonant Monochord",
  "author": "N8Audio",
  "category": "Resonadores Modales & Afinaci\u00f3n",
  "description": "Generacion armonica subgraves y aire impulsando un modelo fisico Karplus con moldeador transiente",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.65,
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
      "name": "Harmonic Exciter",
      "type": 35,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 7000.0,
        "2": 3.0,
        "3": 0.7,
        "4": 75.0,
        "5": 3.0,
        "6": 0.6,
        "7": 0.9
      }
    },
    {
      "id": 2,
      "name": "Karplus Monochord",
      "type": 37,
      "x": 260.0,
      "y": 60.0,
      "params": {
        "1": 98.0,
        "2": 0.45,
        "3": 2.8,
        "4": 0.3,
        "5": 1.0,
        "6": 0.85
      }
    },
    {
      "id": 3,
      "name": "Transient Snap",
      "type": 33,
      "x": 480.0,
      "y": 60.0,
      "params": {
        "1": 0.7,
        "2": 0.2,
        "3": 1.3,
        "4": 1.0,
        "5": 1.5,
        "6": 0.85
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
    }
  ]
})json"
    });

    catalog.push_back({
        "Quantum Plucked Tapestry",
        "Resonadores Modales & Afinación",
        R"json({
  "schemaVersion": 1,
  "name": "Quantum Plucked Tapestry",
  "author": "N8Audio",
  "category": "Resonadores Modales & Afinaci\u00f3n",
  "description": "Cuerda punteada fisica acoplada a congelacion espectral continua con paneo dimensional 3D",
  "dryLevel": 0.45,
  "wetLevel": 0.85,
  "macros": [
    0.75,
    0.7,
    0.65,
    0.5,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Karplus Acoustic Harp",
      "type": 37,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 261.63,
        "2": 0.25,
        "3": 3.5,
        "4": 0.15,
        "5": 1.0,
        "6": 0.9
      }
    },
    {
      "id": 2,
      "name": "Spectral Sustainer",
      "type": 12,
      "x": 260.0,
      "y": 60.0,
      "params": {
        "1": 0.7,
        "2": 0.5,
        "3": 0.2,
        "4": 0.75
      }
    },
    {
      "id": 3,
      "name": "3D Spatial Panner",
      "type": 26,
      "x": 480.0,
      "y": 60.0,
      "params": {
        "1": 45.0,
        "2": -10.0,
        "3": 2.0,
        "4": 0.9
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
    }
  ]
})json"
    });

    catalog.push_back({
        "Tape Braking Stutter",
        "Cintas Analógicas & Lo-Fi",
        R"json({
  "schemaVersion": 1,
  "name": "Tape Braking Stutter",
  "author": "N8Audio",
  "category": "Cintas Anal\u00f3gicas & Lo-Fi",
  "description": "Mecanica de motor de frenado tape-stop en serie con saturador de distorsion, rebanador glitch y reverb",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [
    0.85,
    0.7,
    0.65,
    0.6,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Tape Stop Machine",
      "type": 30,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 0.0,
        "2": 0.7,
        "3": 0.35,
        "4": 2.0,
        "5": 0.5,
        "6": 0.85
      }
    },
    {
      "id": 2,
      "name": "Analog Saturation",
      "type": 8,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 4.0,
        "2": 0.0,
        "3": 6500.0,
        "4": 0.8
      }
    },
    {
      "id": 3,
      "name": "Glitch Stutter Gate",
      "type": 14,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 1.0,
        "2": 0.6,
        "3": 0.25,
        "4": 0.5,
        "5": 0.85
      }
    },
    {
      "id": 4,
      "name": "Diffuse Reverb",
      "type": 6,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": 0.8,
        "2": 3.0,
        "3": 5000.0,
        "4": 20.0,
        "5": 0.65
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

    catalog.push_back({
        "Cassette Wow Melt",
        "Cintas Analógicas & Lo-Fi",
        R"json({
  "schemaVersion": 1,
  "name": "Cassette Wow Melt",
  "author": "N8Audio",
  "category": "Cintas Anal\u00f3gicas & Lo-Fi",
  "description": "Degradacion magnetica con flutter pronunciado, frenado mecanico, chorus estereo y modelado de ecualizacion",
  "dryLevel": 0.45,
  "wetLevel": 0.85,
  "macros": [
    0.75,
    0.6,
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
      "name": "Tape Wow Flutter",
      "type": 21,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 4.5,
        "2": 0.0,
        "3": 0.75,
        "4": 0.7,
        "5": 0.9
      }
    },
    {
      "id": 2,
      "name": "Tape Stop Drag",
      "type": 30,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 0.0,
        "2": 0.8,
        "3": 0.4,
        "4": 1.0,
        "5": 0.6,
        "6": 0.8
      }
    },
    {
      "id": 3,
      "name": "Warm Chorus",
      "type": 17,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 0.6,
        "2": 0.7,
        "3": 0.25,
        "4": 3.0,
        "5": 0.7
      }
    },
    {
      "id": 4,
      "name": "Lo-Fi EQ Curve",
      "type": 27,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": 180.0,
        "2": 3.0,
        "3": 2500.0,
        "4": 1.2,
        "5": -4.0,
        "6": 5500.0,
        "7": -10.0
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

    catalog.push_back({
        "Sub-Zero Drone Machine",
        "Cintas Analógicas & Lo-Fi",
        R"json({
  "schemaVersion": 1,
  "name": "Sub-Zero Drone Machine",
  "author": "N8Audio",
  "category": "Cintas Anal\u00f3gicas & Lo-Fi",
  "description": "Textura de ruido rosa con compresion de umbral profundo, inclinacion espectral y filtro paso-bajos calido",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [
    0.65,
    0.7,
    0.55,
    0.6,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Pink Noise Atmosphere",
      "type": 32,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 1.0,
        "2": 0.6,
        "3": 4500.0,
        "4": 0.4,
        "5": 0.5,
        "6": 0.85
      }
    },
    {
      "id": 2,
      "name": "Spectral Tilt Dark",
      "type": 29,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": -55.0,
        "2": -4.0,
        "3": 0.85
      }
    },
    {
      "id": 3,
      "name": "Lowpass Resonance",
      "type": 4,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 1200.0,
        "2": 1.8,
        "3": 0.0
      }
    },
    {
      "id": 4,
      "name": "VCA Glue Compressor",
      "type": 9,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": -22.0,
        "2": 4.5,
        "3": 10.0,
        "4": 120.0,
        "5": 4.0,
        "6": 1.0
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

    catalog.push_back({
        "Karplus Physical Feedback",
        "Lazos de Realimentación Inestable",
        R"json({
  "schemaVersion": 1,
  "name": "Karplus Physical Feedback",
  "author": "N8Audio",
  "category": "Lazos de Realimentaci\u00f3n Inestable",
  "description": "Cuerda afinada con inyeccion de lazo de feedback con amortiguamiento, pitch shifter y perfilado de ataque",
  "dryLevel": 0.45,
  "wetLevel": 0.9,
  "macros": [
    0.8,
    0.75,
    0.6,
    0.65,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Karplus Core",
      "type": 37,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 110.0,
        "2": 0.4,
        "3": 2.5,
        "4": 0.25,
        "5": 1.0,
        "6": 0.85
      }
    },
    {
      "id": 2,
      "name": "Feedback Container",
      "type": 22,
      "x": 250.0,
      "y": 60.0,
      "params": {
        "1": 0.75,
        "2": 55.0,
        "3": 6000.0,
        "4": 0.9,
        "5": 0.8
      }
    },
    {
      "id": 3,
      "name": "Pitch Shifter +5st",
      "type": 11,
      "x": 460.0,
      "y": 60.0,
      "params": {
        "1": 5.0,
        "2": 0.0,
        "3": 35.0,
        "4": 0.7
      }
    },
    {
      "id": 4,
      "name": "Transient Shaper",
      "type": 33,
      "x": 670.0,
      "y": 60.0,
      "params": {
        "1": 0.4,
        "2": 0.3,
        "3": 1.1,
        "4": 1.0,
        "5": 1.0,
        "6": 0.85
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

    catalog.push_back({
        "Reverse Grain Vortex",
        "Lazos de Realimentación Inestable",
        R"json({
  "schemaVersion": 1,
  "name": "Reverse Grain Vortex",
  "author": "N8Audio",
  "category": "Lazos de Realimentaci\u00f3n Inestable",
  "description": "Nube granular densa alimentando reverberacion inversa temporal con feedback y saturacion de cuantizacion",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [
    0.85,
    0.7,
    0.65,
    0.7,
    0.5,
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
      "y": 60.0,
      "params": {
        "1": 85.0,
        "2": 30.0,
        "3": 30.0,
        "4": -12.0,
        "5": 2.0,
        "6": 0.8,
        "7": 0.85
      }
    },
    {
      "id": 2,
      "name": "Reverse Swell Feedback",
      "type": 38,
      "x": 260.0,
      "y": 60.0,
      "params": {
        "1": 0.8,
        "2": 0.8,
        "3": 7000.0,
        "4": 0.5,
        "5": 0.85
      }
    },
    {
      "id": 3,
      "name": "Bitcrusher Drive",
      "type": 40,
      "x": 480.0,
      "y": 60.0,
      "params": {
        "1": 9.0,
        "2": 2.5,
        "3": 0.05,
        "4": 0.2,
        "5": 3.0,
        "6": 0.8
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
    }
  ]
})json"
    });

    catalog.push_back({
        "Vocoded Spectral Mist",
        "Lazos de Realimentación Inestable",
        R"json({
  "schemaVersion": 1,
  "name": "Vocoded Spectral Mist",
  "author": "N8Audio",
  "category": "Lazos de Realimentaci\u00f3n Inestable",
  "description": "Vocoder de ruido de 16 bandas procesado por ecualizacion espectral FFT y eco estereo cruzado",
  "dryLevel": 0.45,
  "wetLevel": 0.85,
  "macros": [
    0.75,
    0.65,
    0.7,
    0.55,
    0.5,
    0.5,
    0.5,
    0.0
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Noise Carrier Vocoder",
      "type": 36,
      "x": 40.0,
      "y": 60.0,
      "params": {
        "1": 2.0,
        "2": 150.0,
        "3": 1.1,
        "4": 9.0,
        "5": 0.05,
        "6": 0.9
      }
    },
    {
      "id": 2,
      "name": "Spectral Tilt Gate",
      "type": 29,
      "x": 260.0,
      "y": 60.0,
      "params": {
        "1": -40.0,
        "2": 2.0,
        "3": 0.85
      }
    },
    {
      "id": 3,
      "name": "Dual Space Delay",
      "type": 28,
      "x": 480.0,
      "y": 60.0,
      "params": {
        "1": 250.0,
        "2": 375.0,
        "3": 0.45,
        "4": 6000.0,
        "5": 1.0,
        "6": 0.7
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
    }
  ]
})json"
    });

    // =========================================================================
    // ARTURIA-KILLER FLAGSHIP SUITE (Presets 71 - 80)
    // =========================================================================
    // 1. Crystalline Shimmer Cloud (Espacios Infinitos & Shimmer)
    catalog.push_back({
        "Crystalline Shimmer Cloud",
        "Espacios Infinitos & Shimmer",
        R"json({
  "schemaVersion": 1,
  "name": "Crystalline Shimmer Cloud",
  "author": "N8Audio",
  "category": "Espacios Infinitos & Shimmer",
  "description": "Nube de granos flotantes combinada con reverberacion shimmer armonica cristalina +12st y decodificador Mid/Side",
  "dryLevel": 0.4,
  "wetLevel": 0.95,
  "macros": [0.75, 0.65, 0.8, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Granular Cloud", "type": 7, "x": 40.0, "y": 60.0, "params": { "1": 75.0, "2": 35.0, "3": 25.0, "4": 0.0, "5": 0.3, "6": 0.8, "7": 0.85 } },
    { "id": 2, "name": "Shimmer Reverb", "type": 50, "x": 260.0, "y": 60.0, "params": { "1": 6.5, "2": 7500.0, "3": 0.65, "4": 0.0, "5": 1.5, "6": 0.7 } },
    { "id": 3, "name": "Mid/Side Decoder", "type": 25, "x": 480.0, "y": 60.0, "params": { "1": 1.4, "2": 120.0, "3": 0.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // 2. Glitch Stutter Kaleidoscope (Glitch & Deconstrucción Rítmica)
    catalog.push_back({
        "Glitch Stutter Kaleidoscope",
        "Glitch & Deconstrucción Rítmica",
        R"json({
  "schemaVersion": 1,
  "name": "Glitch Stutter Kaleidoscope",
  "author": "N8Audio",
  "category": "Glitch & Deconstrucción Rítmica",
  "description": "Deconstruccion ritmica sincopada mediante AudioSlicer y Glitch, seguido de dispersion ping-pong y saturacion de cinta",
  "dryLevel": 0.35,
  "wetLevel": 0.9,
  "macros": [0.9, 0.7, 0.6, 0.8, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Audio Slicer", "type": 49, "x": 40.0, "y": 60.0, "params": { "1": 0.0, "2": 8.0, "3": -1.0, "4": 1.0, "5": 0.0, "6": 60.0, "7": 4.0, "8": 0.3, "9": 0.0, "10": 12000.0, "11": 0.9 } },
    { "id": 2, "name": "Glitch Slicer", "type": 14, "x": 250.0, "y": 60.0, "params": { "1": 2.0, "2": 0.4, "3": 0.5, "4": 0.8, "5": 0.75 } },
    { "id": 3, "name": "Ping-Pong Delay", "type": 28, "x": 460.0, "y": 60.0, "params": { "1": 180.0, "2": 270.0, "3": 0.45, "4": 6000.0, "5": 1.0, "6": 0.5 } },
    { "id": 4, "name": "Tape Saturation", "type": 21, "x": 670.0, "y": 60.0, "params": { "1": 3.2, "2": 1.0, "3": 0.3, "4": 0.8, "5": 0.85 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    // 3. Sub-Atomic Texture Cloud (Micro-Muestreo Granular)
    catalog.push_back({
        "Sub-Atomic Texture Cloud",
        "Micro-Muestreo Granular",
        R"json({
  "schemaVersion": 1,
  "name": "Sub-Atomic Texture Cloud",
  "author": "N8Audio",
  "category": "Micro-Muestreo Granular",
  "description": "Micro-granos sub-atomicos de 15ms filtrados por formantes vocalicos y disueltos en difusion liquida espectral",
  "dryLevel": 0.45,
  "wetLevel": 0.9,
  "macros": [0.85, 0.75, 0.7, 0.6, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Granular Texture", "type": 7, "x": 40.0, "y": 60.0, "params": { "1": 15.0, "2": 55.0, "3": 40.0, "4": 0.0, "5": 1.2, "6": 0.9, "7": 0.9, "10": 2.0 } },
    { "id": 2, "name": "Formant Filter", "type": 31, "x": 260.0, "y": 60.0, "params": { "1": 0.45, "2": 4.5, "3": 1.1, "4": 0.4, "5": 0.8 } },
    { "id": 3, "name": "Spectral Smear", "type": 52, "x": 480.0, "y": 60.0, "params": { "1": 3.5, "2": 0.85, "3": 0.9, "4": -0.2, "5": 0.75 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // 4. Stochastic Reverse Resonator (Resonadores Modales & Afinación)
    catalog.push_back({
        "Stochastic Reverse Resonator",
        "Resonadores Modales & Afinación",
        R"json({
  "schemaVersion": 1,
  "name": "Stochastic Reverse Resonator",
  "author": "N8Audio",
  "category": "Resonadores Modales & Afinación",
  "description": "Granos estocasticos invertidos que excitan un banco de resonadores modales decantando en reverb FDN",
  "dryLevel": 0.5,
  "wetLevel": 0.85,
  "macros": [0.7, 0.8, 0.6, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Reverse Granular", "type": 7, "x": 40.0, "y": 60.0, "params": { "1": 120.0, "2": 20.0, "3": 60.0, "4": -12.0, "5": 0.0, "6": 0.7, "7": 0.8, "10": 1.0 } },
    { "id": 2, "name": "Modal Resonators", "type": 13, "x": 260.0, "y": 60.0, "params": { "1": 220.0, "2": 2.2, "3": 0.4, "4": 0.2, "5": 0.75 } },
    { "id": 3, "name": "FDN Reverb", "type": 6, "x": 480.0, "y": 60.0, "params": { "1": 0.85, "2": 4.5, "3": 5500.0, "4": 25.0, "5": 0.5 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // 5. Hypersonic Cybernetic Slicer (Dinámica Agresiva & OTT)
    catalog.push_back({
        "Hypersonic Cybernetic Slicer",
        "Dinámica Agresiva & OTT",
        R"json({
  "schemaVersion": 1,
  "name": "Hypersonic Cybernetic Slicer",
  "author": "N8Audio",
  "category": "Dinámica Agresiva & OTT",
  "description": "Segmentacion agresiva en semicorcheas con wavefolding, compresion multibanda OTT y limitador true peak",
  "dryLevel": 0.2,
  "wetLevel": 1.0,
  "macros": [0.95, 0.85, 0.7, 0.6, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Hyper Slicer", "type": 49, "x": 40.0, "y": 60.0, "params": { "1": 0.0, "2": 16.0, "3": -1.0, "4": 1.0, "5": 0.0, "6": 40.0, "7": 6.0, "8": 0.4, "9": 0.0, "10": 16000.0, "11": 1.0 } },
    { "id": 2, "name": "Wavefold Drive", "type": 8, "x": 250.0, "y": 60.0, "params": { "1": 5.5, "2": 2.0, "3": 6500.0, "4": 0.8 } },
    { "id": 3, "name": "Multiband OTT", "type": 15, "x": 460.0, "y": 60.0, "params": { "1": 180.0, "2": 3000.0, "3": 1.3, "4": 1.1, "5": 1.2, "6": 0.9 } },
    { "id": 4, "name": "Brickwall Peak", "type": 39, "x": 670.0, "y": 60.0, "params": { "1": -1.0, "2": -0.2, "3": 40.0, "4": 2.0, "5": 1.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    // 6. Orbital Euclidean Pulsar (Paneo 3D & Espacialización)
    catalog.push_back({
        "Orbital Euclidean Pulsar",
        "Paneo 3D & Espacialización",
        R"json({
  "schemaVersion": 1,
  "name": "Orbital Euclidean Pulsar",
  "author": "N8Audio",
  "category": "Paneo 3D & Espacialización",
  "description": "Paneo orbital 3D con chorus multifasico, retardo escalonado ping-pong y textura bitcrush vintage",
  "dryLevel": 0.45,
  "wetLevel": 0.9,
  "macros": [0.75, 0.6, 0.8, 0.7, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "3D Panner", "type": 26, "x": 40.0, "y": 60.0, "params": { "1": 45.0, "2": 15.0, "3": 2.0, "4": 1.0 } },
    { "id": 2, "name": "Stereo Chorus", "type": 17, "x": 250.0, "y": 60.0, "params": { "1": 0.8, "2": 0.65, "3": 0.3, "4": 4.0, "5": 0.75 } },
    { "id": 3, "name": "Space Delay", "type": 28, "x": 460.0, "y": 60.0, "params": { "1": 240.0, "2": 360.0, "3": 0.5, "4": 7000.0, "5": 1.0, "6": 0.55 } },
    { "id": 4, "name": "Lo-Fi Decimator", "type": 40, "x": 670.0, "y": 60.0, "params": { "1": 12.0, "2": 2.0, "3": 0.02, "4": 0.2, "5": 0.0, "6": 0.4 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    // 7. Dynamic Motion Morph (Modulación Psicoacústica)
    catalog.push_back({
        "Dynamic Motion Morph",
        "Modulación Psicoacústica",
        R"json({
  "schemaVersion": 1,
  "name": "Dynamic Motion Morph",
  "author": "N8Audio",
  "category": "Modulación Psicoacústica",
  "description": "Escultura dinamica con realce de transitorios, barrido bipolar en peine con flanger y ecualizacion quirurgica",
  "dryLevel": 0.35,
  "wetLevel": 0.95,
  "macros": [0.85, 0.7, 0.75, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Transient Punch", "type": 33, "x": 40.0, "y": 60.0, "params": { "1": 0.45, "2": -0.3, "3": 1.2, "4": 1.0, "5": 2.0, "6": 1.0 } },
    { "id": 2, "name": "Comb Flanger", "type": 18, "x": 250.0, "y": 60.0, "params": { "1": 0.35, "2": 0.85, "3": 2.5, "4": 0.75, "5": 0.7 } },
    { "id": 3, "name": "Parametric Tone", "type": 27, "x": 460.0, "y": 60.0, "params": { "1": 90.0, "2": 2.0, "3": 2200.0, "4": 2.0, "5": 4.0, "6": 8500.0, "7": -2.0 } },
    { "id": 4, "name": "VCA Compressor", "type": 9, "x": 670.0, "y": 60.0, "params": { "1": -16.0, "2": 4.0, "3": 8.0, "4": 80.0, "5": 3.0, "6": 4.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    // 8. Celestial Refracted Cathedral (Espacios Infinitos & Shimmer)
    catalog.push_back({
        "Celestial Refracted Cathedral",
        "Espacios Infinitos & Shimmer",
        R"json({
  "schemaVersion": 1,
  "name": "Celestial Refracted Cathedral",
  "author": "N8Audio",
  "category": "Espacios Infinitos & Shimmer",
  "description": "Desdoblamiento en 8 voces unisono refractadas inyectadas en una reverberacion shimmer masiva de 12 segundos y apertura estéreo ultra-amplia",
  "dryLevel": 0.3,
  "wetLevel": 1.0,
  "macros": [0.9, 0.85, 0.8, 0.6, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "8-Voice Refraction", "type": 51, "x": 40.0, "y": 60.0, "params": { "1": 8.0, "2": 22.0, "3": 25.0, "4": 0.95, "5": 0.8, "6": 2.5, "7": 0.85 } },
    { "id": 2, "name": "Shimmer Reverb", "type": 50, "x": 260.0, "y": 60.0, "params": { "1": 12.0, "2": 6000.0, "3": 0.7, "4": 0.0, "5": 1.6, "6": 0.75 } },
    { "id": 3, "name": "M/S Decoder", "type": 25, "x": 480.0, "y": 60.0, "params": { "1": 1.5, "2": 140.0, "3": 0.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // 9. Liquid Spectral Smear Pad (Espectral & Drones Congelados)
    catalog.push_back({
        "Liquid Spectral Smear Pad",
        "Espectral & Drones Congelados",
        R"json({
  "schemaVersion": 1,
  "name": "Liquid Spectral Smear Pad",
  "author": "N8Audio",
  "category": "Espectral & Drones Congelados",
  "description": "Desvanecimiento timbrico liquido infinito mediante STFT FFT de alta densidad con difusion de fase y chorus envolvente",
  "dryLevel": 0.35,
  "wetLevel": 0.95,
  "macros": [0.8, 0.9, 0.7, 0.5, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Spectral Smear", "type": 52, "x": 40.0, "y": 60.0, "params": { "1": 6.0, "2": 0.92, "3": 0.95, "4": -0.15, "5": 0.9 } },
    { "id": 2, "name": "Dimension Chorus", "type": 17, "x": 250.0, "y": 60.0, "params": { "1": 0.6, "2": 0.5, "3": 0.2, "4": 4.0, "5": 0.6 } },
    { "id": 3, "name": "Deep Space Reverb", "type": 6, "x": 460.0, "y": 60.0, "params": { "1": 0.95, "2": 8.0, "3": 4500.0, "4": 35.0, "5": 0.65 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 }
  ]
})json"
    });

    // 10. Prismatic Ambient Halo (Ecos Cinematográficos & Paisajes)
    catalog.push_back({
        "Prismatic Ambient Halo",
        "Ecos Cinematográficos & Paisajes",
        R"json({
  "schemaVersion": 1,
  "name": "Prismatic Ambient Halo",
  "author": "N8Audio",
  "category": "Ecos Cinematográficos & Paisajes",
  "description": "Prisma acustico de 6 voces refractadas con lloro de cinta vintage y shimmer en quinta justa +7st en espacio binaural 3D",
  "dryLevel": 0.4,
  "wetLevel": 0.9,
  "macros": [0.85, 0.8, 0.75, 0.65, 0.5, 0.5, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Prism Refraction", "type": 51, "x": 40.0, "y": 60.0, "params": { "1": 6.0, "2": 16.0, "3": 18.0, "4": 0.85, "5": 0.5, "6": 1.8, "7": 0.8 } },
    { "id": 2, "name": "Tape Flutter", "type": 21, "x": 250.0, "y": 60.0, "params": { "1": 2.8, "2": 0.0, "3": 0.65, "4": 0.75, "5": 0.75 } },
    { "id": 3, "name": "Shimmer Reverb", "type": 50, "x": 460.0, "y": 60.0, "params": { "1": 9.0, "2": 7000.0, "3": 0.6, "4": 1.0, "5": 1.4, "6": 0.65 } },
    { "id": 4, "name": "Spatializer 3D", "type": 26, "x": 670.0, "y": 60.0, "params": { "1": -30.0, "2": 20.0, "3": 2.5, "4": 1.0 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 3, "srcPin": 2, "destNode": 4, "destPin": 1 }
  ]
})json"
    });

    return catalog;
}

} // namespace audio_graph
