#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DelayLine.h"
#include "../core/PhaseVocoder.h"

namespace audio_graph {

/**
 * @brief Transpositor de Tono (+/- 24 Semitonos) híbrido de doble algoritmo (Reglas 3, 5, 8, 14, 17, 34, 46, 47):
 * - Modo 0: Doble cabezal analógico en dominio de tiempo con interpolación cúbica Hermite (baja latencia <5ms).
 * - Modo 1: Spectral Phase Vocoder HD con bloqueo de fase por picos (Identity Phase Locking, Laroche & Dolson).
 */
class PitchShifterNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Semitones = 1,
        FineCents = 2,
        WindowSizeMs = 3,
        DryWet = 4,
        Algorithm = 5,
        PhaseLocking = 6
    };

    PitchShifterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Semitones, "Semitones", 0.0f, -24.0f, 24.0f, true };
        params_[1] = { FineCents, "Fine", 0.0f, -100.0f, 100.0f, true };
        params_[2] = { WindowSizeMs, "Window (ms)", 45.0f, 10.0f, 120.0f, true };
        params_[3] = { DryWet, "Mix", 1.0f, 0.0f, 1.0f, true };
        params_[4] = { Algorithm, "Algorithm", 0.0f, 0.0f, 1.0f, true }; // 0: Dual-Head Time-Domain, 1: Phase Vocoder HD
        params_[5] = { PhaseLocking, "Phase Locking", 1.0f, 0.0f, 1.0f, true }; // 0.0 = Loose/Vintage, 1.0 = Strict Laroche-Dolson
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        updateWindowSize();

        // Reserva para máxima ventana de 120ms en el modo de dominio temporal
        const size_t maxDelay = static_cast<size_t>(spec.sampleRate * 0.35);
        delayL_.prepare(maxDelay);
        delayR_.prepare(maxDelay);

        // Preparar Phase Vocoder de alta definición (FFT 1024, Hop 256, 75% solapamiento)
        vocoderL_.prepare(1024, 256, static_cast<float>(spec.sampleRate));
        vocoderR_.prepare(1024, 256, static_cast<float>(spec.sampleRate));

        const size_t blockCapacity = std::max<size_t>(1024, spec.maximumBlockSize);
        vocoderTempL_.assign(blockCapacity, 0.0f);
        vocoderTempR_.assign(blockCapacity, 0.0f);

        reset();
    }

    void reset() override {
        delayL_.reset();
        delayR_.reset();
        phase_ = 0.0f;

        vocoderL_.reset();
        vocoderR_.reset();
        std::fill(vocoderTempL_.begin(), vocoderTempL_.end(), 0.0f);
        std::fill(vocoderTempR_.begin(), vocoderTempR_.end(), 0.0f);
    }

    void process(ProcessContext& context) override {
        updateWindowSize();

        const float totalSemitones = targetSemitones_ + targetFine_ * 0.01f;
        const float pitchRatio = std::pow(2.0f, totalSemitones / 12.0f);
        const float mix = targetMix_;
        const uint32_t numSamples = context.numSamples;

        float* outL = (context.numOutputChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : nullptr;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1] : inL;

        if (targetAlgorithm_ >= 0.5f) {
            // Modo 1: Phase Vocoder HD con Laroche-Dolson Identity Phase Locking
            const size_t chunkSize = vocoderTempL_.size();
            for (uint32_t offset = 0; offset < numSamples; offset += static_cast<uint32_t>(chunkSize)) {
                const uint32_t currentChunk = std::min(numSamples - offset, static_cast<uint32_t>(chunkSize));
                const float* chunkInL = inL ? (inL + offset) : nullptr;
                const float* chunkInR = inR ? (inR + offset) : nullptr;

                vocoderL_.process(chunkInL, vocoderTempL_.data(), currentChunk, pitchRatio, targetPhaseLocking_);
                vocoderR_.process(chunkInR, vocoderTempR_.data(), currentChunk, pitchRatio, targetPhaseLocking_);

                for (uint32_t s = 0; s < currentChunk; ++s) {
                    const uint32_t idx = offset + s;
                    const float dryL = inL ? inL[idx] : 0.0f;
                    const float dryR = inR ? inR[idx] : dryL;
                    if (outL) outL[idx] = dryL * (1.0f - mix) + vocoderTempL_[s] * mix;
                    if (outR) outR[idx] = dryR * (1.0f - mix) + vocoderTempR_[s] * mix;
                }
            }
        } else {
            // Modo 0: Doble cabezal analógico en dominio de tiempo (Baja Latencia / Cero Artefactos de Ventana)
            const float rate = 1.0f - pitchRatio;
            const float winSize = windowSizeSamples_;
            const float halfWin = winSize * 0.5f;

            for (uint32_t s = 0; s < numSamples; ++s) {
                float sampleL = inL ? inL[s] : 0.0f;
                float sampleR = inR ? inR[s] : sampleL;

                delayL_.write(sampleL);
                delayR_.write(sampleR);

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

                if (outL) outL[s] = sampleL * (1.0f - mix) + wetL * mix;
                if (outR) outR[s] = sampleR * (1.0f - mix) + wetR * mix;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Semitones: targetSemitones_ = std::clamp(value, -24.0f, 24.0f); break;
            case FineCents: targetFine_ = std::clamp(value, -100.0f, 100.0f); break;
            case WindowSizeMs: targetWindowMs_ = std::clamp(value, 10.0f, 120.0f); break;
            case DryWet: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            case Algorithm: targetAlgorithm_ = std::clamp(value, 0.0f, 1.0f); break;
            case PhaseLocking: targetPhaseLocking_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Semitones: return targetSemitones_;
            case FineCents: return targetFine_;
            case WindowSizeMs: return targetWindowMs_;
            case DryWet: return targetMix_;
            case Algorithm: return targetAlgorithm_;
            case PhaseLocking: return targetPhaseLocking_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::PitchShifter; }
    const char* getName() const override { return "Pitch Shifter"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override {
        if (targetAlgorithm_ >= 0.5f) {
            return vocoderL_.getLatencySamples() * 2;
        }
        return static_cast<uint32_t>(windowSizeSamples_ * 2.0f);
    }

    uint32_t getLatencySamples() const override {
        if (targetAlgorithm_ >= 0.5f) {
            return vocoderL_.getLatencySamples();
        }
        return 0;
    }

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

    PhaseVocoder vocoderL_;
    PhaseVocoder vocoderR_;
    std::vector<float> vocoderTempL_;
    std::vector<float> vocoderTempR_;

    float targetSemitones_{ 0.0f };
    float targetFine_{ 0.0f };
    float targetWindowMs_{ 45.0f };
    float targetMix_{ 1.0f };
    float targetAlgorithm_{ 0.0f };
    float targetPhaseLocking_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<PitchShifterNode> registerPitchShifter(NodeType::PitchShifter, "pitch_shifter", "Pitch");

} // namespace audio_graph
