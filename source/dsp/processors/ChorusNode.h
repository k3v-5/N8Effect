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
 * @brief Chorus Multivoz Estéreo estilo BBD Analógico (Reglas 5, 8, 14, 34, 46, 47)
 */
class ChorusNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Rate = 1,
        Depth = 2,
        Feedback = 3,
        Voices = 4,
        Mix = 5
    };

    ChorusNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Rate, "Rate", 1.0f, 0.1f, 5.0f, true };
        params_[1] = { Depth, "Depth", 0.5f, 0.0f, 1.0f, true };
        params_[2] = { Feedback, "Feedback", 0.2f, 0.0f, 0.7f, true };
        params_[3] = { Voices, "Voices", 4.0f, 1.0f, 4.0f, false };
        params_[4] = { Mix, "Mix", 0.5f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        // Bounded delay buffer: máximo 50 ms a 192 kHz = 9600 muestras (Regla 47)
        maxDelaySamples_ = static_cast<size_t>(spec.sampleRate * 0.05);
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
        const float feedback = targetFeedback_;
        const int numVoices = std::clamp(static_cast<int>(std::round(targetVoices_)), 1, 4);
        const float mix = targetMix_;

        const float lfoInc = (2.0f * std::numbers::pi_v<float> * rate) / static_cast<float>(spec_.sampleRate);
        const float baseDelayMs = 15.0f;
        const float modDepthMs = depth * 8.0f;

        const uint32_t channels = std::min(context.numOutputChannels, static_cast<uint32_t>(2));

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            float inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0][s] : 0.0f;
            float inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1][s] : inL;

            // Escribir en buffers de retardo con feedback
            delayBuffers_[0][writeIndex_] = inL + feedbackSample_[0] * feedback;
            delayBuffers_[1][writeIndex_] = inR + feedbackSample_[1] * feedback;

            float chorusAccumL = 0.0f;
            float chorusAccumR = 0.0f;

            // Renderizar voces moduladas con diferentes fases (0, 90, 180, 270 grados)
            for (int v = 0; v < numVoices; ++v) {
                const float voicePhase = lfoPhase_ + static_cast<float>(v) * (std::numbers::pi_v<float> * 0.5f);
                const float lfoMod = std::sin(voicePhase);
                const float delayMs = baseDelayMs + modDepthMs * lfoMod;
                const float delaySamples = delayMs * 0.001f * static_cast<float>(spec_.sampleRate);

                // Lectura interpolada
                const float sampleL = readInterpolated(0, delaySamples);
                const float sampleR = readInterpolated(1, delaySamples);

                // Panorámica cruzada según voz
                const float panL = (v % 2 == 0) ? 0.8f : 0.2f;
                const float panR = 1.0f - panL;

                chorusAccumL += sampleL * panL;
                chorusAccumR += sampleR * panR;
            }

            const float voiceNorm = 1.0f / std::sqrt(static_cast<float>(numVoices));
            chorusAccumL *= voiceNorm;
            chorusAccumR *= voiceNorm;

            feedbackSample_[0] = FastMath::fastTanh(chorusAccumL);
            feedbackSample_[1] = FastMath::fastTanh(chorusAccumR);

            if (channels > 0 && context.outputChannels[0] != nullptr) {
                context.outputChannels[0][s] = (1.0f - mix) * inL + mix * chorusAccumL;
            }
            if (channels > 1 && context.outputChannels[1] != nullptr) {
                context.outputChannels[1][s] = (1.0f - mix) * inR + mix * chorusAccumR;
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
            case Rate: targetRate_ = std::clamp(value, 0.1f, 5.0f); break;
            case Depth: targetDepth_ = std::clamp(value, 0.0f, 1.0f); break;
            case Feedback: targetFeedback_ = std::clamp(value, 0.0f, 0.7f); break;
            case Voices: targetVoices_ = std::clamp(value, 1.0f, 4.0f); break;
            case Mix: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Rate: return targetRate_;
            case Depth: return targetDepth_;
            case Feedback: return targetFeedback_;
            case Voices: return targetVoices_;
            case Mix: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Chorus; }
    const char* getName() const override { return "Chorus"; }

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
    size_t maxDelaySamples_{ 4800 };
    std::array<std::vector<float>, 2> delayBuffers_;
    size_t writeIndex_{ 0 };
    std::array<float, 2> feedbackSample_{ 0.0f, 0.0f };
    float lfoPhase_{ 0.0f };

    float targetRate_{ 1.0f };
    float targetDepth_{ 0.5f };
    float targetFeedback_{ 0.2f };
    float targetVoices_{ 4.0f };
    float targetMix_{ 0.5f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
};

inline AutoRegisterNode<ChorusNode> registerChorus(NodeType::Chorus, "chorus", "Modulation");

} // namespace audio_graph
