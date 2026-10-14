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
 * @brief Dual Bode Frequency Shifter Node (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Desplazador de frecuencia lineal en Hertz inspirado en el Bode Frequency Shifter (Moog 1630).
 * Utiliza red desfasadora Hilbert ortogonal I/Q y oscilador en cuadratura para SSB / rotación armónica en espiral.
 */
class DualBodeFrequencyShifterNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        ShiftHz = 1,
        StereoSpreadHz = 2,
        Feedback = 3,
        DampingHz = 4,
        SidebandBalance = 5,
        Mix = 6
    };

    DualBodeFrequencyShifterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { ShiftHz, "Frequency Shift (Hz)", 0.0f, -2000.0f, 2000.0f, true };
        params_[1] = { StereoSpreadHz, "Stereo Spread Shift (Hz)", 25.0f, -500.0f, 500.0f, true };
        params_[2] = { Feedback, "Feedback (Spiral Loop)", 0.0f, -0.95f, 0.95f, true };
        params_[3] = { DampingHz, "Feedback Damping (Hz)", 5000.0f, 400.0f, 14000.0f, true };
        params_[4] = { SidebandBalance, "Sideband (-1=Lower, 0=DSB, 1=Upper)", 1.0f, -1.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_dual_bode_frequency_shifter_create(44100.0f);
    }

    ~DualBodeFrequencyShifterNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_dual_bode_frequency_shifter_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::DualBodeFrequencyShifter; }
    const char* getName() const override { return "Dual Bode Frequency Shifter"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_dual_bode_frequency_shifter_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_dual_bode_frequency_shifter_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_dual_bode_frequency_shifter_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_dual_bode_frequency_shifter_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case ShiftHz: return params_[0].defaultValue;
            case StereoSpreadHz: return params_[1].defaultValue;
            case Feedback: return params_[2].defaultValue;
            case DampingHz: return params_[3].defaultValue;
            case SidebandBalance: return params_[4].defaultValue;
            case Mix: return params_[5].defaultValue;
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

        n8_rust_dual_bode_frequency_shifter_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustDualBodeFrequencyShifter* rustInstance_{ nullptr };
    ProcessSpec spec_{};
    std::array<PinDescriptor, 2> pins_{};
    std::array<ParameterInfo, 6> params_{};
};

inline AutoRegisterNode<DualBodeFrequencyShifterNode> registerDualBodeFrequencyShifter(
    NodeType::DualBodeFrequencyShifter, "dualbodefrequencyshifter", "Pitch");

} // namespace audio_graph
