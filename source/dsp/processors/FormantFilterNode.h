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
 * @brief Filtro Resonante Multi-Formante Vocal con Morfología Continua (A-E-I-O-U)
 * (Reglas 5, 8, 14, 34, 46, 47)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class FormantFilterNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Vowel = 1,        // Morfología vocal continua (0.0 = A, 1.0 = E, 2.0 = I, 3.0 = O, 4.0 = U)
        Resonance = 2,    // Factor Q de resonancia (1.0 a 25.0)
        FormantShift = 3, // Desplazamiento de formantes / Género (0.5x a 2.0x)
        Warmth = 4,       // Saturación armónica analógica del tracto vocal (0.0 a 1.0)
        Mix = 5           // Dry / Wet
    };

    FormantFilterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Vowel, "Vowel", 0.0f, 0.0f, 4.0f, true };
        params_[1] = { Resonance, "Resonance", 6.0f, 1.0f, 25.0f, true };
        params_[2] = { FormantShift, "Gender / Shift", 1.0f, 0.5f, 2.0f, true };
        params_[3] = { Warmth, "Warmth", 0.3f, 0.0f, 1.0f, true };
        params_[4] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_formant_filter_create(44100.0f);
    }

    ~FormantFilterNode() override {
        if (rustInstance_) {
            n8_rust_formant_filter_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    FormantFilterNode(const FormantFilterNode&) = delete;
    FormantFilterNode& operator=(const FormantFilterNode&) = delete;

    FormantFilterNode(FormantFilterNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    FormantFilterNode& operator=(FormantFilterNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_formant_filter_destroy(rustInstance_);
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
            n8_rust_formant_filter_prepare(rustInstance_, sr);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_formant_filter_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_) {
            n8_rust_formant_filter_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_formant_filter_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::FormantFilter; }
    const char* getName() const override { return "Formant Filter"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47
        if (!rustInstance_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (!inL || !outL) return;

        n8_rust_formant_filter_process(
            rustInstance_,
            inL,
            inR,
            outL,
            outR,
            static_cast<uint32_t>(context.numSamples)
        );
    }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
    RustFormantFilter* rustInstance_{ nullptr };
};

inline AutoRegisterNode<FormantFilterNode> registerFormant(NodeType::FormantFilter, "formant_filter", "Filters");

} // namespace audio_graph
