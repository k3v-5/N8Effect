#pragma once

#include <cmath>
#include <array>
#include <vector>
#include <complex>
#include <span>
#include <algorithm>
#include <numbers>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/FFTEngine.h"
#include "../core/FastMath.h"
#include "../core/DenormalGuards.h"

namespace audio_graph {

/**
 * @brief Spectral Smear & Phase Diffusion Engine (Reglas 5, 8, 9, 14, 18, 45, 46, 47).
 * Procesador espectral de difusión líquida y emborronamiento tímbrico basado en STFT FFT (1024 puntos, 75% OLA)
 * que aleatoriza progresivamente las fases de los bins espectrales, aplica persistencia temporal de magnitud
 * asimétrica y descorrelación estéreo para convertir cualquier señal en pads y estelas etéreas continuas.
 */
class SpectralSmearNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        SmearTime = 1,         // 0.05s a 10.0s (default: 2.5s)
        PhaseDiffusion = 2,    // 0.0 a 1.0 (default: 0.7)
        StereoDecorrel = 3,    // 0.0 a 1.0 (default: 0.85)
        TiltDamping = 4,       // -1.0 a 1.0 (default: 0.0)
        DryWet = 5             // 0.0 a 1.0 (default: 0.5)
    };

    static constexpr size_t FftSize = 1024;
    static constexpr size_t HopSize = 256; // 75% solapamiento
    static constexpr size_t NumBins = FftSize / 2;
    static constexpr size_t OlaBufferSize = FftSize * 2;

    SpectralSmearNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { SmearTime, "Smear Time", 2.5f, 0.05f, 10.0f, true };
        params_[1] = { PhaseDiffusion, "Phase Diff", 0.7f, 0.0f, 1.0f, true };
        params_[2] = { StereoDecorrel, "Stereo Spread", 0.85f, 0.0f, 1.0f, true };
        params_[3] = { TiltDamping, "Tilt", 0.0f, -1.0f, 1.0f, true };
        params_[4] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };

        inputRingL_.assign(FftSize, 0.0f);
        inputRingR_.assign(FftSize, 0.0f);
        olaBufferL_.assign(OlaBufferSize, 0.0f);
        olaBufferR_.assign(OlaBufferSize, 0.0f);
        timeFrameL_.assign(FftSize, 0.0f);
        timeFrameR_.assign(FftSize, 0.0f);
        synthFrameL_.assign(FftSize, 0.0f);
        synthFrameR_.assign(FftSize, 0.0f);

        prevMagsL_.assign(NumBins, 0.0f);
        prevMagsR_.assign(NumBins, 0.0f);
        smoothMagsL_.assign(NumBins, 0.0f);
        smoothMagsR_.assign(NumBins, 0.0f);
        phaseAccumL_.assign(NumBins, 0.0f);
        phaseAccumR_.assign(NumBins, 0.0f);
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;

        fftEngineL_.prepare(FftSize, HopSize);
        fftEngineR_.prepare(FftSize, HopSize);

        inputRingL_.assign(FftSize, 0.0f);
        inputRingR_.assign(FftSize, 0.0f);
        olaBufferL_.assign(OlaBufferSize, 0.0f);
        olaBufferR_.assign(OlaBufferSize, 0.0f);
        timeFrameL_.assign(FftSize, 0.0f);
        timeFrameR_.assign(FftSize, 0.0f);
        synthFrameL_.assign(FftSize, 0.0f);
        synthFrameR_.assign(FftSize, 0.0f);

        prevMagsL_.assign(NumBins, 0.0f);
        prevMagsR_.assign(NumBins, 0.0f);
        smoothMagsL_.assign(NumBins, 0.0f);
        smoothMagsR_.assign(NumBins, 0.0f);
        phaseAccumL_.assign(NumBins, 0.0f);
        phaseAccumR_.assign(NumBins, 0.0f);

        inputWritePos_ = 0;
        hopCounter_ = 0;
        olaReadPos_ = 0;
        rngStateL_ = 123456789u;
        rngStateR_ = 987654321u;

        reset();
    }

    void reset() override {
        fftEngineL_.reset();
        fftEngineR_.reset();
        std::fill(inputRingL_.begin(), inputRingL_.end(), 0.0f);
        std::fill(inputRingR_.begin(), inputRingR_.end(), 0.0f);
        std::fill(olaBufferL_.begin(), olaBufferL_.end(), 0.0f);
        std::fill(olaBufferR_.begin(), olaBufferR_.end(), 0.0f);
        std::fill(timeFrameL_.begin(), timeFrameL_.end(), 0.0f);
        std::fill(timeFrameR_.begin(), timeFrameR_.end(), 0.0f);
        std::fill(synthFrameL_.begin(), synthFrameL_.end(), 0.0f);
        std::fill(synthFrameR_.begin(), synthFrameR_.end(), 0.0f);
        std::fill(prevMagsL_.begin(), prevMagsL_.end(), 0.0f);
        std::fill(prevMagsR_.begin(), prevMagsR_.end(), 0.0f);
        std::fill(smoothMagsL_.begin(), smoothMagsL_.end(), 0.0f);
        std::fill(smoothMagsR_.begin(), smoothMagsR_.end(), 0.0f);
        std::fill(phaseAccumL_.begin(), phaseAccumL_.end(), 0.0f);
        std::fill(phaseAccumR_.begin(), phaseAccumR_.end(), 0.0f);

        inputWritePos_ = 0;
        hopCounter_ = 0;
        olaReadPos_ = 0;
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const uint32_t numSamples = context.numSamples;
        if (numSamples == 0 || inputRingL_.empty()) return;

        const float sr = static_cast<float>(spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0);
        const float mix = targetMix_;
        const float smearTime = std::clamp(targetSmearTime_, 0.05f, 10.0f);
        const float phaseDiff = std::clamp(targetPhaseDiff_, 0.0f, 1.0f);
        const float decorrel = std::clamp(targetDecorrel_, 0.0f, 1.0f);
        const float tilt = std::clamp(targetTilt_, -1.0f, 1.0f);

        // Coeficiente de persistencia temporal asimétrica de magnitud: alpha = exp(-H / (fs * T))
        const float alphaBlur = std::clamp(std::exp(-static_cast<float>(HopSize) / (sr * smearTime)), 0.0f, 0.998f);

        // Factor de compensación de amplitud COLA para ventana Hann 75% solapamiento: 1 / 1.5 = 2/3
        constexpr float colaNormalization = 2.0f / 3.0f;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : nullptr;

        for (uint32_t s = 0; s < numSamples; ++s) {
            const float sampleL = inL ? inL[s] : 0.0f;
            const float sampleR = inR ? inR[s] : sampleL;

            // 1. Escribir en buffer circular de entrada
            inputRingL_[inputWritePos_] = sampleL;
            inputRingR_[inputWritePos_] = sampleR;
            inputWritePos_ = (inputWritePos_ + 1) % FftSize;

            // 2. Extraer muestra reconstruida del acumulador Overlap-Add
            float wetL = olaBufferL_[olaReadPos_];
            float wetR = olaBufferR_[olaReadPos_];
            olaBufferL_[olaReadPos_] = 0.0f;
            olaBufferR_[olaReadPos_] = 0.0f;
            olaReadPos_ = (olaReadPos_ + 1) % OlaBufferSize;

            // 3. Procesamiento STFT por saltos (Hop Size = 256)
            hopCounter_++;
            if (hopCounter_ >= HopSize) {
                hopCounter_ = 0;

                // Desenrollar las últimas FftSize muestras del buffer circular
                for (size_t i = 0; i < FftSize; ++i) {
                    const size_t ringIdx = (inputWritePos_ + i) % FftSize;
                    timeFrameL_[i] = inputRingL_[ringIdx];
                    timeFrameR_[i] = inputRingR_[ringIdx];
                }

                // Transformada Directa FFT con ventana Hann
                fftEngineL_.forward(timeFrameL_.data(), true);
                fftEngineR_.forward(timeFrameR_.data(), true);

                auto& freqL = fftEngineL_.getFrequencyBuffer();
                auto& freqR = fftEngineR_.getFrequencyBuffer();

                // 4. Procesamiento de Bins Espectrales
                for (size_t k = 0; k < NumBins; ++k) {
                    const float magL = std::abs(freqL[k]);
                    const float magR = std::abs(freqR[k]);
                    const float inPhaseL = std::arg(freqL[k]);
                    const float inPhaseR = std::arg(freqR[k]);

                    // Persistencia temporal asimétrica (ataque inmediato, decaimiento etéreo largo)
                    float sMagL = (magL >= prevMagsL_[k]) ? magL : (alphaBlur * prevMagsL_[k] + (1.0f - alphaBlur) * magL);
                    float sMagR = (magR >= prevMagsR_[k]) ? magR : (alphaBlur * prevMagsR_[k] + (1.0f - alphaBlur) * magR);
                    prevMagsL_[k] = sMagL;
                    prevMagsR_[k] = sMagR;

                    smoothMagsL_[k] = sMagL;
                    smoothMagsR_[k] = sMagR;

                    // Inclinación espectral (Spectral Tilt)
                    const float normFreq = static_cast<float>(k) / static_cast<float>(NumBins);
                    const float tiltGain = std::pow(10.0f, (tilt * (normFreq - 0.5f) * 16.0f) * 0.05f);
                    sMagL *= tiltGain;
                    sMagR *= tiltGain;

                    // Difusión estocástica de fases y descorrelación estéreo
                    // Pseudo-RNG de congruencia lineal sin bloqueos (Regla 9)
                    rngStateL_ = rngStateL_ * 1664525u + 1013904223u;
                    rngStateR_ = rngStateR_ * 1664525u + 1013904223u;

                    const float randL = (static_cast<float>(rngStateL_ & 0x00FFFFFFu) / static_cast<float>(0x01000000u) - 0.5f) * (2.0f * std::numbers::pi_v<float>);
                    const float randR = (static_cast<float>(rngStateR_ & 0x00FFFFFFu) / static_cast<float>(0x01000000u) - 0.5f) * (2.0f * std::numbers::pi_v<float>);

                    const float deltaOmega = (2.0f * std::numbers::pi_v<float> * static_cast<float>(k * HopSize)) / static_cast<float>(FftSize);

                    phaseAccumL_[k] += deltaOmega + randL * (phaseDiff * 0.75f);
                    const float decorrRand = randR * decorrel + randL * (1.0f - decorrel);
                    phaseAccumR_[k] += deltaOmega + decorrRand * (phaseDiff * 0.75f);

                    // Envolver fase en [-pi, +pi]
                    while (phaseAccumL_[k] > std::numbers::pi_v<float>) phaseAccumL_[k] -= 2.0f * std::numbers::pi_v<float>;
                    while (phaseAccumL_[k] < -std::numbers::pi_v<float>) phaseAccumL_[k] += 2.0f * std::numbers::pi_v<float>;
                    while (phaseAccumR_[k] > std::numbers::pi_v<float>) phaseAccumR_[k] -= 2.0f * std::numbers::pi_v<float>;
                    while (phaseAccumR_[k] < -std::numbers::pi_v<float>) phaseAccumR_[k] += 2.0f * std::numbers::pi_v<float>;

                    // Fusión suave de fase natural y fase difusa
                    const float finalPhaseL = (1.0f - phaseDiff) * inPhaseL + phaseDiff * phaseAccumL_[k];
                    const float finalPhaseR = (1.0f - phaseDiff) * inPhaseR + phaseDiff * phaseAccumR_[k];

                    freqL[k] = std::polar(sMagL, finalPhaseL);
                    freqR[k] = std::polar(sMagR, finalPhaseR);

                    // Reconstrucción del espectro hermítico para transformada inversa C2R
                    if (k > 0 && k < NumBins) {
                        freqL[FftSize - k] = std::conj(freqL[k]);
                        freqR[FftSize - k] = std::conj(freqR[k]);
                    }
                }

                // 5. Transformada Inversa IFFT con ventana Hann de síntesis
                fftEngineL_.inverse(synthFrameL_.data(), true);
                fftEngineR_.inverse(synthFrameR_.data(), true);

                // 6. Superposición acumulativa en el buffer OLA
                for (size_t i = 0; i < FftSize; ++i) {
                    const size_t olaIdx = (olaReadPos_ + i) % OlaBufferSize;
                    olaBufferL_[olaIdx] += synthFrameL_[i] * colaNormalization;
                    olaBufferR_[olaIdx] += synthFrameR_[i] * colaNormalization;
                }
            }

            // Seguridad de estabilidad numérica (Anti-NaN/Inf)
            if (!std::isfinite(wetL)) wetL = 0.0f;
            if (!std::isfinite(wetR)) wetR = 0.0f;

            if (outL) outL[s] = sampleL * (1.0f - mix) + wetL * mix;
            if (outR) outR[s] = sampleR * (1.0f - mix) + wetR * mix;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case SmearTime: targetSmearTime_ = std::clamp(value, 0.05f, 10.0f); break;
            case PhaseDiffusion: targetPhaseDiff_ = std::clamp(value, 0.0f, 1.0f); break;
            case StereoDecorrel: targetDecorrel_ = std::clamp(value, 0.0f, 1.0f); break;
            case TiltDamping: targetTilt_ = std::clamp(value, -1.0f, 1.0f); break;
            case DryWet: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case SmearTime: return targetSmearTime_;
            case PhaseDiffusion: return targetPhaseDiff_;
            case StereoDecorrel: return targetDecorrel_;
            case TiltDamping: return targetTilt_;
            case DryWet: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::SpectralSmear; }
    const char* getName() const override { return "Spectral Smear"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 5.0); // 5 segundos de tail
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;

    FFTEngine fftEngineL_{};
    FFTEngine fftEngineR_{};

    std::vector<float> inputRingL_;
    std::vector<float> inputRingR_;
    std::vector<float> olaBufferL_;
    std::vector<float> olaBufferR_;
    std::vector<float> timeFrameL_;
    std::vector<float> timeFrameR_;
    std::vector<float> synthFrameL_;
    std::vector<float> synthFrameR_;

    std::vector<float> prevMagsL_;
    std::vector<float> prevMagsR_;
    std::vector<float> smoothMagsL_;
    std::vector<float> smoothMagsR_;
    std::vector<float> phaseAccumL_;
    std::vector<float> phaseAccumR_;

    size_t inputWritePos_{ 0 };
    size_t hopCounter_{ 0 };
    size_t olaReadPos_{ 0 };

    uint32_t rngStateL_{ 123456789u };
    uint32_t rngStateR_{ 987654321u };

    float targetSmearTime_{ 2.5f };
    float targetPhaseDiff_{ 0.7f };
    float targetDecorrel_{ 0.85f };
    float targetTilt_{ 0.0f };
    float targetMix_{ 0.5f };
};

inline AutoRegisterNode<SpectralSmearNode> registerSpectralSmear(NodeType::SpectralSmear, "spectral_smear", "Spectral");

} // namespace audio_graph
