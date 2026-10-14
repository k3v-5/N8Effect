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
 * @brief Resonador y Guía de Ondas Acústicas Acelerado en Rust (Reglas 5, 8, 9, 14, 34, 46, 47)
 * Núcleo matemático de modelado físico compilado en Rust con optimizaciones SIMD AVX2 y LTO Fat.
 */
class RustWaveguideNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Frequency = 1,
        Decay = 2,
        Brightness = 3,
        NonLinearity = 4,
        Coupling = 5,
        Mix = 6
    };

    RustWaveguideNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Frequency, "Frequency", 220.0f, 20.0f, 2000.0f, true };
        params_[1] = { Decay, "Decay / Res", 0.92f, 0.01f, 0.999f, true };
        params_[2] = { Brightness, "Brightness", 0.65f, 0.0f, 1.0f, true };
        params_[3] = { NonLinearity, "Drive / NonLin", 0.25f, 0.0f, 1.0f, true };
        params_[4] = { Coupling, "Coupling", 0.30f, 0.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 0.70f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_waveguide_create(48000.0);
    }

    ~RustWaveguideNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_waveguide_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::RustWaveguide; }
    const char* getName() const override { return "Rust Waveguide"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_waveguide_set_sample_rate(rustInstance_, spec.sampleRate);
            n8_rust_waveguide_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_waveguide_reset(rustInstance_);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (outL == nullptr || rustInstance_ == nullptr || inL == nullptr) return;

        // Despacho directo a la biblioteca estática de Rust (Zero allocations, micro-latencia x64)
        n8_rust_waveguide_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_waveguide_set_parameter(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_ != nullptr) {
            return n8_rust_waveguide_get_parameter(rustInstance_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    RustWaveguide* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<RustWaveguideNode> registerRustWaveguide(NodeType::RustWaveguide, "rustwaveguide", "Physical & Acoustic");

} // namespace audio_graph
