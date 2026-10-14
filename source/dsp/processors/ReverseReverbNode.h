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
 * @brief Reverberación Inversa con Florecimiento Espectral (Reverse Reverb / Swell Bloom)
 * Reproducción en espejo temporal con envolvente de hinchamiento pre-transiente y difusión multietapa
 * Delegado al motor DSP SIMD en Rust (n8_rust_core) (Reglas 5, 8, 14, 34, 46, 47).
 */
class ReverseReverbNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        SwellTime = 1, // Duración del hinchamiento temporal inverso (0.1s a 2.0s)
        Diffusion = 2, // Cantidad de difusión y densidad espectral (0.0 a 1.0)
        Damping = 3,   // Frecuencia de absorción de agudos en Hz (500 Hz a 18000 Hz)
        Feedback = 4,  // Realimentación en cascada (0.0 a 0.85)
        Mix = 5        // Dry / Wet
    };

    ReverseReverbNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { SwellTime, "Swell Time", 0.6f, 0.1f, 2.0f, true };
        params_[1] = { Diffusion, "Diffusion", 0.7f, 0.0f, 1.0f, true };
        params_[2] = { Damping, "Damping Hz", 8000.0f, 500.0f, 18000.0f, true };
        params_[3] = { Feedback, "Feedback", 0.35f, 0.0f, 0.85f, true };
        params_[4] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustReverb_ = n8_rust_reverse_reverb_create(44100.0f);
    }

    ~ReverseReverbNode() override {
        if (rustReverb_) {
            n8_rust_reverse_reverb_destroy(rustReverb_);
            rustReverb_ = nullptr;
        }
    }

    ReverseReverbNode(const ReverseReverbNode&) = delete;
    ReverseReverbNode& operator=(const ReverseReverbNode&) = delete;

    ReverseReverbNode(ReverseReverbNode&& other) noexcept
        : spec_(other.spec_), pins_(other.pins_), params_(other.params_), rustReverb_(other.rustReverb_) {
        other.rustReverb_ = nullptr;
    }

    ReverseReverbNode& operator=(ReverseReverbNode&& other) noexcept {
        if (this != &other) {
            if (rustReverb_) {
                n8_rust_reverse_reverb_destroy(rustReverb_);
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
            n8_rust_reverse_reverb_prepare(rustReverb_, static_cast<float>(spec.sampleRate));
        }
    }

    void reset() override {
        if (rustReverb_) {
            n8_rust_reverse_reverb_reset(rustReverb_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustReverb_) {
            n8_rust_reverse_reverb_set_parameter(rustReverb_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustReverb_) {
            return n8_rust_reverse_reverb_get_parameter(rustReverb_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::ReverseReverb; }
    const char* getName() const override { return "Reverse Reverb"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47
        if (!rustReverb_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) ? context.outputChannels[1] : nullptr;

        n8_rust_reverse_reverb_process(rustReverb_, inL, inR, outL, outR, context.numSamples);
    }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
    RustReverseReverb* rustReverb_{ nullptr };
};

inline AutoRegisterNode<ReverseReverbNode> registerReverseReverb(NodeType::ReverseReverb, "reverse_reverb", "Spatial");

} // namespace audio_graph
