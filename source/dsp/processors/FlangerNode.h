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
 * @brief Procesador Flanger con Feedback Bipolar y Peine Profundo (Reglas 5, 8, 14, 34, 46, 47)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class FlangerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Rate = 1,
        Depth = 2,
        DelayMs = 3,
        Feedback = 4,
        Mix = 5
    };

    FlangerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Rate, "Rate", 0.25f, 0.05f, 5.0f, true };
        params_[1] = { Depth, "Depth", 0.8f, 0.0f, 1.0f, true };
        params_[2] = { DelayMs, "Delay", 2.0f, 0.2f, 10.0f, true };
        params_[3] = { Feedback, "Feedback", 0.7f, -0.95f, 0.95f, true };
        params_[4] = { Mix, "Mix", 0.5f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_flanger_create(44100.0f);
    }

    ~FlangerNode() override {
        if (rustInstance_) {
            n8_rust_flanger_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    FlangerNode(const FlangerNode&) = delete;
    FlangerNode& operator=(const FlangerNode&) = delete;

    FlangerNode(FlangerNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    FlangerNode& operator=(FlangerNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_flanger_destroy(rustInstance_);
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
            n8_rust_flanger_prepare(rustInstance_, sr);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_flanger_reset(rustInstance_);
        }
    }

    NodeType getType() const override { return NodeType::Flanger; }
    const char* getName() const override { return "Flanger"; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;
        if (!rustInstance_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (!inL || !outL) return;

        n8_rust_flanger_process(
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
            n8_rust_flanger_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_flanger_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
    RustFlanger* rustInstance_{ nullptr };
};

inline AutoRegisterNode<FlangerNode> registerFlanger(NodeType::Flanger, "flanger", "Modulation");

} // namespace audio_graph
