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
 * @brief Desplazador de Frecuencia Lineal por Banda Lateral Única (SSB) y Red Hilbert Allpass (Reglas 5, 8, 14, 34, 46, 47)
 */
class FrequencyShifterNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        ShiftHz = 1,
        Feedback = 2,
        Mix = 3
    };

    FrequencyShifterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { ShiftHz, "Shift", 50.0f, -1000.0f, 1000.0f, true };
        params_[1] = { Feedback, "Feedback", 0.0f, 0.0f, 0.85f, true };
        params_[2] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        reset();
    }

    void reset() override {
        for (auto& ch : apHistoryA_) ch.fill(0.0f);
        for (auto& ch : apHistoryB_) ch.fill(0.0f);
        feedbackSample_.fill(0.0f);
        quadPhase_ = 0.0f;
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const float shift = targetShiftHz_;
        const float feedback = targetFeedback_;
        const float mix = targetMix_;

        const float phaseInc = (2.0f * std::numbers::pi_v<float> * shift) / static_cast<float>(spec_.sampleRate);
        const uint32_t channels = std::min(context.numOutputChannels, static_cast<uint32_t>(2));

        // Coeficientes allpass para red desfasadora de 90 grados (Weaver/Hartley)
        constexpr std::array<float, 4> coefsA{ 0.161758f, 0.733029f, 0.945350f, 0.990598f };
        constexpr std::array<float, 4> coefsB{ 0.479401f, 0.876243f, 0.976599f, 0.997500f };

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            const float cosVal = std::cos(quadPhase_);
            const float sinVal = std::sin(quadPhase_);

            for (uint32_t ch = 0; ch < channels; ++ch) {
                const float in = (ch < context.numInputChannels && context.inputChannels[ch] != nullptr)
                    ? context.inputChannels[ch][s]
                    : 0.0f;

                float signal = in + feedbackSample_[ch] * feedback;
                signal = FastMath::fastTanh(signal);

                // Rama en Fase (I)
                float sigI = signal;
                for (size_t stage = 0; stage < 4; ++stage) {
                    const float a = coefsA[stage];
                    const float y = a * sigI + apHistoryA_[ch][stage];
                    apHistoryA_[ch][stage] = sigI - a * y;
                    sigI = y;
                }

                // Rama en Cuadratura (Q) a 90 grados
                float sigQ = signal;
                for (size_t stage = 0; stage < 4; ++stage) {
                    const float b = coefsB[stage];
                    const float y = b * sigQ + apHistoryB_[ch][stage];
                    apHistoryB_[ch][stage] = sigQ - b * y;
                    sigQ = y;
                }

                // Modulación en banda lateral única (SSB)
                // Para shift > 0 (Up): I*cos - Q*sin
                // Para shift < 0 (Down): I*cos + Q*sin
                float shifted = (shift >= 0.0f)
                    ? (sigI * cosVal - sigQ * sinVal)
                    : (sigI * cosVal + sigQ * sinVal);

                feedbackSample_[ch] = shifted;

                context.outputChannels[ch][s] = (1.0f - mix) * in + mix * shifted;
            }

            quadPhase_ += std::abs(phaseInc);
            if (quadPhase_ >= 2.0f * std::numbers::pi_v<float>) {
                quadPhase_ -= 2.0f * std::numbers::pi_v<float>;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case ShiftHz: targetShiftHz_ = std::clamp(value, -1000.0f, 1000.0f); break;
            case Feedback: targetFeedback_ = std::clamp(value, 0.0f, 0.85f); break;
            case Mix: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case ShiftHz: return targetShiftHz_;
            case Feedback: return targetFeedback_;
            case Mix: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::FrequencyShifter; }
    const char* getName() const override { return "FrequencyShifter"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<std::array<float, 4>, 2> apHistoryA_{};
    std::array<std::array<float, 4>, 2> apHistoryB_{};
    std::array<float, 2> feedbackSample_{ 0.0f, 0.0f };
    float quadPhase_{ 0.0f };

    float targetShiftHz_{ 50.0f };
    float targetFeedback_{ 0.0f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 3> params_;
};

inline AutoRegisterNode<FrequencyShifterNode> registerFrequencyShifter(NodeType::FrequencyShifter, "freqshift", "Pitch");

} // namespace audio_graph
