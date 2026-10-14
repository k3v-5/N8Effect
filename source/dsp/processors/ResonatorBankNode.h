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
 * @brief Banco de 6 Resonadores Modales y Armónicos en paralelo con amortiguación y Tails (Reglas 5, 8, 12, 14, 18, 32, 46)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class ResonatorBankNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        FundamentalFreq = 1,
        DecayTime = 2,
        HarmonicSpread = 3,
        Brightness = 4,
        DryWet = 5
    };

    static constexpr size_t NumResonators = 6;

    ResonatorBankNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { FundamentalFreq, "Freq", 220.0f, 20.0f, 2000.0f, true };
        params_[1] = { DecayTime, "Decay", 1.5f, 0.05f, 10.0f, true };
        params_[2] = { HarmonicSpread, "Spread", 0.0f, 0.0f, 1.0f, true };
        params_[3] = { Brightness, "Brightness", 0.0f, -1.0f, 1.0f, true };
        params_[4] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_resonator_bank_create(44100.0f);
    }

    ~ResonatorBankNode() override {
        if (rustInstance_) {
            n8_rust_resonator_bank_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    ResonatorBankNode(const ResonatorBankNode&) = delete;
    ResonatorBankNode& operator=(const ResonatorBankNode&) = delete;

    ResonatorBankNode(ResonatorBankNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    ResonatorBankNode& operator=(ResonatorBankNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_resonator_bank_destroy(rustInstance_);
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
            n8_rust_resonator_bank_prepare(rustInstance_, sr);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_resonator_bank_reset(rustInstance_);
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

        n8_rust_resonator_bank_process(
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
            n8_rust_resonator_bank_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_resonator_bank_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::Resonator; }
    const char* getName() const override { return "Resonator Bank"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override {
        if (rustInstance_) {
            return n8_rust_resonator_bank_get_tail_samples(rustInstance_);
        }
        return static_cast<uint32_t>(spec_.sampleRate * 1.5);
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
    RustResonatorBank* rustInstance_{ nullptr };
};

inline AutoRegisterNode<ResonatorBankNode> registerResonatorBank(NodeType::Resonator, "resonator_bank", "Filter");

} // namespace audio_graph
