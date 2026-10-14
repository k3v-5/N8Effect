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
 * @brief Vocoder Clásico de 16 Bandas (Channel Vocoder)
 * Banco de filtros modulador/portadora con oscilador armónico interno (Reglas 5, 8, 14, 34, 46, 47)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class VocoderNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        CarrierMode = 1,  // 0: Sawtooth, 1: Pulse, 2: Noise, 3: Stereo In (L=Mod, R=Car)
        CarrierPitch = 2, // Frecuencia de la portadora interna en Hz (35 Hz a 500 Hz)
        FormantShift = 3, // Desplazamiento espectral de formantes (0.5x a 2.0x)
        BandQ = 4,        // Factor Q de resonancia de las 16 bandas (1.5 a 15.0)
        ReleaseTime = 5,  // Tiempo de caída de los seguidores de envolvente (0.01s a 0.25s)
        Mix = 6           // Dry / Wet
    };

    static constexpr size_t NumBands = 16;

    VocoderNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { SidechainPinId, "Modulator In", PinType::AudioInput, PinDataType::AudioStereo };

        params_[0] = { CarrierMode, "Carrier Mode", 0.0f, 0.0f, 3.0f, false };
        params_[1] = { CarrierPitch, "Pitch Hz", 110.0f, 35.0f, 500.0f, true };
        params_[2] = { FormantShift, "Formant Shift", 1.0f, 0.5f, 2.0f, true };
        params_[3] = { BandQ, "Band Q", 6.0f, 1.5f, 15.0f, true };
        params_[4] = { ReleaseTime, "Release", 0.05f, 0.01f, 0.25f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_vocoder_create(44100.0);
    }

    ~VocoderNode() override {
        if (rustInstance_) {
            n8_rust_vocoder_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    VocoderNode(const VocoderNode&) = delete;
    VocoderNode& operator=(const VocoderNode&) = delete;

    VocoderNode(VocoderNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    VocoderNode& operator=(VocoderNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_vocoder_destroy(rustInstance_);
            }
            pins_ = other.pins_;
            params_ = other.params_;
            spec_ = other.spec_;
            rustInstance_ = other.rustInstance_;
            other.rustInstance_ = nullptr;
        }
        return *this;
    }

    NodeType getType() const override { return NodeType::Vocoder; }
    const char* getName() const override { return "Channel Vocoder"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_) {
            n8_rust_vocoder_prepare(rustInstance_, spec.sampleRate);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_vocoder_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_) {
            n8_rust_vocoder_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_vocoder_get_parameter(rustInstance_, id);
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

        const bool hasSidechain = (context.numSidechainChannels > 0 && context.sidechainChannels != nullptr &&
                                   context.sidechainChannels[0] != nullptr && context.sidechainChannels != context.inputChannels);
        const float* scLPtr = hasSidechain ? context.sidechainChannels[0] : nullptr;
        const float* scRPtr = (hasSidechain && context.numSidechainChannels > 1 && context.sidechainChannels[1])
            ? context.sidechainChannels[1] : scLPtr;

        n8_rust_vocoder_process(rustInstance_, inL, inR, scLPtr, scRPtr, outL, outR, context.numSamples);
    }

private:
    RustVocoder* rustInstance_{ nullptr };
    ProcessSpec spec_{ 44100.0, 256, 2, 2 };
    std::array<PinDescriptor, 3> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<VocoderNode> registerVocoder(NodeType::Vocoder, "vocoder", "Spectral");

} // namespace audio_graph
