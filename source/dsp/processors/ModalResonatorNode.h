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
 * @brief Resonador Físico Modal de 8 Modos para Modelado Acústico Acelerado en Rust (Reglas 5, 8, 9, 14, 34, 46, 47)
 * Modela cuerpos físicos (Campana metálica, Madera/Marimba, Membrana de tambor y Cuerda) con inarmonicidad y amortiguación en Rust.
 */
class ModalResonatorNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        FundFreq = 1,
        Structure = 2, // 0: Bell, 1: Wood, 2: Drum, 3: String
        DecaySec = 3,
        Inharmonicity = 4,
        Damping = 5,
        Mix = 6
    };

    ModalResonatorNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { FundFreq, "Fund Freq", 220.0f, 20.0f, 2000.0f, true };
        params_[1] = { Structure, "Material", 0.0f, 0.0f, 3.0f, false };
        params_[2] = { DecaySec, "Decay", 1.2f, 0.05f, 6.0f, true };
        params_[3] = { Inharmonicity, "Inharmonic", 0.15f, 0.0f, 1.0f, true };
        params_[4] = { Damping, "Damping", 0.35f, 0.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 0.6f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_modal_resonator_create(48000.0);
    }

    ~ModalResonatorNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_modal_resonator_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::ModalResonator; }
    const char* getName() const override { return "Modal Resonator"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_modal_resonator_set_sample_rate(rustInstance_, spec.sampleRate);
            n8_rust_modal_resonator_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_modal_resonator_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_modal_resonator_set_parameter(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_ != nullptr) {
            return n8_rust_modal_resonator_get_parameter(rustInstance_, static_cast<uint32_t>(id));
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

        n8_rust_modal_resonator_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    ProcessSpec spec_;
    RustModalResonator* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<ModalResonatorNode> registerModalResonator(NodeType::ModalResonator, "modalresonator", "Resonator");

} // namespace audio_graph
