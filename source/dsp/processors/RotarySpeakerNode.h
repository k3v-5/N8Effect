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
 * @brief Simulador de Altavoz Giratorio (Rotary Speaker / Leslie Cabinet)
 * Emulación dual de rotor de agudos (Horn) y tambor de graves (Drum) con Doppler, AM e inercia mecánica
 * (Reglas 5, 8, 14, 34, 46, 47)
 */
class RotarySpeakerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        SpeedMode = 1, // 0: Slow (Chorale), 1: Fast (Tremolo), 2: Brake (Off)
        HornSpeed = 2, // Escala de velocidad de agudos (0.5x a 1.5x)
        DrumSpeed = 3, // Escala de velocidad de graves (0.5x a 1.5x)
        Drive = 4,     // Saturación a válvulas del preamplificador (1.0 a 6.0)
        Spread = 5,    // Ancho estéreo de los micrófonos (0.0 a 1.0)
        Mix = 6        // Dry / Wet
    };

    RotarySpeakerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { SpeedMode, "Speed Mode", 1.0f, 0.0f, 2.0f, false };
        params_[1] = { HornSpeed, "Horn Rate", 1.0f, 0.5f, 1.5f, true };
        params_[2] = { DrumSpeed, "Drum Rate", 1.0f, 0.5f, 1.5f, true };
        params_[3] = { Drive, "Drive", 1.5f, 1.0f, 6.0f, true };
        params_[4] = { Spread, "Spread", 0.85f, 0.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        // Delay buffer para efecto Doppler: máximo 5 ms (Regla 47)
        maxDelaySamples_ = std::max<size_t>(static_cast<size_t>(spec.sampleRate * 0.005), 256);
        for (auto& buf : hornDelayBuffers_) {
            buf.assign(maxDelaySamples_, 0.0f);
        }
        delayWriteIdx_ = 0;

        const double sr = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
        for (size_t ch = 0; ch < 2; ++ch) {
            crossoverHPF_[ch].setCoefficients(BiquadFilter::Type::Highpass, sr, 800.0f, 0.707f);
            crossoverLPF_[ch].setCoefficients(BiquadFilter::Type::Lowpass, sr, 800.0f, 0.707f);
        }

        hornAngle_ = 0.0f;
        drumAngle_ = 0.0f;
        currentHornSpeed_ = 6.8f;
        currentDrumSpeed_ = 5.8f;

        reset();
    }

    void reset() override {
        for (auto& buf : hornDelayBuffers_) std::fill(buf.begin(), buf.end(), 0.0f);
        delayWriteIdx_ = 0;
        for (auto& f : crossoverHPF_) f.reset();
        for (auto& f : crossoverLPF_) f.reset();
        hornAngle_ = 0.0f;
        drumAngle_ = 0.0f;
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case SpeedMode: targetMode_ = static_cast<int>(std::round(value)); break;
            case HornSpeed: targetHornRate_ = std::clamp(value, 0.5f, 1.5f); break;
            case DrumSpeed: targetDrumRate_ = std::clamp(value, 0.5f, 1.5f); break;
            case Drive:     targetDrive_ = std::clamp(value, 1.0f, 6.0f); break;
            case Spread:    targetSpread_ = std::clamp(value, 0.0f, 1.0f); break;
            case Mix:       targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case SpeedMode: return static_cast<float>(targetMode_);
            case HornSpeed: return targetHornRate_;
            case DrumSpeed: return targetDrumRate_;
            case Drive:     return targetDrive_;
            case Spread:    return targetSpread_;
            case Mix:       return targetMix_;
            default:        return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::RotarySpeaker; }
    const char* getName() const override { return "Rotary Speaker"; }

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
        const float dt = 1.0f / sampleRate;

        // Velocidad objetivo según modo
        float targetHornHz = 0.0f;
        float targetDrumHz = 0.0f;
        if (targetMode_ == 0) { // Slow (Chorale)
            targetHornHz = 0.8f * targetHornRate_;
            targetDrumHz = 0.7f * targetDrumRate_;
        } else if (targetMode_ == 1) { // Fast (Tremolo)
            targetHornHz = 6.8f * targetHornRate_;
            targetDrumHz = 5.8f * targetDrumRate_;
        }

        // Inercia mecánica: el Horn acelera/frena en ~1.2s, el Drum en ~3.8s
        const float hornInertiaRate = dt * (targetMode_ == 1 ? 2.5f : 0.9f);
        const float drumInertiaRate = dt * (targetMode_ == 1 ? 0.9f : 0.35f);

        const float drive = targetDrive_;
        const float spread = targetSpread_;
        const float mix = targetMix_;
        constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;

        for (size_t s = 0; s < numSamples; ++s) {
            const float rawInL = inL ? inL[s] : 0.0f;
            const float rawInR = inR ? inR[s] : rawInL;

            // 1. Simulación de preamplificador de válvulas
            const float drivenL = FastMath::fastTanh(rawInL * drive);
            const float drivenR = FastMath::fastTanh(rawInR * drive);

            // 2. Crossover Linkwitz-Riley 800 Hz
            const float hornInL = crossoverHPF_[0].processSample(drivenL);
            const float hornInR = crossoverHPF_[1].processSample(drivenR);
            const float drumInL = crossoverLPF_[0].processSample(drivenL);
            const float drumInR = crossoverLPF_[1].processSample(drivenR);

            // Escribir en buffer de delay para Doppler del rotor de agudos
            hornDelayBuffers_[0][delayWriteIdx_] = hornInL;
            hornDelayBuffers_[1][delayWriteIdx_] = hornInR;

            // 3. Actualizar inercia de velocidad
            currentHornSpeed_ += hornInertiaRate * (targetHornHz - currentHornSpeed_);
            currentDrumSpeed_ += drumInertiaRate * (targetDrumHz - currentDrumSpeed_);

            hornAngle_ += twoPi * currentHornSpeed_ * dt;
            if (hornAngle_ >= twoPi) hornAngle_ -= twoPi;

            drumAngle_ -= twoPi * currentDrumSpeed_ * dt; // El tambor gira en sentido opuesto
            if (drumAngle_ <= -twoPi) drumAngle_ += twoPi;

            // 4. Modulación Doppler e intensidad AM del Horn (Agudos)
            const float hornSin = std::sin(hornAngle_);
            const float hornCos = std::cos(hornAngle_);

            // Doppler delay modulation (0.2 ms a 1.2 ms)
            const float dopplerSamplesL = (0.001f + 0.0003f * hornSin * spread) * sampleRate;
            const float dopplerSamplesR = (0.001f - 0.0003f * hornSin * spread) * sampleRate;

            const float hornAudioL = readInterpolated(0, dopplerSamplesL);
            const float hornAudioR = readInterpolated(1, dopplerSamplesR);

            // Modulación AM en contragiro
            const float hornAmL = 0.65f + 0.35f * hornCos;
            const float hornAmR = 0.65f - 0.35f * hornCos;

            const float hornOutL = hornAudioL * hornAmL;
            const float hornOutR = hornAudioR * hornAmR;

            // 5. Modulación AM del Drum (Graves)
            const float drumSin = std::sin(drumAngle_);
            const float drumAmL = 0.75f + 0.25f * drumSin * spread;
            const float drumAmR = 0.75f - 0.25f * drumSin * spread;

            const float drumOutL = drumInL * drumAmL;
            const float drumOutR = drumInR * drumAmR;

            delayWriteIdx_ = (delayWriteIdx_ + 1) % maxDelaySamples_;

            // 6. Suma de rotores y mezcla Dry/Wet
            const float wetL = (hornOutL + drumOutL) * 1.15f;
            const float wetR = (hornOutR + drumOutR) * 1.15f;

            if (outL) outL[s] = (1.0f - mix) * rawInL + mix * wetL;
            if (outR) outR[s] = (1.0f - mix) * rawInR + mix * wetR;
        }
    }

