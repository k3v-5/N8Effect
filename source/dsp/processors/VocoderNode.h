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
 * @brief Vocoder Clásico de 16 Bandas (Channel Vocoder)
 * Banco de filtros modulador/portadora con oscilador armónico interno (Reglas 5, 8, 14, 34, 46, 47)
 */
class VocoderNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        CarrierMode = 1,  // 0: Sawtooth, 1: Pulse, 2: Noise, 3: Stereo In (L=Mod, R=Car)
        CarrierPitch = 2, // Frecuencia de la portadora interna en Hz (35 Hz a 500 Hz)
        FormantShift = 3, // Desplazamiento espectral de formantes (0.5x a 2.0x)
        BandQ = 4,        // Factor Q de resonancia de las 16 bandas (1.5 a 15.0)
        ReleaseTime = 5,  // Tiempo de caída de los seguidores de envolvente (0.01s a 0.25s)
        Mix = 6           // Dry / Wet
    };

    static constexpr size_t NumBands = 16;

    VocoderNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { SidechainPinId, "Modulator In", PinType::AudioInput, PinDataType::AudioStereo };

        params_[0] = { CarrierMode, "Carrier Mode", 0.0f, 0.0f, 3.0f, false };
        params_[1] = { CarrierPitch, "Pitch Hz", 110.0f, 35.0f, 500.0f, true };
        params_[2] = { FormantShift, "Formant Shift", 1.0f, 0.5f, 2.0f, true };
        params_[3] = { BandQ, "Band Q", 6.0f, 1.5f, 15.0f, true };
        params_[4] = { ReleaseTime, "Release", 0.05f, 0.01f, 0.25f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        oscPhase_ = 0.0;
        rngState_ = 0x55AA1234u;
        updateFilters();
        reset();
    }

    void reset() override {
        for (size_t b = 0; b < NumBands; ++b) {
            modFilters_[b].reset();
            carFilters_[b].reset();
            envelopes_[b] = 0.0f;
        }
        oscPhase_ = 0.0;
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case CarrierMode:  targetMode_ = static_cast<int>(std::round(value)); break;
            case CarrierPitch: targetPitch_ = std::clamp(value, 35.0f, 500.0f); break;
            case FormantShift: targetShift_ = std::clamp(value, 0.5f, 2.0f); updateFilters(); break;
            case BandQ:        targetQ_ = std::clamp(value, 1.5f, 15.0f); updateFilters(); break;
            case ReleaseTime:  targetRelease_ = std::clamp(value, 0.01f, 0.25f); updateCoeffs(); break;
            case Mix:          targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case CarrierMode:  return static_cast<float>(targetMode_);
            case CarrierPitch: return targetPitch_;
            case FormantShift: return targetShift_;
            case BandQ:        return targetQ_;
            case ReleaseTime:  return targetRelease_;
            case Mix:          return targetMix_;
            default:           return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Vocoder; }
    const char* getName() const override { return "Channel Vocoder"; }

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

        const double sampleRate = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        const double phaseIncrement = static_cast<double>(targetPitch_) / sampleRate;

        const int mode = targetMode_;
        const float attCoeff = 0.25f; // Ataque rápido para consonantes claras (~4ms)
        const float relCoeff = releaseCoeff_;
        const float mix = targetMix_;

        const bool hasSidechain = (context.numSidechainChannels > 0 && context.sidechainChannels != nullptr &&
                                   context.sidechainChannels[0] != nullptr && context.sidechainChannels != context.inputChannels);
        const float* scLPtr = hasSidechain ? context.sidechainChannels[0] : nullptr;
        const float* scRPtr = (hasSidechain && context.numSidechainChannels > 1 && context.sidechainChannels[1])
            ? context.sidechainChannels[1] : scLPtr;

        for (size_t s = 0; s < numSamples; ++s) {
            const float rawInL = inL ? inL[s] : 0.0f;
            const float rawInR = inR ? inR[s] : rawInL;

            float modSample = 0.0f;
            float carSample = 0.0f;

            if (hasSidechain) {
                // Entrada Sidechain modular: Audio In es la Portadora (Carrier) y Sidechain In es el Modulador (Voz)
                const float rawScL = scLPtr ? scLPtr[s] : 0.0f;
                const float rawScR = scRPtr ? scRPtr[s] : rawScL;
                modSample = 0.5f * (rawScL + rawScR);
                carSample = 0.5f * (rawInL + rawInR);
            } else {
                // 1. Determinar señal moduladora (voz/ritmo de entrada)
                modSample = (mode == 3) ? rawInL : 0.5f * (rawInL + rawInR);

                // 2. Generar o tomar señal portadora (Carrier)
                switch (mode) {
                    case 0: // Sierra interna rica en armónicos pares e impares
                        carSample = static_cast<float>(2.0 * oscPhase_ - 1.0);
                        break;
                    case 1: // Pulso/Cuadrada interna
                        carSample = (oscPhase_ < 0.5) ? 0.9f : -0.9f;
                        break;
                    case 2: // Ruido blanco (Vocoder susurrante)
                        carSample = nextNoise();
                        break;
                    case 3: // Entrada estéreo: L = Modulador, R = Portadora
                        carSample = rawInR;
                        break;
                    default:
                        carSample = static_cast<float>(2.0 * oscPhase_ - 1.0);
                        break;
                }
            }

            oscPhase_ += phaseIncrement;
            if (oscPhase_ >= 1.0) oscPhase_ -= 1.0;

            // 3. Procesar las 16 bandas del banco de filtros
            float vocodedOut = 0.0f;

            for (size_t b = 0; b < NumBands; ++b) {
                // Filtrar modulador y extraer amplitud
                const float modBand = modFilters_[b].processSample(modSample);
                const float modMag = std::abs(modBand);

                // Seguidor de envolvente por banda
                if (modMag > envelopes_[b]) {
                    envelopes_[b] += attCoeff * (modMag - envelopes_[b]);
                } else {
                    envelopes_[b] += relCoeff * (modMag - envelopes_[b]);
                }

                // Filtrar portadora en la misma banda (o desplazada en formante)
                const float carBand = carFilters_[b].processSample(carSample);

                // Modular la portadora por la envolvente del modulador
                vocodedOut += carBand * envelopes_[b];
            }

            // Normalización y ganancia de síntesis
            const float wet = FastMath::fastTanh(vocodedOut * 4.5f) * 1.3f;

            if (outL) outL[s] = (1.0f - mix) * rawInL + mix * wet;
            if (outR) outR[s] = (1.0f - mix) * rawInR + mix * wet;
        }
    }

