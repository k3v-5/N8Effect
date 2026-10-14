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
 * @brief Disintegration Tape Looper Node (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Simulador de degradación magnética progresiva inspirado en "The Disintegration Loops".
 * Modela pérdida cumulativa de óxido por vuelta, wow & flutter no-lineal, dropouts aleatorios y saturación de cinta.
 */
class DisintegrationLooperNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        LoopLengthMs = 1,
        Feedback = 2,
        OxideDecay = 3,
        WowFlutter = 4,
        DropoutIntensity = 5,
        TapeWarmth = 6,
        Mix = 7
    };

    DisintegrationLooperNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { LoopLengthMs, "Loop Length (ms)", 1500.0f, 200.0f, 2800.0f, true };
        params_[1] = { Feedback, "Feedback", 0.85f, 0.10f, 1.00f, true };
        params_[2] = { OxideDecay, "Oxide Decay Rate", 0.45f, 0.0f, 1.0f, true };
        params_[3] = { WowFlutter, "Wow & Flutter", 0.35f, 0.0f, 1.0f, true };
        params_[4] = { DropoutIntensity, "Dropout Intensity", 0.25f, 0.0f, 1.0f, true };
        params_[5] = { TapeWarmth, "Tape Warmth / Sat", 0.50f, 0.0f, 1.0f, true };
        params_[6] = { Mix, "Mix", 0.70f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_disintegration_looper_create(44100.0f);
    }

    ~DisintegrationLooperNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_disintegration_looper_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::DisintegrationLooper; }
    const char* getName() const override { return "Disintegration Tape Looper"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_disintegration_looper_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_disintegration_looper_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_disintegration_looper_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_disintegration_looper_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case LoopLengthMs: return params_[0].defaultValue;
            case Feedback: return params_[1].defaultValue;
            case OxideDecay: return params_[2].defaultValue;
            case WowFlutter: return params_[3].defaultValue;
            case DropoutIntensity: return params_[4].defaultValue;
            case TapeWarmth: return params_[5].defaultValue;
            case Mix: return params_[6].defaultValue;
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

        n8_rust_disintegration_looper_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustDisintegrationLooper* rustInstance_{ nullptr };
    ProcessSpec spec_{};
    std::array<PinDescriptor, 2> pins_{};
    std::array<ParameterInfo, 7> params_{};
};

inline AutoRegisterNode<DisintegrationLooperNode> registerDisintegrationLooper(
    NodeType::DisintegrationLooper, "disintegrationlooper", "Delay");

} // namespace audio_graph
