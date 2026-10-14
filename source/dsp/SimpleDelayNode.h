#pragma once

#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <span>
#include "../graph/AudioProcessorNode.h"
#include "../graph/NodeFactory.h"
#include "rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Nodo de Delay con Feedback e interpolación suave (Reglas 5, 8, 14, 18, 35, 46, 47)
 * Delegado al motor SIMD de alto rendimiento en Rust (n8_rust_simple_delay).
 */
class SimpleDelayNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        DelayTimeMs = 1,
        Feedback = 2,
        DryWet = 3
    };

    SimpleDelayNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        params_[0] = { DelayTimeMs, "Time", 250.0f, 1.0f, 2000.0f, true };
        params_[1] = { Feedback, "Feedback", 0.4f, 0.0f, 0.95f, true };
        params_[2] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };

        rustNode_ = n8_rust_simple_delay_create();
    }

    ~SimpleDelayNode() override {
        if (rustNode_) {
            n8_rust_simple_delay_destroy(rustNode_);
            rustNode_ = nullptr;
        }
    }

    SimpleDelayNode(const SimpleDelayNode&) = delete;
    SimpleDelayNode& operator=(const SimpleDelayNode&) = delete;

    SimpleDelayNode(SimpleDelayNode&& other) noexcept
        : spec_(other.spec_),
          pins_(other.pins_),
          params_(other.params_),
          rustNode_(other.rustNode_) {
        other.rustNode_ = nullptr;
    }

    SimpleDelayNode& operator=(SimpleDelayNode&& other) noexcept {
        if (this != &other) {
            if (rustNode_) {
                n8_rust_simple_delay_destroy(rustNode_);
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
            n8_rust_simple_delay_prepare(rustNode_, spec.sampleRate);
        }
    }

    void reset() override {
        if (rustNode_) {
            n8_rust_simple_delay_reset(rustNode_);
        }
    }

    void process(ProcessContext& context) override {
        if (!rustNode_ || context.numSamples == 0 || context.outputChannels == nullptr) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : nullptr;

        n8_rust_simple_delay_process(
            rustNode_,
            inL,
            inR,
            outL,
            outR,
            static_cast<uint32_t>(context.numSamples)
        );
    }

    void setParameter(ParameterId id, float value) override {
        if (rustNode_) {
            n8_rust_simple_delay_set_parameter(rustNode_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustNode_) {
            return n8_rust_simple_delay_get_parameter(rustNode_, id);
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::Delay; }
    const char* getName() const override { return "Stereo Delay"; }
    bool supportsTail() const override { return true; } // Soporte explícito de tails (Regla 18)
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 3.0); // Hasta 3 segundos de tail
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    RustSimpleDelay* rustNode_{ nullptr };
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 3> params_;
};

inline AutoRegisterNode<SimpleDelayNode> registerSimpleDelay(NodeType::Delay, "delay", "Time");

} // namespace audio_graph
