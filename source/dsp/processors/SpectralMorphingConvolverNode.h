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
 * @brief Spectral Morphing Convolver Node (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Motor de convolución y metamorfosis acústica continua entre dos respuestas al impulso sintéticas parametrizadas:
 * Catedral Cósmica vs Tanque de Cristal Resonante, con interpolación espectral sin desfases ni artefactos.
 */
class SpectralMorphingConvolverNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Morph = 1,
        DecayTimeS = 2,
        Damping = 3,
        PredelayMs = 4,
        StereoSpread = 5,
        Mix = 6
    };

    SpectralMorphingConvolverNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Morph, "Morph (0=Cathedral, 1=Crystal)", 0.50f, 0.0f, 1.0f, true };
        params_[1] = { DecayTimeS, "Decay Time (s)", 3.50f, 0.30f, 10.0f, true };
        params_[2] = { Damping, "High Freq Damping", 0.45f, 0.0f, 1.0f, true };
        params_[3] = { PredelayMs, "Pre-delay (ms)", 15.0f, 0.0f, 100.0f, true };
        params_[4] = { StereoSpread, "Stereo Spread", 0.80f, 0.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 0.65f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_spectral_morphing_convolver_create(44100.0f);
    }

    ~SpectralMorphingConvolverNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_spectral_morphing_convolver_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::SpectralMorphingConvolver; }
    const char* getName() const override { return "Spectral Morphing Convolver"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_spectral_morphing_convolver_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_spectral_morphing_convolver_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_spectral_morphing_convolver_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_spectral_morphing_convolver_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Morph: return params_[0].defaultValue;
            case DecayTimeS: return params_[1].defaultValue;
            case Damping: return params_[2].defaultValue;
            case PredelayMs: return params_[3].defaultValue;
            case StereoSpread: return params_[4].defaultValue;
            case Mix: return params_[5].defaultValue;
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

        n8_rust_spectral_morphing_convolver_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustSpectralMorphingConvolver* rustInstance_{ nullptr };
    ProcessSpec spec_{};
    std::array<PinDescriptor, 2> pins_{};
    std::array<ParameterInfo, 6> params_{};
};

inline AutoRegisterNode<SpectralMorphingConvolverNode> registerSpectralMorphingConvolver(
    NodeType::SpectralMorphingConvolver, "spectralmorphingconvolver", "Reverb");

} // namespace audio_graph
