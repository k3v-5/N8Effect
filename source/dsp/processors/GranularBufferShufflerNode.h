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
 * @brief Granular Buffer Shuffler & Reverse Memory Node (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Motor granular estocástico de memoria temporal circular que extrae granos con dirección normal/reversa,
 * saltos de tono armónicos y distribución espacial panorámica.
 */
class GranularBufferShufflerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        BufferLengthMs = 1,
        GrainSizeMs = 2,
        Density = 3,
        ReverseProb = 4,
        PitchMode = 5,
        ScatterPan = 6,
        Freeze = 7,
        Mix = 8
    };

    GranularBufferShufflerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { BufferLengthMs, "Buffer Length (ms)", 1200.0f, 200.0f, 2500.0f, true };
        params_[1] = { GrainSizeMs, "Grain Size (ms)", 120.0f, 20.0f, 400.0f, true };
        params_[2] = { Density, "Grain Density", 12.0f, 1.0f, 30.0f, true };
        params_[3] = { ReverseProb, "Reverse Probability", 0.35f, 0.0f, 1.0f, true };
        params_[4] = { PitchMode, "Pitch Mode (0=Norm, 1=+Oct, 2=-Oct, 3=+5th, 4=Rnd)", 0.0f, 0.0f, 4.0f, true };
        params_[5] = { ScatterPan, "Stereo Scatter Pan", 0.70f, 0.0f, 1.0f, true };
        params_[6] = { Freeze, "Buffer Freeze", 0.0f, 0.0f, 1.0f, true };
        params_[7] = { Mix, "Mix", 0.75f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_granular_buffer_shuffler_create(44100.0f);
    }

    ~GranularBufferShufflerNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_granular_buffer_shuffler_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::GranularBufferShuffler; }
    const char* getName() const override { return "Granular Buffer Shuffler"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_granular_buffer_shuffler_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_granular_buffer_shuffler_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_granular_buffer_shuffler_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_granular_buffer_shuffler_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case BufferLengthMs: return params_[0].defaultValue;
            case GrainSizeMs: return params_[1].defaultValue;
            case Density: return params_[2].defaultValue;
            case ReverseProb: return params_[3].defaultValue;
            case PitchMode: return params_[4].defaultValue;
            case ScatterPan: return params_[5].defaultValue;
            case Freeze: return params_[6].defaultValue;
            case Mix: return params_[7].defaultValue;
            default: return 0.0f;
        }
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (outL == nullptr || rustInstance_ == nullptr || inL == nullptr) return;

        n8_rust_granular_buffer_shuffler_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustGranularBufferShuffler* rustInstance_{ nullptr };
    ProcessSpec spec_{};
    std::array<PinDescriptor, 2> pins_{};
    std::array<ParameterInfo, 8> params_{};
};

inline AutoRegisterNode<GranularBufferShufflerNode> registerGranularBufferShuffler(
    NodeType::GranularBufferShuffler, "granularbuffershuffler", "Granular");

} // namespace audio_graph
