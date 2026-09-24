#pragma once

#include <cmath>
#include <array>
#include <algorithm>
#include <span>
#include "../graph/AudioProcessorNode.h"
#include "../graph/NodeFactory.h"

namespace audio_graph {

/**
 * @brief Nodo de utilidad Passthrough / Ganancia (Regla 5, 8, 35)
 */
class PassthroughNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Gain = 1
    };

    PassthroughNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        params_[0] = { Gain, "Gain", 1.0f, 0.0f, 2.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        currentGain_ = targetGain_;
    }

    void process(ProcessContext& context) override {
        const float alpha = 0.005f; // Suavizado anti-click (Regla 35)
        for (uint32_t s = 0; s < context.numSamples; ++s) {
            currentGain_ += alpha * (targetGain_ - currentGain_);
            for (uint32_t ch = 0; ch < context.numOutputChannels; ++ch) {
                float sample = (ch < context.numInputChannels && context.inputChannels[ch] != nullptr)
                    ? context.inputChannels[ch][s]
                    : 0.0f;
                context.outputChannels[ch][s] = sample * currentGain_;
            }
        }
    }

    void reset() override {
        currentGain_ = targetGain_;
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

    NodeType getType() const override { return NodeType::Passthrough; }
    const char* getName() const override { return "Gain / Passthrough"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    float targetGain_{ 1.0f };
    float currentGain_{ 1.0f };
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 1> params_;
};

// Registro automático en NodeFactory (Regla 19 y 20)
inline AutoRegisterNode<PassthroughNode> registerPassthrough(NodeType::Passthrough, "passthrough", "Utility");

} // namespace audio_graph
