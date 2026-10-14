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
 * @brief Simulador de Frenado y Arranque de Cinta/Vinilo (Tape Stop & Spin-Up)
 * (Reglas 5, 8, 14, 34, 35, 46, 47)
 * Delegado al motor DSP SIMD en Rust (n8_rust_core).
 */
class TapeStopNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Trigger = 1,     // 0 = Normal / Play, 1 = Brake / Stop
        StopTime = 2,    // Segundos de frenado (0.1s a 4.0s)
        SpinUpTime = 3,  // Segundos de arranque (0.05s a 2.0s)
        Curve = 4,       // 0: Lineal, 1: Exponencial, 2: Curva S
        Inertia = 5,     // Fluctuación/wobble de arrastre mecánico
        Mix = 6          // Dry / Wet
    };

    TapeStopNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Trigger, "Stop", 0.0f, 0.0f, 1.0f, false };
        params_[1] = { StopTime, "Stop Time", 0.6f, 0.05f, 4.0f, true };
        params_[2] = { SpinUpTime, "Spin Up", 0.3f, 0.05f, 2.0f, true };
        params_[3] = { Curve, "Curve", 1.0f, 0.0f, 2.0f, false };
        params_[4] = { Inertia, "Inertia", 0.35f, 0.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustStop_ = n8_rust_tape_stop_create(44100.0f);
    }

    ~TapeStopNode() override {
        if (rustStop_) {
            n8_rust_tape_stop_destroy(rustStop_);
            rustStop_ = nullptr;
        }
    }

    TapeStopNode(const TapeStopNode&) = delete;
    TapeStopNode& operator=(const TapeStopNode&) = delete;

    TapeStopNode(TapeStopNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), rustStop_(other.rustStop_) {
        other.rustStop_ = nullptr;
    }

    TapeStopNode& operator=(TapeStopNode&& other) noexcept {
        if (this != &other) {
            if (rustStop_) {
                n8_rust_tape_stop_destroy(rustStop_);
            }
            pins_ = other.pins_;
            params_ = other.params_;
            rustStop_ = other.rustStop_;
            other.rustStop_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        if (rustStop_) {
            n8_rust_tape_stop_prepare(rustStop_, static_cast<float>(spec.sampleRate));
        }
    }

    void reset() override {
        if (rustStop_) {
            n8_rust_tape_stop_reset(rustStop_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustStop_) {
            n8_rust_tape_stop_set_parameter(rustStop_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustStop_) {
            return n8_rust_tape_stop_get_parameter(rustStop_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::TapeStop; }
    const char* getName() const override { return "Tape Stop"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47
        if (!rustStop_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : nullptr;

        n8_rust_tape_stop_process(rustStop_, inL, inR, outL, outR, context.numSamples);
    }

private:
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
    RustTapeStop* rustStop_{ nullptr };
};

inline AutoRegisterNode<TapeStopNode> registerTapeStop(NodeType::TapeStop, "tape_stop", "Special / Time");

} // namespace audio_graph
