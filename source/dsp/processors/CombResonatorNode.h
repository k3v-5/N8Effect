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
 * @brief Comb Filter Resonator (Phase 9 - Large Effect Library, Rules 5, 8, 9, 12, 34, 46, 47)
 * Resonador de peine sintonizado con feedback bipolar, amortiguamiento acústico y detune estéreo.
 */
class CombResonatorNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        FreqHz = 1,
        Feedback = 2,
        DampingHz = 3,
        StereoDetuneCents = 4,
        Mix = 5
    };

    CombResonatorNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { FreqHz, "Freq", 220.0f, 20.0f, 5000.0f, true };
        params_[1] = { Feedback, "Feedback", 0.85f, -0.98f, 0.98f, true };
        params_[2] = { DampingHz, "Damping", 8000.0f, 500.0f, 20000.0f, true };
        params_[3] = { StereoDetuneCents, "Detune", 12.0f, 0.0f, 50.0f, true };
        params_[4] = { Mix, "Mix", 0.5f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_comb_resonator_create(44100.0f);
    }

    ~CombResonatorNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_comb_resonator_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::CombResonator; }
    const char* getName() const override { return "Comb Resonator"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_comb_resonator_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_comb_resonator_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_comb_resonator_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_comb_resonator_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case FreqHz: return params_[0].defaultValue;
            case Feedback: return params_[1].defaultValue;
            case DampingHz: return params_[2].defaultValue;
            case StereoDetuneCents: return params_[3].defaultValue;
            case Mix: return params_[4].defaultValue;
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

        n8_rust_comb_resonator_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    ProcessSpec spec_;
    RustCombResonator* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
};

inline AutoRegisterNode<CombResonatorNode> registerCombResonator(NodeType::CombResonator, "combresonator", "Resonator");

} // namespace audio_graph
