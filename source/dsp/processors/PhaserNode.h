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
 * @brief Procesador Phaser Analógico de 4 a 8 Etapas Allpass (Reglas 5, 8, 14, 34, 46, 47)
 */
class PhaserNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Rate = 1,
        Depth = 2,
        Feedback = 3,
        BaseFreq = 4,
        Mix = 5
    };

    PhaserNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Rate, "Rate", 0.5f, 0.05f, 8.0f, true };
        params_[1] = { Depth, "Depth", 0.75f, 0.0f, 1.0f, true };
        params_[2] = { Feedback, "Feedback", 0.6f, 0.0f, 0.95f, true };
        params_[3] = { BaseFreq, "Base Freq", 800.0f, 100.0f, 3000.0f, true };
        params_[4] = { Mix, "Mix", 0.5f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        reset();
    }

    void reset() override {
        for (auto& ch : apHistoryX_) ch.fill(0.0f);
        for (auto& ch : apHistoryY_) ch.fill(0.0f);
        feedbackSample_.fill(0.0f);
        lfoPhase_ = 0.0f;
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const float rate = targetRate_;
        const float depth = targetDepth_;
        const float feedback = targetFeedback_;
        const float baseFreq = targetBaseFreq_;
        const float mix = targetMix_;

        const float lfoInc = (2.0f * std::numbers::pi_v<float> * rate) / static_cast<float>(spec_.sampleRate);
        const uint32_t channels = std::min(context.numOutputChannels, static_cast<uint32_t>(2));

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            // LFO estéreo en cuadratura (90 grados de desfase entre canales)
            const float lfoL = 0.5f + 0.5f * std::sin(lfoPhase_);
            const float lfoR = 0.5f + 0.5f * std::sin(lfoPhase_ + std::numbers::pi_v<float> * 0.5f);
            lfoPhase_ += lfoInc;
            if (lfoPhase_ >= 2.0f * std::numbers::pi_v<float>) {
                lfoPhase_ -= 2.0f * std::numbers::pi_v<float>;
            }

            for (uint32_t ch = 0; ch < channels; ++ch) {
                const float in = (ch < context.numInputChannels && context.inputChannels[ch] != nullptr)
                    ? context.inputChannels[ch][s]
                    : 0.0f;

                // Frecuencia modulada por etapa
                const float lfoVal = (ch == 0) ? lfoL : lfoR;
                const float sweptFreq = baseFreq * std::pow(4.0f, lfoVal * depth);
                const float clampedFreq = std::clamp(sweptFreq, 50.0f, static_cast<float>(spec_.sampleRate) * 0.45f);

                // Coeficiente de filtro allpass de primer orden
                const float w0 = std::tan(std::numbers::pi_v<float> * clampedFreq / static_cast<float>(spec_.sampleRate));
                const float a = (w0 - 1.0f) / (w0 + 1.0f);

                // Entrada con retroalimentación saturada suavemente (Regla 12 y 47)
                float signal = in + feedbackSample_[ch] * feedback;
                signal = FastMath::fastTanh(signal);

                // Cascada de 6 etapas allpass
                for (size_t stage = 0; stage < 6; ++stage) {
                    const float x = signal;
                    const float y = a * x + apHistoryX_[ch][stage] - a * apHistoryY_[ch][stage];
                    apHistoryX_[ch][stage] = x;
                    apHistoryY_[ch][stage] = y;
                    signal = y;
                }

                feedbackSample_[ch] = signal;

                // Mezcla de salida con señal limpia para generar las cancelaciones en peine (peaking / notching)
                context.outputChannels[ch][s] = (1.0f - mix) * in + mix * signal;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Rate: targetRate_ = std::clamp(value, 0.05f, 8.0f); break;
            case Depth: targetDepth_ = std::clamp(value, 0.0f, 1.0f); break;
            case Feedback: targetFeedback_ = std::clamp(value, 0.0f, 0.95f); break;
            case BaseFreq: targetBaseFreq_ = std::clamp(value, 100.0f, 3000.0f); break;
            case Mix: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Rate: return targetRate_;
            case Depth: return targetDepth_;
            case Feedback: return targetFeedback_;
            case BaseFreq: return targetBaseFreq_;
            case Mix: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Phaser; }
    const char* getName() const override { return "Phaser"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<std::array<float, 6>, 2> apHistoryX_{};
    std::array<std::array<float, 6>, 2> apHistoryY_{};
    std::array<float, 2> feedbackSample_{ 0.0f, 0.0f };
    float lfoPhase_{ 0.0f };

    float targetRate_{ 0.5f };
    float targetDepth_{ 0.75f };
    float targetFeedback_{ 0.6f };
    float targetBaseFreq_{ 800.0f };
    float targetMix_{ 0.5f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
};

inline AutoRegisterNode<PhaserNode> registerPhaser(NodeType::Phaser, "phaser", "Modulation");

} // namespace audio_graph
