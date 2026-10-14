#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Spectral Smear & Phase Diffusion Engine (Reglas 5, 8, 9, 14, 18, 45, 46, 47).
 * Procesador espectral de difusión líquida y emborronamiento tímbrico basado en STFT FFT (1024 puntos, 75% OLA).
 * Delegado al motor SIMD de alto rendimiento Rust (n8_rust_spectral_smear).
 */
class SpectralSmearNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        SmearTime = 1,         // 0.05s a 10.0s (default: 2.5s)
        PhaseDiffusion = 2,    // 0.0 a 1.0 (default: 0.7)
        StereoDecorrel = 3,    // 0.0 a 1.0 (default: 0.85)
        TiltDamping = 4,       // -1.0 a 1.0 (default: 0.0)
        DryWet = 5             // 0.0 a 1.0 (default: 0.5)
    };

    SpectralSmearNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { SmearTime, "Smear Time", 2.5f, 0.05f, 10.0f, true };
        params_[1] = { PhaseDiffusion, "Phase Diff", 0.7f, 0.0f, 1.0f, true };
        params_[2] = { StereoDecorrel, "Stereo Spread", 0.85f, 0.0f, 1.0f, true };
        params_[3] = { TiltDamping, "Tilt", 0.0f, -1.0f, 1.0f, true };
        params_[4] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };

        rustNode_ = n8_rust_spectral_smear_create();
    }

    ~SpectralSmearNode() override {
        if (rustNode_) {
            n8_rust_spectral_smear_destroy(rustNode_);
            rustNode_ = nullptr;
        }
    }

    SpectralSmearNode(const SpectralSmearNode&) = delete;
    SpectralSmearNode& operator=(const SpectralSmearNode&) = delete;

    SpectralSmearNode(SpectralSmearNode&& other) noexcept
        : spec_(other.spec_), pins_(other.pins_), params_(other.params_), rustNode_(other.rustNode_) {
        other.rustNode_ = nullptr;
    }

    SpectralSmearNode& operator=(SpectralSmearNode&& other) noexcept {
        if (this != &other) {
            if (rustNode_) {
                n8_rust_spectral_smear_destroy(rustNode_);
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
            n8_rust_spectral_smear_prepare(rustNode_, static_cast<float>(spec.sampleRate));
        }
    }

    void reset() override {
        if (rustNode_) {
            n8_rust_spectral_smear_reset(rustNode_);
        }
    }

    void process(ProcessContext& context) override {
        if (!rustNode_ || context.numSamples == 0 || context.numInputChannels == 0 || context.numOutputChannels == 0) return;

        const float* inL = context.inputChannels[0];
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : inL;

        float* outL = context.outputChannels[0];
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : outL;

        n8_rust_spectral_smear_process(rustNode_, inL, inR, outL, outR, context.numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        if (rustNode_) {
            n8_rust_spectral_smear_set_parameter(rustNode_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustNode_) {
            return n8_rust_spectral_smear_get_parameter(rustNode_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::SpectralSmear; }
    const char* getName() const override { return "Spectral Smear"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 5.0);
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
    RustSpectralSmear* rustNode_{ nullptr };
};

inline AutoRegisterNode<SpectralSmearNode> registerSpectralSmear(NodeType::SpectralSmear, "spectral_smear", "Spectral");

} // namespace audio_graph
