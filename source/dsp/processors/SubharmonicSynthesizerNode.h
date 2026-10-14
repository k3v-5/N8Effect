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
 * @brief Subharmonic Synthesizer & Analog Bass Extender Node (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Sintetizador de sub-octavas analÃ³gicas (estilo dbx 120XP).
 * Filtra la fundamental grave con paso-banda, divide la frecuencia mediante flip-flops
 * de cruce por cero en -1 octava (f/2) y -2 octavas (f/4), suaviza en ondas senoidales puras,
 * modula la amplitud con un seguidor dinÃ¡mico y satura con calor analÃ³gico.
 */
class SubharmonicSynthesizerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        SubBandFreq = 1,
        SubOctave1_Level = 2,
        SubOctave2_Level = 3,
        Drive = 4,
        DryBassLevel = 5,
        Mix = 6
    };

    SubharmonicSynthesizerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { SubBandFreq, "Sub Tracking Freq (Hz)", 85.0f, 35.0f, 160.0f, true };
        params_[1] = { SubOctave1_Level, "Sub -1 Oct Level", 0.7f, 0.0f, 1.2f, true };
        params_[2] = { SubOctave2_Level, "Sub -2 Oct Level", 0.4f, 0.0f, 1.2f, true };
        params_[3] = { Drive, "Sub Drive", 1.2f, 1.0f, 4.0f, true };
        params_[4] = { DryBassLevel, "Direct Bass Level", 1.0f, 0.0f, 2.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_subharmonic_synthesizer_create(44100.0f);
    }

    ~SubharmonicSynthesizerNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_subharmonic_synthesizer_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::SubharmonicSynthesizer; }
    const char* getName() const override { return "Subharmonic Synthesizer"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_subharmonic_synthesizer_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_subharmonic_synthesizer_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_subharmonic_synthesizer_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_subharmonic_synthesizer_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case SubBandFreq: return params_[0].defaultValue;
            case SubOctave1_Level: return params_[1].defaultValue;
            case SubOctave2_Level: return params_[2].defaultValue;
            case Drive: return params_[3].defaultValue;
            case DryBassLevel: return params_[4].defaultValue;
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

        n8_rust_subharmonic_synthesizer_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustSubharmonicSynthesizer* rustInstance_{ nullptr };
    ProcessSpec spec_{};
    std::array<PinDescriptor, 2> pins_{};
    std::array<ParameterInfo, 6> params_{};
};

inline AutoRegisterNode<SubharmonicSynthesizerNode> registerSubharmonicSynthesizer(
    NodeType::SubharmonicSynthesizer, "subharmonicsynthesizer", "Dynamics");

} // namespace audio_graph
