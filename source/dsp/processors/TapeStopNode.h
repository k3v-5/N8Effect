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
 * @brief Simulador de Frenado y Arranque de Cinta/Vinilo (Tape Stop & Spin-Up)
 * (Reglas 5, 8, 14, 34, 35, 46, 47)
 */
class TapeStopNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Trigger = 1,     // 0 = Normal / Play, 1 = Brake / Stop
        StopTime = 2,    // Segundos de frenado (0.1s a 4.0s)
        SpinUpTime = 3,  // Segundos de arranque (0.05s a 2.0s)
        Curve = 4,       // 0: Lineal, 1: Exponencial, 2: Curva S
        Inertia = 5,     // Fluctuación/wobble de arrastre mecánico
        Mix = 6          // Dry / Wet
    };

    TapeStopNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Trigger, "Stop", 0.0f, 0.0f, 1.0f, false };
        params_[1] = { StopTime, "Stop Time", 0.6f, 0.05f, 4.0f, true };
        params_[2] = { SpinUpTime, "Spin Up", 0.3f, 0.05f, 2.0f, true };
        params_[3] = { Curve, "Curve", 1.0f, 0.0f, 2.0f, false };
        params_[4] = { Inertia, "Inertia", 0.35f, 0.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        // Bounded ring buffer: 4 segundos de audio prealocado (Regla 47)
        maxDelaySamples_ = std::max<size_t>(static_cast<size_t>(spec.sampleRate * 4.0), 1024);
        for (auto& buf : ringBuffers_) {
            buf.assign(maxDelaySamples_, 0.0f);
        }

        writeIndex_ = 0;
        readIndex_ = 0.0;
        currentSpeed_ = 1.0f;
        targetSpeed_ = 1.0f;
        progress_ = 0.0f;
        wobblePhase_ = 0.0f;

        for (auto& f : lpfFilters_) {
            f.reset();
            f.setCoefficients(BiquadFilter::Type::Lowpass, spec_.sampleRate, 18000.0f, 0.707f);
        }

        reset();
    }

    void reset() override {
        for (auto& buf : ringBuffers_) {
            std::fill(buf.begin(), buf.end(), 0.0f);
        }
        writeIndex_ = 0;
        readIndex_ = 0.0;
        currentSpeed_ = 1.0f;
        targetSpeed_ = 1.0f;
        progress_ = 0.0f;
        wobblePhase_ = 0.0f;
        for (auto& f : lpfFilters_) f.reset();
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Trigger:
                targetTrigger_ = (value >= 0.5f);
                targetSpeed_ = targetTrigger_ ? 0.0f : 1.0f;
                break;
            case StopTime:   targetStopTime_ = std::clamp(value, 0.05f, 4.0f); break;
            case SpinUpTime: targetSpinUpTime_ = std::clamp(value, 0.05f, 2.0f); break;
            case Curve:      targetCurve_ = static_cast<int>(std::round(value)); break;
            case Inertia:    targetInertia_ = std::clamp(value, 0.0f, 1.0f); break;
            case Mix:        targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Trigger:    return targetTrigger_ ? 1.0f : 0.0f;
            case StopTime:   return targetStopTime_;
            case SpinUpTime: return targetSpinUpTime_;
            case Curve:      return static_cast<float>(targetCurve_);
            case Inertia:    return targetInertia_;
            case Mix:        return targetMix_;
            default:         return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::TapeStop; }
    const char* getName() const override { return "Tape Stop"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47

        if (context.numSamples == 0 || maxDelaySamples_ == 0) return;

        const size_t numSamples = context.numSamples;
        const size_t numChannels = std::min<size_t>(context.numOutputChannels, 2);

        const float* inL = (context.numInputChannels > 0) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : inL;
        float* outL = (numChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (numChannels > 1) ? context.outputChannels[1] : outL;

        const float sampleRate = static_cast<float>(spec_.sampleRate > 0 ? spec_.sampleRate : 44100.0);
        const float stopRateDelta = 1.0f / (std::max(targetStopTime_, 0.05f) * sampleRate);
        const float spinRateDelta = 1.0f / (std::max(targetSpinUpTime_, 0.05f) * sampleRate);

        for (size_t s = 0; s < numSamples; ++s) {
            const float rawInL = inL ? inL[s] : 0.0f;
            const float rawInR = inR ? inR[s] : rawInL;

            // 1. Escribir audio en el buffer circular
            ringBuffers_[0][writeIndex_] = rawInL;
            ringBuffers_[1][writeIndex_] = rawInR;

            // 2. Transición de velocidad (Frenado / Arranque)
            if (targetTrigger_) {
                // Frenando
                if (currentSpeed_ > 0.0001f) {
                    if (targetCurve_ == 1) { // Exponencial
                        currentSpeed_ = std::max(0.0f, currentSpeed_ - stopRateDelta * (currentSpeed_ + 0.1f));
                    } else if (targetCurve_ == 2) { // Curva S
                        float t = 1.0f - currentSpeed_;
                        float factor = 3.0f * t * (1.0f - t) + 0.2f;
                        currentSpeed_ = std::max(0.0f, currentSpeed_ - stopRateDelta * factor);
                    } else { // Lineal
                        currentSpeed_ = std::max(0.0f, currentSpeed_ - stopRateDelta);
                    }
                } else {
                    currentSpeed_ = 0.0f;
                }
            } else {
                // Acelerando (Spin-Up)
                if (currentSpeed_ < 0.9999f) {
                    currentSpeed_ = std::min(1.0f, currentSpeed_ + spinRateDelta);
                } else {
                    currentSpeed_ = 1.0f;
                    readIndex_ = static_cast<double>(writeIndex_);
                }
            }

            // 3. Fluctuación analógica (Wow/Wobble al frenar)
            float speedWithWobble = currentSpeed_;
            if (currentSpeed_ > 0.01f && currentSpeed_ < 0.95f && targetInertia_ > 0.01f) {
                wobblePhase_ += (4.0f * currentSpeed_) / sampleRate;
                if (wobblePhase_ >= 1.0f) wobblePhase_ -= 1.0f;
                float wobble = std::sin(wobblePhase_ * 2.0f * std::numbers::pi_v<float>) * 0.08f * targetInertia_ * (1.0f - currentSpeed_);
                speedWithWobble = std::max(0.0f, currentSpeed_ + wobble);
            }

            // 4. Lectura con interpolación cúbica Hermite de 4 puntos
            float wetL = 0.0f;
            float wetR = 0.0f;

            if (currentSpeed_ > 0.0005f) {
                wetL = readHermite(0, readIndex_);
                wetR = readHermite(1, readIndex_);

                // Filtro pasobajos dinámico que atenúa frecuencias altas al frenar
                float cutoff = std::clamp(18000.0f * (currentSpeed_ * currentSpeed_), 250.0f, 18000.0f);
                lpfFilters_[0].setCoefficients(BiquadFilter::Type::Lowpass, sampleRate, cutoff, 0.707f);
                lpfFilters_[1].setCoefficients(BiquadFilter::Type::Lowpass, sampleRate, cutoff, 0.707f);
                wetL = lpfFilters_[0].processSample(wetL);
                wetR = lpfFilters_[1].processSample(wetR);

                // Avanzar puntero de lectura proporcional a la velocidad
                readIndex_ += static_cast<double>(speedWithWobble);
                if (readIndex_ >= static_cast<double>(maxDelaySamples_)) {
                    readIndex_ -= static_cast<double>(maxDelaySamples_);
                }
            } else {
                // Detenido por completo: silencio absoluto sin ruidos
                wetL = 0.0f;
                wetR = 0.0f;
            }

            // Incrementar índice de escritura
            writeIndex_ = (writeIndex_ + 1) % maxDelaySamples_;

            // 5. Mezcla Dry/Wet con suavizado anti-click
            const float mix = targetMix_;
            if (outL) outL[s] = (1.0f - mix) * rawInL + mix * wetL;
            if (outR) outR[s] = (1.0f - mix) * rawInR + mix * wetR;
        }
    }

private:
    float readHermite(size_t ch, double pos) const noexcept {
        const double wrappedPos = std::fmod(pos, static_cast<double>(maxDelaySamples_));
        const double validPos = (wrappedPos < 0.0) ? (wrappedPos + static_cast<double>(maxDelaySamples_)) : wrappedPos;

        const size_t i1 = static_cast<size_t>(validPos);
        const size_t i0 = (i1 + maxDelaySamples_ - 1) % maxDelaySamples_;
        const size_t i2 = (i1 + 1) % maxDelaySamples_;
        const size_t i3 = (i1 + 2) % maxDelaySamples_;

        const float frac = static_cast<float>(validPos - static_cast<double>(i1));

        const float y0 = ringBuffers_[ch][i0];
        const float y1 = ringBuffers_[ch][i1];
        const float y2 = ringBuffers_[ch][i2];
        const float y3 = ringBuffers_[ch][i3];

        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    ProcessSpec spec_;
    size_t maxDelaySamples_{ 0 };
    std::array<std::vector<float>, 2> ringBuffers_;
    size_t writeIndex_{ 0 };
    double readIndex_{ 0.0 };

    float currentSpeed_{ 1.0f };
    float targetSpeed_{ 1.0f };
    bool targetTrigger_{ false };
    float targetStopTime_{ 0.6f };
    float targetSpinUpTime_{ 0.3f };
    int targetCurve_{ 1 };
    float targetInertia_{ 0.35f };
    float targetMix_{ 1.0f };

    float progress_{ 0.0f };
    float wobblePhase_{ 0.0f };
    std::array<BiquadFilter, 2> lpfFilters_;

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<TapeStopNode> registerTapeStop(NodeType::TapeStop, "tape_stop", "Special / Time");

} // namespace audio_graph
