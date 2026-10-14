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
 * @brief Ecualizador Paramétrico de 3 Bandas (Low Shelf, Bell, High Shelf) (Reglas 5, 8, 12, 14, 34)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class ParametricEQNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        LowFreq = 1,
        LowGain = 2,
        MidFreq = 3,
        MidQ = 4,
        MidGain = 5,
        HighFreq = 6,
        HighGain = 7
    };

    ParametricEQNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { LowFreq, "Low Freq", 100.0f, 20.0f, 500.0f, true };
        params_[1] = { LowGain, "Low Gain", 0.0f, -18.0f, 18.0f, true };
        params_[2] = { MidFreq, "Mid Freq", 1000.0f, 100.0f, 8000.0f, true };
        params_[3] = { MidQ, "Mid Q", 1.0f, 0.1f, 10.0f, true };
        params_[4] = { MidGain, "Mid Gain", 0.0f, -18.0f, 18.0f, true };
        params_[5] = { HighFreq, "High Freq", 8000.0f, 1000.0f, 18000.0f, true };
        params_[6] = { HighGain, "High Gain", 0.0f, -18.0f, 18.0f, true };

        rustInstance_ = n8_rust_parametric_eq_create(44100.0f);
    }

    ~ParametricEQNode() override {
        if (rustInstance_) {
            n8_rust_parametric_eq_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    ParametricEQNode(const ParametricEQNode&) = delete;
    ParametricEQNode& operator=(const ParametricEQNode&) = delete;

    ParametricEQNode(ParametricEQNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    ParametricEQNode& operator=(ParametricEQNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_parametric_eq_destroy(rustInstance_);
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
            n8_rust_parametric_eq_prepare(rustInstance_, sr);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_parametric_eq_reset(rustInstance_);
        }
    }

    NodeType getType() const override { return NodeType::ParametricEQ; }
    const char* getName() const override { return "Parametric EQ"; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;
        if (!rustInstance_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (!inL || !outL) return;

        n8_rust_parametric_eq_process(
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
            n8_rust_parametric_eq_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_parametric_eq_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 7> params_;
    RustParametricEq* rustInstance_{ nullptr };
};

inline AutoRegisterNode<ParametricEQNode> registerParametricEQ(NodeType::ParametricEQ, "parametric_eq", "Filters");

} // namespace audio_graph

