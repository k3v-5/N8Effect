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
 * @brief Fractal Resonator / Golden-Ratio Comb Matrix (Rules 5, 8, 9, 12, 14, 19, 34, 46, 47)
 * Matriz de 6 resonadores fÃ­sicos acoplados con afinaciones basadas en la ProporciÃ³n Ãurea (phi^k).
 * Interconectados mediante una matriz unitaria de dispersiÃ³n de Householder con amortiguamiento acÃºstico,
 * produciendo campanas de vidrio microtonales, placas metÃ¡licas difusas y espacios fÃ­sicos hiperbÃ³licos.
 */
class FractalResonatorNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        BaseFreqHz = 1,
        FractalRatio = 2,
        Decay = 3,
        DampingHz = 4,
        StereoWidth = 5,
        Mix = 6
    };

    FractalResonatorNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { BaseFreqHz, "Base Freq", 261.63f, 20.0f, 4000.0f, true };
        params_[1] = { FractalRatio, "Fractal Ratio", 0.618f, 0.1f, 2.0f, true };
        params_[2] = { Decay, "Decay", 0.88f, 0.0f, 0.995f, true };
        params_[3] = { DampingHz, "Damping", 6500.0f, 400.0f, 20000.0f, true };
        params_[4] = { StereoWidth, "Width", 0.85f, 0.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 0.5f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_fractal_resonator_create(44100.0f);
    }

    ~FractalResonatorNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_fractal_resonator_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::FractalResonator; }
    const char* getName() const override { return "Fractal Resonator"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_fractal_resonator_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_fractal_resonator_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_fractal_resonator_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_fractal_resonator_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case BaseFreqHz: return params_[0].defaultValue;
            case FractalRatio: return params_[1].defaultValue;
            case Decay: return params_[2].defaultValue;
            case DampingHz: return params_[3].defaultValue;
            case StereoWidth: return params_[4].defaultValue;
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

        n8_rust_fractal_resonator_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    ProcessSpec spec_;
    RustFractalResonator* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<FractalResonatorNode> registerFractalResonator(NodeType::FractalResonator, "fractalresonator", "Resonator");

} // namespace audio_graph
