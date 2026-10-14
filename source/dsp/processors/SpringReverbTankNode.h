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
 * @brief Helical Spring Reverb Tank Node (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Emulador físico de tanque de reverberación de muelles electromagnéticos (estilo Accutronics 3-spring).
 * Modela dispersión chirp dependiente de la frecuencia (allpass ladder dispersivo), boing/twang no lineal y excitación de bobina.
 */
class SpringReverbTankNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Tension = 1,
        DecayTime = 2,
        BoingDepth = 3,
        Drive = 4,
        DampingHz = 5,
        Mix = 6
    };

    SpringReverbTankNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Tension, "Spring Tension (Dispersion)", 0.65f, 0.10f, 0.95f, true };
        params_[1] = { DecayTime, "Decay Time T60 (s)", 2.80f, 0.30f, 8.00f, true };
        params_[2] = { BoingDepth, "Boing / Chirp Intensity", 0.70f, 0.0f, 1.0f, true };
        params_[3] = { Drive, "Input Coil Drive", 1.20f, 0.50f, 4.00f, true };
        params_[4] = { DampingHz, "High Frequency Damping (Hz)", 3500.0f, 500.0f, 10000.0f, true };
        params_[5] = { Mix, "Mix", 0.60f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_spring_reverb_tank_create(44100.0f);
    }

    ~SpringReverbTankNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_spring_reverb_tank_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::SpringReverbTank; }
    const char* getName() const override { return "Spring Reverb Tank"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_spring_reverb_tank_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_spring_reverb_tank_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_spring_reverb_tank_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_spring_reverb_tank_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Tension: return params_[0].defaultValue;
            case DecayTime: return params_[1].defaultValue;
            case BoingDepth: return params_[2].defaultValue;
            case Drive: return params_[3].defaultValue;
            case DampingHz: return params_[4].defaultValue;
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

        n8_rust_spring_reverb_tank_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustSpringReverbTank* rustInstance_{ nullptr };
    ProcessSpec spec_{};
    std::array<PinDescriptor, 2> pins_{};
    std::array<ParameterInfo, 6> params_{};
};

inline AutoRegisterNode<SpringReverbTankNode> registerSpringReverbTank(
    NodeType::SpringReverbTank, "springreverbtank", "Reverb");

} // namespace audio_graph
