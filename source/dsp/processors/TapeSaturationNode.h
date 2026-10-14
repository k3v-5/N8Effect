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
 * @brief Simulador de Cinta Magnética Analógica e Histéresis Jiles-Atherton Acelerado en Rust (Reglas 5, 8, 14, 34, 46, 47)
 * Emula la respuesta ferromagnética no lineal, head bump de graves y modulación wow/flutter en Rust SIMD.
 */
class TapeSaturationNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Drive = 1,
        TapeSpeed = 2, // 0: 7.5 ips, 1: 15 ips, 2: 30 ips
        WowFlutter = 3,
        Warmth = 4,
        Mix = 5,
        Hysteresis = 6 // Intensidad de memoria magnética Jiles-Atherton (0.0 a 1.0)
    };

    TapeSaturationNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Drive, "Drive", 2.0f, 1.0f, 10.0f, true };
        params_[1] = { TapeSpeed, "Speed", 1.0f, 0.0f, 2.0f, false };
        params_[2] = { WowFlutter, "Flutter", 0.25f, 0.0f, 1.0f, true };
        params_[3] = { Warmth, "Warmth", 0.5f, 0.0f, 1.0f, true };
        params_[4] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
        params_[5] = { Hysteresis, "Hysteresis", 0.5f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_tape_saturation_create(44100.0);
    }

    ~TapeSaturationNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_tape_saturation_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::Tape; }
    const char* getName() const override { return "Tape"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_tape_saturation_set_sample_rate(rustInstance_, spec.sampleRate);
            n8_rust_tape_saturation_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_tape_saturation_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_tape_saturation_set_parameter(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_ != nullptr) {
            return n8_rust_tape_saturation_get_parameter(rustInstance_, static_cast<uint32_t>(id));
        }
        return 0.0f;
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

        n8_rust_tape_saturation_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    ProcessSpec spec_;
    RustTapeSaturation* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<TapeSaturationNode> registerTape(NodeType::Tape, "tape", "Distortion");

} // namespace audio_graph
