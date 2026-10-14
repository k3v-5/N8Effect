#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Spectral Resonance Tamer Node (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Supresor espectral dinámico inteligente de resonancias estridentes y frecuencias acumuladas (estilo Soothe2).
 * Monitorea picos no lineales respecto a la envolvente espectral media y aplica atenuación selectiva en tiempo real.
 */
class SpectralResonanceTamerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Sensitivity = 1,
        Depth = 2,
        SharpnessQ = 3,
        AttackMs = 4,
        ReleaseMs = 5,
        LowCutHz = 6,
        HighCutHz = 7,
        DeltaListen = 8,
        Mix = 9
    };

    SpectralResonanceTamerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Sensitivity, "Resonance Sensitivity", 0.60f, 0.0f, 1.0f, true };
        params_[1] = { Depth, "Suppression Depth", 0.70f, 0.0f, 1.0f, true };
        params_[2] = { SharpnessQ, "Sharpness (Filter Q)", 4.50f, 1.0f, 12.0f, true };
        params_[3] = { AttackMs, "Attack Time (ms)", 10.0f, 1.0f, 80.0f, true };
        params_[4] = { ReleaseMs, "Release Time (ms)", 80.0f, 10.0f, 500.0f, true };
        params_[5] = { LowCutHz, "Low Frequency Cut (Hz)", 120.0f, 20.0f, 1500.0f, true };
        params_[6] = { HighCutHz, "High Frequency Cut (Hz)", 14000.0f, 2000.0f, 20000.0f, true };
        params_[7] = { DeltaListen, "Delta / Difference Listen", 0.0f, 0.0f, 1.0f, true };
        params_[8] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_spectral_resonance_tamer_create(44100.0f);
    }

    ~SpectralResonanceTamerNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_spectral_resonance_tamer_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::SpectralResonanceTamer; }
    const char* getName() const override { return "Spectral Resonance Tamer"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_spectral_resonance_tamer_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_spectral_resonance_tamer_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_spectral_resonance_tamer_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_spectral_resonance_tamer_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Sensitivity: return params_[0].defaultValue;
            case Depth: return params_[1].defaultValue;
            case SharpnessQ: return params_[2].defaultValue;
            case AttackMs: return params_[3].defaultValue;
            case ReleaseMs: return params_[4].defaultValue;
            case LowCutHz: return params_[5].defaultValue;
            case HighCutHz: return params_[6].defaultValue;
            case DeltaListen: return params_[7].defaultValue;
            case Mix: return params_[8].defaultValue;
            default: return 0.0f;
        }
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (outL == nullptr || rustInstance_ == nullptr || inL == nullptr) return;

        n8_rust_spectral_resonance_tamer_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustSpectralResonanceTamer* rustInstance_{ nullptr };
    ProcessSpec spec_{};
    std::array<PinDescriptor, 2> pins_{};
    std::array<ParameterInfo, 9> params_{};
};

inline AutoRegisterNode<SpectralResonanceTamerNode> registerSpectralResonanceTamer(
    NodeType::SpectralResonanceTamer, "spectralresonancetamer", "Dynamics");

} // namespace audio_graph
