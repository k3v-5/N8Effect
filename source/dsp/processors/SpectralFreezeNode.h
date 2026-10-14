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
 * @brief Procesador Espectral Avanzado: Freeze (congelamiento de magnitudes) y Smear (difusión temporal) (Reglas 5, 8, 14, 18, 19, 32, 46)
 * Delegado al motor SIMD de alto rendimiento Rust (n8_rust_spectral_freeze).
 */
class SpectralFreezeNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Freeze = 1,
        Smear = 2,
        Brightness = 3,
        DryWet = 4
    };

    SpectralFreezeNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Freeze, "Freeze", 0.0f, 0.0f, 1.0f, false };
        params_[1] = { Smear, "Smear", 0.0f, 0.0f, 0.98f, true };
        params_[2] = { Brightness, "Brightness", 0.0f, -1.0f, 1.0f, true };
        params_[3] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };

        rustNode_ = n8_rust_spectral_freeze_create();
    }

    ~SpectralFreezeNode() override {
        if (rustNode_) {
            n8_rust_spectral_freeze_destroy(rustNode_);
            rustNode_ = nullptr;
        }
    }

    SpectralFreezeNode(const SpectralFreezeNode&) = delete;
    SpectralFreezeNode& operator=(const SpectralFreezeNode&) = delete;

    SpectralFreezeNode(SpectralFreezeNode&& other) noexcept
        : spec_(other.spec_), pins_(other.pins_), params_(other.params_), rustNode_(other.rustNode_) {
        other.rustNode_ = nullptr;
    }

    SpectralFreezeNode& operator=(SpectralFreezeNode&& other) noexcept {
        if (this != &other) {
            if (rustNode_) {
                n8_rust_spectral_freeze_destroy(rustNode_);
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
            n8_rust_spectral_freeze_prepare(rustNode_, static_cast<float>(spec.sampleRate));
        }
    }

    void reset() override {
        if (rustNode_) {
            n8_rust_spectral_freeze_reset(rustNode_);
        }
    }

    void process(ProcessContext& context) override {
        if (!rustNode_ || context.numSamples == 0 || context.numInputChannels == 0 || context.numOutputChannels == 0) return;

        const float* inL = context.inputChannels[0];
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : inL;

        float* outL = context.outputChannels[0];
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : outL;

        n8_rust_spectral_freeze_process(rustNode_, inL, inR, outL, outR, context.numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        if (rustNode_) {
            n8_rust_spectral_freeze_set_parameter(rustNode_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustNode_) {
            return n8_rust_spectral_freeze_get_parameter(rustNode_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::Spectral; }
    const char* getName() const override { return "Spectral Freeze"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 3.0);
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 4> params_;
    RustSpectralFreeze* rustNode_{ nullptr };
};

inline AutoRegisterNode<SpectralFreezeNode> registerSpectralFreeze(NodeType::Spectral, "spectral_freeze", "Spectral");

} // namespace audio_graph
