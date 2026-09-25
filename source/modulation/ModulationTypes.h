#pragma once

#include <cstdint>
#include <string_view>
#include "../core/Types.h"

namespace audio_graph {

// Identificador de fuentes de modulación universales (Regla 7)
enum class ModSourceType : uint16_t {
    None = 0,
    LFO1,
    LFO2,
    LFO3,
    LFO4,
    Envelope1,
    Envelope2,
    StepSeq,
    AudioFollower,
    Random,
    MacroTexture,
    MacroMotion,
    MacroSpace,
    MacroColor,
    MacroChaos,
    MacroDensity,
    MacroEnergy,
    MacroMorph,
    PadX,
    PadY,
    AnalysisTransient,
    AnalysisPitch,
    AnalysisCentroid,
    AnalysisFlux,
    MSEG1,
    MSEG2,
    Euclidean1,
    Euclidean2,
    ChaosX,
    ChaosY,
    ChaosZ,
    Count
};

// Formas de onda del LFO (Regla 28)
enum class LFOShape : uint8_t {
    Sine = 0,
    Triangle,
    SawUp,
    SawDown,
    Square,
    Pulse,
    SampleAndHold,
    SmoothRandom
};

// Divisiones métricas de sincronización con el tempo del DAW (Regla 28 y 37)
enum class SyncDivision : uint8_t {
    FreeHz = 0,
    Bar_1,      // 1 bar (4 beats en 4/4)
    Half,       // 1/2
    Quarter,    // 1/4 (1 beat)
    Eighth,     // 1/8
    Sixteenth,  // 1/16
    ThirtySecond,// 1/32
    Triplet_Quarter, // 1/4T
    Triplet_Eighth,  // 1/8T
    Dotted_Eighth,   // 1/8D
    Dotted_Quarter   // 1/4D
};

// Ruta individual dentro de la Matriz de Modulación Universal (Regla 7)
struct ModulationRoute {
    ModSourceType source{ ModSourceType::None };
    NodeId targetNodeId{ InvalidNodeId };
    ParameterId targetParamId{ InvalidParameterId };
    float amount{ 0.0f }; // -1.0f a +1.0f
    bool bipolar{ true };
    bool isActive{ false };
};

} // namespace audio_graph
