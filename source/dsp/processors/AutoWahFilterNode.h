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
 * @brief Filtro Auto-Wah / Envelope Filter Dinámico Estilo Analógico (Reglas 5, 8, 9, 14, 34, 46, 47)
 * Modula la frecuencia de resonancia de forma reactiva a la envolvente dinámica del audio entrante o sidechain.
 * Acelerado con núcleo en Rust de alto rendimiento (Cytomic SVF).
 */
class AutoWahFilterNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        BaseFreq = 1,
        Sensitivity = 2,
        Resonance = 3,
        AttackMs = 4,
        DecayMs = 5,
        FilterMode = 6, // 0: Lowpass, 1: Bandpass, 2: Highpass
        Mix = 7
    };

    AutoWahFilterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { SidechainPinId, "Sidechain In", PinType::AudioInput, PinDataType::AudioStereo };

        params_[0] = { BaseFreq, "Base Freq", 350.0f, 60.0f, 3000.0f, true };
        params_[1] = { Sensitivity, "Sens", 0.6f, 0.0f, 1.0f, true };
        params_[2] = { Resonance, "Q / Res", 4.5f, 0.7f, 15.0f, true };
        params_[3] = { AttackMs, "Attack", 15.0f, 1.0f, 100.0f, true };
        params_[4] = { DecayMs, "Decay", 180.0f, 10.0f, 800.0f, true };
        params_[5] = { FilterMode, "Mode", 1.0f, 0.0f, 2.0f, false };
        params_[6] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_auto_wah_create(44100.0f);
    }

    ~AutoWahFilterNode() override {
        if (rustInstance_) {
            n8_rust_auto_wah_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    AutoWahFilterNode(const AutoWahFilterNode&) = delete;
    AutoWahFilterNode& operator=(const AutoWahFilterNode&) = delete;

    AutoWahFilterNode(AutoWahFilterNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    AutoWahFilterNode& operator=(AutoWahFilterNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_auto_wah_destroy(rustInstance_);
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
            n8_rust_auto_wah_prepare(rustInstance_, sr);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_auto_wah_reset(rustInstance_);
        }
    }

    NodeType getType() const override { return NodeType::AutoWahFilter; }
    const char* getName() const override { return "Auto-Wah Filter"; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;
        if (!rustInstance_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;
        const float* scL = (context.numSidechainChannels > 0 && context.sidechainChannels != nullptr) ? context.sidechainChannels[0] : nullptr;
        const float* scR = (context.numSidechainChannels > 1 && context.sidechainChannels != nullptr) ? context.sidechainChannels[1] : scL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (!inL || !outL) return;

        n8_rust_auto_wah_process(
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
            n8_rust_auto_wah_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_auto_wah_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 3> pins_;
    std::array<ParameterInfo, 7> params_;
    RustAutoWah* rustInstance_{ nullptr };
};

inline AutoRegisterNode<AutoWahFilterNode> registerAutoWahFilter(NodeType::AutoWahFilter, "autowah", "Filters");

} // namespace audio_graph

