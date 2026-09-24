#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include <numbers>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"

namespace audio_graph {

/**
 * @brief Codificador Estéreo a Mid/Side (Reglas 5, 8, 13, 16, 34, 46, 47).
 * Transforma un par estéreo L/R en componentes Mid (centro/suma) y Side (lateral/diferencia)
 * con conservación rigurosa de potencia normalizada por 1/sqrt(2).
 */
class MidSideEncoderNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Gain = 1
    };

    MidSideEncoderNode() {
        pins_[0] = { 1, "Stereo In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Mid/Side Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Gain, "Gain", 1.0f, 0.0f, 2.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        currentGain_ = targetGain_;
    }

    void reset() override {
        currentGain_ = targetGain_;
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Reglas 34 y 47
        const float alpha = 0.005f;        // Anti-click (Regla 35)
        constexpr float invSqrt2 = 0.70710678f;

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            currentGain_ += alpha * (targetGain_ - currentGain_);

            const float inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr)
                ? context.inputChannels[0][s] : 0.0f;
            const float inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr)
                ? context.inputChannels[1][s] : inL;

            // M = (L + R) * 0.7071
            // S = (L - R) * 0.7071
            const float mid = (inL + inR) * invSqrt2 * currentGain_;
            const float side = (inL - inR) * invSqrt2 * currentGain_;

            if (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) {
                context.outputChannels[0][s] = mid;
            }
            if (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) {
                context.outputChannels[1][s] = side;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (id == Gain) {
            targetGain_ = std::clamp(value, 0.0f, 2.0f);
        }
    }

    float getParameter(ParameterId id) const override {
        if (id == Gain) return targetGain_;
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::MidSideEncoder; }
    const char* getName() const override { return "M/S Encoder"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    float targetGain_{ 1.0f };
    float currentGain_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 1> params_;
};

inline AutoRegisterNode<MidSideEncoderNode> registerMidSideEncoderNode(
    NodeType::MidSideEncoder, "M/S Encoder", "Spatial"
);

} // namespace audio_graph
