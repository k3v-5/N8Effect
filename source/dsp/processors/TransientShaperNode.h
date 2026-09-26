#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../core/FastMath.h"
#include "../core/LinkwitzRileyFilter.h"

namespace audio_graph {

/**
 * @brief Moldeador Dinámico de Transientes (Transient Shaper / Designer)
 * Control independiente de Ataque (Punch) y Sostenido (Sustain) en modo Banda Ancha o Multibanda LR4 de 3 bandas (Reglas 5, 8, 14, 32, 34, 46, 47)
 */
class TransientShaperNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Attack = 1,          // -1.0 (-100%) a +1.0 (+100%) [Global / Master]
        Sustain = 2,         // -1.0 (-100%) a +1.0 (+100%) [Global / Master]
        Speed = 3,           // Velocidad de respuesta de transientes (0.5x a 2.0x)
        SoftClip = 4,        // 0: Off, 1: On (Saturador no lineal anti-picos)
        OutputGain = 5,      // Ganancia de salida en dB (-12 dB a +12 dB)
        Mix = 6,             // Dry / Wet
        Mode = 7,            // 0: Wideband (Banda Completa), 1: Multiband (3 Bandas LR4)
        LowAttack = 8,       // -1.0 a +1.0 (Punch sub/bombo)
        MidAttack = 9,       // -1.0 a +1.0 (Cuerpo caja/guitarras/voz)
        HighAttack = 10,     // -1.0 a +1.0 (Chasquido charles/crack)
        LowSustain = 11,     // -1.0 a +1.0 (Cola graves)
        MidSustain = 12,     // -1.0 a +1.0 (Resonancia medios)
        HighSustain = 13,    // -1.0 a +1.0 (Brillo/aire sustain)
        CrossoverLowMid = 14,// Frecuencia de corte Low-Mid (40 a 1000 Hz)
        CrossoverMidHigh = 15// Frecuencia de corte Mid-High (1000 a 12000 Hz)
    };

    TransientShaperNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0]  = { Attack, "Attack", 0.0f, -1.0f, 1.0f, true };
        params_[1]  = { Sustain, "Sustain", 0.0f, -1.0f, 1.0f, true };
        params_[2]  = { Speed, "Speed", 1.0f, 0.5f, 2.0f, true };
        params_[3]  = { SoftClip, "Soft Clip", 1.0f, 0.0f, 1.0f, false };
        params_[4]  = { OutputGain, "Gain dB", 0.0f, -12.0f, 12.0f, true };
        params_[5]  = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
        params_[6]  = { Mode, "Mode", 0.0f, 0.0f, 1.0f, false };
        params_[7]  = { LowAttack, "Low Attack", 0.0f, -1.0f, 1.0f, true };
        params_[8]  = { MidAttack, "Mid Attack", 0.0f, -1.0f, 1.0f, true };
        params_[9]  = { HighAttack, "High Attack", 0.0f, -1.0f, 1.0f, true };
        params_[10] = { LowSustain, "Low Sustain", 0.0f, -1.0f, 1.0f, true };
        params_[11] = { MidSustain, "Mid Sustain", 0.0f, -1.0f, 1.0f, true };
        params_[12] = { HighSustain, "High Sustain", 0.0f, -1.0f, 1.0f, true };
        params_[13] = { CrossoverLowMid, "Xover Low-Mid", 250.0f, 40.0f, 1000.0f, true };
        params_[14] = { CrossoverMidHigh, "Xover Mid-High", 3000.0f, 1000.0f, 12000.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        crossover_.prepare(spec.sampleRate);
        crossover_.setCrossoverFrequencies(targetXoverLowMid_, targetXoverMidHigh_);
        updateCoeffs();
        reset();
    }

    void reset() override {
        crossover_.reset();
        for (size_t b = 0; b < 3; ++b) {
            for (size_t ch = 0; ch < 2; ++ch) {
                fastEnv_[b][ch] = 0.0f;
                slowEnv_[b][ch] = 0.0f;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Attack:           targetAttack_ = std::clamp(value, -1.0f, 1.0f); break;
            case Sustain:          targetSustain_ = std::clamp(value, -1.0f, 1.0f); break;
            case Speed:            targetSpeed_ = std::clamp(value, 0.5f, 2.0f); updateCoeffs(); break;
            case SoftClip:         targetSoftClip_ = (value >= 0.5f); break;
            case OutputGain:       targetGain_ = std::clamp(value, -12.0f, 12.0f); break;
            case Mix:              targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            case Mode:             targetMode_ = (value >= 0.5f) ? 1.0f : 0.0f; break;
            case LowAttack:        targetLowAttack_ = std::clamp(value, -1.0f, 1.0f); break;
            case MidAttack:        targetMidAttack_ = std::clamp(value, -1.0f, 1.0f); break;
            case HighAttack:       targetHighAttack_ = std::clamp(value, -1.0f, 1.0f); break;
            case LowSustain:       targetLowSustain_ = std::clamp(value, -1.0f, 1.0f); break;
            case MidSustain:       targetMidSustain_ = std::clamp(value, -1.0f, 1.0f); break;
            case HighSustain:      targetHighSustain_ = std::clamp(value, -1.0f, 1.0f); break;
            case CrossoverLowMid:  targetXoverLowMid_ = std::clamp(value, 40.0f, 1000.0f); crossover_.setCrossoverFrequencies(targetXoverLowMid_, targetXoverMidHigh_); break;
            case CrossoverMidHigh: targetXoverMidHigh_ = std::clamp(value, 1000.0f, 12000.0f); crossover_.setCrossoverFrequencies(targetXoverLowMid_, targetXoverMidHigh_); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Attack:           return targetAttack_;
            case Sustain:          return targetSustain_;
            case Speed:            return targetSpeed_;
            case SoftClip:         return targetSoftClip_ ? 1.0f : 0.0f;
            case OutputGain:       return targetGain_;
            case Mix:              return targetMix_;
            case Mode:             return targetMode_;
            case LowAttack:        return targetLowAttack_;
            case MidAttack:        return targetMidAttack_;
            case HighAttack:       return targetHighAttack_;
            case LowSustain:       return targetLowSustain_;
            case MidSustain:       return targetMidSustain_;
            case HighSustain:      return targetHighSustain_;
            case CrossoverLowMid:  return targetXoverLowMid_;
            case CrossoverMidHigh: return targetXoverMidHigh_;
            default:               return 0.0f;
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
        float* outR = (numChannels > 1) ? context.outputChannels[1] : nullptr;

        const float outGainLin = std::pow(10.0f, targetGain_ / 20.0f);
        const bool softClip = targetSoftClip_;
        const float mix = targetMix_;
        const bool isMultiband = (targetMode_ >= 0.5f);

        if (!isMultiband) {
            // MODO 1: BANDA ANCHA (Eficiente y retrocompatible)
            const float attackAmount = targetAttack_ * 2.0f;
            const float sustainAmount = targetSustain_ * 1.5f;

            for (size_t s = 0; s < numSamples; ++s) {
                const float rawInL = inL ? inL[s] : 0.0f;
                const float rawInR = inR ? inR[s] : rawInL;

                const float inLMag = std::abs(rawInL);
                const float inRMag = std::abs(rawInR);

                fastEnv_[0][0] = (inLMag > fastEnv_[0][0]) ? (fastEnv_[0][0] + fastAttCoeff_ * (inLMag - fastEnv_[0][0]))
                                                            : (fastEnv_[0][0] + fastRelCoeff_ * (inLMag - fastEnv_[0][0]));
                slowEnv_[0][0] = (inLMag > slowEnv_[0][0]) ? (slowEnv_[0][0] + slowAttCoeff_ * (inLMag - slowEnv_[0][0]))
                                                            : (slowEnv_[0][0] + slowRelCoeff_ * (inLMag - slowEnv_[0][0]));

                fastEnv_[0][1] = (inRMag > fastEnv_[0][1]) ? (fastEnv_[0][1] + fastAttCoeff_ * (inRMag - fastEnv_[0][1]))
                                                            : (fastEnv_[0][1] + fastRelCoeff_ * (inRMag - fastEnv_[0][1]));
                slowEnv_[0][1] = (inRMag > slowEnv_[0][1]) ? (slowEnv_[0][1] + slowAttCoeff_ * (inRMag - slowEnv_[0][1]))
                                                            : (slowEnv_[0][1] + slowRelCoeff_ * (inRMag - slowEnv_[0][1]));

                const float deltaL = fastEnv_[0][0] - slowEnv_[0][0];
                const float deltaR = fastEnv_[0][1] - slowEnv_[0][1];

                float gainL = 1.0f + attackAmount * deltaL + sustainAmount * slowEnv_[0][0];
                float gainR = 1.0f + attackAmount * deltaR + sustainAmount * slowEnv_[0][1];

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
        } else {
            // MODO 2: MULTIBANDA DE 3 VÍAS LINKWITZ-RILEY LR4 (Punto 2)
            const float lowAttAmount  = std::clamp(targetLowAttack_  + targetAttack_, -1.0f, 1.0f) * 2.0f;
            const float midAttAmount  = std::clamp(targetMidAttack_  + targetAttack_, -1.0f, 1.0f) * 2.0f;
            const float highAttAmount = std::clamp(targetHighAttack_ + targetAttack_, -1.0f, 1.0f) * 2.0f;

            const float lowSusAmount  = std::clamp(targetLowSustain_  + targetSustain_, -1.0f, 1.0f) * 1.5f;
            const float midSusAmount  = std::clamp(targetMidSustain_  + targetSustain_, -1.0f, 1.0f) * 1.5f;
            const float highSusAmount = std::clamp(targetHighSustain_ + targetSustain_, -1.0f, 1.0f) * 1.5f;

            for (size_t s = 0; s < numSamples; ++s) {
                const float rawInL = inL ? inL[s] : 0.0f;
                const float rawInR = inR ? inR[s] : rawInL;

                // División en 3 bandas LR4 con suma plana a 0 dB
                const auto bandL = crossover_.processSample(0, rawInL);
                const auto bandR = crossover_.processSample(1, rawInR);

                const float bandsL[3] = { bandL.low, bandL.mid, bandL.high };
                const float bandsR[3] = { bandR.low, bandR.mid, bandR.high };
                const float attAmounts[3] = { lowAttAmount, midAttAmount, highAttAmount };
                const float susAmounts[3] = { lowSusAmount, midSusAmount, highSusAmount };

                float sumWetL = 0.0f;
                float sumWetR = 0.0f;

                for (size_t b = 0; b < 3; ++b) {
                    const float bLMag = std::abs(bandsL[b]);
                    const float bRMag = std::abs(bandsR[b]);

                    fastEnv_[b][0] = (bLMag > fastEnv_[b][0]) ? (fastEnv_[b][0] + fastAttCoeff_ * (bLMag - fastEnv_[b][0]))
                                                              : (fastEnv_[b][0] + fastRelCoeff_ * (bLMag - fastEnv_[b][0]));
                    slowEnv_[b][0] = (bLMag > slowEnv_[b][0]) ? (slowEnv_[b][0] + slowAttCoeff_ * (bLMag - slowEnv_[b][0]))
                                                              : (slowEnv_[b][0] + slowRelCoeff_ * (bLMag - slowEnv_[b][0]));

                    fastEnv_[b][1] = (bRMag > fastEnv_[b][1]) ? (fastEnv_[b][1] + fastAttCoeff_ * (bRMag - fastEnv_[b][1]))
                                                              : (fastEnv_[b][1] + fastRelCoeff_ * (bRMag - fastEnv_[b][1]));
                    slowEnv_[b][1] = (bRMag > slowEnv_[b][1]) ? (slowEnv_[b][1] + slowAttCoeff_ * (bRMag - slowEnv_[b][1]))
                                                              : (slowEnv_[b][1] + slowRelCoeff_ * (bRMag - slowEnv_[b][1]));

                    const float deltaL = fastEnv_[b][0] - slowEnv_[b][0];
                    const float deltaR = fastEnv_[b][1] - slowEnv_[b][1];

                    float gainL = 1.0f + attAmounts[b] * deltaL + susAmounts[b] * slowEnv_[b][0];
                    float gainR = 1.0f + attAmounts[b] * deltaR + susAmounts[b] * slowEnv_[b][1];

                    gainL = std::clamp(gainL, 0.0f, 4.0f);
                    gainR = std::clamp(gainR, 0.0f, 4.0f);

                    sumWetL += bandsL[b] * gainL;
                    sumWetR += bandsR[b] * gainR;
                }

                float wetL = sumWetL * outGainLin;
                float wetR = sumWetR * outGainLin;

                if (softClip) {
                    wetL = FastMath::fastTanh(wetL);
                    wetR = FastMath::fastTanh(wetR);
                }

                if (outL) outL[s] = (1.0f - mix) * rawInL + mix * wetL;
                if (outR) outR[s] = (1.0f - mix) * rawInR + mix * wetR;
            }
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
    LinkwitzRileyCrossover3Way crossover_;

    // [3 bandas: 0 = Low, 1 = Mid, 2 = High][2 canales: 0 = L, 1 = R]
    std::array<std::array<float, 2>, 3> fastEnv_{};
    std::array<std::array<float, 2>, 3> slowEnv_{};

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
    float targetMode_{ 0.0f }; // 0: Wideband, 1: Multiband

    float targetLowAttack_{ 0.0f };
    float targetMidAttack_{ 0.0f };
    float targetHighAttack_{ 0.0f };
    float targetLowSustain_{ 0.0f };
    float targetMidSustain_{ 0.0f };
    float targetHighSustain_{ 0.0f };
    float targetXoverLowMid_{ 250.0f };
    float targetXoverMidHigh_{ 3000.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 15> params_;
};

inline AutoRegisterNode<TransientShaperNode> registerTransientShaper(NodeType::TransientShaper, "transient_shaper", "Dynamics");

} // namespace audio_graph