private:
    float nextNoise() noexcept {
        rngState_ ^= (rngState_ << 13);
        rngState_ ^= (rngState_ >> 17);
        rngState_ ^= (rngState_ << 5);
        return (static_cast<float>(rngState_ & 0x7FFFFFFF) / 1073741824.0f) - 1.0f;
    }

    void updateCoeffs() noexcept {
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        releaseCoeff_ = static_cast<float>(1.0 - std::exp(-1.0 / (static_cast<double>(targetRelease_) * sr)));
    }

    void updateFilters() noexcept {
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        const double nyquist = sr * 0.48;

        // Distribución logarítmica de las 16 frecuencias centrales entre 140 Hz y 6800 Hz
        constexpr double minFreq = 140.0;
        constexpr double maxFreq = 6800.0;

        for (size_t b = 0; b < NumBands; ++b) {
            const double t = static_cast<double>(b) / static_cast<double>(NumBands - 1);
            const double fc = minFreq * std::pow(maxFreq / minFreq, t);

            // Modulator bandpass
            modFilters_[b].setCoefficients(BiquadFilter::Type::Bandpass, sr, static_cast<float>(std::min(fc, nyquist)), targetQ_);

            // Carrier bandpass con desplazamiento de formantes
            const double carFc = std::min(fc * static_cast<double>(targetShift_), nyquist);
            carFilters_[b].setCoefficients(BiquadFilter::Type::Bandpass, sr, static_cast<float>(carFc), targetQ_);
        }

        updateCoeffs();
    }

    ProcessSpec spec_;
    std::array<BiquadFilter, NumBands> modFilters_;
    std::array<BiquadFilter, NumBands> carFilters_;
    std::array<float, NumBands> envelopes_{ 0.0f };

    double oscPhase_{ 0.0 };
    uint32_t rngState_{ 0x55AA1234u };
    float releaseCoeff_{ 0.01f };

    int targetMode_{ 0 };
    float targetPitch_{ 110.0f };
    float targetShift_{ 1.0f };
    float targetQ_{ 6.0f };
    float targetRelease_{ 0.05f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 3> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<VocoderNode> registerVocoder(NodeType::Vocoder, "vocoder", "Spectral");

} // namespace audio_graph
