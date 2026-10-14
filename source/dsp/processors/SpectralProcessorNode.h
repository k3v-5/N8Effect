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
 * @brief Procesador Espectral en dominio de frecuencia con compuerta y tilt espectral (Reglas 5, 8, 19, 34)
 * Delegado al motor DSP SIMD en Rust (n8_rust_core).
 */
class SpectralProcessorNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        SpectralGateDb = 1,
        SpectralTilt = 2,
        DryWet = 3
    };

    SpectralProcessorNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { SpectralGateDb, "Gate Threshold", -50.0f, -80.0f, 0.0f, true };
        params_[1] = { SpectralTilt, "Tilt", 0.0f, -6.0f, 6.0f, true };
        params_[2] = { DryWet, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustProcessor_ = n8_rust_spectral_processor_create();
    }

    ~SpectralProcessorNode() override {
        if (rustProcessor_) {
            n8_rust_spectral_processor_destroy(rustProcessor_);
            rustProcessor_ = nullptr;
        }
    }

    SpectralProcessorNode(const SpectralProcessorNode&) = delete;
    SpectralProcessorNode& operator=(const SpectralProcessorNode&) = delete;

    SpectralProcessorNode(SpectralProcessorNode&& other) noexcept
        : spec_(other.spec_), pins_(other.pins_), params_(other.params_), rustProcessor_(other.rustProcessor_) {
        other.rustProcessor_ = nullptr;
    }

    SpectralProcessorNode& operator=(SpectralProcessorNode&& other) noexcept {
        if (this != &other) {
            if (rustProcessor_) {
                n8_rust_spectral_processor_destroy(rustProcessor_);
            }
            spec_ = other.spec_;
            pins_ = other.pins_;
            params_ = other.params_;
            rustProcessor_ = other.rustProcessor_;
            other.rustProcessor_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustProcessor_) {
            n8_rust_spectral_processor_prepare(rustProcessor_, static_cast<float>(spec.sampleRate));
        }
    }

    void reset() override {
        if (rustProcessor_) {
            n8_rust_spectral_processor_reset(rustProcessor_);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47
        if (!rustProcessor_ || context.numSamples == 0 || context.numOutputChannels == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) ? context.outputChannels[1] : nullptr;

        n8_rust_spectral_processor_process(rustProcessor_, inL, inR, outL, outR, context.numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        if (rustProcessor_) {
            n8_rust_spectral_processor_set_parameter(rustProcessor_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustProcessor_) {
            return n8_rust_spectral_processor_get_parameter(rustProcessor_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::SpectralProcessor; }
    const char* getName() const override { return "Spectral Processor"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 3> params_;
    RustSpectralProcessor* rustProcessor_{ nullptr };
};

inline AutoRegisterNode<SpectralProcessorNode> registerSpectralProcessor(NodeType::SpectralProcessor, "spectral_processor", "Spectral");

} // namespace audio_graph
