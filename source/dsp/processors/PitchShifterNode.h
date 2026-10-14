#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Transpositor de Tono (+/- 24 Semitonos) híbrido de doble algoritmo (Reglas 3, 5, 8, 14, 17, 34, 46, 47):
 * - Modo 0: Doble cabezal analógico en dominio de tiempo con interpolación cúbica Hermite (baja latencia <5ms).
 * - Modo 1: Spectral Phase Vocoder HD con bloqueo de fase por picos (Identity Phase Locking, Laroche & Dolson).
 * Delegado al motor SIMD de alto rendimiento Rust (n8_rust_pitch_shifter).
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

        rustNode_ = n8_rust_pitch_shifter_create();
    }

    ~PitchShifterNode() override {
        if (rustNode_) {
            n8_rust_pitch_shifter_destroy(rustNode_);
            rustNode_ = nullptr;
        }
    }

    PitchShifterNode(const PitchShifterNode&) = delete;
    PitchShifterNode& operator=(const PitchShifterNode&) = delete;

    PitchShifterNode(PitchShifterNode&& other) noexcept
        : spec_(other.spec_),
          windowSizeSamples_(other.windowSizeSamples_),
          targetSemitones_(other.targetSemitones_),
          targetFine_(other.targetFine_),
          targetWindowMs_(other.targetWindowMs_),
          targetMix_(other.targetMix_),
          targetAlgorithm_(other.targetAlgorithm_),
          targetPhaseLocking_(other.targetPhaseLocking_),
          pins_(other.pins_),
          params_(other.params_),
          rustNode_(other.rustNode_) {
        other.rustNode_ = nullptr;
    }

    PitchShifterNode& operator=(PitchShifterNode&& other) noexcept {
        if (this != &other) {
            if (rustNode_) {
                n8_rust_pitch_shifter_destroy(rustNode_);
            }
            spec_ = other.spec_;
            windowSizeSamples_ = other.windowSizeSamples_;
            targetSemitones_ = other.targetSemitones_;
            targetFine_ = other.targetFine_;
            targetWindowMs_ = other.targetWindowMs_;
            targetMix_ = other.targetMix_;
            targetAlgorithm_ = other.targetAlgorithm_;
            targetPhaseLocking_ = other.targetPhaseLocking_;
            pins_ = other.pins_;
            params_ = other.params_;
            rustNode_ = other.rustNode_;
            other.rustNode_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        updateWindowSize();
        if (rustNode_) {
            n8_rust_pitch_shifter_prepare(rustNode_, static_cast<float>(spec.sampleRate));
        }
    }

    void reset() override {
        if (rustNode_) {
            n8_rust_pitch_shifter_reset(rustNode_);
        }
    }

    void process(ProcessContext& context) override {
        if (!rustNode_ || context.numSamples == 0 || context.numInputChannels == 0 || context.numOutputChannels == 0) return;

        updateWindowSize();

        const float* inL = context.inputChannels[0];
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : inL;

        float* outL = context.outputChannels[0];
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : outL;

        n8_rust_pitch_shifter_process(rustNode_, inL, inR, outL, outR, context.numSamples);
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
        if (rustNode_) {
            n8_rust_pitch_shifter_set_parameter(rustNode_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustNode_) {
            return n8_rust_pitch_shifter_get_parameter(rustNode_, static_cast<uint32_t>(id));
        }
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
            return 2048; // 1024 * 2
        }
        return static_cast<uint32_t>(windowSizeSamples_ * 2.0f);
    }

    uint32_t getLatencySamples() const override {
        if (targetAlgorithm_ >= 0.5f) {
            return 1024;
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
    float windowSizeSamples_{ 2048.0f };

    float targetSemitones_{ 0.0f };
    float targetFine_{ 0.0f };
    float targetWindowMs_{ 45.0f };
    float targetMix_{ 1.0f };
    float targetAlgorithm_{ 0.0f };
    float targetPhaseLocking_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
    RustPitchShifter* rustNode_{ nullptr };
};

inline AutoRegisterNode<PitchShifterNode> registerPitchShifter(NodeType::PitchShifter, "pitch_shifter", "Pitch");

} // namespace audio_graph
