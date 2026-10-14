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
 * @brief Excitador Armónico Dual (Harmonic Exciter & Sub-Harmonic Bass Synthesizer)
 * Generación de brillo y aire psicoacústico + subgraves sintetizados (Reglas 5, 8, 14, 34, 46, 47)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class HarmonicExciterNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        AirFreq = 1,   // Frecuencia paso-alto de aire (3000 Hz a 12000 Hz)
        AirDrive = 2,  // Generación armónica en agudos (0.0 a 6.0)
        AirMix = 3,    // Nivel de brillo de aire (0.0 a 1.0)
        SubFreq = 4,   // Frecuencia de corte de sub-bajos (40 Hz a 120 Hz)
        SubDrive = 5,  // Intensidad de sub-octava (0.0 a 6.0)
        SubMix = 6,    // Nivel de subgraves sintetizados (0.0 a 1.0)
        Mix = 7        // Dry / Wet general
    };

    HarmonicExciterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { AirFreq, "Air Freq", 6000.0f, 3000.0f, 12000.0f, true };
        params_[1] = { AirDrive, "Air Drive", 2.0f, 0.0f, 6.0f, true };
        params_[2] = { AirMix, "Air Mix", 0.5f, 0.0f, 1.0f, true };
        params_[3] = { SubFreq, "Sub Freq", 80.0f, 40.0f, 120.0f, true };
        params_[4] = { SubDrive, "Sub Drive", 1.5f, 0.0f, 6.0f, true };
        params_[5] = { SubMix, "Sub Mix", 0.4f, 0.0f, 1.0f, true };
        params_[6] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_harmonic_exciter_create(44100.0);
    }

    ~HarmonicExciterNode() override {
        if (rustInstance_) {
            n8_rust_harmonic_exciter_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    HarmonicExciterNode(const HarmonicExciterNode&) = delete;
    HarmonicExciterNode& operator=(const HarmonicExciterNode&) = delete;

    HarmonicExciterNode(HarmonicExciterNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    HarmonicExciterNode& operator=(HarmonicExciterNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_harmonic_exciter_destroy(rustInstance_);
            }
            pins_ = other.pins_;
            params_ = other.params_;
            spec_ = other.spec_;
            rustInstance_ = other.rustInstance_;
            other.rustInstance_ = nullptr;
        }
        return *this;
    }

    NodeType getType() const override { return NodeType::HarmonicExciter; }
    const char* getName() const override { return "Harmonic Exciter"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_) {
            n8_rust_harmonic_exciter_prepare(rustInstance_, spec.sampleRate);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_harmonic_exciter_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_) {
            n8_rust_harmonic_exciter_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_harmonic_exciter_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        if (!rustInstance_ || context.numSamples == 0) return;

        const size_t numChannels = std::min<size_t>(context.numOutputChannels, 2);
        const float* inL = (context.numInputChannels > 0) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : inL;
        float* outL = (numChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (numChannels > 1) ? context.outputChannels[1] : outL;

        if (!outL || !inL) return;

        n8_rust_harmonic_exciter_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustHarmonicExciter* rustInstance_{ nullptr };
    ProcessSpec spec_{ 44100.0, 256, 2, 2 };
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 7> params_;
};

inline AutoRegisterNode<HarmonicExciterNode> registerHarmonicExciter(NodeType::HarmonicExciter, "harmonic_exciter", "Color");

} // namespace audio_graph
