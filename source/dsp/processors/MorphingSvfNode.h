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
 * @brief Nonlinear Morphing State-Variable Filter (Topology Warp) (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Filtro SVF ZDF/TPT con barrido continuo de topologÃ­a 360 grados (LP -> BP -> HP -> Notch -> Peak),
 * saturaciÃ³n suave de realimentaciÃ³n no lineal (auto-oscilaciÃ³n sin explosiÃ³n numÃ©rica),
 * calentamiento por drive asimÃ©trico y pin de modulaciÃ³n de frecuencia FM a audio-rate.
 */
class MorphingSvfNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        CutoffHz = 1,
        Resonance = 2,
        Morph = 3,
        Drive = 4,
        FmDepth = 5
    };

    MorphingSvfNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { 4, "FM In", PinType::AudioRateModulationInput, PinDataType::AudioStereo };

        params_[0] = { CutoffHz, "Cutoff", 1200.0f, 20.0f, 20000.0f, true };
        params_[1] = { Resonance, "Resonance", 1.5f, 0.1f, 30.0f, true };
        params_[2] = { Morph, "Morph", 0.0f, 0.0f, 1.0f, true };
        params_[3] = { Drive, "Drive", 1.2f, 1.0f, 10.0f, true };
        params_[4] = { FmDepth, "FM Depth", 0.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_morphing_svf_create(44100.0f);
    }

    ~MorphingSvfNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_morphing_svf_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::MorphingSVF; }
    const char* getName() const override { return "Morphing SVF"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_morphing_svf_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_morphing_svf_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_morphing_svf_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_morphing_svf_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case CutoffHz: return params_[0].defaultValue;
            case Resonance: return params_[1].defaultValue;
            case Morph: return params_[2].defaultValue;
            case Drive: return params_[3].defaultValue;
            case FmDepth: return params_[4].defaultValue;
            default: return 0.0f;
        }
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;
        const float* fmL = (context.numAudioRateModChannels > 0 && context.audioRateModChannels != nullptr && context.audioRateModChannels[0] != nullptr)
            ? context.audioRateModChannels[0] : nullptr;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (outL == nullptr || rustInstance_ == nullptr || inL == nullptr) return;

        n8_rust_morphing_svf_process(rustInstance_, inL, inR, fmL, outL, outR, context.numSamples);
    }

private:
    ProcessSpec spec_;
    RustMorphingSvf* rustInstance_{ nullptr };

    std::array<PinDescriptor, 3> pins_;
    std::array<ParameterInfo, 5> params_;
};

inline AutoRegisterNode<MorphingSvfNode> registerMorphingSvf(NodeType::MorphingSVF, "morphingsvf", "Filter");

} // namespace audio_graph
