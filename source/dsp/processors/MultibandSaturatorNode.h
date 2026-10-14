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
 * @brief Multiband Saturator (Phase 9 - Large Effect Library, Rules 5, 8, 9, 32, 34, 46, 47)
 * Saturador tribanda estilo FabFilter Saturn con crossovers LR4 Linkwitz-Riley y modelos independientes.
 */
class MultibandSaturatorNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        CrossoverLowHz = 1,
        CrossoverHighHz = 2,
        DriveLowDb = 3,
        DriveMidDb = 4,
        DriveHighDb = 5,
        ModelLow = 6,   // 0: Tape, 1: Tube, 2: Foldback
        ModelMid = 7,
        ModelHigh = 8,
        Mix = 9
    };

    MultibandSaturatorNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { CrossoverLowHz, "Xover Low", 250.0f, 80.0f, 1000.0f, true };
        params_[1] = { CrossoverHighHz, "Xover High", 3500.0f, 1000.0f, 12000.0f, true };
        params_[2] = { DriveLowDb, "Drive Low", 4.0f, 0.0f, 36.0f, true };
        params_[3] = { DriveMidDb, "Drive Mid", 6.0f, 0.0f, 36.0f, true };
        params_[4] = { DriveHighDb, "Drive High", 3.0f, 0.0f, 36.0f, true };
        params_[5] = { ModelLow, "Model Low", 0.0f, 0.0f, 2.0f, false };
        params_[6] = { ModelMid, "Model Mid", 1.0f, 0.0f, 2.0f, false };
        params_[7] = { ModelHigh, "Model High", 0.0f, 0.0f, 2.0f, false };
        params_[8] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_multiband_saturator_create(44100.0f);
    }

    ~MultibandSaturatorNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_multiband_saturator_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::MultibandSaturator; }
    const char* getName() const override { return "Multiband Saturator"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_multiband_saturator_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_multiband_saturator_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_multiband_saturator_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_multiband_saturator_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case CrossoverLowHz: return params_[0].defaultValue;
            case CrossoverHighHz: return params_[1].defaultValue;
            case DriveLowDb: return params_[2].defaultValue;
            case DriveMidDb: return params_[3].defaultValue;
            case DriveHighDb: return params_[4].defaultValue;
            case ModelLow: return params_[5].defaultValue;
            case ModelMid: return params_[6].defaultValue;
            case ModelHigh: return params_[7].defaultValue;
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

        n8_rust_multiband_saturator_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    ProcessSpec spec_;
    RustMultibandSaturator* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 9> params_;
};

inline AutoRegisterNode<MultibandSaturatorNode> registerMultibandSaturator(NodeType::MultibandSaturator, "multibandsaturator", "Distortion");

} // namespace audio_graph