private:
    float readInterpolated(size_t ch, float delaySamples) const noexcept {
        const float readPos = static_cast<float>(delayWriteIdx_) - delaySamples;
        float wrappedPos = std::fmod(readPos, static_cast<float>(maxDelaySamples_));
        if (wrappedPos < 0.0f) wrappedPos += static_cast<float>(maxDelaySamples_);

        const size_t idx0 = static_cast<size_t>(wrappedPos);
        const size_t idx1 = (idx0 + 1) % maxDelaySamples_;
        const float frac = wrappedPos - static_cast<float>(idx0);

        return (1.0f - frac) * hornDelayBuffers_[ch][idx0] + frac * hornDelayBuffers_[ch][idx1];
    }

    ProcessSpec spec_;
    size_t maxDelaySamples_{ 512 };
    std::array<std::vector<float>, 2> hornDelayBuffers_;
    size_t delayWriteIdx_{ 0 };

    std::array<BiquadFilter, 2> crossoverHPF_;
    std::array<BiquadFilter, 2> crossoverLPF_;

    float hornAngle_{ 0.0f };
    float drumAngle_{ 0.0f };
    float currentHornSpeed_{ 6.8f };
    float currentDrumSpeed_{ 5.8f };

    int targetMode_{ 1 };
    float targetHornRate_{ 1.0f };
    float targetDrumRate_{ 1.0f };
    float targetDrive_{ 1.5f };
    float targetSpread_{ 0.85f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<RotarySpeakerNode> registerRotary(NodeType::RotarySpeaker, "rotary_speaker", "Modulation");

} // namespace audio_graph
