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
 * @brief Formant Pitch Shifter Node (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Desacoplamiento vocal independiente de tono fundamental (pitch) y geometría de formantes/garganta.
 * Permite mutación de timbre anatómico (throat expansion/contraction) sin alterar la afinación melódica.
 */
class FormantPitchShifterNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        PitchSemitones = 1,
        FormantShiftSemitones = 2,
        ThroatMorph = 3,
        FormantIntensity = 4,
        Mix = 5
    };

    FormantPitchShifterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { PitchSemitones, "Pitch Shift (st)", 0.0f, -12.0f, 12.0f, true };
        params_[1] = { FormantShiftSemitones, "Formant Shift (st)", 0.0f, -12.0f, 12.0f, true };
        params_[2] = { ThroatMorph, "Throat Length Morph (-1=Child, +1=Deep)", 0.0f, -1.0f, 1.0f, true };
        params_[3] = { FormantIntensity, "Formant Intensity", 0.75f, 0.0f, 1.0f, true };
        params_[4] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_formant_pitch_shifter_create(44100.0f);
    }

    ~FormantPitchShifterNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_formant_pitch_shifter_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::FormantPitchShifter; }
    const char* getName() const override { return "Formant Pitch Shifter"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_formant_pitch_shifter_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_formant_pitch_shifter_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_formant_pitch_shifter_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_formant_pitch_shifter_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case PitchSemitones: return params_[0].defaultValue;
            case FormantShiftSemitones: return params_[1].defaultValue;
            case ThroatMorph: return params_[2].defaultValue;
            case FormantIntensity: return params_[3].defaultValue;
            case Mix: return params_[4].defaultValue;
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

        n8_rust_formant_pitch_shifter_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustFormantPitchShifter* rustInstance_{ nullptr };
    ProcessSpec spec_{};
    std::array<PinDescriptor, 2> pins_{};
    std::array<ParameterInfo, 5> params_{};
};

inline AutoRegisterNode<FormantPitchShifterNode> registerFormantPitchShifter(
    NodeType::FormantPitchShifter, "formantpitchshifter", "Pitch");

} // namespace audio_graph
