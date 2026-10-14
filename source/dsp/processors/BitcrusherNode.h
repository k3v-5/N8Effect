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
 * @brief Procesador Bitcrusher y Reductor de Sample Rate / Lo-Fi Vintage (Reglas 5, 8, 9, 14, 34, 46, 47)
 * Permite degradación digital de alta fidelidad: reducción de profundidad de bits (2 a 16 bits),
 * diezma de frecuencia de muestreo (Sample Rate Decimator), fluctuación estocástica de reloj (Jitter),
 * saturación de entrada (Drive) y filtro suavizador post-aliasing.
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class BitcrusherNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        BitDepth = 1,       // 2.0 a 16.0 bits (Resolución de cuantización)
        Downsample = 2,     // 1.0 a 50.0 factor (Diezmador de sample rate)
        Jitter = 3,         // 0.0 a 1.0 (Inestabilidad de reloj del convertidor DAC)
        AntiAliasing = 4,   // 0.0 a 1.0 (Filtro pasa-bajos post-cuantización)
        Drive = 5,          // 0.0 a 24.0 dB (Empuje armónico previo a cuantización)
        Mix = 6             // 0.0 a 1.0 (Mezcla Dry / Wet)
    };

    BitcrusherNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { BitDepth, "Bit Depth", 8.0f, 2.0f, 16.0f, true };
        params_[1] = { Downsample, "Downsample", 4.0f, 1.0f, 50.0f, true };
        params_[2] = { Jitter, "Jitter", 0.05f, 0.0f, 1.0f, true };
        params_[3] = { AntiAliasing, "Post Filter", 0.0f, 0.0f, 1.0f, true };
        params_[4] = { Drive, "Drive", 0.0f, 0.0f, 24.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_bitcrusher_create(44100.0f);
    }

    ~BitcrusherNode() override {
        if (rustInstance_) {
            n8_rust_bitcrusher_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    BitcrusherNode(const BitcrusherNode&) = delete;
    BitcrusherNode& operator=(const BitcrusherNode&) = delete;

    BitcrusherNode(BitcrusherNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    BitcrusherNode& operator=(BitcrusherNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_bitcrusher_destroy(rustInstance_);
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
            n8_rust_bitcrusher_prepare(rustInstance_, sr);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_bitcrusher_reset(rustInstance_);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;
        if (!rustInstance_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0]) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1]) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0]) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1]) ? context.outputChannels[1] : nullptr;

        if (!inL || !outL) return;

        n8_rust_bitcrusher_process(
            rustInstance_,
            inL,
            inR,
            outL,
            outR,
            static_cast<uint32_t>(context.numSamples)
        );
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_) {
            n8_rust_bitcrusher_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_bitcrusher_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::Bitcrusher; }
    const char* getName() const override { return "Bitcrusher"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
    RustBitcrusher* rustInstance_{ nullptr };
};

inline AutoRegisterNode<BitcrusherNode> registerBitcrusher(NodeType::Bitcrusher, "bitcrusher", "Distortion");

} // namespace audio_graph
