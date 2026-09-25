#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

namespace audio_graph {

// Identificadores numéricos estables (Regla 8: sin strings mágicos dispersos)
using NodeId = uint32_t;
using PinId = uint32_t;
using ConnectionId = uint32_t;
using ParameterId = uint32_t;

constexpr NodeId InvalidNodeId = 0;
constexpr PinId InvalidPinId = 0;
constexpr ParameterId InvalidParameterId = 0;

enum class NodeType : uint32_t {
    Unknown = 0,
    Input = 1,
    Output = 2,
    Passthrough = 3,
    Filter = 4,
    Delay = 5,
    Reverb = 6,
    Granular = 7,
    Distortion = 8,
    Compressor = 9,
    Container = 10,
    PitchShifter = 11,
    Spectral = 12,
    Resonator = 13,
    Glitch = 14,
    Multiband = 15,
    Phaser = 16,
    Chorus = 17,
    Flanger = 18,
    RingModulator = 19,
    FrequencyShifter = 20,
    Tape = 21,
    Feedback = 22,
    EventContainer = 23,
    MidSideEncoder = 24,
    MidSideDecoder = 25,
    SpatialPanner = 26,
    ParametricEQ = 27,
    AdvancedDelay = 28,
    SpectralProcessor = 29,
    TapeStop = 30,
    FormantFilter = 31,
    NoiseTexture = 32,
    TransientShaper = 33,
    RotarySpeaker = 34,
    HarmonicExciter = 35,
    Vocoder = 36,
    KarplusStrong = 37,
    ReverseReverb = 38,
    BrickwallLimiter = 39,
    Bitcrusher = 40,
    NoiseGate = 41,
    DeEsser = 42,
    MidiArpeggiator = 43,
    MidiChordEngine = 44,
    MidiScaleQuantizer = 45,
    ExternalSidechain = 46,
    Custom = 100
};

constexpr PinId SidechainPinId = 3;

enum class PinType : uint8_t {
    AudioInput,
    AudioOutput,
    EventInput,
    EventOutput,
    ModulationInput,
    ModulationOutput
};

enum class PinDataType : uint8_t {
    AudioStereo,
    AudioMono,
    EventMessage,
    ModulationScalar
};

// Ciclo de vida explícito de eventos (Regla 2)
enum class EventLifecycleState : uint8_t {
    Created,
    Playing,
    Releasing,
    Frozen,
    Looping,
    Killed,
    Destroyed
};

// Comportamiento al desaparecer la fuente (Regla 3)
enum class SourceDisappearanceBehavior : uint8_t {
    Cut,
    Short,
    Fade,
    Natural,
    Hold,
    Freeze
};

// Especificación de preparación independiente del host (Reglas 14 y 15)
struct ProcessSpec {
    double sampleRate{ 44100.0 };
    uint32_t maximumBlockSize{ 512 };
    uint32_t numInputChannels{ 2 };
    uint32_t numOutputChannels{ 2 };
};

// Contexto de procesamiento en tiempo real (Regla 9: sin allocations)
struct ProcessContext {
    const float* const* inputChannels{ nullptr };
    float* const* outputChannels{ nullptr };
    uint32_t numInputChannels{ 0 };
    uint32_t numOutputChannels{ 0 };
    uint32_t numSamples{ 0 };

    // Sidechain modular e inter-nodal (Regla 6)
    const float* const* sidechainChannels{ nullptr };
    uint32_t numSidechainChannels{ 0 };
    
    // Información de sincronización del host (Regla 37)
    double bpm{ 120.0 };
    double ppqPosition{ 0.0 };
    bool isPlaying{ false };
};

} // namespace audio_graph
