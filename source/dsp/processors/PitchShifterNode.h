#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DelayLine.h"

namespace audio_graph {

/**
 * @brief Transpositor de Tono (+/- 24 Semitonos) de doble cabezal con crossfade suave (Reglas 5, 8, 14, 17, 34)
 */
class PitchShifterNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Semitones = 1,
        FineCents = 2,
        WindowSizeMs = 3,
        DryWet = 4
    };

    PitchShifterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Semitones, "Semitones", 0.0f, -24.0f, 24.0f, true };
        params_[1] = { FineCents, "Fine", 0.0f, -100.0f, 100.0f, true };
        params_[2] = { WindowSizeMs, "Window (ms)", 45.0f, 10.0f, 120.0f, true };
        params_[3] = { DryWet, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        updateWindowSize();
        // Reserva para máxima ventana de 120ms
        const size_t maxDelay = static_cast<size_t>(spec.sampleRate * 0.35);
        delayL_.prepare(maxDelay);
        delayR_.prepare(maxDelay);
        reset();
    }

    void reset() override {
        delayL_.reset();
        delayR_.reset();
        phase_ = 0.0f;
    }

    void process(ProcessContext& context) override {
        updateWindowSize();

        const float totalSemitones = targetSemitones_ + targetFine_ * 0.01f;
        const float pitchRatio = std::pow(2.0f, totalSemitones / 12.0f);
        // Velocidad relativa del cabezal de lectura
        const float rate = 1.0f - pitchRatio;
        const float winSize = windowSizeSamples_;
        const float halfWin = winSize * 0.5f;
        const float mix = targetMix_;

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            float inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0][s] : 0.0f;
            float inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1][s] : inL;

            delayL_.write(inL);
            delayR_.write(inR);

            // Cabezal 1
            float d1 = phase_;
            while (d1 < 0.0f) d1 += winSize;
            while (d1 >= winSize) d1 -= winSize;

            // Cabezal 2 desfasado 180 grados
            float d2 = phase_ + halfWin;
            while (d2 < 0.0f) d2 += winSize;
            while (d2 >= winSize) d2 -= winSize;

            // Envolvente de crossfade triangular
            float env1 = 1.0f - std::abs((2.0f * d1 / winSize) - 1.0f);
            float env2 = 1.0f - std::abs((2.0f * d2 / winSize) - 1.0f);

            // Lectura interpolada Hermite cúbica para evitar artefactos (Regla 34)
            float wetL = delayL_.readCubic(d1 + 10.0f) * env1 + delayL_.readCubic(d2 + 10.0f) * env2;
            float wetR = delayR_.readCubic(d1 + 10.0f) * env1 + delayR_.readCubic(d2 + 10.0f) * env2;

            phase_ += rate;
            if (phase_ >= winSize) phase_ -= winSize;
            else if (phase_ < 0.0f) phase_ += winSize;

            if (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) {
                context.outputChannels[0][s] = inL * (1.0f - mix) + wetL * mix;
            }
            if (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) {
                context.outputChannels[1][s] = inR * (1.0f - mix) + wetR * mix;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Semitones: targetSemitones_ = std::clamp(value, -24.0f, 24.0f); break;
            case FineCents: targetFine_ = std::clamp(value, -100.0f, 100.0f); break;
            case WindowSizeMs: targetWindowMs_ = std::clamp(value, 10.0f, 120.0f); break;
            case DryWet: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Semitones: return targetSemitones_;
            case FineCents: return targetFine_;
            case WindowSizeMs: return targetWindowMs_;
            case DryWet: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::PitchShifter; }
    const char* getName() const override { return "Pitch Shifter"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override { return static_cast<uint32_t>(windowSizeSamples_ * 2.0f); }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateWindowSize() noexcept {
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        windowSizeSamples_ = std::max(64.0f, static_cast<float>(sr * targetWindowMs_ * 0.001));
    }

    ProcessSpec spec_;
    DelayLine delayL_;
    DelayLine delayR_;
    float windowSizeSamples_{ 2048.0f };
    float phase_{ 0.0f };

    float targetSemitones_{ 0.0f };
    float targetFine_{ 0.0f };
    float targetWindowMs_{ 45.0f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 4> params_;
};

inline AutoRegisterNode<PitchShifterNode> registerPitchShifter(NodeType::PitchShifter, "pitch_shifter", "Pitch");

} // namespace audio_graph
