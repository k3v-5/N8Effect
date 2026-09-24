#pragma once

#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <span>
#include "../graph/AudioProcessorNode.h"
#include "../graph/NodeFactory.h"

namespace audio_graph {

/**
 * @brief Nodo de Delay con Feedback e interpolación suave (Reglas 5, 8, 14, 18, 35)
 */
class SimpleDelayNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        DelayTimeMs = 1,
        Feedback = 2,
        DryWet = 3
    };

    SimpleDelayNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        params_[0] = { DelayTimeMs, "Time", 250.0f, 1.0f, 2000.0f, true };
        params_[1] = { Feedback, "Feedback", 0.4f, 0.0f, 0.95f, true };
        params_[2] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        // Buffer para máximo 2.5 segundos de delay
        maxDelaySamples_ = static_cast<size_t>(spec.sampleRate * 2.5);
        bufferL_.assign(maxDelaySamples_, 0.0f);
        bufferR_.assign(maxDelaySamples_, 0.0f);
        writeIndex_ = 0;
        reset();
    }

    void reset() override {
        std::fill(bufferL_.begin(), bufferL_.end(), 0.0f);
        std::fill(bufferR_.begin(), bufferR_.end(), 0.0f);
        writeIndex_ = 0;
    }

    void process(ProcessContext& context) override {
        if (maxDelaySamples_ == 0) return;

        const float delayInSamples = std::clamp(
            (delayTimeMs_ * 0.001f) * static_cast<float>(spec_.sampleRate),
            1.0f,
            static_cast<float>(maxDelaySamples_ - 2)
        );

        const float fb = std::clamp(feedback_, 0.0f, 0.95f); // Protección contra runaway feedback (Regla 12)
        const float mix = std::clamp(dryWet_, 0.0f, 1.0f);

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            const float inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0][s] : 0.0f;
            const float inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1][s] : inL;

            // Lectura con interpolación lineal
            float readPos = static_cast<float>(writeIndex_) - delayInSamples;
            if (readPos < 0.0f) readPos += static_cast<float>(maxDelaySamples_);

            size_t idx0 = static_cast<size_t>(readPos) % maxDelaySamples_;
            size_t idx1 = (idx0 + 1) % maxDelaySamples_;
            float frac = readPos - std::floor(readPos);

            float delayedL = bufferL_[idx0] + frac * (bufferL_[idx1] - bufferL_[idx0]);
            float delayedR = bufferR_[idx0] + frac * (bufferR_[idx1] - bufferR_[idx0]);

            // Escritura en buffer circular
            bufferL_[writeIndex_] = inL + delayedL * fb;
            bufferR_[writeIndex_] = inR + delayedR * fb;

            writeIndex_ = (writeIndex_ + 1) % maxDelaySamples_;

            if (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) {
                context.outputChannels[0][s] = inL * (1.0f - mix) + delayedL * mix;
            }
            if (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) {
                context.outputChannels[1][s] = inR * (1.0f - mix) + delayedR * mix;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case DelayTimeMs: delayTimeMs_ = std::clamp(value, 1.0f, 2000.0f); break;
            case Feedback: feedback_ = std::clamp(value, 0.0f, 0.95f); break;
            case DryWet: dryWet_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case DelayTimeMs: return delayTimeMs_;
            case Feedback: return feedback_;
            case DryWet: return dryWet_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Delay; }
    const char* getName() const override { return "Stereo Delay"; }
    bool supportsTail() const override { return true; } // Soporte explícito de tails (Regla 18)
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 3.0); // Hasta 3 segundos de tail
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    size_t maxDelaySamples_{ 0 };
    size_t writeIndex_{ 0 };
    std::vector<float> bufferL_;
    std::vector<float> bufferR_;

    float delayTimeMs_{ 250.0f };
    float feedback_{ 0.4f };
    float dryWet_{ 0.5f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 3> params_;
};

inline AutoRegisterNode<SimpleDelayNode> registerSimpleDelay(NodeType::Delay, "delay", "Time");

} // namespace audio_graph
