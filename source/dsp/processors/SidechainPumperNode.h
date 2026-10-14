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
 * @brief Kickstart-Style Sidechain Pumper & Multiband Ducking Node (Rules 5, 8, 9, 14, 19, 34, 35, 37, 46, 47)
 * Efecto de bombeo de volumen / sidechain ducking sincronizado al tempo del host y posiciÃ³n PPQ.
 * Incluye mÃºltiples curvas estilÃ­sticas (EDM, Techno, Snappy, S-Curve), desfase temporal,
 * crossover multibanda (ducking exclusivo de frecuencias graves) y detector de trigger por audio sidechain.
 */
class SidechainPumperNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        SyncRate = 1,
        Depth = 2,
        CurveShape = 3,
        CurveSkew = 4,
        PhaseOffset = 5,
        BandSplit = 6,
        CrossoverHz = 7,
        TriggerMode = 8,
        Mix = 9
    };

    SidechainPumperNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { SidechainPinId, "Sidechain In", PinType::AudioInput, PinDataType::AudioStereo };

        params_[0] = { SyncRate, "Sync Rate", 2.0f, 0.0f, 6.0f, false };     // 2 = 1/4 note default
        params_[1] = { Depth, "Depth", 1.0f, 0.0f, 1.0f, true };             // 100% ducking
        params_[2] = { CurveShape, "Curve Shape", 0.0f, 0.0f, 5.0f, false }; // 0 = Classic EDM
        params_[3] = { CurveSkew, "Curve Skew", 1.0f, 0.2f, 3.5f, true };
        params_[4] = { PhaseOffset, "Phase Offset", 0.0f, -0.5f, 0.5f, true };
        params_[5] = { BandSplit, "Band Split", 0.0f, 0.0f, 1.0f, false };   // 0 = Full, 1 = Low Only
        params_[6] = { CrossoverHz, "Crossover (Hz)", 200.0f, 40.0f, 1200.0f, true };
        params_[7] = { TriggerMode, "Trigger Mode", 0.0f, 0.0f, 2.0f, false };// 0 = Host Sync, 1 = Audio SC
        params_[8] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_sidechain_pumper_create(44100.0f);
    }

    ~SidechainPumperNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_sidechain_pumper_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::SidechainPumper; }
    const char* getName() const override { return "Sidechain Pumper"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_sidechain_pumper_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_sidechain_pumper_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_sidechain_pumper_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_sidechain_pumper_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case SyncRate: return params_[0].defaultValue;
            case Depth: return params_[1].defaultValue;
            case CurveShape: return params_[2].defaultValue;
            case CurveSkew: return params_[3].defaultValue;
            case PhaseOffset: return params_[4].defaultValue;
            case BandSplit: return params_[5].defaultValue;
            case CrossoverHz: return params_[6].defaultValue;
            case TriggerMode: return params_[7].defaultValue;
            case Mix: return params_[8].defaultValue;
            default: return 0.0f;
        }
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (outL == nullptr || rustInstance_ == nullptr || inL == nullptr) return;

        // Sidechain input signals
        const float* scL = (context.numSidechainChannels > 0 && context.sidechainChannels != nullptr) ? context.sidechainChannels[0] : nullptr;
        const float* scR = (context.numSidechainChannels > 1 && context.sidechainChannels != nullptr) ? context.sidechainChannels[1] : scL;

        // Pass host synchronization
        float bpm = static_cast<float>(context.bpm > 10.0 ? context.bpm : 120.0);
        n8_rust_sidechain_pumper_set_host_sync(rustInstance_, bpm, context.ppqPosition, context.isPlaying);

        n8_rust_sidechain_pumper_process(rustInstance_, inL, inR, outL, outR, scL, scR, context.numSamples);
    }

private:
    RustSidechainPumper* rustInstance_{ nullptr };
    ProcessSpec spec_{};
    std::array<PinDescriptor, 3> pins_{};
    std::array<ParameterInfo, 9> params_{};
};

inline AutoRegisterNode<SidechainPumperNode> registerSidechainPumper(NodeType::SidechainPumper, "sidechainpumper", "Dynamics");

} // namespace audio_graph
