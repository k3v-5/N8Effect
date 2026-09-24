#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include <numbers>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../core/FastMath.h"

namespace audio_graph {

/**
 * @brief Modulador en Anillo de 4 Cuadrantes con Oscilador Multiforma (Reglas 5, 8, 14, 34, 46, 47)
 */
class RingModulatorNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Frequency = 1,
        Waveform = 2, // 0: Sine, 1: Triangle, 2: Saw, 3: Square
        CarrierBleed = 3,
        Mix = 4
    };

    RingModulatorNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Frequency, "Freq", 440.0f, 1.0f, 5000.0f, true };
        params_[1] = { Waveform, "Wave", 0.0f, 0.0f, 3.0f, false };
        params_[2] = { CarrierBleed, "Bleed", 0.0f, 0.0f, 1.0f, true };
        params_[3] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        reset();
    }

    void reset() override {
        carrierPhase_ = 0.0f;
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const float freq = targetFreq_;
        const int wave = targetWave_;
        const float bleed = targetBleed_;
        const float mix = targetMix_;

        const float phaseInc = (2.0f * std::numbers::pi_v<float> * freq) / static_cast<float>(spec_.sampleRate);
        const uint32_t channels = std::min(context.numOutputChannels, static_cast<uint32_t>(2));

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            // Generar forma de onda portadora
            float carrier = 0.0f;
            const float normPhase = carrierPhase_ / (2.0f * std::numbers::pi_v<float>); // 0..1

            switch (wave) {
                case 0: // Seno
                    carrier = std::sin(carrierPhase_);
                    break;
                case 1: // Triángulo
                    carrier = (normPhase < 0.5f) ? (4.0f * normPhase - 1.0f) : (3.0f - 4.0f * normPhase);
                    break;
                case 2: // Sierra
                    carrier = 2.0f * normPhase - 1.0f;
                    break;
                case 3: // Cuadrada
                    carrier = (normPhase < 0.5f) ? 1.0f : -1.0f;
                    break;
                default:
                    carrier = std::sin(carrierPhase_);
                    break;
            }

            for (uint32_t ch = 0; ch < channels; ++ch) {
                const float in = (ch < context.numInputChannels && context.inputChannels[ch] != nullptr)
                    ? context.inputChannels[ch][s]
                    : 0.0f;

                // Modulación en 4 cuadrantes balanceada con carrier bleed
                const float ringMod = in * carrier + bleed * carrier * 0.2f;

                context.outputChannels[ch][s] = (1.0f - mix) * in + mix * ringMod;
            }

            carrierPhase_ += phaseInc;
            if (carrierPhase_ >= 2.0f * std::numbers::pi_v<float>) {
                carrierPhase_ -= 2.0f * std::numbers::pi_v<float>;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Frequency: targetFreq_ = std::clamp(value, 1.0f, 5000.0f); break;
            case Waveform: targetWave_ = std::clamp(static_cast<int>(std::round(value)), 0, 3); break;
            case CarrierBleed: targetBleed_ = std::clamp(value, 0.0f, 1.0f); break;
            case Mix: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Frequency: return targetFreq_;
            case Waveform: return static_cast<float>(targetWave_);
            case CarrierBleed: return targetBleed_;
            case Mix: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::RingModulator; }
    const char* getName() const override { return "RingModulator"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    float carrierPhase_{ 0.0f };

    float targetFreq_{ 440.0f };
    int targetWave_{ 0 };
    float targetBleed_{ 0.0f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 4> params_;
};

inline AutoRegisterNode<RingModulatorNode> registerRingModulator(NodeType::RingModulator, "ringmod", "Modulation");

} // namespace audio_graph
