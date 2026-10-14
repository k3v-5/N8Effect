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
 * @brief Simulador de Altavoz Giratorio (Rotary Speaker / Leslie Cabinet)
 * Emulación dual de rotor de agudos (Horn) y tambor de graves (Drum) con Doppler, AM e inercia mecánica
 * Acelerado con núcleo en Rust de alto rendimiento (Reglas 5, 8, 14, 34, 46, 47).
 */
class RotarySpeakerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        SpeedMode = 1, // 0: Slow (Chorale), 1: Fast (Tremolo), 2: Brake (Off)
        HornSpeed = 2, // Escala de velocidad de agudos (0.5x a 1.5x)
        DrumSpeed = 3, // Escala de velocidad de graves (0.5x a 1.5x)
        Drive = 4,     // Saturación a válvulas del preamplificador (1.0 a 6.0)
        Spread = 5,    // Ancho estéreo de los micrófonos (0.0 a 1.0)
        Mix = 6        // Dry / Wet
    };

    RotarySpeakerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { SpeedMode, "Speed Mode", 1.0f, 0.0f, 2.0f, false };
        params_[1] = { HornSpeed, "Horn Rate", 1.0f, 0.5f, 1.5f, true };
        params_[2] = { DrumSpeed, "Drum Rate", 1.0f, 0.5f, 1.5f, true };
        params_[3] = { Drive, "Drive", 1.5f, 1.0f, 6.0f, true };
        params_[4] = { Spread, "Spread", 0.85f, 0.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_rotary_speaker_create(44100.0);
    }

    ~RotarySpeakerNode() override {
        if (rustInstance_) {
            n8_rust_rotary_speaker_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    RotarySpeakerNode(const RotarySpeakerNode&) = delete;
    RotarySpeakerNode& operator=(const RotarySpeakerNode&) = delete;

    RotarySpeakerNode(RotarySpeakerNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    RotarySpeakerNode& operator=(RotarySpeakerNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_rotary_speaker_destroy(rustInstance_);
            }
            pins_ = other.pins_;
            params_ = other.params_;
            spec_ = other.spec_;
            rustInstance_ = other.rustInstance_;
            other.rustInstance_ = nullptr;
        }
        return *this;
    }

    NodeType getType() const override { return NodeType::RotarySpeaker; }
    const char* getName() const override { return "Rotary Speaker"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_) {
            n8_rust_rotary_speaker_prepare(rustInstance_, spec.sampleRate);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_rotary_speaker_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_) {
            n8_rust_rotary_speaker_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_rotary_speaker_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        if (!rustInstance_ || context.numSamples == 0) return;

        const size_t numChannels = std::min<size_t>(context.numOutputChannels, 2);
        const float* inL = (context.numInputChannels > 0) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : inL;
        float* outL = (numChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (numChannels > 1) ? context.outputChannels[1] : outL;

        if (!outL || !inL) return;

        n8_rust_rotary_speaker_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustRotarySpeaker* rustInstance_{ nullptr };
    ProcessSpec spec_{ 44100.0, 256, 2, 2 };
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<RotarySpeakerNode> registerRotary(NodeType::RotarySpeaker, "rotary_speaker", "Modulation");

} // namespace audio_graph
