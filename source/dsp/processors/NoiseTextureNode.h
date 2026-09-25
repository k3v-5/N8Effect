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
 * @brief Generador de Ruido y Texturas Orgánicas (Vinyl, Tape, Pink, White, Rain)
 * con Sidechain Ducking y Modelado Espectral (Reglas 5, 8, 9, 14, 34, 46, 47)
 */
class NoiseTextureNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Mode = 1,          // 0: White, 1: Pink, 2: Tape Hiss, 3: Vinyl Crackle, 4: Rain/Ambient
        Density = 2,       // Frecuencia de chasquidos / densidad textural (0.0 a 1.0)
        Tone = 3,          // Brillo / Corte de tono (300 Hz a 18000 Hz)
        SidechainDuck = 4, // Atenuación automática del ruido ante señal entrante (0.0 a 1.0)
        Level = 5,         // Volumen de inyección de textura (0.0 a 1.0)
        Mix = 6            // Mezcla general
    };

    NoiseTextureNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Mode, "Mode", 3.0f, 0.0f, 4.0f, false };
        params_[1] = { Density, "Density", 0.45f, 0.0f, 1.0f, true };
        params_[2] = { Tone, "Tone Hz", 8000.0f, 300.0f, 18000.0f, true };
        params_[3] = { SidechainDuck, "Duck", 0.5f, 0.0f, 1.0f, true };
        params_[4] = { Level, "Level", 0.4f, 0.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        rngState_ = 0x12345678u;
        for (auto& f : toneFilters_) {
            f.reset();
            f.setCoefficients(BiquadFilter::Type::Lowpass, spec_.sampleRate, targetTone_, 0.707f);
        }
        for (auto& f : vinylBandpass_) {
            f.reset();
            f.setCoefficients(BiquadFilter::Type::Bandpass, spec_.sampleRate, 2500.0f, 4.0f);
        }
        pinkB_.fill(0.0f);
        envFollower_ = 0.0f;
        crackleHold_ = 0;
        reset();
    }

    void reset() override {
        for (auto& f : toneFilters_) f.reset();
        for (auto& f : vinylBandpass_) f.reset();
        pinkB_.fill(0.0f);
        envFollower_ = 0.0f;
        crackleHold_ = 0;
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Mode:          targetMode_ = static_cast<int>(std::round(value)); break;
            case Density:       targetDensity_ = std::clamp(value, 0.0f, 1.0f); break;
            case Tone:
                targetTone_ = std::clamp(value, 300.0f, 18000.0f);
                updateToneFilter();
                break;
            case SidechainDuck: targetDuck_ = std::clamp(value, 0.0f, 1.0f); break;
            case Level:         targetLevel_ = std::clamp(value, 0.0f, 1.0f); break;
            case Mix:           targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Mode:          return static_cast<float>(targetMode_);
            case Density:       return targetDensity_;
            case Tone:          return targetTone_;
            case SidechainDuck: return targetDuck_;
            case Level:         return targetLevel_;
            case Mix:           return targetMix_;
            default:            return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::NoiseTexture; }
    const char* getName() const override { return "Noise & Texture"; }

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

        const int mode = targetMode_;
        const float density = targetDensity_;
        const float duckAmt = targetDuck_;
        const float level = targetLevel_;
        const float mix = targetMix_;

        for (size_t s = 0; s < numSamples; ++s) {
            const float rawInL = inL ? inL[s] : 0.0f;
            const float rawInR = inR ? inR[s] : rawInL;

            // 1. Detección de envolvente para Sidechain Ducking
            const float inMag = 0.5f * (std::abs(rawInL) + std::abs(rawInR));
            envFollower_ = (inMag > envFollower_) ? (0.95f * envFollower_ + 0.05f * inMag)
                                                  : (0.999f * envFollower_ + 0.001f * inMag);
            const float duckGain = std::clamp(1.0f - (envFollower_ * duckAmt * 2.5f), 0.0f, 1.0f);

            // 2. Generación de ruido crudo base
            const float whiteL = nextWhite();
            const float whiteR = nextWhite();

            float rawNoiseL = 0.0f;
            float rawNoiseR = 0.0f;

            switch (mode) {
                case 0: // White Noise
                    rawNoiseL = whiteL * 0.4f;
                    rawNoiseR = whiteR * 0.4f;
                    break;

                case 1: // Pink Noise (Kellet 3-pole filter)
                    rawNoiseL = processPink(whiteL);
                    rawNoiseR = processPink(whiteR);
                    break;

                case 2: // Tape Hiss (Banda acotada cálida)
                    rawNoiseL = processPink(whiteL) * 0.6f + whiteL * 0.15f;
                    rawNoiseR = processPink(whiteR) * 0.6f + whiteR * 0.15f;
                    break;

                case 3: { // Vinyl Crackle
                    // Ruido de fondo de superficie constante y sutil
                    float surfaceL = processPink(whiteL) * 0.15f;
                    float surfaceR = processPink(whiteR) * 0.15f;

                    // Micro-chasquidos estocásticos Poisson
                    float crackleL = 0.0f;
                    float crackleR = 0.0f;
                    const uint32_t threshold = static_cast<uint32_t>(density * 6500.0f);
                    if ((xorshift32() & 0x0FFFFF) < threshold) {
                        float spike = (nextWhite() > 0.0f ? 1.0f : -1.0f) * (0.6f + 0.4f * std::abs(nextWhite()));
                        crackleL = spike;
                        crackleR = spike * 0.85f;
                    }

                    crackleL = vinylBandpass_[0].processSample(crackleL);
                    crackleR = vinylBandpass_[1].processSample(crackleR);

                    rawNoiseL = surfaceL + crackleL * 2.2f;
                    rawNoiseR = surfaceR + crackleR * 2.2f;
                    break;
                }

                case 4: { // Rain / Organic Ambient
                    float pink = processPink(whiteL);
                    // Modulación sutil de amplitud estocástica
                    rainModPhase_ += 0.0001f;
                    float rainLFO = 0.7f + 0.3f * std::sin(rainModPhase_);
                    rawNoiseL = pink * rainLFO * 0.5f;
                    rawNoiseR = processPink(whiteR) * (1.4f - rainLFO) * 0.5f;
                    break;
                }

                default:
                    rawNoiseL = whiteL * 0.3f;
                    rawNoiseR = whiteR * 0.3f;
                    break;
            }

            // 3. Filtrado de tono (Tone LPF)
            float filteredNoiseL = toneFilters_[0].processSample(rawNoiseL);
            float filteredNoiseR = toneFilters_[1].processSample(rawNoiseR);

            // 4. Aplicar nivel y ducking
            filteredNoiseL *= (level * duckGain);
            filteredNoiseR *= (level * duckGain);

            // 5. Inyección a la salida y mezcla Dry/Wet
            const float wetL = rawInL + filteredNoiseL;
            const float wetR = rawInR + filteredNoiseR;

            if (outL) outL[s] = (1.0f - mix) * rawInL + mix * wetL;
            if (outR) outR[s] = (1.0f - mix) * rawInR + mix * wetR;
        }
    }

