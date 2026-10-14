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
 * @brief Microtonal Harmonizer Node (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Armonizador microtonal dual con cuantización de escalas no-temperadas:
 * 12-TET, 24-EDO (cuartos de tono), 19-EDO, 31-EDO y Just Intonation (5-limit).
 */
class MicrotonalHarmonizerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Voice1Semitones = 1,
        Voice1Cents = 2,
        Voice1Pan = 3,
        Voice2Semitones = 4,
        Voice2Cents = 5,
        Voice2Pan = 6,
        QuantizeScale = 7,
        Feedback = 8,
        Mix = 9
    };

    MicrotonalHarmonizerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Voice1Semitones, "Voice 1 Semitones", 0.0f, -24.0f, 24.0f, true };
        params_[1] = { Voice1Cents, "Voice 1 Cents Detune", 31.0f, -100.0f, 100.0f, true };
        params_[2] = { Voice1Pan, "Voice 1 Pan", -0.6f, -1.0f, 1.0f, true };
        params_[3] = { Voice2Semitones, "Voice 2 Semitones", 7.0f, -24.0f, 24.0f, true };
        params_[4] = { Voice2Cents, "Voice 2 Cents Detune", -14.0f, -100.0f, 100.0f, true };
        params_[5] = { Voice2Pan, "Voice 2 Pan", 0.6f, -1.0f, 1.0f, true };
        params_[6] = { QuantizeScale, "Scale (0=Free, 1=24-EDO, 2=19-EDO, 3=31-EDO, 4=Just)", 0.0f, 0.0f, 4.0f, true };
        params_[7] = { Feedback, "Harmonizer Feedback", 0.0f, 0.0f, 0.85f, true };
        params_[8] = { Mix, "Mix", 0.85f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_microtonal_harmonizer_create(44100.0f);
    }

    ~MicrotonalHarmonizerNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_microtonal_harmonizer_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::MicrotonalHarmonizer; }
    const char* getName() const override { return "Microtonal Harmonizer"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_microtonal_harmonizer_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_microtonal_harmonizer_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_microtonal_harmonizer_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_microtonal_harmonizer_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Voice1Semitones: return params_[0].defaultValue;
            case Voice1Cents: return params_[1].defaultValue;
            case Voice1Pan: return params_[2].defaultValue;
            case Voice2Semitones: return params_[3].defaultValue;
            case Voice2Cents: return params_[4].defaultValue;
            case Voice2Pan: return params_[5].defaultValue;
            case QuantizeScale: return params_[6].defaultValue;
            case Feedback: return params_[7].defaultValue;
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

        n8_rust_microtonal_harmonizer_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustMicrotonalHarmonizer* rustInstance_{ nullptr };
    ProcessSpec spec_{};
    std::array<PinDescriptor, 2> pins_{};
    std::array<ParameterInfo, 9> params_{};
};

inline AutoRegisterNode<MicrotonalHarmonizerNode> registerMicrotonalHarmonizer(
    NodeType::MicrotonalHarmonizer, "microtonalharmonizer", "Pitch");

} // namespace audio_graph
