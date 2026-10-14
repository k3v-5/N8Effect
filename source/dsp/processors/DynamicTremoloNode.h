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
 * @brief Dynamic Tremolo & Harmonic Auto-Pan (Phase 9 - Large Effect Library, Rules 5, 8, 9, 34, 46, 47)
 * Tremolo óptico y armónico con modulación de velocidad por envolvente dinámica y control de fase estéreo.
 */
class DynamicTremoloNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        RateHz = 1,
        Depth = 2,
        Shape = 3,          // 0: Sine, 1: Optical, 2: Triangle, 3: Harmonic
        StereoPhaseDeg = 4, // 0 deg tremolo, 90 deg autopan, 180 deg pingpong
        Dynamics = 5,       // -1.0 to +1.0 envelope rate warp
        Mix = 6
    };

    DynamicTremoloNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { RateHz, "Rate", 4.0f, 0.1f, 20.0f, true };
        params_[1] = { Depth, "Depth", 0.7f, 0.0f, 1.0f, true };
        params_[2] = { Shape, "Shape", 0.0f, 0.0f, 3.0f, false };
        params_[3] = { StereoPhaseDeg, "Phase", 90.0f, 0.0f, 360.0f, true };
        params_[4] = { Dynamics, "Dynamics", 0.3f, -1.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_dynamic_tremolo_create(44100.0f);
    }

    ~DynamicTremoloNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_dynamic_tremolo_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::DynamicTremolo; }
    const char* getName() const override { return "Dynamic Tremolo"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_dynamic_tremolo_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_dynamic_tremolo_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_dynamic_tremolo_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_dynamic_tremolo_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case RateHz: return params_[0].defaultValue;
            case Depth: return params_[1].defaultValue;
            case Shape: return params_[2].defaultValue;
            case StereoPhaseDeg: return params_[3].defaultValue;
            case Dynamics: return params_[4].defaultValue;
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

        n8_rust_dynamic_tremolo_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    ProcessSpec spec_;
    RustDynamicTremolo* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<DynamicTremoloNode> registerDynamicTremolo(NodeType::DynamicTremolo, "dynamictremolo", "Modulation");

} // namespace audio_graph
