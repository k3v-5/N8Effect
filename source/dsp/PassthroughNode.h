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
 * @brief Nodo de utilidad Passthrough / Ganancia (Regla 5, 8, 35, 46, 47)
 * Delegado al motor SIMD de alto rendimiento en Rust (n8_rust_passthrough).
 */
class PassthroughNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Gain = 1
    };

    PassthroughNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        params_[0] = { Gain, "Gain", 1.0f, 0.0f, 2.0f, true };

        rustNode_ = n8_rust_passthrough_create();
    }

    ~PassthroughNode() override {
        if (rustNode_) {
            n8_rust_passthrough_destroy(rustNode_);
            rustNode_ = nullptr;
        }
    }

    PassthroughNode(const PassthroughNode&) = delete;
    PassthroughNode& operator=(const PassthroughNode&) = delete;

    PassthroughNode(PassthroughNode&& other) noexcept
        : spec_(other.spec_),
          pins_(other.pins_),
          params_(other.params_),
          rustNode_(other.rustNode_) {
        other.rustNode_ = nullptr;
    }

    PassthroughNode& operator=(PassthroughNode&& other) noexcept {
        if (this != &other) {
            if (rustNode_) {
                n8_rust_passthrough_destroy(rustNode_);
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
            n8_rust_passthrough_prepare(rustNode_);
        }
    }

    void process(ProcessContext& context) override {
        if (!rustNode_ || context.numSamples == 0 || context.outputChannels == nullptr) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : nullptr;

        n8_rust_passthrough_process(
            rustNode_,
            inL,
            inR,
            outL,
            outR,
            static_cast<uint32_t>(context.numSamples)
        );
    }

    void reset() override {
        if (rustNode_) {
            n8_rust_passthrough_reset(rustNode_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustNode_) {
            n8_rust_passthrough_set_parameter(rustNode_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustNode_) {
            return n8_rust_passthrough_get_parameter(rustNode_, id);
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::Passthrough; }
    const char* getName() const override { return "Gain / Passthrough"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    RustPassthrough* rustNode_{ nullptr };
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 1> params_;
};

// Registro automático en NodeFactory (Regla 19 y 20)
inline AutoRegisterNode<PassthroughNode> registerPassthrough(NodeType::Passthrough, "passthrough", "Utility");

} // namespace audio_graph
