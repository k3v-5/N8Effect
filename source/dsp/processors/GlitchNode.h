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
 * @brief Procesador de Glitch, Stutter y Rebanado rítmico con suavizado Anti-Click (Reglas 5, 8, 9, 35, 37)
 * Delegado al motor DSP SIMD en Rust (n8_rust_core).
 */
class GlitchNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Division = 1,
        RepeatProbability = 2,
        ReverseProbability = 3,
        StutterGate = 4,
        DryWet = 5
    };

    GlitchNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Division, "Division", 2.0f, 0.0f, 4.0f, false };
        params_[1] = { RepeatProbability, "Repeat Prob", 0.5f, 0.0f, 1.0f, true };
        params_[2] = { ReverseProbability, "Reverse Prob", 0.2f, 0.0f, 1.0f, true };
        params_[3] = { StutterGate, "Stutter Gate", 1.0f, 0.05f, 1.0f, true };
        params_[4] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };

        rustGlitch_ = n8_rust_glitch_create(44100.0f);
    }

    ~GlitchNode() override {
        if (rustGlitch_) {
            n8_rust_glitch_destroy(rustGlitch_);
            rustGlitch_ = nullptr;
        }
    }

    GlitchNode(const GlitchNode&) = delete;
    GlitchNode& operator=(const GlitchNode&) = delete;

    GlitchNode(GlitchNode&& other) noexcept
        : spec_(other.spec_), pins_(other.pins_), params_(other.params_), rustGlitch_(other.rustGlitch_) {
        other.rustGlitch_ = nullptr;
    }

    GlitchNode& operator=(GlitchNode&& other) noexcept {
        if (this != &other) {
            if (rustGlitch_) {
                n8_rust_glitch_destroy(rustGlitch_);
            }
            spec_ = other.spec_;
            pins_ = other.pins_;
            params_ = other.params_;
            rustGlitch_ = other.rustGlitch_;
            other.rustGlitch_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustGlitch_) {
            n8_rust_glitch_prepare(rustGlitch_, static_cast<float>(spec.sampleRate));
        }
    }

    void reset() override {
        if (rustGlitch_) {
            n8_rust_glitch_reset(rustGlitch_);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47
        if (!rustGlitch_ || context.numSamples == 0 || context.numInputChannels == 0 || context.numOutputChannels == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) ? context.outputChannels[1] : nullptr;

        n8_rust_glitch_process(rustGlitch_, inL, inR, outL, outR, context.numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        if (rustGlitch_) {
            n8_rust_glitch_set_parameter(rustGlitch_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustGlitch_) {
            return n8_rust_glitch_get_parameter(rustGlitch_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::Glitch; }
    const char* getName() const override { return "Glitch"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
    RustGlitch* rustGlitch_{ nullptr };
};

inline AutoRegisterNode<GlitchNode> registerGlitch(NodeType::Glitch, "glitch", "Glitch");

} // namespace audio_graph
