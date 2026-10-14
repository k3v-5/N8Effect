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
 * @brief Pitch Delay & Crystals Shifter (Phase 9 - Large Effect Library, Rules 5, 8, 9, 14, 18, 34, 46, 47)
 * Delay con transpositor de pitch granular estilo Eventide Crystals, micro-detuning y cross-feedback.
 */
class PitchDelayNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        DelayMsL = 1,
        DelayMsR = 2,
        PitchSemitones = 3,
        DetuneCents = 4,
        Feedback = 5,
        ReverseMode = 6,
        DampingHz = 7,
        Mix = 8
    };

    PitchDelayNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { DelayMsL, "Time L", 375.0f, 1.0f, 2000.0f, true };
        params_[1] = { DelayMsR, "Time R", 500.0f, 1.0f, 2000.0f, true };
        params_[2] = { PitchSemitones, "Pitch", 12.0f, -24.0f, 24.0f, true };
        params_[3] = { DetuneCents, "Detune", 7.0f, -50.0f, 50.0f, true };
        params_[4] = { Feedback, "Feedback", 0.45f, 0.0f, 0.95f, true };
        params_[5] = { ReverseMode, "Reverse", 0.0f, 0.0f, 1.0f, false };
        params_[6] = { DampingHz, "Damping", 8000.0f, 500.0f, 20000.0f, true };
        params_[7] = { Mix, "Mix", 0.5f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_pitch_delay_create(44100.0f);
    }

    ~PitchDelayNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_pitch_delay_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::PitchDelay; }
    const char* getName() const override { return "Pitch Delay"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override { return static_cast<uint32_t>(spec_.sampleRate * 4.0); }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_pitch_delay_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_pitch_delay_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_pitch_delay_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_pitch_delay_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case DelayMsL: return params_[0].defaultValue;
            case DelayMsR: return params_[1].defaultValue;
            case PitchSemitones: return params_[2].defaultValue;
            case DetuneCents: return params_[3].defaultValue;
            case Feedback: return params_[4].defaultValue;
            case ReverseMode: return params_[5].defaultValue;
            case DampingHz: return params_[6].defaultValue;
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

        n8_rust_pitch_delay_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    ProcessSpec spec_;
    RustPitchDelay* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 8> params_;
};

inline AutoRegisterNode<PitchDelayNode> registerPitchDelay(NodeType::PitchDelay, "pitchdelay", "Delay");

} // namespace audio_graph
