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
 * @brief Spectral Frequency-Split Spatial Dynamic Panner Node (Rules 5, 8, 9, 14, 19, 34, 35, 46, 47)
 * Crossover Linkwitz-Riley de 4to orden (24 dB/oct) que divide el audio en bandas grave y aguda.
 * Permite posicionar o mover las frecuencias agudas hacia un extremo y los graves al opuesto,
 * con modulaciÃ³n reactiva dependiente del contenido espectral dinÃ¡mico en tiempo real,
 * profundidad psicoacÃºstica Haas ITD y protecciÃ³n de sub-graves en mono.
 */
class FrequencySplitPannerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        CrossoverHz = 1,
        LowPan = 2,
        HighPan = 3,
        DynamicAmount = 4,
        InertiaMs = 5,
        HaasSpread = 6,
        MonoBassHz = 7,
        Mix = 8
    };

    FrequencySplitPannerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { CrossoverHz, "Crossover (Hz)", 650.0f, 100.0f, 5000.0f, true };
        params_[1] = { LowPan, "Low Pan", -0.7f, -1.0f, 1.0f, true };        // Default left
        params_[2] = { HighPan, "High Pan", 0.7f, -1.0f, 1.0f, true };       // Default right
        params_[3] = { DynamicAmount, "Dynamic Reactivity", 0.6f, -1.0f, 1.0f, true };
        params_[4] = { InertiaMs, "Inertia (ms)", 25.0f, 5.0f, 200.0f, true };
        params_[5] = { HaasSpread, "Haas 3D Spread", 0.4f, 0.0f, 1.0f, true };
        params_[6] = { MonoBassHz, "Mono Bass (Hz)", 90.0f, 20.0f, 250.0f, true };
        params_[7] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_frequency_split_panner_create(44100.0f);
    }

    ~FrequencySplitPannerNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_frequency_split_panner_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::FrequencySplitPanner; }
    const char* getName() const override { return "Frequency Split Panner"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_frequency_split_panner_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_frequency_split_panner_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_frequency_split_panner_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_frequency_split_panner_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case CrossoverHz: return params_[0].defaultValue;
            case LowPan: return params_[1].defaultValue;
            case HighPan: return params_[2].defaultValue;
            case DynamicAmount: return params_[3].defaultValue;
            case InertiaMs: return params_[4].defaultValue;
            case HaasSpread: return params_[5].defaultValue;
            case MonoBassHz: return params_[6].defaultValue;
            case Mix: return params_[7].defaultValue;
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

        n8_rust_frequency_split_panner_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustFrequencySplitPanner* rustInstance_{ nullptr };
    ProcessSpec spec_{};
    std::array<PinDescriptor, 2> pins_{};
    std::array<ParameterInfo, 8> params_{};
};

inline AutoRegisterNode<FrequencySplitPannerNode> registerFrequencySplitPanner(NodeType::FrequencySplitPanner, "frequencysplitpanner", "Spatial");

} // namespace audio_graph