private:
    uint32_t xorshift32() noexcept {
        rngState_ ^= (rngState_ << 13);
        rngState_ ^= (rngState_ >> 17);
        rngState_ ^= (rngState_ << 5);
        return rngState_;
    }

    float nextWhite() noexcept {
        return (static_cast<float>(xorshift32() & 0x7FFFFFFF) / 1073741824.0f) - 1.0f;
    }

    float processPink(float white) noexcept {
        pinkB_[0] = 0.99886f * pinkB_[0] + white * 0.0555179f;
        pinkB_[1] = 0.99332f * pinkB_[1] + white * 0.0750759f;
        pinkB_[2] = 0.96900f * pinkB_[2] + white * 0.1538520f;
        pinkB_[3] = 0.86650f * pinkB_[3] + white * 0.3104856f;
        pinkB_[4] = 0.55000f * pinkB_[4] + white * 0.5329522f;
        pinkB_[5] = -0.7616f * pinkB_[5] - white * 0.0168980f;
        return (pinkB_[0] + pinkB_[1] + pinkB_[2] + pinkB_[3] + pinkB_[4] + pinkB_[5] + white * 0.5362f) * 0.18f;
    }

    void updateToneFilter() noexcept {
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        for (auto& f : toneFilters_) {
            f.setCoefficients(BiquadFilter::Type::Lowpass, sr, targetTone_, 0.707f);
        }
    }

    ProcessSpec spec_;
    uint32_t rngState_{ 0x12345678u };
    std::array<float, 6> pinkB_{ 0.0f };
    std::array<BiquadFilter, 2> toneFilters_;
    std::array<BiquadFilter, 2> vinylBandpass_;
    float envFollower_{ 0.0f };
    int crackleHold_{ 0 };
    float rainModPhase_{ 0.0f };

    int targetMode_{ 3 };
    float targetDensity_{ 0.45f };
    float targetTone_{ 8000.0f };
    float targetDuck_{ 0.5f };
    float targetLevel_{ 0.4f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<NoiseTextureNode> registerNoiseTexture(NodeType::NoiseTexture, "noise_texture", "Generators");

} // namespace audio_graph
