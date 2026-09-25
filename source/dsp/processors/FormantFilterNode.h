#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include <numbers>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../core/FastMath.h"
#include "../core/BiquadFilter.h"

namespace audio_graph {

/**
 * @brief Filtro Resonante Multi-Formante Vocal con Morfología Continua (A-E-I-O-U)
 * (Reglas 5, 8, 14, 34, 46, 47)
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

    struct FormantFreqs {
        float f1, f2, f3;
        float gain1, gain2, gain3;
    };

    FormantFilterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Vowel, "Vowel", 0.0f, 0.0f, 4.0f, true };
        params_[1] = { Resonance, "Resonance", 6.0f, 1.0f, 25.0f, true };
        params_[2] = { FormantShift, "Gender / Shift", 1.0f, 0.5f, 2.0f, true };
        params_[3] = { Warmth, "Warmth", 0.3f, 0.0f, 1.0f, true };
        params_[4] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        updateFilters();
        reset();
    }

    void reset() override {
        for (auto& chFilters : bandpassFilters_) {
            for (auto& f : chFilters) {
                f.reset();
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Vowel:        targetVowel_ = std::clamp(value, 0.0f, 4.0f); updateFilters(); break;
            case Resonance:    targetResonance_ = std::clamp(value, 1.0f, 25.0f); updateFilters(); break;
            case FormantShift: targetShift_ = std::clamp(value, 0.5f, 2.0f); updateFilters(); break;
            case Warmth:       targetWarmth_ = std::clamp(value, 0.0f, 1.0f); break;
            case Mix:          targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Vowel:        return targetVowel_;
            case Resonance:    return targetResonance_;
            case FormantShift: return targetShift_;
            case Warmth:       return targetWarmth_;
            case Mix:          return targetMix_;
            default:           return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::FormantFilter; }
    const char* getName() const override { return "Formant Filter"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47

        if (context.numSamples == 0) return;

        const size_t numSamples = context.numSamples;
        const size_t numChannels = std::min<size_t>(context.numOutputChannels, 2);

        const float* inL = (context.numInputChannels > 0) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : inL;
        float* outL = (numChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (numChannels > 1) ? context.outputChannels[1] : outL;

        const float warmth = targetWarmth_;
        const float mix = targetMix_;

        for (size_t s = 0; s < numSamples; ++s) {
            const float rawInL = inL ? inL[s] : 0.0f;
            const float rawInR = inR ? inR[s] : rawInL;

            // Procesar los 3 formantes en paralelo para el canal L
            float formantSumL = currentGains_[0] * bandpassFilters_[0][0].processSample(rawInL)
                              + currentGains_[1] * bandpassFilters_[0][1].processSample(rawInL)
                              + currentGains_[2] * bandpassFilters_[0][2].processSample(rawInL);

            // Procesar los 3 formantes en paralelo para el canal R
            float formantSumR = currentGains_[0] * bandpassFilters_[1][0].processSample(rawInR)
                              + currentGains_[1] * bandpassFilters_[1][1].processSample(rawInR)
                              + currentGains_[2] * bandpassFilters_[1][2].processSample(rawInR);

            // Saturación armónica de tubo vocal cálido
            if (warmth > 0.01f) {
                const float drive = 1.0f + warmth * 2.5f;
                formantSumL = FastMath::fastTanh(formantSumL * drive) * 1.15f;
                formantSumR = FastMath::fastTanh(formantSumR * drive) * 1.15f;
            }

            // Normalización suave y mezcla Dry/Wet
            const float wetL = formantSumL * 1.8f;
            const float wetR = formantSumR * 1.8f;

            if (outL) outL[s] = (1.0f - mix) * rawInL + mix * wetL;
            if (outR) outR[s] = (1.0f - mix) * rawInR + mix * wetR;
        }
    }

private:
    void updateFilters() noexcept {
        const float sr = static_cast<float>(spec_.sampleRate > 0 ? spec_.sampleRate : 44100.0);
        const float nyquist = sr * 0.48f;

        // Frecuencias canónicas de formantes vocales
        // 0: A, 1: E, 2: I, 3: O, 4: U
        static constexpr std::array<FormantFreqs, 5> vowelTable = {{
            { 800.0f, 1200.0f, 2500.0f, 1.0f, 0.7f, 0.35f }, // A
            { 400.0f, 2000.0f, 2800.0f, 1.0f, 0.8f, 0.30f }, // E
            { 250.0f, 2300.0f, 3000.0f, 1.0f, 0.85f, 0.40f }, // I
            { 500.0f,  850.0f, 2400.0f, 1.0f, 0.65f, 0.25f }, // O
            { 300.0f,  700.0f, 2200.0f, 1.0f, 0.60f, 0.20f }  // U
        }};

        const float v = std::clamp(targetVowel_, 0.0f, 3.999f);
        const size_t idx0 = static_cast<size_t>(v);
        const size_t idx1 = std::min(idx0 + 1, size_t{ 4 });
        const float frac = v - static_cast<float>(idx0);

        // Interpolación continua de formantes y ganancias
        const auto& tab0 = vowelTable[idx0];
        const auto& tab1 = vowelTable[idx1];

        const float f1 = std::clamp((tab0.f1 + frac * (tab1.f1 - tab0.f1)) * targetShift_, 100.0f, nyquist);
        const float f2 = std::clamp((tab0.f2 + frac * (tab1.f2 - tab0.f2)) * targetShift_, 200.0f, nyquist);
        const float f3 = std::clamp((tab0.f3 + frac * (tab1.f3 - tab0.f3)) * targetShift_, 500.0f, nyquist);

        currentGains_[0] = tab0.gain1 + frac * (tab1.gain1 - tab0.gain1);
        currentGains_[1] = tab0.gain2 + frac * (tab1.gain2 - tab0.gain2);
        currentGains_[2] = tab0.gain3 + frac * (tab1.gain3 - tab0.gain3);

        const float q = std::clamp(targetResonance_, 1.0f, 25.0f);

        // Configurar los 3 filtros pasa-banda por canal estéreo
        for (size_t ch = 0; ch < 2; ++ch) {
            bandpassFilters_[ch][0].setCoefficients(BiquadFilter::Type::Bandpass, sr, f1, q);
            bandpassFilters_[ch][1].setCoefficients(BiquadFilter::Type::Bandpass, sr, f2, q);
            bandpassFilters_[ch][2].setCoefficients(BiquadFilter::Type::Bandpass, sr, f3, q);
        }
    }

    ProcessSpec spec_;
    // 2 canales estéreo x 3 formantes resonantes (F1, F2, F3)
    std::array<std::array<BiquadFilter, 3>, 2> bandpassFilters_;
    std::array<float, 3> currentGains_{ 1.0f, 0.7f, 0.35f };

    float targetVowel_{ 0.0f };
    float targetResonance_{ 6.0f };
    float targetShift_{ 1.0f };
    float targetWarmth_{ 0.3f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
};

inline AutoRegisterNode<FormantFilterNode> registerFormant(NodeType::FormantFilter, "formant_filter", "Filters");

} // namespace audio_graph
