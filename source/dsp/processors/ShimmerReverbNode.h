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
 * @brief Shimmer Reverb Engine algorítmico de alta densidad con lazo de realimentación armónico
 * transponible (+12st, +7st, -12st), difusores Allpass de entrada, tanque FDN 4-line,
 * amortiguamiento IIR, saturación sigmoidal analógica suave y ensanchador Mid/Side (Reglas 5, 8, 9, 14, 18, 45, 46, 47).
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class ShimmerReverbNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        DecayTime = 1,       // 0.2s a 30.0s (default: 4.0s)
        DampingHz = 2,       // 1000.0Hz a 20000.0Hz (default: 6500.0Hz)
        ShimmerMix = 3,      // 0.0 a 1.0 (default: 0.5)
        PitchInterval = 4,   // 0: +12st (Octava), 1: +7st (Quinta), 2: -12st (Sub-Octava)
        StereoWidth = 5,     // 0.0 a 2.0 (default: 1.2)
        DryWet = 6           // 0.0 a 1.0 (default: 0.45)
    };

    ShimmerReverbNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { DecayTime, "Decay", 4.0f, 0.2f, 30.0f, true };
        params_[1] = { DampingHz, "Damping", 6500.0f, 1000.0f, 20000.0f, true };
        params_[2] = { ShimmerMix, "Shimmer", 0.5f, 0.0f, 1.0f, true };
        params_[3] = { PitchInterval, "Interval", 0.0f, 0.0f, 2.0f, false };
        params_[4] = { StereoWidth, "Width", 1.2f, 0.0f, 2.0f, true };
        params_[5] = { DryWet, "Mix", 0.45f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_shimmer_reverb_create(44100.0);
    }

    ~ShimmerReverbNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_shimmer_reverb_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_shimmer_reverb_set_sample_rate(rustInstance_, spec.sampleRate);
            n8_rust_shimmer_reverb_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_shimmer_reverb_reset(rustInstance_);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const uint32_t numSamples = context.numSamples;
        if (numSamples == 0 || rustInstance_ == nullptr) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        n8_rust_shimmer_reverb_process(rustInstance_, inL, inR, outL, outR, numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_shimmer_reverb_set_parameter(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_ != nullptr) {
            return n8_rust_shimmer_reverb_get_parameter(rustInstance_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::ShimmerReverb; }
    const char* getName() const override { return "Shimmer Reverb"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 30.0); // Hasta 30s de cola
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    RustShimmerReverb* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<ShimmerReverbNode> registerShimmerReverb(NodeType::ShimmerReverb, "shimmer_reverb", "Space");

} // namespace audio_graph
