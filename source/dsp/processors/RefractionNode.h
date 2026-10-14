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
 * @brief Polyphonic Refraction Dispersion Engine (Reglas 5, 8, 9, 14, 18, 45, 46, 47).
 * Divide la señal estéreo en 2 a 8 voces refractadas polifónicas con micro-desafinación (detune),
 * retardo escalonado Haas anti-peinado, paneo de potencia constante estéreo, banco de dispersión armónica resonante
 * y normalización de energía de voces 1/sqrt(N).
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class RefractionNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        VoiceCount = 1,        // 2.0 a 8.0 voces (default: 4.0)
        Detune = 2,            // 0.0 a 100.0 cents (default: 18.0)
        DelaySpreadMs = 3,     // 0.0 a 50.0 ms (default: 15.0)
        StereoSpread = 4,      // 0.0 a 1.0 (default: 0.85)
        HarmonicDisp = 5,      // 0.0 a 2.0 (default: 0.5)
        Resonance = 6,         // 0.1 a 10.0 Q (default: 1.2)
        DryWet = 7             // 0.0 a 1.0 (default: 0.5)
    };

    RefractionNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { VoiceCount, "Voices", 4.0f, 2.0f, 8.0f, false };
        params_[1] = { Detune, "Detune", 18.0f, 0.0f, 100.0f, true };
        params_[2] = { DelaySpreadMs, "Delay Spread", 15.0f, 0.0f, 50.0f, true };
        params_[3] = { StereoSpread, "Stereo Spread", 0.85f, 0.0f, 1.0f, true };
        params_[4] = { HarmonicDisp, "Harmonic", 0.5f, 0.0f, 2.0f, true };
        params_[5] = { Resonance, "Resonance", 1.2f, 0.1f, 10.0f, true };
        params_[6] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_refraction_create(44100.0);
    }

    ~RefractionNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_refraction_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_refraction_set_sample_rate(rustInstance_, spec.sampleRate);
            n8_rust_refraction_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_refraction_reset(rustInstance_);
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

        n8_rust_refraction_process(rustInstance_, inL, inR, outL, outR, numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_refraction_set_parameter(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_ != nullptr) {
            return n8_rust_refraction_get_parameter(rustInstance_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::Refraction; }
    const char* getName() const override { return "Refraction"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 0.5); // 500ms
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    RustRefraction* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 7> params_;
};

inline AutoRegisterNode<RefractionNode> registerRefraction(NodeType::Refraction, "refraction", "Modulation");

} // namespace audio_graph
