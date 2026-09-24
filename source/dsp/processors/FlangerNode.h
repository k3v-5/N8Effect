#pragma once

#include <cmath>
#include <array>
#include <vector>
#include <span>
#include <algorithm>
#include <numbers>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../core/FastMath.h"

namespace audio_graph {

/**
 * @brief Procesador Flanger con Feedback Bipolar y Peine Profundo (Reglas 5, 8, 14, 34, 46, 47)
 */
class FlangerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Rate = 1,
        Depth = 2,
        DelayMs = 3,
        Feedback = 4,
        Mix = 5
    };

    FlangerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Rate, "Rate", 0.25f, 0.05f, 5.0f, true };
        params_[1] = { Depth, "Depth", 0.8f, 0.0f, 1.0f, true };
        params_[2] = { DelayMs, "Delay", 2.0f, 0.2f, 10.0f, true };
        params_[3] = { Feedback, "Feedback", 0.7f, -0.95f, 0.95f, true };
        params_[4] = { Mix, "Mix", 0.5f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        // Bounded delay buffer: máximo 20 ms a 192 kHz = 3840 muestras (Regla 47)
        maxDelaySamples_ = static_cast<size_t>(spec.sampleRate * 0.02);
        delayBuffers_[0].assign(maxDelaySamples_, 0.0f);
        delayBuffers_[1].assign(maxDelaySamples_, 0.0f);
        writeIndex_ = 0;
        lfoPhase_ = 0.0f;
        feedbackSample_.fill(0.0f);
    }

    void reset() override {
        for (auto& buf : delayBuffers_) std::fill(buf.begin(), buf.end(), 0.0f);
        feedbackSample_.fill(0.0f);
        writeIndex_ = 0;
        lfoPhase_ = 0.0f;
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        if (delayBuffers_[0].empty()) return;

        const float rate = targetRate_;
        const float depth = targetDepth_;
        const float baseDelayMs = targetDelayMs_;
        const float feedback = targetFeedback_;
        const float mix = targetMix_;

        const float lfoInc = (2.0f * std::numbers::pi_v<float> * rate) / static_cast<float>(spec_.sampleRate);
        const float maxModMs = baseDelayMs * depth * 0.85f;

        const uint32_t channels = std::min(context.numOutputChannels, static_cast<uint32_t>(2));

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            // LFO triangular/senoidal suave
            const float lfoL = 0.5f + 0.5f * std::sin(lfoPhase_);
            const float lfoR = 0.5f + 0.5f * std::sin(lfoPhase_ + std::numbers::pi_v<float> * 0.5f);

            for (uint32_t ch = 0; ch < channels; ++ch) {
                const float in = (ch < context.numInputChannels && context.inputChannels[ch] != nullptr)
                    ? context.inputChannels[ch][s]
                    : 0.0f;

                const float lfoVal = (ch == 0) ? lfoL : lfoR;
                const float totalDelayMs = std::max(0.1f, baseDelayMs + maxModMs * (2.0f * lfoVal - 1.0f));
                const float delaySamples = totalDelayMs * 0.001f * static_cast<float>(spec_.sampleRate);

                // Escribir en buffer de retardo con feedback saturado
                const float writeVal = in + feedbackSample_[ch] * feedback;
                delayBuffers_[ch][writeIndex_] = FastMath::fastSoftClip(writeVal);

                // Lectura con interpolación lineal
                const float wet = readInterpolated(ch, delaySamples);
                feedbackSample_[ch] = wet;

                // Salida combinada
                context.outputChannels[ch][s] = (1.0f - mix) * in + mix * wet;
            }

            writeIndex_ = (writeIndex_ + 1) % maxDelaySamples_;
            lfoPhase_ += lfoInc;
            if (lfoPhase_ >= 2.0f * std::numbers::pi_v<float>) {
                lfoPhase_ -= 2.0f * std::numbers::pi_v<float>;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Rate: targetRate_ = std::clamp(value, 0.05f, 5.0f); break;
            case Depth: targetDepth_ = std::clamp(value, 0.0f, 1.0f); break;
            case DelayMs: targetDelayMs_ = std::clamp(value, 0.2f, 10.0f); break;
            case Feedback: targetFeedback_ = std::clamp(value, -0.95f, 0.95f); break;
            case Mix: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Rate: return targetRate_;
            case Depth: return targetDepth_;
            case DelayMs: return targetDelayMs_;
            case Feedback: return targetFeedback_;
            case Mix: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Flanger; }
    const char* getName() const override { return "Flanger"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    float readInterpolated(size_t ch, float delaySamples) const noexcept {
        const float readPos = static_cast<float>(writeIndex_) - delaySamples;
        float wrappedPos = std::fmod(readPos, static_cast<float>(maxDelaySamples_));
        if (wrappedPos < 0.0f) wrappedPos += static_cast<float>(maxDelaySamples_);

        const size_t idx0 = static_cast<size_t>(wrappedPos);
        const size_t idx1 = (idx0 + 1) % maxDelaySamples_;
        const float frac = wrappedPos - static_cast<float>(idx0);

        return (1.0f - frac) * delayBuffers_[ch][idx0] + frac * delayBuffers_[ch][idx1];
    }

    ProcessSpec spec_;
    size_t maxDelaySamples_{ 1920 };
    std::array<std::vector<float>, 2> delayBuffers_;
    size_t writeIndex_{ 0 };
    std::array<float, 2> feedbackSample_{ 0.0f, 0.0f };
    float lfoPhase_{ 0.0f };

    float targetRate_{ 0.25f };
    float targetDepth_{ 0.8f };
    float targetDelayMs_{ 2.0f };
    float targetFeedback_{ 0.7f };
    float targetMix_{ 0.5f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
};

inline AutoRegisterNode<FlangerNode> registerFlanger(NodeType::Flanger, "flanger", "Modulation");

} // namespace audio_graph
