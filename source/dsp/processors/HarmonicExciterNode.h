#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../core/FastMath.h"
#include "../core/BiquadFilter.h"

namespace audio_graph {

/**
 * @brief Excitador Armónico Dual (Harmonic Exciter & Sub-Harmonic Bass Synthesizer)
 * Generación de brillo y aire psicoacústico + subgraves sintetizados (Reglas 5, 8, 14, 34, 46, 47)
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
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        updateFilters();
        reset();
    }

    void reset() override {
        for (auto& f : airHPF_) f.reset();
        for (auto& f : airPhaseAllpass_) f.reset();
        for (auto& f : subLPF_) f.reset();
        for (auto& f : subPostLPF_) f.reset();
        subFlipFlop_.fill(1.0f);
        prevSubSample_.fill(0.0f);
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case AirFreq:  targetAirFreq_ = std::clamp(value, 3000.0f, 12000.0f); updateFilters(); break;
            case AirDrive: targetAirDrive_ = std::clamp(value, 0.0f, 6.0f); break;
            case AirMix:   targetAirMix_ = std::clamp(value, 0.0f, 1.0f); break;
            case SubFreq:  targetSubFreq_ = std::clamp(value, 40.0f, 120.0f); updateFilters(); break;
            case SubDrive: targetSubDrive_ = std::clamp(value, 0.0f, 6.0f); break;
            case SubMix:   targetSubMix_ = std::clamp(value, 0.0f, 1.0f); break;
            case Mix:      targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case AirFreq:  return targetAirFreq_;
            case AirDrive: return targetAirDrive_;
            case AirMix:   return targetAirMix_;
            case SubFreq:  return targetSubFreq_;
            case SubDrive: return targetSubDrive_;
            case SubMix:   return targetSubMix_;
            case Mix:      return targetMix_;
            default:       return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::HarmonicExciter; }
    const char* getName() const override { return "Harmonic Exciter"; }

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

        const float airDrive = 1.0f + targetAirDrive_;
        const float airMix = targetAirMix_;
        const float subDrive = 1.0f + targetSubDrive_;
        const float subMix = targetSubMix_;
        const float mix = targetMix_;

        for (size_t s = 0; s < numSamples; ++s) {
            const float rawInL = inL ? inL[s] : 0.0f;
            const float rawInR = inR ? inR[s] : rawInL;

            // 1. Rama de Agudos (Air Exciter): Paso-alto -> saturación asimétrica -> rotación de fase
            float highL = airHPF_[0].processSample(rawInL);
            float highR = airHPF_[1].processSample(rawInR);

            // Generador asimétrico de armónicos pares e impares
            highL = (highL * airDrive) + 0.35f * (highL * highL * airDrive) - 0.15f * (highL * highL * highL * airDrive);
            highR = (highR * airDrive) + 0.35f * (highR * highR * airDrive) - 0.15f * (highR * highR * highR * airDrive);

            // Rotación de fase pasa-todo para acoplamiento armónico natural
            highL = airPhaseAllpass_[0].processSample(highL);
            highR = airPhaseAllpass_[1].processSample(highR);

            // 2. Rama de Subgraves (Sub-Harmonic Synthesizer)
            float lowL = subLPF_[0].processSample(rawInL);
            float lowR = subLPF_[1].processSample(rawInR);

            // Detección de cruce por cero para conmutación de sub-octava limpia (f0 / 2)
            if (lowL > 0.0f && prevSubSample_[0] <= 0.0f) subFlipFlop_[0] = -subFlipFlop_[0];
            if (lowR > 0.0f && prevSubSample_[1] <= 0.0f) subFlipFlop_[1] = -subFlipFlop_[1];
            prevSubSample_[0] = lowL;
            prevSubSample_[1] = lowR;

            float subL = std::abs(lowL) * subFlipFlop_[0] * subDrive;
            float subR = std::abs(lowR) * subFlipFlop_[1] * subDrive;

            // Suavizar onda para conservar solo el fundamental subgrave
            subL = subPostLPF_[0].processSample(subL);
            subR = subPostLPF_[1].processSample(subR);

            // 3. Mezcla de las bandas excitadas con el audio original
            const float wetL = rawInL + highL * airMix + subL * subMix;
            const float wetR = rawInR + highR * airMix + subR * subMix;

            if (outL) outL[s] = (1.0f - mix) * rawInL + mix * wetL;
            if (outR) outR[s] = (1.0f - mix) * rawInR + mix * wetR;
        }
    }

private:
    void updateFilters() noexcept {
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        for (size_t ch = 0; ch < 2; ++ch) {
            airHPF_[ch].setCoefficients(BiquadFilter::Type::Highpass, sr, targetAirFreq_, 0.707f);
            airPhaseAllpass_[ch].setCoefficients(BiquadFilter::Type::Allpass, sr, targetAirFreq_ * 0.8f, 1.0f);
            subLPF_[ch].setCoefficients(BiquadFilter::Type::Lowpass, sr, targetSubFreq_, 0.707f);
            subPostLPF_[ch].setCoefficients(BiquadFilter::Type::Lowpass, sr, targetSubFreq_, 0.707f);
        }
    }

    ProcessSpec spec_;
    std::array<BiquadFilter, 2> airHPF_;
    std::array<BiquadFilter, 2> airPhaseAllpass_;
    std::array<BiquadFilter, 2> subLPF_;
    std::array<BiquadFilter, 2> subPostLPF_;

    std::array<float, 2> subFlipFlop_{ 1.0f, 1.0f };
    std::array<float, 2> prevSubSample_{ 0.0f, 0.0f };

    float targetAirFreq_{ 6000.0f };
    float targetAirDrive_{ 2.0f };
    float targetAirMix_{ 0.5f };
    float targetSubFreq_{ 80.0f };
    float targetSubDrive_{ 1.5f };
    float targetSubMix_{ 0.4f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 7> params_;
};

inline AutoRegisterNode<HarmonicExciterNode> registerHarmonicExciter(NodeType::HarmonicExciter, "harmonic_exciter", "Color");

} // namespace audio_graph
