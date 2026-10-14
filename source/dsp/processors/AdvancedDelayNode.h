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
 * @brief Nodo de Delay Estéreo Avanzado con Ping-Pong, amortiguación y soporte de Tails (Reglas 5, 8, 12, 14, 18)
 * Delegado al motor DSP SIMD en Rust (n8_rust_core).
 */
class AdvancedDelayNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        TimeLeft = 1,
        TimeRight = 2,
        Feedback = 3,
        Damping = 4,
        PingPong = 5,
        DryWet = 6
    };

    AdvancedDelayNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { TimeLeft, "Time L", 300.0f, 1.0f, 2500.0f, true };
        params_[1] = { TimeRight, "Time R", 450.0f, 1.0f, 2500.0f, true };
        params_[2] = { Feedback, "Feedback", 0.45f, 0.0f, 0.95f, true };
        params_[3] = { Damping, "Damping", 6000.0f, 1000.0f, 20000.0f, true };
        params_[4] = { PingPong, "Ping Pong", 0.0f, 0.0f, 1.0f, false };
        params_[5] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };

        rustDelay_ = n8_rust_advanced_delay_create(44100.0f);
    }

    ~AdvancedDelayNode() override {
        if (rustDelay_) {
            n8_rust_advanced_delay_destroy(rustDelay_);
            rustDelay_ = nullptr;
        }
    }

    AdvancedDelayNode(const AdvancedDelayNode&) = delete;
    AdvancedDelayNode& operator=(const AdvancedDelayNode&) = delete;

    AdvancedDelayNode(AdvancedDelayNode&& other) noexcept
        : spec_(other.spec_), pins_(other.pins_), params_(other.params_), rustDelay_(other.rustDelay_) {
        other.rustDelay_ = nullptr;
    }

    AdvancedDelayNode& operator=(AdvancedDelayNode&& other) noexcept {
        if (this != &other) {
            if (rustDelay_) {
                n8_rust_advanced_delay_destroy(rustDelay_);
            }
            spec_ = other.spec_;
            pins_ = other.pins_;
            params_ = other.params_;
            rustDelay_ = other.rustDelay_;
            other.rustDelay_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustDelay_) {
            n8_rust_advanced_delay_prepare(rustDelay_, static_cast<float>(spec.sampleRate));
        }
    }

    void reset() override {
        if (rustDelay_) {
            n8_rust_advanced_delay_reset(rustDelay_);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47
        if (!rustDelay_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) ? context.outputChannels[1] : nullptr;

        n8_rust_advanced_delay_process(rustDelay_, inL, inR, outL, outR, context.numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        if (rustDelay_) {
            n8_rust_advanced_delay_set_parameter(rustDelay_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustDelay_) {
            return n8_rust_advanced_delay_get_parameter(rustDelay_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::AdvancedDelay; }
    const char* getName() const override { return "Advanced Delay"; }
    bool supportsTail() const override { return true; } // Soporte explícito de tails (Regla 18)
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 5.0); // 5 segundos de tail
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
    RustAdvancedDelay* rustDelay_{ nullptr };
};

inline AutoRegisterNode<AdvancedDelayNode> registerAdvancedDelay(NodeType::AdvancedDelay, "advanced_delay", "Time");

} // namespace audio_graph
