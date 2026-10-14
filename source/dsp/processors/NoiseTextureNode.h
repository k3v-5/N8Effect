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
 * @brief Generador de Ruido y Texturas Orgánicas (Vinyl, Tape, Pink, White, Rain)
 * con Sidechain Ducking y Modelado Espectral (Reglas 5, 8, 9, 14, 34, 46, 47)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class NoiseTextureNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Mode = 1,          // 0: White, 1: Pink, 2: Tape Hiss, 3: Vinyl Crackle, 4: Rain/Ambient
        Density = 2,       // Frecuencia de chasquidos / densidad textural (0.0 a 1.0)
        Tone = 3,          // Brillo / Corte de tono (300 Hz a 18000 Hz)
        SidechainDuck = 4, // Atenuación automática del ruido ante señal entrante (0.0 a 1.0)
        Level = 5,         // Volumen de inyección de textura (0.0 a 1.0)
        Mix = 6            // Mezcla general
    };

    NoiseTextureNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Mode, "Mode", 3.0f, 0.0f, 4.0f, false };
        params_[1] = { Density, "Density", 0.45f, 0.0f, 1.0f, true };
        params_[2] = { Tone, "Tone Hz", 8000.0f, 300.0f, 18000.0f, true };
        params_[3] = { SidechainDuck, "Duck", 0.5f, 0.0f, 1.0f, true };
        params_[4] = { Level, "Level", 0.4f, 0.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_noise_texture_create(44100.0f);
    }

    ~NoiseTextureNode() override {
        if (rustInstance_) {
            n8_rust_noise_texture_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NoiseTextureNode(const NoiseTextureNode&) = delete;
    NoiseTextureNode& operator=(const NoiseTextureNode&) = delete;

    NoiseTextureNode(NoiseTextureNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    NoiseTextureNode& operator=(NoiseTextureNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_noise_texture_destroy(rustInstance_);
            }
            pins_ = other.pins_;
            params_ = other.params_;
            spec_ = other.spec_;
            rustInstance_ = other.rustInstance_;
            other.rustInstance_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        const float sr = spec.sampleRate > 0.0 ? static_cast<float>(spec.sampleRate) : 44100.0f;
        if (rustInstance_) {
            n8_rust_noise_texture_prepare(rustInstance_, sr);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_noise_texture_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_) {
            n8_rust_noise_texture_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_noise_texture_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::NoiseTexture; }
    const char* getName() const override { return "Noise & Texture"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47
        if (!rustInstance_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (!outL) return;

        n8_rust_noise_texture_process(
            rustInstance_,
            inL,
            inR,
            outL,
            outR,
            static_cast<uint32_t>(context.numSamples)
        );
    }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
    RustNoiseTexture* rustInstance_{ nullptr };
};

inline AutoRegisterNode<NoiseTextureNode> registerNoiseTexture(NodeType::NoiseTexture, "noise_texture", "Generators");

} // namespace audio_graph
