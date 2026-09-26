#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include <numbers>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DelayLine.h"
#include "../core/BiquadFilter.h"
#include "../core/FastMath.h"
#include "../core/DenormalGuards.h"

namespace audio_graph {

/**
 * @brief Shimmer Reverb Engine algorítmico de alta densidad con lazo de realimentación armónico
 * transponible (+12st, +7st, -12st), difusores Allpass de entrada, tanque FDN 4-line,
 * amortiguamiento IIR, saturación sigmoidal analógica suave y ensanchador Mid/Side (Reglas 5, 8, 9, 14, 18, 45, 46, 47).
 */
class ShimmerReverbNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        DecayTime = 1,       // 0.2s a 30.0s (default: 4.0s)
        DampingHz = 2,       // 1000.0Hz a 20000.0Hz (default: 6500.0Hz)
        ShimmerMix = 3,      // 0.0 a 1.0 (default: 0.5)
        PitchInterval = 4,   // 0: +12st (Octava), 1: +7st (Quinta), 2: -12st (Sub-Octava)
        StereoWidth = 5,     // 0.0 a 2.0 (default: 1.2)
        DryWet = 6           // 0.0 a 1.0 (default: 0.45)
    };

    ShimmerReverbNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { DecayTime, "Decay", 4.0f, 0.2f, 30.0f, true };
        params_[1] = { DampingHz, "Damping", 6500.0f, 1000.0f, 20000.0f, true };
        params_[2] = { ShimmerMix, "Shimmer", 0.5f, 0.0f, 1.0f, true };
        params_[3] = { PitchInterval, "Interval", 0.0f, 0.0f, 2.0f, false };
        params_[4] = { StereoWidth, "Width", 1.2f, 0.0f, 2.0f, true };
        params_[5] = { DryWet, "Mix", 0.45f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        const double sr = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

        // Longitudes base mutuamente primas para máxima densidad modal en FDN
        baseLengthsMs_ = { 29.7f, 37.1f, 41.3f, 47.9f };

        // 1. Predelay y difusores Allpass de entrada (Schroeder)
        predelayLine_.prepare(static_cast<size_t>(sr * 0.25)); // 250ms
        diffuserLine1_.prepare(static_cast<size_t>(sr * 0.05));
        diffuserLine2_.prepare(static_cast<size_t>(sr * 0.05));

        // 2. Tanque FDN de 4 líneas
        for (size_t i = 0; i < 4; ++i) {
            const size_t maxDelay = static_cast<size_t>(sr * 0.5); // 500ms
            fdnLines_[i].prepare(maxDelay);
            dampFilters_[i].reset();
        }

        // 3. Líneas de delay para Pitch Shifter Hermite de doble cabezal (150ms)
        const size_t pitchDelayCap = static_cast<size_t>(sr * 0.2);
        pitchDelayL_.prepare(pitchDelayCap);
        pitchDelayR_.prepare(pitchDelayCap);
        pitchWindowSamples_ = std::max(64.0f, static_cast<float>(sr * 0.045)); // 45ms

        reset();
        updateFilters();
    }

    void reset() override {
        predelayLine_.reset();
        diffuserLine1_.reset();
        diffuserLine2_.reset();
        for (size_t i = 0; i < 4; ++i) {
            fdnLines_[i].reset();
            dampFilters_[i].reset();
        }
        pitchDelayL_.reset();
        pitchDelayR_.reset();
        pitchPhase_ = 0.0f;
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;
        updateFilters();

        const uint32_t numSamples = context.numSamples;
        if (numSamples == 0) return;

        const float sr = static_cast<float>(spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0);
        const float mix = targetMix_;
        const float shimmerMix = targetShimmerMix_;
        const float stereoWidth = targetWidth_;
        const float t60 = std::clamp(targetDecay_, 0.2f, 30.0f);

        // Intervalo de transposición
        float pitchRatio = 2.0f; // Default +12st (Octava arriba)
        const int intervalIdx = static_cast<int>(std::round(targetPitchInterval_));
        if (intervalIdx == 1) {
            pitchRatio = 1.498307f; // +7st (Quinta justa: 2^(7/12))
        } else if (intervalIdx == 2) {
            pitchRatio = 0.5f;      // -12st (Sub-octava: 2^(-12/12))
        }

        // Tasa de velocidad para el pitch shifter de doble cabezal
        const float pitchRate = 1.0f - pitchRatio;
        const float winSize = pitchWindowSamples_;
        const float halfWin = winSize * 0.5f;

        // Longitudes de delay y ganancias de decaimiento por línea FDN
        // g_i = 10^(-3 * d_i / T60)
        std::array<float, 4> delaySamples;
        std::array<float, 4> feedbackGains;
        for (size_t i = 0; i < 4; ++i) {
            const float dSec = (baseLengthsMs_[i] * 0.001f);
            delaySamples[i] = dSec * sr;
            // Coeficiente acotado rígidamente a <= 0.985 para prevenir feedback runaway (Reglas 11, 47)
            const float g = std::pow(10.0f, -3.0f * dSec / t60);
            feedbackGains[i] = std::min(0.985f, g);
        }

        // Retardos en muestras para difusores allpass de entrada (mutuamente primos)
        const float diffDelay1 = std::max(4.0f, sr * 0.0032f); // ~141 muestras a 44.1k
        const float diffDelay2 = std::max(4.0f, sr * 0.0086f); // ~379 muestras a 44.1k
        constexpr float allpassCoeff = 0.618f;                 // Proporción áurea

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : nullptr;

        for (uint32_t s = 0; s < numSamples; ++s) {
            const float dryL = inL ? inL[s] : 0.0f;
            const float dryR = inR ? inR[s] : dryL;
            const float monoIn = 0.5f * (dryL + dryR);

            // 1. Difusión de entrada Schroeder Allpass en 2 etapas
            // ap1: y = -g*x + x_del + g*y_del
            const float ap1Read = diffuserLine1_.readCubic(diffDelay1);
            const float ap1Out = -allpassCoeff * monoIn + ap1Read;
            diffuserLine1_.write(monoIn + allpassCoeff * ap1Out);

            const float ap2Read = diffuserLine2_.readCubic(diffDelay2);
            const float ap2Out = -allpassCoeff * ap1Out + ap2Read;
            diffuserLine2_.write(ap1Out + allpassCoeff * ap2Out);

            const float diffusedInput = ap2Out;

            // 2. Lectura y filtrado de amortiguamiento IIR de las 4 líneas FDN
            std::array<float, 4> lineOutputs;
            for (size_t i = 0; i < 4; ++i) {
                const float readVal = fdnLines_[i].readCubic(delaySamples[i]);
                lineOutputs[i] = dampFilters_[i].processSample(readVal);
            }

            // 3. Matriz ortogonal de Householder (lossless diffusion): (H * y)_i = y_i - 0.5 * sum(y)
            const float sumY = lineOutputs[0] + lineOutputs[1] + lineOutputs[2] + lineOutputs[3];
            const float halfSum = 0.5f * sumY;

            std::array<float, 4> diffused;
            for (size_t i = 0; i < 4; ++i) {
                diffused[i] = lineOutputs[i] - halfSum;
            }

            // 4. Extracción estéreo intermedia del tanque para alimentar la etapa Shimmer
            const float tankL = 0.5f * (diffused[0] + diffused[2]);
            const float tankR = 0.5f * (diffused[1] + diffused[3]);

            // 5. Transpositor armónico de tono en dominio de tiempo con lectura Hermite cúbica
            pitchDelayL_.write(tankL);
            pitchDelayR_.write(tankR);

            float d1 = pitchPhase_;
            while (d1 < 0.0f) d1 += winSize;
            while (d1 >= winSize) d1 -= winSize;

            float d2 = pitchPhase_ + halfWin;
            while (d2 < 0.0f) d2 += winSize;
            while (d2 >= winSize) d2 -= winSize;

            const float env1 = 1.0f - std::abs((2.0f * d1 / winSize) - 1.0f);
            const float env2 = 1.0f - std::abs((2.0f * d2 / winSize) - 1.0f);

            const float shimmerL = pitchDelayL_.readCubic(d1 + 10.0f) * env1 + pitchDelayL_.readCubic(d2 + 10.0f) * env2;
            const float shimmerR = pitchDelayR_.readCubic(d1 + 10.0f) * env1 + pitchDelayR_.readCubic(d2 + 10.0f) * env2;

            pitchPhase_ += pitchRate;
            if (pitchPhase_ >= winSize) pitchPhase_ -= winSize;
            else if (pitchPhase_ < 0.0f) pitchPhase_ += winSize;

            // 6. Inyección de retorno al tanque con saturación suave analógica FastMath::fastTanh
            // Previene runaway feedback y dispersa armónicos adicionales (Reglas 11, 47)
            const float returnL = FastMath::fastTanh(shimmerL);
            const float returnR = FastMath::fastTanh(shimmerR);

            // Re-escritura en las 4 líneas FDN
            const float feed0 = (1.0f - shimmerMix) * diffused[0] + shimmerMix * returnL;
            const float feed1 = (1.0f - shimmerMix) * diffused[1] + shimmerMix * returnR;
            const float feed2 = (1.0f - shimmerMix) * diffused[2] + shimmerMix * returnL;
            const float feed3 = (1.0f - shimmerMix) * diffused[3] + shimmerMix * returnR;

            fdnLines_[0].write(diffusedInput + feed0 * feedbackGains[0]);
            fdnLines_[1].write(diffusedInput + feed1 * feedbackGains[1]);
            fdnLines_[2].write(diffusedInput + feed2 * feedbackGains[2]);
            fdnLines_[3].write(diffusedInput + feed3 * feedbackGains[3]);

            // 7. Ensanchamiento espacial Mid/Side de la señal húmeda
            float wetL = tankL * (1.0f - shimmerMix * 0.3f) + returnL * (shimmerMix * 0.5f);
            float wetR = tankR * (1.0f - shimmerMix * 0.3f) + returnR * (shimmerMix * 0.5f);

            const float midSignal = 0.5f * (wetL + wetR);
            const float sideSignal = 0.5f * (wetL - wetR);
            wetL = midSignal + sideSignal * stereoWidth;
            wetR = midSignal - sideSignal * stereoWidth;

            // 8. Salida Dry/Wet
            if (outL) outL[s] = dryL * (1.0f - mix) + wetL * mix;
            if (outR) outR[s] = dryR * (1.0f - mix) + wetR * mix;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case DecayTime: targetDecay_ = std::clamp(value, 0.2f, 30.0f); break;
            case DampingHz: targetDamping_ = std::clamp(value, 1000.0f, 20000.0f); break;
            case ShimmerMix: targetShimmerMix_ = std::clamp(value, 0.0f, 1.0f); break;
            case PitchInterval: targetPitchInterval_ = std::clamp(value, 0.0f, 2.0f); break;
            case StereoWidth: targetWidth_ = std::clamp(value, 0.0f, 2.0f); break;
            case DryWet: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case DecayTime: return targetDecay_;
            case DampingHz: return targetDamping_;
            case ShimmerMix: return targetShimmerMix_;
            case PitchInterval: return targetPitchInterval_;
            case StereoWidth: return targetWidth_;
            case DryWet: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::ShimmerReverb; }
    const char* getName() const override { return "Shimmer Reverb"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 30.0); // Hasta 30s de cola
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateFilters() noexcept {
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        for (size_t i = 0; i < 4; ++i) {
            dampFilters_[i].setCoefficients(BiquadFilter::Type::Lowpass, sr, targetDamping_, 0.707f);
        }
    }

    ProcessSpec spec_;
    std::array<float, 4> baseLengthsMs_{ 29.7f, 37.1f, 41.3f, 47.9f };

    DelayLine predelayLine_;
    DelayLine diffuserLine1_;
    DelayLine diffuserLine2_;
    std::array<DelayLine, 4> fdnLines_;
    std::array<BiquadFilter, 4> dampFilters_;

    DelayLine pitchDelayL_;
    DelayLine pitchDelayR_;
    float pitchWindowSamples_{ 1984.0f };
    float pitchPhase_{ 0.0f };

    float targetDecay_{ 4.0f };
    float targetDamping_{ 6500.0f };
    float targetShimmerMix_{ 0.5f };
    float targetPitchInterval_{ 0.0f };
    float targetWidth_{ 1.2f };
    float targetMix_{ 0.45f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<ShimmerReverbNode> registerShimmerReverb(NodeType::ShimmerReverb, "shimmer_reverb", "Space");

} // namespace audio_graph
