#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Nodo Procesador Granular en tiempo real con nubes de hasta 128 granos (Reglas 5, 8, 9, 10, 11, 18, 35, 46, 47)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class GranularNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        GrainSizeMs = 1,
        Density = 2,
        PositionSprayMs = 3,
        PitchSemitones = 4,
        PitchSpray = 5,
        PanSpray = 6,
        DryWet = 7,
        ScrubPosition = 8,
        Freeze = 9,
        SpawnMode = 10,
        SpectralSensitivity = 11
    };

    struct GrainCloudPoint {
        float normPosition{ 0.0f }; // 0.0 to 1.0
        float pitchRatio{ 1.0f };
        float pan{ 0.5f };
        float envelope{ 0.0f };
        bool active{ false };
    };

    GranularNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { GrainSizeMs, "Grain Size", 80.0f, 10.0f, 500.0f, true };
        params_[1] = { Density, "Density", 15.0f, 1.0f, 60.0f, true };
        params_[2] = { PositionSprayMs, "Pos Spray", 20.0f, 0.0f, 200.0f, true };
        params_[3] = { PitchSemitones, "Pitch", 0.0f, -24.0f, 24.0f, true };
        params_[4] = { PitchSpray, "Pitch Spray", 0.0f, 0.0f, 12.0f, true };
        params_[5] = { PanSpray, "Pan Spray", 0.5f, 0.0f, 1.0f, true };
        params_[6] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };
        params_[7] = { ScrubPosition, "Scrub Pos", 0.0f, 0.0f, 1.0f, true };
        params_[8] = { Freeze, "Freeze", 0.0f, 0.0f, 1.0f, true };
        params_[9] = { SpawnMode, "Spawn Mode", 0.0f, 0.0f, 2.0f, false };
        params_[10] = { SpectralSensitivity, "Spectral Sens", 0.5f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_granular_create(44100.0);
    }

    ~GranularNode() override {
        if (rustInstance_) {
            n8_rust_granular_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    GranularNode(const GranularNode&) = delete;
    GranularNode& operator=(const GranularNode&) = delete;

    GranularNode(GranularNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    GranularNode& operator=(GranularNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_granular_destroy(rustInstance_);
            }
            pins_ = other.pins_;
            params_ = other.params_;
            spec_ = other.spec_;
            rustInstance_ = other.rustInstance_;
            other.rustInstance_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_) {
            n8_rust_granular_prepare(rustInstance_, spec.sampleRate);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_granular_reset(rustInstance_);
        }
    }

    void process(ProcessContext& context) override {
        if (!rustInstance_ || context.numSamples == 0 || context.numInputChannels == 0 || context.numOutputChannels == 0) return;

        const float* inL = context.inputChannels[0];
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : context.inputChannels[0];
        float* outL = context.outputChannels[0];
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : context.outputChannels[0];

        n8_rust_granular_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_) {
            n8_rust_granular_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_granular_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::Granular; }
    const char* getName() const override { return "Granular"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 0.5);
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    size_t copyWaveformOverview(float* outMinMax, size_t numPairs) const noexcept {
        if (!rustInstance_ || outMinMax == nullptr || numPairs == 0) return 0;
        return n8_rust_granular_copy_waveform_overview(rustInstance_, outMinMax, numPairs);
    }

    size_t copyActiveGrainsSnapshot(std::span<GrainCloudPoint> outPoints) const noexcept {
        if (!rustInstance_ || outPoints.empty()) return 0;
        static_assert(sizeof(GrainCloudPoint) == sizeof(RustGrainCloudPoint), "Layout mismatch in GrainCloudPoint");
        return n8_rust_granular_copy_active_grains_snapshot(
            rustInstance_,
            reinterpret_cast<RustGrainCloudPoint*>(outPoints.data()),
            outPoints.size()
        );
    }

    float getWritePositionNormalized() const noexcept {
        if (!rustInstance_) return 0.0f;
        return n8_rust_granular_get_write_position_normalized(rustInstance_);
    }

private:
    RustGranular* rustInstance_{ nullptr };
    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 11> params_;
};

inline AutoRegisterNode<GranularNode> registerGranular(NodeType::Granular, "granular", "Granular");

} // namespace audio_graph
