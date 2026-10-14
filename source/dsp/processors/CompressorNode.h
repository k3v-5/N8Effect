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
 * @brief Nodo de Compresor VCA profesional con Soft-Knee (Reglas 5, 8, 11, 14, 34, 35)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class CompressorNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Threshold = 1,
        Ratio = 2,
        Attack = 3,
        Release = 4,
        Makeup = 5,
        Knee = 6
    };

    CompressorNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { SidechainPinId, "Sidechain", PinType::AudioInput, PinDataType::AudioStereo };

        params_[0] = { Threshold, "Threshold", -18.0f, -60.0f, 0.0f, true };
        params_[1] = { Ratio, "Ratio", 4.0f, 1.0f, 20.0f, true };
        params_[2] = { Attack, "Attack", 15.0f, 0.1f, 200.0f, true };
        params_[3] = { Release, "Release", 100.0f, 5.0f, 1000.0f, true };
        params_[4] = { Makeup, "Makeup", 0.0f, 0.0f, 30.0f, true };
        params_[5] = { Knee, "Knee", 4.0f, 0.0f, 20.0f, true };

        rustInstance_ = n8_rust_compressor_create(44100.0f);
    }

    ~CompressorNode() override {
        if (rustInstance_) {
            n8_rust_compressor_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    CompressorNode(const CompressorNode&) = delete;
    CompressorNode& operator=(const CompressorNode&) = delete;

    CompressorNode(CompressorNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    CompressorNode& operator=(CompressorNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_compressor_destroy(rustInstance_);
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
            n8_rust_compressor_prepare(rustInstance_, sr);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_compressor_reset(rustInstance_);
        }
    }

    NodeType getType() const override { return NodeType::Compressor; }
    const char* getName() const override { return "Compressor"; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;
        if (!rustInstance_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;

        const float* scL = (context.numSidechainChannels > 0 && context.sidechainChannels != nullptr && context.sidechainChannels[0] != nullptr)
            ? context.sidechainChannels[0] : inL;
        const float* scR = (context.numSidechainChannels > 1 && context.sidechainChannels != nullptr && context.sidechainChannels[1] != nullptr)
            ? context.sidechainChannels[1] : (scL ? scL : inR);

        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (!inL || !outL) return;

        n8_rust_compressor_process(
            rustInstance_,
            inL,
            inR,
            scL,
            scR,
            outL,
            outR,
            static_cast<uint32_t>(context.numSamples)
        );
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_) {
            n8_rust_compressor_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_compressor_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 3> pins_;
    std::array<ParameterInfo, 6> params_;
    RustCompressor* rustInstance_{ nullptr };
};

inline AutoRegisterNode<CompressorNode> registerCompressor(NodeType::Compressor, "compressor", "Dynamics");

} // namespace audio_graph

