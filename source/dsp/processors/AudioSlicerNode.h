#pragma once

#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <span>
#include "../../core/Types.h"
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Buffer Audio Slicer & Frozen Granular Playground (Nodo DSP #49).
 * Captura audio continuo en un buffer circular de 3 segundos, permitiendo slicing rítmico,
 * saltos de rebanada, scrubbing, stutters, reproducción inversa y síntesis granular congelada.
 * Acelerado con núcleo en Rust de alto rendimiento (Reglas 5, 8, 9, 14, 34, 35, 46, 47).
 */
class AudioSlicerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Freeze = 1,              // 0.0: Captura continua / Grabación, 1.0: Congelado en memoria
        SliceCount = 2,          // 4, 8, 16, 32 rebanadas
        ActiveSlice = 3,         // -1: Automático secuencial, 0..N: Rebanada fija en bucle
        PlaybackSpeed = 4,       // -2.0 a +2.0 (Valores negativos reproducen en reversa)
        PositionScrub = 5,       // 0.0 a 1.0 (Posición de reproducción dentro del buffer)
        GrainSizeMs = 6,         // 10.0 a 500.0 ms
        GrainDensity = 7,        // 1 a 8 voces de granos concurrentes
        Jitter = 8,              // 0.0 a 1.0 (Dispersión aleatoria de posición de granos)
        PitchShiftSemitones = 9, // -24.0 a +24.0 semitonos
        FilterCutoff = 10,       // 20.0 a 20000.0 Hz
        Mix = 11                 // 0.0 a 1.0 (Dry / Wet)
    };

    struct GrainVoice {
        bool active{ false };
        double bufferPos{ 0.0 };
        double playbackRate{ 1.0 };
        size_t currentSample{ 0 };
        size_t totalSamples{ 2400 };
    };

    static constexpr size_t MaxGrains = 8;

    AudioSlicerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { SidechainPinId, "Trigger In", PinType::AudioInput, PinDataType::AudioMono };
        pins_[3] = { AudioRateModPinId, "Position Mod In", PinType::AudioRateModulationInput, PinDataType::AudioRateSignal };

        params_[0] = { Freeze, "Freeze", 0.0f, 0.0f, 1.0f, false };
        params_[1] = { SliceCount, "Slices", 8.0f, 2.0f, 32.0f, false };
        params_[2] = { ActiveSlice, "Active Slice", -1.0f, -1.0f, 31.0f, false };
        params_[3] = { PlaybackSpeed, "Speed", 1.0f, -2.0f, 2.0f, true };
        params_[4] = { PositionScrub, "Position", 0.0f, 0.0f, 1.0f, true };
        params_[5] = { GrainSizeMs, "Grain Size", 80.0f, 10.0f, 500.0f, true };
        params_[6] = { GrainDensity, "Density", 4.0f, 1.0f, 8.0f, false };
        params_[7] = { Jitter, "Jitter", 0.15f, 0.0f, 1.0f, true };
        params_[8] = { PitchShiftSemitones, "Pitch", 0.0f, -24.0f, 24.0f, true };
        params_[9] = { FilterCutoff, "Cutoff", 18000.0f, 20.0f, 20000.0f, true };
        params_[10] = { Mix, "Mix", 0.85f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_audio_slicer_create(48000.0);
    }

    ~AudioSlicerNode() override {
        if (rustInstance_) {
            n8_rust_audio_slicer_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    AudioSlicerNode(const AudioSlicerNode&) = delete;
    AudioSlicerNode& operator=(const AudioSlicerNode&) = delete;

    AudioSlicerNode(AudioSlicerNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    AudioSlicerNode& operator=(AudioSlicerNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_audio_slicer_destroy(rustInstance_);
            }
            pins_ = other.pins_;
            params_ = other.params_;
            spec_ = other.spec_;
            rustInstance_ = other.rustInstance_;
            other.rustInstance_ = nullptr;
        }
        return *this;
    }

    NodeType getType() const noexcept override { return NodeType::AudioSlicer; }
    const char* getName() const noexcept override { return "Audio Slicer"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_) {
            n8_rust_audio_slicer_prepare(rustInstance_, spec.sampleRate);
        }
    }

    void reset() noexcept override {
        if (rustInstance_) {
            n8_rust_audio_slicer_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) noexcept override {
        if (rustInstance_) {
            n8_rust_audio_slicer_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const noexcept override {
        if (rustInstance_) {
            return n8_rust_audio_slicer_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    std::span<const PinDescriptor> getPins() const noexcept override { return pins_; }
    std::span<const ParameterInfo> getParameters() const noexcept override { return params_; }

    void process(ProcessContext& ctx) noexcept override {
        if (!rustInstance_ || ctx.numSamples == 0) return;

        const float* inL = (ctx.numInputChannels > 0 && ctx.inputChannels[0]) ? ctx.inputChannels[0] : nullptr;
        const float* inR = (ctx.numInputChannels > 1 && ctx.inputChannels[1]) ? ctx.inputChannels[1] : inL;
        float* outL = (ctx.numOutputChannels > 0 && ctx.outputChannels[0]) ? ctx.outputChannels[0] : nullptr;
        float* outR = (ctx.numOutputChannels > 1 && ctx.outputChannels[1]) ? ctx.outputChannels[1] : outL;

        if (!outL || !inL) return;

        n8_rust_audio_slicer_process(rustInstance_, inL, inR, outL, outR, ctx.numSamples);
    }

    void getWaveformThumbnail(std::vector<float>& outPoints, size_t numPoints) const {
        outPoints.assign(numPoints, 0.0f);
        if (!rustInstance_ || numPoints == 0) return;
        n8_rust_audio_slicer_copy_waveform_thumbnail(rustInstance_, outPoints.data(), numPoints);
    }

private:
    RustAudioSlicer* rustInstance_{ nullptr };
    ProcessSpec spec_{ 48000.0, 256, 2, 2 };
    std::array<PinDescriptor, 4> pins_;
    std::array<ParameterInfo, 11> params_;
};

inline AutoRegisterNode<AudioSlicerNode> registerAudioSlicer(NodeType::AudioSlicer, "audioslicer", "Glitch & Granular");

} // namespace audio_graph
