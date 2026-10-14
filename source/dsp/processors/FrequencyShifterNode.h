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
 * @brief Desplazador de Frecuencia Lineal por Banda Lateral Única (SSB) y Red Hilbert Allpass (Reglas 5, 8, 14, 34, 46, 47)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class FrequencyShifterNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        ShiftHz = 1,
        Feedback = 2,
        Mix = 3
    };

    FrequencyShifterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { ShiftHz, "Shift", 50.0f, -1000.0f, 1000.0f, true };
        params_[1] = { Feedback, "Feedback", 0.0f, 0.0f, 0.85f, true };
        params_[2] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_frequency_shifter_create(44100.0f);
    }

    ~FrequencyShifterNode() override {
        if (rustInstance_) {
            n8_rust_frequency_shifter_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    FrequencyShifterNode(const FrequencyShifterNode&) = delete;
    FrequencyShifterNode& operator=(const FrequencyShifterNode&) = delete;

    FrequencyShifterNode(FrequencyShifterNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    FrequencyShifterNode& operator=(FrequencyShifterNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_frequency_shifter_destroy(rustInstance_);
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
            n8_rust_frequency_shifter_prepare(rustInstance_, sr);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_frequency_shifter_reset(rustInstance_);
        }
    }

    NodeType getType() const override { return NodeType::FrequencyShifter; }
    const char* getName() const override { return "FrequencyShifter"; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;
        if (!rustInstance_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (!inL || !outL) return;

        n8_rust_frequency_shifter_process(
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
            n8_rust_frequency_shifter_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_frequency_shifter_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 3> params_;
    RustFrequencyShifter* rustInstance_{ nullptr };
};

inline AutoRegisterNode<FrequencyShifterNode> registerFrequencyShifter(NodeType::FrequencyShifter, "freqshift", "Pitch");

} // namespace audio_graph
