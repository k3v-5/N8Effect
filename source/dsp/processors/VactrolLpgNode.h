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
 * @brief Buchla 292 Vactrol Low-Pass Gate & Resonant Strike (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Modelo fÃ­sico de optoacoplador vactrol (LED + CdS fotorresistencia con memoria lenta de doble decaimiento)
 * combinado con filtro pasabajos Sallen-Key saturado por diodos y VCA.
 * Produce los caracterÃ­sticos timbres percusivos "Buchla bongo", ataques secos y colas acÃºsticas orgÃ¡nicas.
 */
class VactrolLpgNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Cutoff = 1,
        Resonance = 2,
        DecayMs = 3,
        Strike = 4,
        ModeBalance = 5,
        Drive = 6,
        Mix = 7
    };

    VactrolLpgNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { 4, "Strike In", PinType::AudioRateModulationInput, PinDataType::AudioStereo };

        params_[0] = { Cutoff, "Cutoff", 0.15f, 0.0f, 1.0f, true };
        params_[1] = { Resonance, "Resonance", 0.40f, 0.0f, 0.95f, true };
        params_[2] = { DecayMs, "Decay (ms)", 180.0f, 10.0f, 2500.0f, true };
        params_[3] = { Strike, "Strike Ping", 0.85f, 0.0f, 1.0f, true };
        params_[4] = { ModeBalance, "Mode (VCF/Both/VCA)", 0.5f, 0.0f, 1.0f, true };
        params_[5] = { Drive, "Drive", 1.5f, 1.0f, 5.0f, true };
        params_[6] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_vactrol_lpg_create(44100.0f);
    }

    ~VactrolLpgNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_vactrol_lpg_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::VactrolLPG; }
    const char* getName() const override { return "Vactrol LPG"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_vactrol_lpg_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_vactrol_lpg_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_vactrol_lpg_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_vactrol_lpg_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Cutoff: return params_[0].defaultValue;
            case Resonance: return params_[1].defaultValue;
            case DecayMs: return params_[2].defaultValue;
            case Strike: return params_[3].defaultValue;
            case ModeBalance: return params_[4].defaultValue;
            case Drive: return params_[5].defaultValue;
            case Mix: return params_[6].defaultValue;
            default: return 0.0f;
        }
    }

    void triggerStrike() {
        if (rustInstance_ != nullptr) {
            n8_rust_vactrol_lpg_trigger_strike(rustInstance_);
        }
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;
        const float* strikeIn = (context.numAudioRateModChannels > 0 && context.audioRateModChannels != nullptr && context.audioRateModChannels[0] != nullptr)
            ? context.audioRateModChannels[0] : nullptr;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (outL == nullptr || rustInstance_ == nullptr || inL == nullptr) return;

        n8_rust_vactrol_lpg_process(rustInstance_, inL, inR, strikeIn, outL, outR, context.numSamples);
    }

private:
    ProcessSpec spec_;
    RustVactrolLPG* rustInstance_{ nullptr };

    std::array<PinDescriptor, 3> pins_;
    std::array<ParameterInfo, 7> params_;
};

inline AutoRegisterNode<VactrolLpgNode> registerVactrolLpg(NodeType::VactrolLPG, "vactrollpg", "Filter");

} // namespace audio_graph
