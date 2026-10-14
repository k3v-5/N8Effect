#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/EnvelopeDetector.h"
#include "../core/DenormalGuards.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Nodo de entrada para capturar el canal de Sidechain externo del host DAW (Reglas 4, 5, 6, 9, 47).
 * Permite introducir la señal de Sidechain del DAW directamente en el grafo para modular otros efectos.
 * Procesamiento acelerado en Rust con protección denormal y suavizado anti-click.
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

        rustCore_ = n8_rust_external_sidechain_create();
        updateParameters();
    }

    ~ExternalSidechainNode() override {
        if (rustCore_) {
            n8_rust_external_sidechain_free(rustCore_);
            rustCore_ = nullptr;
        }
    }

    ExternalSidechainNode(const ExternalSidechainNode& other)
        : AudioProcessorNode(other)
        , spec_(other.spec_)
        , targetGainDb_(other.targetGainDb_)
        , targetMute_(other.targetMute_)
        , pins_(other.pins_)
        , params_(other.params_)
    {
        rustCore_ = n8_rust_external_sidechain_create();
        if (rustCore_ && spec_.sampleRate > 0) {
            n8_rust_external_sidechain_prepare(rustCore_, static_cast<float>(spec_.sampleRate));
        }
        updateParameters();
    }

    ExternalSidechainNode& operator=(const ExternalSidechainNode& other) {
        if (this != &other) {
            AudioProcessorNode::operator=(other);
            spec_ = other.spec_;
            targetGainDb_ = other.targetGainDb_;
            targetMute_ = other.targetMute_;
            pins_ = other.pins_;
            params_ = other.params_;
            if (rustCore_) {
                n8_rust_external_sidechain_free(rustCore_);
            }
            rustCore_ = n8_rust_external_sidechain_create();
            if (rustCore_ && spec_.sampleRate > 0) {
                n8_rust_external_sidechain_prepare(rustCore_, static_cast<float>(spec_.sampleRate));
            }
            updateParameters();
        }
        return *this;
    }

    ExternalSidechainNode(ExternalSidechainNode&& other) noexcept
        : AudioProcessorNode(std::move(other))
        , spec_(other.spec_)
        , targetGainDb_(other.targetGainDb_)
        , targetMute_(other.targetMute_)
        , pins_(other.pins_)
        , params_(other.params_)
        , rustCore_(other.rustCore_)
    {
        other.rustCore_ = nullptr;
    }

    ExternalSidechainNode& operator=(ExternalSidechainNode&& other) noexcept {
        if (this != &other) {
            AudioProcessorNode::operator=(std::move(other));
            if (rustCore_) {
                n8_rust_external_sidechain_free(rustCore_);
            }
            spec_ = other.spec_;
            targetGainDb_ = other.targetGainDb_;
            targetMute_ = other.targetMute_;
            pins_ = other.pins_;
            params_ = other.params_;
            rustCore_ = other.rustCore_;
            other.rustCore_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustCore_) {
            n8_rust_external_sidechain_prepare(rustCore_, static_cast<float>(spec.sampleRate));
        }
        updateParameters();
    }

    void reset() override {
        if (rustCore_) {
            n8_rust_external_sidechain_reset(rustCore_);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const uint32_t numSamples = context.numSamples;
        const uint32_t numOut = context.numOutputChannels;
        float* outL = (numOut > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (numOut > 1) ? context.outputChannels[1] : nullptr;

        if (outL == nullptr || numSamples == 0 || rustCore_ == nullptr) return;

        const float* scL = (context.numSidechainChannels > 0 && context.sidechainChannels != nullptr && context.sidechainChannels[0] != nullptr)
            ? context.sidechainChannels[0] : nullptr;
        const float* scR = (context.numSidechainChannels > 1 && context.sidechainChannels != nullptr && context.sidechainChannels[1] != nullptr)
            ? context.sidechainChannels[1] : nullptr;

        n8_rust_external_sidechain_process(rustCore_, scL, scR, outL, outR, numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Gain: targetGainDb_ = std::clamp(value, -60.0f, 12.0f); break;
            case Mute: targetMute_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
        updateParameters();
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
    void updateParameters() {
        if (rustCore_) {
            n8_rust_external_sidechain_set_parameters(rustCore_, targetGainDb_, targetMute_);
        }
    }

    ProcessSpec spec_;
    float targetGainDb_{ 0.0f };
    float targetMute_{ 0.0f };

    std::array<PinDescriptor, 1> pins_;
    std::array<ParameterInfo, 2> params_;

    RustExternalSidechain* rustCore_{ nullptr };
};

inline AutoRegisterNode<ExternalSidechainNode> registerExternalSidechain(NodeType::ExternalSidechain, "ext_sidechain", "Routing");

} // namespace audio_graph
