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
 * @brief Allpass Phase Dispersion Cascade / Dispersion Phantasm (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Cascada de 24 filtros allpass de 2do orden con retardo de grupo no lineal, feedback saturado y amortiguamiento.
 * Convierte transientes en chirps lÃ¡ser, genera colas de difusiÃ³n lÃ­quida y halos acÃºsticos extraterrestres.
 */
class PhaseDispersionNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Stages = 1,
        DispersionFreq = 2,
        Spread = 3,
        DampingHz = 4,
        Feedback = 5,
        Mix = 6
    };

    PhaseDispersionNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Stages, "Stages", 16.0f, 4.0f, 24.0f, false };
        params_[1] = { DispersionFreq, "Disp Freq", 1200.0f, 80.0f, 14000.0f, true };
        params_[2] = { Spread, "Spread", 0.65f, 0.05f, 0.98f, true };
        params_[3] = { DampingHz, "Damping", 9000.0f, 500.0f, 20000.0f, true };
        params_[4] = { Feedback, "Feedback", 0.4f, -0.95f, 0.95f, true };
        params_[5] = { Mix, "Mix", 0.7f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_phase_dispersion_create(44100.0f);
    }

    ~PhaseDispersionNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_phase_dispersion_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::PhaseDispersion; }
    const char* getName() const override { return "Phase Dispersion"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_phase_dispersion_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_phase_dispersion_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_phase_dispersion_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_phase_dispersion_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Stages: return params_[0].defaultValue;
            case DispersionFreq: return params_[1].defaultValue;
            case Spread: return params_[2].defaultValue;
            case DampingHz: return params_[3].defaultValue;
            case Feedback: return params_[4].defaultValue;
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

        n8_rust_phase_dispersion_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    ProcessSpec spec_;
    RustPhaseDispersion* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<PhaseDispersionNode> registerPhaseDispersion(NodeType::PhaseDispersion, "phasedispersion", "Filter");

} // namespace audio_graph
