#pragma once

#include <cmath>
#include <array>
#include <vector>
#include <span>
#include <memory>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DelayLine.h"
#include "../core/BiquadFilter.h"
#include "../core/DenormalGuards.h"
#include "../core/FastMath.h"
#include "../../core/RealtimePools.h"

namespace audio_graph {

/**
 * @brief Contenedor de Lazos de Realimentación Controlados con Protección Anti-Runaway (Reglas 6, 11, 12, 34, 46, 47).
 * Encapsula un lazo de feedback con retardo configurable, filtrado de amortiguamiento (damping),
 * bloqueo de continua (DC blocker), detector de energía RMS para limitación dinámica de sobre-ganancia
 * y saturación sigmoidal Padé (FastMath::fastTanh) para garantizar rigurosamente la estabilidad numérica.
 */
class FeedbackContainerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Feedback = 1,
        DelayTime = 2,
        Damping = 3,
        Threshold = 4,
        Mix = 5
    };

    FeedbackContainerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Feedback, "Feedback", 0.5f, 0.0f, 1.5f, true };
        params_[1] = { DelayTime, "Delay (ms)", 50.0f, 1.0f, 500.0f, true };
        params_[2] = { Damping, "Damping", 7000.0f, 1000.0f, 20000.0f, true };
        params_[3] = { Threshold, "Anti-Runaway Thresh", 1.0f, 0.5f, 1.5f, true };
        params_[4] = { Mix, "Mix", 0.5f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        
        // Bounded preallocated delay line: 550 ms máximo (Regla 47)
        const size_t maxDelaySamples = static_cast<size_t>(spec.sampleRate * 0.55);
        delayLine_[0].prepare(maxDelaySamples);
        delayLine_[1].prepare(maxDelaySamples);

        forwardBuffer_.prepare(spec.numOutputChannels, spec.maximumBlockSize);
        innerBuffer_.prepare(spec.numOutputChannels, spec.maximumBlockSize);

        updateFilters();
        reset();

        if (innerProcessor_) {
            innerProcessor_->prepare(spec);
        }

        currentFeedback_ = targetFeedback_;
        currentMix_ = targetMix_;
        currentDelayMs_ = targetDelayMs_;
    }

    void reset() override {
        delayLine_[0].reset();
        delayLine_[1].reset();
        dampingFilter_[0].reset();
        dampingFilter_[1].reset();
        dcBlocker_[0].reset();
        dcBlocker_[1].reset();
        rmsEnvelope_[0] = 0.0f;
        rmsEnvelope_[1] = 0.0f;
        currentFeedback_ = targetFeedback_;
        currentMix_ = targetMix_;
        currentDelayMs_ = targetDelayMs_;

        if (innerProcessor_) {
            innerProcessor_->reset();
        }
    }

    void setInnerProcessor(std::unique_ptr<AudioProcessorNode> processor) {
        innerProcessor_ = std::move(processor);
        if (innerProcessor_ && spec_.sampleRate > 0) {
            innerProcessor_->prepare(spec_);
        }
    }

    AudioProcessorNode* getInnerProcessor() noexcept { return innerProcessor_.get(); }
    const AudioProcessorNode* getInnerProcessor() const noexcept { return innerProcessor_.get(); }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Reglas 34 y 47
        const float alpha = 0.005f;        // Anti-click smoothing (Regla 35)

        // Factor de decaimiento del detector RMS de envolvente
        const float rmsAttack = 0.05f;
        const float rmsRelease = 0.001f;

        // Si tenemos un procesador interno, alimentamos su entrada con Audio In + Feedback
        float* forwardL = forwardBuffer_.getWritePointer(0);
        float* forwardR = forwardBuffer_.getWritePointer(1);

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            currentFeedback_ += alpha * (targetFeedback_ - currentFeedback_);
            currentMix_ += alpha * (targetMix_ - currentMix_);
            currentDelayMs_ += alpha * (targetDelayMs_ - currentDelayMs_);

            const float delaySamples = (currentDelayMs_ * 0.001f) * static_cast<float>(spec_.sampleRate);

            // 1. Leer muestras retroalimentadas desde las líneas de retardo
            const float fbReadL = delayLine_[0].readCubic(delaySamples);
            const float fbReadR = delayLine_[1].readCubic(delaySamples);

            const float inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr)
                ? context.inputChannels[0][s] : 0.0f;
            const float inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr)
                ? context.inputChannels[1][s] : inL;

            // Inyección al camino de avance
            forwardL[s] = inL + fbReadL * currentFeedback_;
            forwardR[s] = inR + fbReadR * currentFeedback_;
        }

        // Si hay procesador interno en el lazo, procesar en bloque (Regla 6)
        const float* wetSrcL = forwardL;
        const float* wetSrcR = forwardR;

        if (innerProcessor_) {
            const float* inPtrs[2] = { forwardL, forwardR };
            float* outPtrs[2] = { innerBuffer_.getWritePointer(0), innerBuffer_.getWritePointer(1) };
            ProcessContext innerCtx{
                .inputChannels = inPtrs,
                .outputChannels = outPtrs,
                .numInputChannels = 2,
                .numOutputChannels = 2,
                .numSamples = context.numSamples
            };
            innerProcessor_->process(innerCtx);
            wetSrcL = innerBuffer_.getReadPointer(0);
            wetSrcR = innerBuffer_.getReadPointer(1);
        }

        // 2. Procesar señal de realimentación: Damping + DC Blocker + Anti-Runaway + Padé tanh (Reglas 11, 12, 47)
        for (uint32_t s = 0; s < context.numSamples; ++s) {
            float sampleL = wetSrcL[s];
            float sampleR = wetSrcR[s];

            // Damping acústico pasa-bajos
            sampleL = dampingFilter_[0].processSample(sampleL);
            sampleR = dampingFilter_[1].processSample(sampleR);

            // Bloqueo de componente continua (DC Blocker)
            sampleL = dcBlocker_[0].processSample(sampleL);
            sampleR = dcBlocker_[1].processSample(sampleR);

            // Detección de energía RMS para protección anti-runaway (Regla 11 y 12)
            const float energyL = sampleL * sampleL;
            const float energyR = sampleR * sampleR;

            rmsEnvelope_[0] += (energyL > rmsEnvelope_[0] ? rmsAttack : rmsRelease) * (energyL - rmsEnvelope_[0]);
            rmsEnvelope_[1] += (energyR > rmsEnvelope_[1] ? rmsAttack : rmsRelease) * (energyR - rmsEnvelope_[1]);

            const float currentRmsL = std::sqrt(rmsEnvelope_[0]);
            const float currentRmsR = std::sqrt(rmsEnvelope_[1]);

            // Atenuación automática si la energía supera el umbral de seguridad
            if (currentRmsL > targetThreshold_) {
                const float excessL = currentRmsL - targetThreshold_;
                const float duckFactorL = targetThreshold_ / (targetThreshold_ + 4.0f * excessL);
                sampleL *= duckFactorL;
            }

            if (currentRmsR > targetThreshold_) {
                const float excessR = currentRmsR - targetThreshold_;
                const float duckFactorR = targetThreshold_ / (targetThreshold_ + 4.0f * excessR);
                sampleR *= duckFactorR;
            }

            // Saturador no lineal sigmoidal Padé: garantiza rango estricto [-1.0f, +1.0f] (Regla 47)
            const float satL = FastMath::fastTanh(sampleL);
            const float satR = FastMath::fastTanh(sampleR);

            // Almacenar en la línea de retardo para el siguiente ciclo
            delayLine_[0].write(satL);
            delayLine_[1].write(satR);

            // Mezcla Wet/Dry hacia la salida final del bloque
            const float inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr)
                ? context.inputChannels[0][s] : 0.0f;
            const float inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr)
                ? context.inputChannels[1][s] : inL;

            if (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) {
                context.outputChannels[0][s] = inL * (1.0f - currentMix_) + satL * currentMix_;
            }
            if (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) {
                context.outputChannels[1][s] = inR * (1.0f - currentMix_) + satR * currentMix_;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Feedback:  targetFeedback_ = std::clamp(value, 0.0f, 1.5f); break;
            case DelayTime: targetDelayMs_ = std::clamp(value, 1.0f, 500.0f); break;
            case Damping:   targetDamping_ = std::clamp(value, 1000.0f, 20000.0f); updateFilters(); break;
            case Threshold: targetThreshold_ = std::clamp(value, 0.5f, 1.5f); break;
            case Mix:       targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Feedback:  return targetFeedback_;
            case DelayTime: return targetDelayMs_;
            case Damping:   return targetDamping_;
            case Threshold: return targetThreshold_;
            case Mix:       return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Feedback; }
    const char* getName() const override { return "Feedback Loop"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateFilters() {
        if (spec_.sampleRate > 0) {
            dampingFilter_[0].setCoefficients(BiquadFilter::Type::Lowpass, spec_.sampleRate, targetDamping_, 0.707f);
            dampingFilter_[1].setCoefficients(BiquadFilter::Type::Lowpass, spec_.sampleRate, targetDamping_, 0.707f);
            dcBlocker_[0].setCoefficients(BiquadFilter::Type::Highpass, spec_.sampleRate, 35.0f, 0.707f);
            dcBlocker_[1].setCoefficients(BiquadFilter::Type::Highpass, spec_.sampleRate, 35.0f, 0.707f);
        }
    }

    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    DelayLine delayLine_[2];
    BiquadFilter dampingFilter_[2];
    BiquadFilter dcBlocker_[2];
    PreallocatedBuffer forwardBuffer_;
    PreallocatedBuffer innerBuffer_;
    std::unique_ptr<AudioProcessorNode> innerProcessor_;

    float targetFeedback_{ 0.5f };
    float currentFeedback_{ 0.5f };
    float targetDelayMs_{ 50.0f };
    float currentDelayMs_{ 50.0f };
    float targetDamping_{ 7000.0f };
    float targetThreshold_{ 1.0f };
    float targetMix_{ 0.5f };
    float currentMix_{ 0.5f };

    float rmsEnvelope_[2]{ 0.0f, 0.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
};

inline AutoRegisterNode<FeedbackContainerNode> registerFeedbackContainerNode(
    NodeType::Feedback, "Feedback Loop", "Containers"
);

} // namespace audio_graph
