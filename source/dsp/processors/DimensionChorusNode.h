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
 * @brief Roland Dimension D / Juno BBD Chorus Emulation (Phase 9 - Large Effect Library, Rules 5, 8, 9, 34, 46, 47)
 * Proporciona ensanchamiento espacial estéreo tridimensional con 4 botones de modo BBD y cross-canceling analógico.
 */
class DimensionChorusNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Mode = 1,
        RateHz = 2,
        Depth = 3,
        StereoSpread = 4,
        Drive = 5,
        Mix = 6
    };

    DimensionChorusNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Mode, "Mode", 2.0f, 0.0f, 4.0f, false };
        params_[1] = { RateHz, "Rate", 0.5f, 0.05f, 5.0f, true };
        params_[2] = { Depth, "Depth", 0.6f, 0.0f, 1.0f, true };
        params_[3] = { StereoSpread, "Spread", 1.0f, 0.0f, 2.0f, true };
        params_[4] = { Drive, "Drive", 0.2f, 0.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 0.5f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_dimension_chorus_create(44100.0f);
    }

    ~DimensionChorusNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_dimension_chorus_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::DimensionChorus; }
    const char* getName() const override { return "Dimension Chorus"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_dimension_chorus_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_dimension_chorus_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_dimension_chorus_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_dimension_chorus_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Mode: return params_[0].defaultValue;
            case RateHz: return params_[1].defaultValue;
            case Depth: return params_[2].defaultValue;
            case StereoSpread: return params_[3].defaultValue;
            case Drive: return params_[4].defaultValue;
            case Mix: return params_[5].defaultValue;
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

        n8_rust_dimension_chorus_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    ProcessSpec spec_;
    RustDimensionChorus* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<DimensionChorusNode> registerDimensionChorus(NodeType::DimensionChorus, "dimensionchorus", "Modulation");

} // namespace audio_graph
