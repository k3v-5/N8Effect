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
 * @brief Resonador de Modelado Físico de Cuerda Pulsada (Karplus-Strong) Acelerado en Rust
 * Núcleo DSP físico migrado a Rust con filtro en lazo, dispersión all-pass y caja resonante (Reglas 5, 8, 9, 14, 34, 46, 47).
 */
class KarplusStrongNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Pitch = 1,        // Frecuencia fundamental en Hz (25 Hz a 2000 Hz)
        Damping = 2,      // Amortiguamiento de alta frecuencia (0.01 a 0.99)
        Decay = 3,        // Duración de decaimiento en segundos (0.1s a 5.0s)
        PickPosition = 4, // Posición de ataque en la cuerda (0.05 a 0.95)
        AudioTrigger = 5, // 0: Resonador Continuo, 1: Disparo por Golpes de Audio
        Mix = 6,          // Dry / Wet
        BodySize = 7,     // Escala de tamaño del cuerpo acústico (0.5 a 2.0)
        BodyDecay = 8,    // Resonancia / Q de la madera (0.1 a 3.0)
        BodyMix = 9       // Nivel de caja acústica (0.0 a 1.0)
    };

    KarplusStrongNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Pitch, "Pitch Hz", 220.0f, 25.0f, 2000.0f, true };
        params_[1] = { Damping, "Damping", 0.5f, 0.01f, 0.99f, true };
        params_[2] = { Decay, "Decay Time", 1.5f, 0.1f, 5.0f, true };
        params_[3] = { PickPosition, "Pick Pos", 0.25f, 0.05f, 0.95f, true };
        params_[4] = { AudioTrigger, "Trigger Mode", 1.0f, 0.0f, 1.0f, false };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
        params_[6] = { BodySize, "Body Size", 1.0f, 0.5f, 2.0f, true };
        params_[7] = { BodyDecay, "Body Decay", 1.0f, 0.1f, 3.0f, true };
        params_[8] = { BodyMix, "Body Mix", 0.4f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_karplus_create(44100.0);
    }

    ~KarplusStrongNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_karplus_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::KarplusStrong; }
    const char* getName() const override { return "Karplus-Strong"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_karplus_set_sample_rate(rustInstance_, spec.sampleRate);
            n8_rust_karplus_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_karplus_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_karplus_set_parameter(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_ != nullptr) {
            return n8_rust_karplus_get_parameter(rustInstance_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (outL == nullptr || rustInstance_ == nullptr || inL == nullptr) return;

        // Despacho directo a la biblioteca estática de Rust (Zero allocations, micro-latencia x64)
        n8_rust_karplus_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    ProcessSpec spec_;
    RustKarplusStrong* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 9> params_;
};

inline AutoRegisterNode<KarplusStrongNode> registerKarplus(NodeType::KarplusStrong, "karplus_strong", "Resonance");

} // namespace audio_graph
