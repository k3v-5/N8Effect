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
 * @brief Ecualizador Paramétrico Dinámico de 4 Bandas (Reglas 5, 8, 9, 14, 34, 46, 47)
 * Permite atenuación o realce dinámico quirúrgico según la energía por banda en tiempo real.
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class DynamicEqNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Band1Freq = 1,
        Band1Gain = 2,
        Band1Thresh = 3,
        Band2Freq = 4,
        Band2Gain = 5,
        Band2Thresh = 6,
        Band3Freq = 7,
        Band3Gain = 8,
        Band3Thresh = 9,
        Band4Freq = 10,
        Band4Gain = 11,
        Band4Thresh = 12,
        DynamicRatio = 13,
        AttackMs = 14,
        ReleaseMs = 15,
        Mix = 16
    };

    DynamicEqNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { SidechainPinId, "Sidechain", PinType::AudioInput, PinDataType::AudioStereo };

        params_[0]  = { Band1Freq, "Low Freq", 100.0f, 20.0f, 500.0f, true };
        params_[1]  = { Band1Gain, "Low Gain", 0.0f, -18.0f, 18.0f, true };
        params_[2]  = { Band1Thresh, "Low Thr", -18.0f, -60.0f, 0.0f, true };
        params_[3]  = { Band2Freq, "Mid1 Freq", 500.0f, 80.0f, 4000.0f, true };
        params_[4]  = { Band2Gain, "Mid1 Gain", 0.0f, -18.0f, 18.0f, true };
        params_[5]  = { Band2Thresh, "Mid1 Thr", -18.0f, -60.0f, 0.0f, true };
        params_[6]  = { Band3Freq, "Mid2 Freq", 2500.0f, 300.0f, 12000.0f, true };
        params_[7]  = { Band3Gain, "Mid2 Gain", 0.0f, -18.0f, 18.0f, true };
        params_[8]  = { Band3Thresh, "Mid2 Thr", -18.0f, -60.0f, 0.0f, true };
        params_[9]  = { Band4Freq, "High Freq", 8000.0f, 1000.0f, 20000.0f, true };
        params_[10] = { Band4Gain, "High Gain", 0.0f, -18.0f, 18.0f, true };
        params_[11] = { Band4Thresh, "High Thr", -18.0f, -60.0f, 0.0f, true };
        params_[12] = { DynamicRatio, "Ratio", 2.5f, 1.0f, 10.0f, true };
        params_[13] = { AttackMs, "Attack", 10.0f, 1.0f, 100.0f, true };
        params_[14] = { ReleaseMs, "Release", 100.0f, 10.0f, 1000.0f, true };
        params_[15] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_dynamic_eq_create(48000.0);
    }

    ~DynamicEqNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_dynamic_eq_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_dynamic_eq_set_sample_rate(rustInstance_, spec.sampleRate);
            n8_rust_dynamic_eq_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_dynamic_eq_reset(rustInstance_);
        }
    }

    NodeType getType() const override { return NodeType::DynamicEq; }
    const char* getName() const override { return "Dynamic EQ"; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const uint32_t numSamples = context.numSamples;
        if (numSamples == 0 || rustInstance_ == nullptr) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        n8_rust_dynamic_eq_process(rustInstance_, inL, inR, outL, outR, numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_dynamic_eq_set_parameter(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_ != nullptr) {
            return n8_rust_dynamic_eq_get_parameter(rustInstance_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    RustDynamicEq* rustInstance_{ nullptr };

    std::array<PinDescriptor, 3> pins_;
    std::array<ParameterInfo, 16> params_;
};

inline AutoRegisterNode<DynamicEqNode> registerDynamicEq(NodeType::DynamicEq, "dynamic_eq", "Dynamics");

} // namespace audio_graph
