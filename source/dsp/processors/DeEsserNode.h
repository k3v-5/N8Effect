#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/BiquadFilter.h"
#include "../core/EnvelopeDetector.h"
#include "../core/DenormalGuards.h"

namespace audio_graph {

/**
 * @brief De-Esser Dinámico Profesional de Precisión Vocal (Reglas 5, 8, 9, 14, 34, 46, 47)
 * Detección y atenuación quirúrgica de sibilancias ("S", "T", "Ch" entre 3 y 10 kHz).
 * Soporta modo Split-Band (atenuación exclusiva de la banda sibilante sin tocar el cuerpo de la voz),
 * modo Wideband (reducción global), ajuste de Q/frecuencia y modo Listen para calibración de monitoreo.
 */
class DeEsserNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Frequency = 1,  // 2000.0 a 12000.0 Hz (Frecuencia central de sibilancia)
        Bandwidth = 2,  // 0.5 a 5.0 Q (Ancho de banda de detección)
        Threshold = 3,  // -50.0 a 0.0 dB (Umbral de activación)
        Reduction = 4,  // 0.0 a 30.0 dB (Atenuación máxima de sibilantes)
        Mode = 5,       // 0.0 = Split Band, 1.0 = Wideband
        Listen = 6      // 0.0 = Normal, 1.0 = Monitoreo de banda aislada
    };

    DeEsserNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Frequency, "Frequency", 6500.0f, 2000.0f, 12000.0f, true };
        params_[1] = { Bandwidth, "Bandwidth Q", 1.5f, 0.5f, 5.0f, true };
        params_[2] = { Threshold, "Threshold", -20.0f, -50.0f, 0.0f, true };
        params_[3] = { Reduction, "Reduction", 12.0f, 0.0f, 30.0f, true };
        params_[4] = { Mode, "Mode", 0.0f, 0.0f, 1.0f, true };
        params_[5] = { Listen, "Listen Band", 0.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        const double sr = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

        detector_.prepare(sr);
        detector_.setAttackMs(1.0f);   // Ataque rápido para atrapar transitorios sibilantes
        detector_.setReleaseMs(35.0f); // Recuperación ágil

        currentGainReduction_ = 1.0f;

        bandpassFilters_[0].reset();
        bandpassFilters_[1].reset();

        updateFilters();
    }

    void reset() override {
        detector_.reset();
        currentGainReduction_ = 1.0f;
        bandpassFilters_[0].reset();
        bandpassFilters_[1].reset();
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const float threshold = targetThreshold_;
        const float maxReductionLin = EnvelopeDetector::dbToLinear(-targetReduction_);
        const bool isWideband = targetMode_ >= 0.5f;
        const bool isListen = targetListen_ >= 0.5f;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0]) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1]) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0]) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1]) ? context.outputChannels[1] : nullptr;

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            const float rawInL = inL ? inL[s] : 0.0f;
            const float rawInR = inR ? inR[s] : 0.0f;

            // 1. Filtrar banda sibilante por canal estéreo
            const float sibilantL = bandpassFilters_[0].processSample(rawInL);
            const float sibilantR = bandpassFilters_[1].processSample(rawInR);

            // Modo Listen: audición exclusiva de la banda sibilante para calibración de afinación
            if (isListen) {
                if (outL) outL[s] = sibilantL * 2.0f;
                if (outR) outR[s] = sibilantR * 2.0f;
                continue;
            }

            // 2. Detección de energía en la banda de sibilancias
            const float maxSibilant = std::max(std::abs(sibilantL), std::abs(sibilantR));
            const float env = detector_.processSample(maxSibilant);
            const float envDb = EnvelopeDetector::linearToDb(env);

            // 3. Cálculo de Gain Reduction
            float targetGrLinear = 1.0f;
            if (envDb > threshold) {
                const float excessDb = envDb - threshold;
                // Atenuación suave proporcional al exceso
                const float grDb = -std::min(excessDb * 1.5f, targetReduction_);
                targetGrLinear = std::max(maxReductionLin, EnvelopeDetector::dbToLinear(grDb));
            }

            // Suavizado anti-click del Gain Reduction
            currentGainReduction_ += 0.08f * (targetGrLinear - currentGainReduction_);

            // 4. Aplicación de reducción según modo
            if (isWideband) {
                // Modo Wideband: Atenúa toda la señal durante la sibilancia
                if (outL) outL[s] = rawInL * currentGainReduction_;
                if (outR) outR[s] = rawInR * currentGainReduction_;
            } else {
                // Modo Split-Band: Resta la banda sibilante, atenúa solo dicha banda y la suma de nuevo
                const float nonSibilantL = rawInL - sibilantL;
                const float nonSibilantR = rawInR - sibilantR;

                if (outL) outL[s] = nonSibilantL + sibilantL * currentGainReduction_;
                if (outR) outR[s] = nonSibilantR + sibilantR * currentGainReduction_;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Frequency: {
                targetFrequency_ = std::clamp(value, 2000.0f, 12000.0f);
                updateFilters();
                break;
            }
            case Bandwidth: {
                targetBandwidth_ = std::clamp(value, 0.5f, 5.0f);
                updateFilters();
                break;
            }
            case Threshold: targetThreshold_ = std::clamp(value, -50.0f, 0.0f); break;
            case Reduction: targetReduction_ = std::clamp(value, 0.0f, 30.0f); break;
            case Mode: targetMode_ = value >= 0.5f ? 1.0f : 0.0f; break;
            case Listen: targetListen_ = value >= 0.5f ? 1.0f : 0.0f; break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Frequency: return targetFrequency_;
            case Bandwidth: return targetBandwidth_;
            case Threshold: return targetThreshold_;
            case Reduction: return targetReduction_;
            case Mode: return targetMode_;
            case Listen: return targetListen_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::DeEsser; }
    const char* getName() const override { return "De-Esser"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateFilters() noexcept {
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        const float nyquist = static_cast<float>(sr * 0.49);
        const float f = std::clamp(targetFrequency_, 2000.0f, nyquist);
        const float q = std::clamp(targetBandwidth_, 0.5f, 5.0f);

        bandpassFilters_[0].setCoefficients(BiquadFilter::Type::Bandpass, sr, f, q);
        bandpassFilters_[1].setCoefficients(BiquadFilter::Type::Bandpass, sr, f, q);
    }

    ProcessSpec spec_;
    std::array<BiquadFilter, 2> bandpassFilters_;
    EnvelopeDetector detector_;

    float currentGainReduction_{ 1.0f };

    float targetFrequency_{ 6500.0f };
    float targetBandwidth_{ 1.5f };
    float targetThreshold_{ -20.0f };
    float targetReduction_{ 12.0f };
    float targetMode_{ 0.0f };
    float targetListen_{ 0.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<DeEsserNode> registerDeEsser(NodeType::DeEsser, "de_esser", "Dynamics");

} // namespace audio_graph
