#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../core/FastMath.h"

namespace audio_graph {

/**
 * @brief Moldeador Dinámico de Transientes (Transient Shaper / Designer)
 * Control independiente de Ataque (Punch) y Sostenido (Sustain) sin umbral rígido (Reglas 5, 8, 14, 34, 46, 47)
 */
class TransientShaperNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Attack = 1,    // -1.0 (-100%) a +1.0 (+100%)
        Sustain = 2,   // -1.0 (-100%) a +1.0 (+100%)
        Speed = 3,     // Velocidad de respuesta de transientes (0.5x a 2.0x)
        SoftClip = 4,  // 0: Off, 1: On (Saturador no lineal anti-picos)
        OutputGain = 5,// Ganancia de salida en dB (-12 dB a +12 dB)
        Mix = 6        // Dry / Wet
    };

    TransientShaperNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Attack, "Attack", 0.0f, -1.0f, 1.0f, true };
        params_[1] = { Sustain, "Sustain", 0.0f, -1.0f, 1.0f, true };
        params_[2] = { Speed, "Speed", 1.0f, 0.5f, 2.0f, true };
        params_[3] = { SoftClip, "Soft Clip", 1.0f, 0.0f, 1.0f, false };
        params_[4] = { OutputGain, "Gain dB", 0.0f, -12.0f, 12.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        updateCoeffs();
        reset();
    }

    void reset() override {
        for (size_t ch = 0; ch < 2; ++ch) {
            fastEnv_[ch] = 0.0f;
            slowEnv_[ch] = 0.0f;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Attack:     targetAttack_ = std::clamp(value, -1.0f, 1.0f); break;
            case Sustain:    targetSustain_ = std::clamp(value, -1.0f, 1.0f); break;
            case Speed:      targetSpeed_ = std::clamp(value, 0.5f, 2.0f); updateCoeffs(); break;
            case SoftClip:   targetSoftClip_ = (value >= 0.5f); break;
            case OutputGain: targetGain_ = std::clamp(value, -12.0f, 12.0f); break;
            case Mix:        targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Attack:     return targetAttack_;
            case Sustain:    return targetSustain_;
            case Speed:      return targetSpeed_;
            case SoftClip:   return targetSoftClip_ ? 1.0f : 0.0f;
            case OutputGain: return targetGain_;
            case Mix:        return targetMix_;
            default:         return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::TransientShaper; }
    const char* getName() const override { return "Transient Shaper"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47

        if (context.numSamples == 0) return;

        const size_t numSamples = context.numSamples;
        const size_t numChannels = std::min<size_t>(context.numOutputChannels, 2);

        const float* inL = (context.numInputChannels > 0) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : inL;
        float* outL = (numChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (numChannels > 1) ? context.outputChannels[1] : outL;

        const float attackAmount = targetAttack_ * 2.0f;
        const float sustainAmount = targetSustain_ * 1.5f;
        const float outGainLin = std::pow(10.0f, targetGain_ / 20.0f);
        const bool softClip = targetSoftClip_;
        const float mix = targetMix_;

        for (size_t s = 0; s < numSamples; ++s) {
            const float rawInL = inL ? inL[s] : 0.0f;
            const float rawInR = inR ? inR[s] : rawInL;

            const float inLMag = std::abs(rawInL);
            const float inRMag = std::abs(rawInR);

            // Actualizar seguidores diferenciales rápidos (Attack) y lentos (Sustain)
            fastEnv_[0] = (inLMag > fastEnv_[0]) ? (fastEnv_[0] + fastAttCoeff_ * (inLMag - fastEnv_[0]))
                                                 : (fastEnv_[0] + fastRelCoeff_ * (inLMag - fastEnv_[0]));
            slowEnv_[0] = (inLMag > slowEnv_[0]) ? (slowEnv_[0] + slowAttCoeff_ * (inLMag - slowEnv_[0]))
                                                 : (slowEnv_[0] + slowRelCoeff_ * (inLMag - slowEnv_[0]));

            fastEnv_[1] = (inRMag > fastEnv_[1]) ? (fastEnv_[1] + fastAttCoeff_ * (inRMag - fastEnv_[1]))
                                                 : (fastEnv_[1] + fastRelCoeff_ * (inRMag - fastEnv_[1]));
            slowEnv_[1] = (inRMag > slowEnv_[1]) ? (slowEnv_[1] + slowAttCoeff_ * (inRMag - slowEnv_[1]))
                                                 : (slowEnv_[1] + slowRelCoeff_ * (inRMag - slowEnv_[1]));

            // Delta transiente = rápido - lento (positivo durante el ataque, cero o negativo en sustain)
            const float deltaL = fastEnv_[0] - slowEnv_[0];
            const float deltaR = fastEnv_[1] - slowEnv_[1];

            // Curva de ganancia por canal
            float gainL = 1.0f + attackAmount * deltaL + sustainAmount * slowEnv_[0];
            float gainR = 1.0f + attackAmount * deltaR + sustainAmount * slowEnv_[1];

            gainL = std::clamp(gainL, 0.0f, 4.0f);
            gainR = std::clamp(gainR, 0.0f, 4.0f);

            float wetL = rawInL * gainL * outGainLin;
            float wetR = rawInR * gainR * outGainLin;

            if (softClip) {
                wetL = FastMath::fastTanh(wetL);
                wetR = FastMath::fastTanh(wetR);
            }

            if (outL) outL[s] = (1.0f - mix) * rawInL + mix * wetL;
            if (outR) outR[s] = (1.0f - mix) * rawInR + mix * wetR;
        }
    }

private:
    void updateCoeffs() noexcept {
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        const double speed = static_cast<double>(targetSpeed_);

        // Envolvente rápida (Ataque): 2.0 ms ataque, 15.0 ms release
        fastAttCoeff_ = static_cast<float>(1.0 - std::exp(-1.0 / ((0.002 / speed) * sr)));
        fastRelCoeff_ = static_cast<float>(1.0 - std::exp(-1.0 / ((0.015 / speed) * sr)));

        // Envolvente lenta (Sustain): 25.0 ms ataque, 120.0 ms release
        slowAttCoeff_ = static_cast<float>(1.0 - std::exp(-1.0 / ((0.025 / speed) * sr)));
        slowRelCoeff_ = static_cast<float>(1.0 - std::exp(-1.0 / ((0.120 / speed) * sr)));
    }

    ProcessSpec spec_;
    std::array<float, 2> fastEnv_{ 0.0f, 0.0f };
    std::array<float, 2> slowEnv_{ 0.0f, 0.0f };

    float fastAttCoeff_{ 0.1f };
    float fastRelCoeff_{ 0.01f };
    float slowAttCoeff_{ 0.01f };
    float slowRelCoeff_{ 0.002f };

    float targetAttack_{ 0.0f };
    float targetSustain_{ 0.0f };
    float targetSpeed_{ 1.0f };
    bool targetSoftClip_{ true };
    float targetGain_{ 0.0f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<TransientShaperNode> registerTransientShaper(NodeType::TransientShaper, "transient_shaper", "Dynamics");

} // namespace audio_graph
