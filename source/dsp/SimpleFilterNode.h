#pragma once

#include <cmath>
#include <array>
#include <algorithm>
#include <span>
#include "../graph/AudioProcessorNode.h"
#include "../graph/NodeFactory.h"
#include "rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Filtro State-Variable de 2 polos (SVF) (Regla 5, 8, 14, 34, 38, 46, 47)
 * Implementa Lowpass, Highpass y Bandpass matemáticamente exactos con TPT (Topology-Preserving Transform).
 * Delegado al motor SIMD de alto rendimiento en Rust (n8_rust_simple_filter).
 */
class SimpleFilterNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        CutoffHz = 1,
        Resonance = 2,
        Mode = 3 // 0: LP, 1: HP, 2: BP
    };

    SimpleFilterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { 4, "FM In", PinType::AudioRateModulationInput, PinDataType::AudioStereo };
        params_[0] = { CutoffHz, "Cutoff", 1000.0f, 20.0f, 20000.0f, true };
        params_[1] = { Resonance, "Resonance", 0.707f, 0.1f, 10.0f, true };
        params_[2] = { Mode, "Mode", 0.0f, 0.0f, 2.0f, false };

        rustNode_ = n8_rust_simple_filter_create();
    }

    ~SimpleFilterNode() override {
        if (rustNode_) {
            n8_rust_simple_filter_destroy(rustNode_);
            rustNode_ = nullptr;
        }
    }

    SimpleFilterNode(const SimpleFilterNode&) = delete;
    SimpleFilterNode& operator=(const SimpleFilterNode&) = delete;

    SimpleFilterNode(SimpleFilterNode&& other) noexcept
        : spec_(other.spec_),
          pins_(other.pins_),
          params_(other.params_),
          rustNode_(other.rustNode_) {
        other.rustNode_ = nullptr;
    }

    SimpleFilterNode& operator=(SimpleFilterNode&& other) noexcept {
        if (this != &other) {
            if (rustNode_) {
                n8_rust_simple_filter_destroy(rustNode_);
            }
            spec_ = other.spec_;
            pins_ = other.pins_;
            params_ = other.params_;
            rustNode_ = other.rustNode_;
            other.rustNode_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustNode_) {
            n8_rust_simple_filter_prepare(rustNode_, spec.sampleRate);
        }
    }

    void reset() override {
        if (rustNode_) {
            n8_rust_simple_filter_reset(rustNode_);
        }
    }

    void process(ProcessContext& context) override {
        if (!rustNode_ || context.numSamples == 0 || context.outputChannels == nullptr) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1] : inL;

        const float* fmL = (context.numAudioRateModChannels > 0 && context.audioRateModChannels != nullptr && context.audioRateModChannels[0] != nullptr)
            ? context.audioRateModChannels[0] : nullptr;

        float* outL = (context.numOutputChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : nullptr;

        n8_rust_simple_filter_process(
            rustNode_,
            inL,
            inR,
            fmL,
            outL,
            outR,
            static_cast<uint32_t>(context.numSamples)
        );
    }

    void setParameter(ParameterId id, float value) override {
        if (rustNode_) {
            n8_rust_simple_filter_set_parameter(rustNode_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustNode_) {
            return n8_rust_simple_filter_get_parameter(rustNode_, id);
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::Filter; }
    const char* getName() const override { return "SVF Filter"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    RustSimpleFilter* rustNode_{ nullptr };
    std::array<PinDescriptor, 3> pins_;
    std::array<ParameterInfo, 3> params_;
};

inline AutoRegisterNode<SimpleFilterNode> registerSimpleFilter(NodeType::Filter, "filter", "Filters");

} // namespace audio_graph
