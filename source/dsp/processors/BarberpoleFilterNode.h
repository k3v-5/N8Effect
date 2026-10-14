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
 * @brief Shepard-Risset Barberpole Filter & Infinite Motion Comb (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * IlusiÃ³n psicoacÃºstica de movimiento espectral continuo e infinito (ascenso o descenso eterno).
 * Emplea 8 etapas de filtros pasatodo escalonadas con envolvente de Hann/Raised-Cosine,
 * realimentaciÃ³n saturante no-lineal y desfase estÃ©reo en cuadratura.
 */
class BarberpoleFilterNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        RateHz = 1,
        Feedback = 2,
        StereoSpread = 3,
        FreqMin = 4,
        FreqMax = 5,
        Mix = 6
    };

    BarberpoleFilterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { RateHz, "Rate (Hz)", 0.35f, -4.0f, 4.0f, true };
        params_[1] = { Feedback, "Feedback", 0.72f, -0.95f, 0.95f, true };
        params_[2] = { StereoSpread, "Stereo Spread", 0.5f, 0.0f, 1.0f, true };
        params_[3] = { FreqMin, "Min Freq", 80.0f, 40.0f, 500.0f, true };
        params_[4] = { FreqMax, "Max Freq", 12000.0f, 2000.0f, 18000.0f, true };
        params_[5] = { Mix, "Mix", 0.65f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_barberpole_filter_create(44100.0f);
    }

    ~BarberpoleFilterNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_barberpole_filter_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::BarberpoleFilter; }
    const char* getName() const override { return "Barberpole Filter"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_barberpole_filter_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_barberpole_filter_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_barberpole_filter_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_barberpole_filter_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case RateHz: return params_[0].defaultValue;
            case Feedback: return params_[1].defaultValue;
            case StereoSpread: return params_[2].defaultValue;
            case FreqMin: return params_[3].defaultValue;
            case FreqMax: return params_[4].defaultValue;
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

        n8_rust_barberpole_filter_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    ProcessSpec spec_;
    RustBarberpoleFilter* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<BarberpoleFilterNode> registerBarberpoleFilter(NodeType::BarberpoleFilter, "barberpolefilter", "Filter");

} // namespace audio_graph
