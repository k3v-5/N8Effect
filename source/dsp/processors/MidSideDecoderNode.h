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
 * @brief Decodificador Mid/Side a Estéreo con Mono Bass Maker (Reglas 5, 8, 13, 16, 34, 46, 47).
 * Reconstruye la imagen estéreo L/R a partir de componentes Mid/Side, proporcionando control
 * continuo de ancho estéreo (Width) y filtro Mono Bass Maker (filtro pasa-altos Butterworth en Side)
 * para garantizar subgraves monofónicos sin desfases destructivos.
 * Delegado al motor DSP SIMD en Rust (n8_rust_core).
 */
class MidSideDecoderNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Width = 1,
        MonoBassFreq = 2,
        MonoBassActive = 3,
        OutputGain = 4
    };

    MidSideDecoderNode() {
        pins_[0] = { 1, "Mid/Side In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Stereo Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Width, "Width", 1.0f, 0.0f, 2.0f, true };
        params_[1] = { MonoBassFreq, "Mono Bass Freq", 120.0f, 20.0f, 400.0f, true };
        params_[2] = { MonoBassActive, "Mono Bass Active", 1.0f, 0.0f, 1.0f, false };
        params_[3] = { OutputGain, "Gain", 1.0f, 0.0f, 2.0f, true };

        rustDecoder_ = n8_rust_mid_side_decoder_create(44100.0f);
    }

    ~MidSideDecoderNode() override {
        if (rustDecoder_) {
            n8_rust_mid_side_decoder_destroy(rustDecoder_);
            rustDecoder_ = nullptr;
        }
    }

    MidSideDecoderNode(const MidSideDecoderNode&) = delete;
    MidSideDecoderNode& operator=(const MidSideDecoderNode&) = delete;

    MidSideDecoderNode(MidSideDecoderNode&& other) noexcept
        : spec_(other.spec_), pins_(other.pins_), params_(other.params_), rustDecoder_(other.rustDecoder_) {
        other.rustDecoder_ = nullptr;
    }

    MidSideDecoderNode& operator=(MidSideDecoderNode&& other) noexcept {
        if (this != &other) {
            if (rustDecoder_) {
                n8_rust_mid_side_decoder_destroy(rustDecoder_);
            }
            spec_ = other.spec_;
            pins_ = other.pins_;
            params_ = other.params_;
            rustDecoder_ = other.rustDecoder_;
            other.rustDecoder_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustDecoder_) {
            n8_rust_mid_side_decoder_prepare(rustDecoder_, static_cast<float>(spec.sampleRate));
        }
    }

    void reset() override {
        if (rustDecoder_) {
            n8_rust_mid_side_decoder_reset(rustDecoder_);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Reglas 34 y 47
        if (!rustDecoder_ || context.numSamples == 0 || context.numOutputChannels == 0) return;

        const float* inMid = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inSide = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1] : nullptr;
        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) ? context.outputChannels[1] : nullptr;

        n8_rust_mid_side_decoder_process(rustDecoder_, inMid, inSide, outL, outR, context.numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        if (rustDecoder_) {
            n8_rust_mid_side_decoder_set_parameter(rustDecoder_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustDecoder_) {
            return n8_rust_mid_side_decoder_get_parameter(rustDecoder_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::MidSideDecoder; }
    const char* getName() const override { return "M/S Decoder"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 4> params_;
    RustMidSideDecoder* rustDecoder_{ nullptr };
};

inline AutoRegisterNode<MidSideDecoderNode> registerMidSideDecoderNode(
    NodeType::MidSideDecoder, "M/S Decoder", "Spatial"
);

} // namespace audio_graph
