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
 * @brief Reverb Algorítmica FDN (Feedback Delay Network) de 4 líneas con matriz ortogonal y Tails (Reglas 5, 8, 14, 15, 18, 34)
 * Delegado al motor DSP SIMD en Rust (n8_rust_core).
 */
class ReverbNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        RoomSize = 1,
        DecayTime = 2,
        Damping = 3,
        Predelay = 4,
        DryWet = 5
    };

    ReverbNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { RoomSize, "Size", 0.7f, 0.1f, 1.0f, true };
        params_[1] = { DecayTime, "Decay", 2.5f, 0.2f, 10.0f, true };
        params_[2] = { Damping, "Damping", 5000.0f, 500.0f, 20000.0f, true };
        params_[3] = { Predelay, "Predelay", 20.0f, 0.0f, 100.0f, true };
        params_[4] = { DryWet, "Mix", 0.4f, 0.0f, 1.0f, true };

        rustReverb_ = n8_rust_reverb_create(44100.0f);
    }

    ~ReverbNode() override {
        if (rustReverb_) {
            n8_rust_reverb_destroy(rustReverb_);
            rustReverb_ = nullptr;
        }
    }

    ReverbNode(const ReverbNode&) = delete;
    ReverbNode& operator=(const ReverbNode&) = delete;

    ReverbNode(ReverbNode&& other) noexcept
        : spec_(other.spec_), pins_(other.pins_), params_(other.params_), rustReverb_(other.rustReverb_) {
        other.rustReverb_ = nullptr;
    }

    ReverbNode& operator=(ReverbNode&& other) noexcept {
        if (this != &other) {
            if (rustReverb_) {
                n8_rust_reverb_destroy(rustReverb_);
            }
            spec_ = other.spec_;
            pins_ = other.pins_;
            params_ = other.params_;
            rustReverb_ = other.rustReverb_;
            other.rustReverb_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustReverb_) {
            n8_rust_reverb_prepare(rustReverb_, static_cast<float>(spec.sampleRate));
        }
    }

    void reset() override {
        if (rustReverb_) {
            n8_rust_reverb_reset(rustReverb_);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47
        if (!rustReverb_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) ? context.outputChannels[1] : nullptr;

        n8_rust_reverb_process(rustReverb_, inL, inR, outL, outR, context.numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        if (rustReverb_) {
            n8_rust_reverb_set_parameter(rustReverb_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustReverb_) {
            return n8_rust_reverb_get_parameter(rustReverb_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::Reverb; }
    const char* getName() const override { return "Algorithmic Reverb"; }
    bool supportsTail() const override { return true; } // Soporte explícito de tails (Regla 18)
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 10.0); // 10 segundos de tail máximo
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
    RustReverb* rustReverb_{ nullptr };
};

inline AutoRegisterNode<ReverbNode> registerReverb(NodeType::Reverb, "reverb", "Space");

} // namespace audio_graph
