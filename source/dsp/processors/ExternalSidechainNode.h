#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/EnvelopeDetector.h"
#include "../core/DenormalGuards.h"

namespace audio_graph {

/**
 * @brief Nodo de entrada para capturar el canal de Sidechain externo del host DAW (Reglas 4, 5, 6, 9, 47).
 * Permite introducir la señal de Sidechain del DAW directamente en el grafo para modular otros efectos.
 */
class ExternalSidechainNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Gain = 1,
        Mute = 2
    };

    ExternalSidechainNode() {
        pins_[0] = { 1, "SC Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Gain, "Gain", 0.0f, -60.0f, 12.0f, true };
        params_[1] = { Mute, "Mute", 0.0f, 0.0f, 1.0f, false };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        currentGain_ = 1.0f;
    }

    void reset() override {
        currentGain_ = 1.0f;
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const uint32_t numSamples = context.numSamples;
        const uint32_t numOut = context.numOutputChannels;
        float* outL = (numOut > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (numOut > 1) ? context.outputChannels[1] : outL;

        if (outL == nullptr || numSamples == 0) return;

        const bool isMuted = (targetMute_ > 0.5f);
        const float targetGainLin = isMuted ? 0.0f : EnvelopeDetector::dbToLinear(targetGainDb_);

        const float* scL = (context.numSidechainChannels > 0 && context.sidechainChannels != nullptr && context.sidechainChannels[0] != nullptr)
            ? context.sidechainChannels[0] : nullptr;
        const float* scR = (context.numSidechainChannels > 1 && context.sidechainChannels != nullptr && context.sidechainChannels[1] != nullptr)
            ? context.sidechainChannels[1] : scL;

        for (uint32_t s = 0; s < numSamples; ++s) {
            currentGain_ += 0.01f * (targetGainLin - currentGain_); // Suavizado anti-click (Regla 35)

            const float sampleL = (scL != nullptr) ? (scL[s] * currentGain_) : 0.0f;
            const float sampleR = (scR != nullptr) ? (scR[s] * currentGain_) : sampleL;

            outL[s] = sampleL;
            if (outR != nullptr) outR[s] = sampleR;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Gain: targetGainDb_ = std::clamp(value, -60.0f, 12.0f); break;
            case Mute: targetMute_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Gain: return targetGainDb_;
            case Mute: return targetMute_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::ExternalSidechain; }
    const char* getName() const override { return "Ext Sidechain"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    float targetGainDb_{ 0.0f };
    float targetMute_{ 0.0f };
    float currentGain_{ 1.0f };

    std::array<PinDescriptor, 1> pins_;
    std::array<ParameterInfo, 2> params_;
};

inline AutoRegisterNode<ExternalSidechainNode> registerExternalSidechain(NodeType::ExternalSidechain, "ext_sidechain", "Routing");

} // namespace audio_graph
