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
#include "../core/BiquadFilter.h"

namespace audio_graph {

/**
 * @brief Reverberación Inversa con Florecimiento Espectral (Reverse Reverb / Swell Bloom)
 * Reproducción en espejo temporal con envolvente de hinchamiento pre-transiente y difusión multietapa
 * (Reglas 5, 8, 14, 34, 46, 47)
 */
class ReverseReverbNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        SwellTime = 1, // Duración del hinchamiento temporal inverso (0.1s a 2.0s)
        Diffusion = 2, // Cantidad de difusión y densidad espectral (0.0 a 1.0)
        Damping = 3,   // Frecuencia de absorción de agudos en Hz (500 Hz a 18000 Hz)
        Feedback = 4,  // Realimentación en cascada (0.0 a 0.85)
        Mix = 5        // Dry / Wet
    };

    ReverseReverbNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { SwellTime, "Swell Time", 0.6f, 0.1f, 2.0f, true };
        params_[1] = { Diffusion, "Diffusion", 0.7f, 0.0f, 1.0f, true };
        params_[2] = { Damping, "Damping Hz", 8000.0f, 500.0f, 18000.0f, true };
        params_[3] = { Feedback, "Feedback", 0.35f, 0.0f, 0.85f, true };
        params_[4] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        // Bounded ring buffer: 2.0 segundos de audio prealocado (Regla 47)
        maxSamples_ = std::max<size_t>(static_cast<size_t>(spec.sampleRate * 2.0), 1024);
        for (auto& buf : ringBuffers_) {
            buf.assign(maxSamples_, 0.0f);
        }

        writeIdx_ = 0;
        grainPhase_ = 0;

        // Difusores allpass multietapa (4 etapas estéreo con retardos primos)
        diffuserDelays_ = { 113, 163, 241, 353 };
        for (size_t stage = 0; stage < 4; ++stage) {
            for (size_t ch = 0; ch < 2; ++ch) {
                diffuserBuffers_[stage][ch].assign(diffuserDelays_[stage] + 16, 0.0f);
                diffuserIndices_[stage][ch] = 0;
            }
        }

        const double sr = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
        for (auto& f : dampingLPF_) {
            f.reset();
            f.setCoefficients(BiquadFilter::Type::Lowpass, sr, targetDamping_, 0.707f);
        }

        reset();
    }

    void reset() override {
        for (auto& buf : ringBuffers_) std::fill(buf.begin(), buf.end(), 0.0f);
        writeIdx_ = 0;
        grainPhase_ = 0;

        for (size_t stage = 0; stage < 4; ++stage) {
            for (size_t ch = 0; ch < 2; ++ch) {
                std::fill(diffuserBuffers_[stage][ch].begin(), diffuserBuffers_[stage][ch].end(), 0.0f);
                diffuserIndices_[stage][ch] = 0;
            }
        }
        for (auto& f : dampingLPF_) f.reset();
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case SwellTime: targetSwellTime_ = std::clamp(value, 0.1f, 2.0f); break;
            case Diffusion: targetDiffusion_ = std::clamp(value, 0.0f, 1.0f); break;
            case Damping:
                targetDamping_ = std::clamp(value, 500.0f, 18000.0f);
                updateDampingFilter();
                break;
            case Feedback:  targetFeedback_ = std::clamp(value, 0.0f, 0.85f); break;
            case Mix:       targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case SwellTime: return targetSwellTime_;
            case Diffusion: return targetDiffusion_;
            case Damping:   return targetDamping_;
            case Feedback:  return targetFeedback_;
            case Mix:       return targetMix_;
            default:        return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::ReverseReverb; }
    const char* getName() const override { return "Reverse Reverb"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47

        if (context.numSamples == 0 || maxSamples_ == 0) return;

        const size_t numSamples = context.numSamples;
        const size_t numChannels = std::min<size_t>(context.numOutputChannels, 2);

        const float* inL = (context.numInputChannels > 0) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : inL;
        float* outL = (numChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (numChannels > 1) ? context.outputChannels[1] : outL;

        const double sampleRate = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        const size_t swellWindowSamples = std::clamp<size_t>(
            static_cast<size_t>(static_cast<double>(targetSwellTime_) * sampleRate),
            256,
            maxSamples_ - 16
        );

        const float diffusion = targetDiffusion_ * 0.7f;
        const float feedback = targetFeedback_;
        const float mix = targetMix_;
        constexpr float pi = std::numbers::pi_v<float>;

        for (size_t s = 0; s < numSamples; ++s) {
            const float rawInL = inL ? inL[s] : 0.0f;
            const float rawInR = inR ? inR[s] : rawInL;

            // 1. Grabar en el buffer circular
            ringBuffers_[0][writeIdx_] = rawInL;
            ringBuffers_[1][writeIdx_] = rawInR;

            // 2. Lectura en espejo inverso respecto a la posición actual
            // En fase actual, leemos hacia atrás desde swellWindowSamples hasta 0
            const float progress = static_cast<float>(grainPhase_) / static_cast<float>(swellWindowSamples);

            // Envolvente de hinchamiento inversa (crece de 0 a 1 exponencialmente)
            const float swellEnv = std::sin(progress * (pi * 0.5f));

            // Índice de lectura invertido en el tiempo
            const size_t reverseOffset = swellWindowSamples - grainPhase_;
            const size_t readIdx = (writeIdx_ + maxSamples_ - reverseOffset) % maxSamples_;

            float revL = ringBuffers_[0][readIdx] * swellEnv;
            float revR = ringBuffers_[1][readIdx] * swellEnv;

            // 3. Difusores pasa-todo en cascada para transformar el reverso en nube densa
            for (size_t stage = 0; stage < 4; ++stage) {
                revL = processAllpass(stage, 0, revL, diffusion);
                revR = processAllpass(stage, 1, revR, diffusion);
            }

            // 4. Filtro de absorción de agudos (Damping)
            revL = dampingLPF_[0].processSample(revL);
            revR = dampingLPF_[1].processSample(revR);

            // Realimentar al buffer para colas extendidas
            ringBuffers_[0][writeIdx_] += revL * feedback;
            ringBuffers_[1][writeIdx_] += revR * feedback;

            writeIdx_ = (writeIdx_ + 1) % maxSamples_;
            grainPhase_ = (grainPhase_ + 1) % swellWindowSamples;

            // 5. Mezcla Dry/Wet
            const float wetL = revL * 1.5f;
            const float wetR = revR * 1.5f;

            if (outL) outL[s] = (1.0f - mix) * rawInL + mix * wetL;
            if (outR) outR[s] = (1.0f - mix) * rawInR + mix * wetR;
        }
    }

private:
    float processAllpass(size_t stage, size_t ch, float input, float coeff) noexcept {
        const size_t delayLen = diffuserDelays_[stage];
        auto& buf = diffuserBuffers_[stage][ch];
        size_t& idx = diffuserIndices_[stage][ch];

        const float delayed = buf[idx];
        const float output = -input * coeff + delayed;
        buf[idx] = input + delayed * coeff;

        idx = (idx + 1) % delayLen;
        return output;
    }

    void updateDampingFilter() noexcept {
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        for (auto& f : dampingLPF_) {
            f.setCoefficients(BiquadFilter::Type::Lowpass, sr, targetDamping_, 0.707f);
        }
    }

    ProcessSpec spec_;
    size_t maxSamples_{ 1024 };
    std::array<std::vector<float>, 2> ringBuffers_;
    size_t writeIdx_{ 0 };
    size_t grainPhase_{ 0 };

    std::array<size_t, 4> diffuserDelays_{ 113, 163, 241, 353 };
    std::array<std::array<std::vector<float>, 2>, 4> diffuserBuffers_;
    std::array<std::array<size_t, 2>, 4> diffuserIndices_{};

    std::array<BiquadFilter, 2> dampingLPF_;

    float targetSwellTime_{ 0.6f };
    float targetDiffusion_{ 0.7f };
    float targetDamping_{ 8000.0f };
    float targetFeedback_{ 0.35f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
};

inline AutoRegisterNode<ReverseReverbNode> registerReverseReverb(NodeType::ReverseReverb, "reverse_reverb", "Spatial");

} // namespace audio_graph
