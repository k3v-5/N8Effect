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
 * @brief Emulador de Cinta y Cassette Lo-Fi Analógico (Reglas 5, 8, 9, 14, 34, 46, 47)
 * Modela Wow & Flutter estocástico, saturación de cabezal magnético y pérdida de óxido.
 * Delegado al motor DSP SIMD en Rust (n8_rust_core).
 */
class LoFiCassetteNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Wow = 1,
        Flutter = 2,
        Age = 3,
        Saturation = 4,
        Dropouts = 5,
        Mix = 6
    };

    LoFiCassetteNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Wow, "Wow", 0.2f, 0.0f, 1.0f, true };
        params_[1] = { Flutter, "Flutter", 0.15f, 0.0f, 1.0f, true };
        params_[2] = { Age, "Tape Age", 0.3f, 0.0f, 1.0f, true };
        params_[3] = { Saturation, "Drive", 0.25f, 0.0f, 1.0f, true };
        params_[4] = { Dropouts, "Dropouts", 0.05f, 0.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustCassette_ = n8_rust_lofi_cassette_create(48000.0f);
    }

    ~LoFiCassetteNode() override {
        if (rustCassette_) {
            n8_rust_lofi_cassette_destroy(rustCassette_);
            rustCassette_ = nullptr;
        }
    }

    LoFiCassetteNode(const LoFiCassetteNode&) = delete;
    LoFiCassetteNode& operator=(const LoFiCassetteNode&) = delete;

    LoFiCassetteNode(LoFiCassetteNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), rustCassette_(other.rustCassette_) {
        other.rustCassette_ = nullptr;
    }

    LoFiCassetteNode& operator=(LoFiCassetteNode&& other) noexcept {
        if (this != &other) {
            if (rustCassette_) {
                n8_rust_lofi_cassette_destroy(rustCassette_);
            }
            pins_ = other.pins_;
            params_ = other.params_;
            rustCassette_ = other.rustCassette_;
            other.rustCassette_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustCassette_) {
            n8_rust_lofi_cassette_prepare(rustCassette_, static_cast<float>(spec.sampleRate));
        }
    }

    void reset() override {
        if (rustCassette_) {
            n8_rust_lofi_cassette_reset(rustCassette_);
        }
    }

    NodeType getType() const override { return NodeType::LoFiCassette; }
    const char* getName() const override { return "Lo-Fi Cassette"; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;
        if (!rustCassette_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        n8_rust_lofi_cassette_process(rustCassette_, inL, inR, outL, outR, context.numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        if (rustCassette_) {
            n8_rust_lofi_cassette_set_parameter(rustCassette_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustCassette_) {
            return n8_rust_lofi_cassette_get_parameter(rustCassette_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_{ 48000.0, 512, 2, 2 };
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
    RustLoFiCassette* rustCassette_{ nullptr };
};

inline AutoRegisterNode<LoFiCassetteNode> registerLoFiCassette(NodeType::LoFiCassette, "loficassette", "Distortion");

} // namespace audio_graph
