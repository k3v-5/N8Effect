#pragma once

#include <cmath>
#include <array>
#include <vector>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/EnvelopeDetector.h"
#include "../core/DenormalGuards.h"
#include "../core/FastMath.h"

namespace audio_graph {

/**
 * @brief Limitador de Pico Brickwall y Maximizador Profesional (Reglas 5, 8, 9, 14, 34, 46, 47)
 * Proporciona limitación transparente con búfer de lookahead circular de capacidad acotada,
 * detector de picos adelantado, envolvente de decaimiento suave y techo de salida inexpugnable.
 */
class BrickwallLimiterNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Threshold = 1,  // -24.0 a 0.0 dB (Drive / Umbral)
        Ceiling = 2,    // -12.0 a 0.0 dB (Techo absoluto)
        Release = 3,    // 2.0 a 500.0 ms (Tiempo de recuperación)
        Lookahead = 4,  // 0.2 a 5.0 ms (Anticipación en lookahead)
        AutoMakeup = 5  // 0.0 o 1.0 (Compensación automática de ganancia)
    };

    BrickwallLimiterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Threshold, "Threshold", -1.0f, -24.0f, 0.0f, true };
        params_[1] = { Ceiling, "Ceiling", -0.1f, -12.0f, 0.0f, true };
        params_[2] = { Release, "Release", 40.0f, 2.0f, 500.0f, true };
        params_[3] = { Lookahead, "Lookahead", 2.0f, 0.2f, 5.0f, true };
        params_[4] = { AutoMakeup, "Auto Makeup", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        const double sr = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
        
        // Capacidad acotada para un máximo de 10 ms a 192 kHz (~1920 muestras) -> 2048
        maxLookaheadSamples_ = 2048;
        lookaheadBuffers_[0].assign(maxLookaheadSamples_, 0.0f);
        lookaheadBuffers_[1].assign(maxLookaheadSamples_, 0.0f);
        writeIdx_ = 0;

        currentGainReduction_ = 1.0f;
        targetGainReduction_ = 1.0f;
        peakHold_ = 0.0f;

        updateCoefficients(sr);
    }

    void reset() override {
        for (auto& buf : lookaheadBuffers_) {
            std::fill(buf.begin(), buf.end(), 0.0f);
        }
        writeIdx_ = 0;
        currentGainReduction_ = 1.0f;
        targetGainReduction_ = 1.0f;
        peakHold_ = 0.0f;
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        const size_t lookaheadSamples = std::clamp<size_t>(
            static_cast<size_t>(targetLookahead_ * 0.001 * sr),
            1,
            maxLookaheadSamples_ - 1
        );

        const float inputGain = targetAutoMakeup_ > 0.5f 
            ? EnvelopeDetector::dbToLinear(-targetThreshold_) 
            : 1.0f;
        const float ceilingLinear = EnvelopeDetector::dbToLinear(targetCeiling_);

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0]) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1]) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0]) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1]) ? context.outputChannels[1] : nullptr;

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            const float rawInL = inL ? inL[s] : 0.0f;
            const float rawInR = inR ? inR[s] : 0.0f;

            // 1. Aplicar ganancia de entrada / Threshold
            const float boostedL = rawInL * inputGain;
            const float boostedR = rawInR * inputGain;

            // 2. Guardar en el búfer circular de lookahead
            lookaheadBuffers_[0][writeIdx_] = boostedL;
            lookaheadBuffers_[1][writeIdx_] = boostedR;

            // 3. Detección de picos adelantada (Peak Ahead)
            const float currentPeak = std::max(std::abs(boostedL), std::abs(boostedR));
            if (currentPeak > peakHold_) {
                peakHold_ = currentPeak;
            } else {
                peakHold_ *= releaseCoeff_;
            }

            // 4. Cálculo de Gain Reduction necesario para no exceder el Ceiling
            if (peakHold_ > ceilingLinear && peakHold_ > 1e-6f) {
                targetGainReduction_ = ceilingLinear / peakHold_;
            } else {
                targetGainReduction_ = 1.0f;
            }

            // 5. Suavizado balístico del Gain Reduction (Ataque inmediato sin overshoot, release suave)
            if (targetGainReduction_ < currentGainReduction_) {
                currentGainReduction_ = targetGainReduction_; // Ataque instantáneo gracias al lookahead
            } else {
                currentGainReduction_ += (1.0f - releaseCoeff_) * (targetGainReduction_ - currentGainReduction_);
            }

            // 6. Leer señal retrasada por el búfer de lookahead
            const size_t readIdx = (writeIdx_ + maxLookaheadSamples_ - lookaheadSamples) % maxLookaheadSamples_;
            const float delayedL = lookaheadBuffers_[0][readIdx];
            const float delayedR = lookaheadBuffers_[1][readIdx];

            writeIdx_ = (writeIdx_ + 1) % maxLookaheadSamples_;

            // 7. Aplicar Gain Reduction
            float limitedL = delayedL * currentGainReduction_;
            float limitedR = delayedR * currentGainReduction_;

            // 8. Protección True Peak absoluta (Brickwall Hard Clamp al Ceiling exacto)
            limitedL = std::clamp(limitedL, -ceilingLinear, ceilingLinear);
            limitedR = std::clamp(limitedR, -ceilingLinear, ceilingLinear);

            if (outL) outL[s] = limitedL;
            if (outR) outR[s] = limitedR;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Threshold: targetThreshold_ = std::clamp(value, -24.0f, 0.0f); break;
            case Ceiling: targetCeiling_ = std::clamp(value, -12.0f, 0.0f); break;
            case Release: {
                targetRelease_ = std::clamp(value, 2.0f, 500.0f);
                updateCoefficients(spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0);
                break;
            }
            case Lookahead: targetLookahead_ = std::clamp(value, 0.2f, 5.0f); break;
            case AutoMakeup: targetAutoMakeup_ = value >= 0.5f ? 1.0f : 0.0f; break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Threshold: return targetThreshold_;
            case Ceiling: return targetCeiling_;
            case Release: return targetRelease_;
            case Lookahead: return targetLookahead_;
            case AutoMakeup: return targetAutoMakeup_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::BrickwallLimiter; }
    const char* getName() const override { return "Brickwall Limiter"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateCoefficients(double sampleRate) noexcept {
        const float relSamples = (targetRelease_ * 0.001f) * static_cast<float>(sampleRate);
        releaseCoeff_ = std::exp(-1.0f / std::max(1.0f, relSamples));
    }

    ProcessSpec spec_;
    size_t maxLookaheadSamples_{ 2048 };
    std::array<std::vector<float>, 2> lookaheadBuffers_;
    size_t writeIdx_{ 0 };

    float releaseCoeff_{ 0.999f };
    float peakHold_{ 0.0f };
    float currentGainReduction_{ 1.0f };
    float targetGainReduction_{ 1.0f };

    float targetThreshold_{ -1.0f };
    float targetCeiling_{ -0.1f };
    float targetRelease_{ 40.0f };
    float targetLookahead_{ 2.0f };
    float targetAutoMakeup_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
};

inline AutoRegisterNode<BrickwallLimiterNode> registerLimiter(NodeType::BrickwallLimiter, "brickwall_limiter", "Dynamics");

} // namespace audio_graph
