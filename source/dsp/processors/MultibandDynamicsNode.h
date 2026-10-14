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
 * @brief Procesador de Dinámica Multibanda (3 bandas) estilo OTT con compresión Upward y Downward (Reglas 5, 8, 32, 33, 34, 46)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class MultibandDynamicsNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        CrossoverLowMid = 1,
        CrossoverMidHigh = 2,
        LowDynamics = 3,
        MidDynamics = 4,
        HighDynamics = 5,
        DryWet = 6
    };

    MultibandDynamicsNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { CrossoverLowMid, "Xover Low-Mid", 250.0f, 60.0f, 1000.0f, true };
        params_[1] = { CrossoverMidHigh, "Xover Mid-High", 2500.0f, 800.0f, 12000.0f, true };
        params_[2] = { LowDynamics, "Low Dyn", 1.0f, 0.0f, 2.0f, true };
        params_[3] = { MidDynamics, "Mid Dyn", 1.0f, 0.0f, 2.0f, true };
        params_[4] = { HighDynamics, "High Dyn", 1.0f, 0.0f, 2.0f, true };
        params_[5] = { DryWet, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_multiband_dynamics_create(44100.0f);
    }

    ~MultibandDynamicsNode() override {
        if (rustInstance_) {
            n8_rust_multiband_dynamics_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    MultibandDynamicsNode(const MultibandDynamicsNode&) = delete;
    MultibandDynamicsNode& operator=(const MultibandDynamicsNode&) = delete;

    MultibandDynamicsNode(MultibandDynamicsNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    MultibandDynamicsNode& operator=(MultibandDynamicsNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_multiband_dynamics_destroy(rustInstance_);
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
            n8_rust_multiband_dynamics_prepare(rustInstance_, sr);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_multiband_dynamics_reset(rustInstance_);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;
        if (!rustInstance_ || context.numSamples == 0 || context.numInputChannels == 0 || context.numOutputChannels == 0) return;

        const float* inL = context.inputChannels[0];
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1]) ? context.inputChannels[1] : inL;

        float* outL = context.outputChannels[0];
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1]) ? context.outputChannels[1] : outL;

        if (!inL || !outL) return;

        n8_rust_multiband_dynamics_process(
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
            n8_rust_multiband_dynamics_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_multiband_dynamics_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::Multiband; }
    const char* getName() const override { return "Multiband Dynamics"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
    RustMultibandDynamics* rustInstance_{ nullptr };
};

inline AutoRegisterNode<MultibandDynamicsNode> registerMultibandDynamics(NodeType::Multiband, "multiband_dynamics", "Dynamics");

} // namespace audio_graph
