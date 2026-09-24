#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/FFTEngine.h"

namespace audio_graph {

/**
 * @brief Procesador Espectral en dominio de frecuencia con compuerta y tilt espectral (Reglas 5, 8, 19, 34)
 */
class SpectralProcessorNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        SpectralGateDb = 1,
        SpectralTilt = 2,
        DryWet = 3
    };

    SpectralProcessorNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { SpectralGateDb, "Gate Threshold", -50.0f, -80.0f, 0.0f, true };
        params_[1] = { SpectralTilt, "Tilt", 0.0f, -6.0f, 6.0f, true };
        params_[2] = { DryWet, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        fftEngineL_.prepare(512, 128);
        fftEngineR_.prepare(512, 128);

        timeInL_.assign(512, 0.0f);
        timeInR_.assign(512, 0.0f);
        timeOutL_.assign(512, 0.0f);
        timeOutR_.assign(512, 0.0f);
        bufferPos_ = 0;
        reset();
    }

    void reset() override {
        fftEngineL_.reset();
        fftEngineR_.reset();
        std::fill(timeInL_.begin(), timeInL_.end(), 0.0f);
        std::fill(timeInR_.begin(), timeInR_.end(), 0.0f);
        std::fill(timeOutL_.begin(), timeOutL_.end(), 0.0f);
        std::fill(timeOutR_.begin(), timeOutR_.end(), 0.0f);
        bufferPos_ = 0;
    }

    void process(ProcessContext& context) override {
        const float gateThresholdLinear = std::pow(10.0f, targetGateDb_ * 0.05f);
        const float tilt = targetTilt_;
        const float mix = targetMix_;
        const size_t fftSize = fftEngineL_.getFFTSize();

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            float inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0][s] : 0.0f;
            float inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1][s] : inL;

            timeInL_[bufferPos_] = inL;
            timeInR_[bufferPos_] = inR;

            float wetL = timeOutL_[bufferPos_];
            float wetR = timeOutR_[bufferPos_];

            bufferPos_++;
            if (bufferPos_ >= fftSize) {
                bufferPos_ = 0;

                // 1. Análisis hacia frecuencia
                fftEngineL_.forward(timeInL_.data());
                fftEngineR_.forward(timeInR_.data());

                auto& freqL = fftEngineL_.getFrequencyBuffer();
                auto& freqR = fftEngineR_.getFrequencyBuffer();

                // 2. Procesamiento en dominio de frecuencia (Gating espectral y Tilt)
                const size_t numBins = fftSize / 2;
                for (size_t k = 0; k < numBins; ++k) {
                    float magL = std::abs(freqL[k]);
                    float magR = std::abs(freqR[k]);

                    // Ponderación de tilt por frecuencia
                    float normFreq = static_cast<float>(k) / static_cast<float>(numBins);
                    float tiltGain = std::pow(10.0f, (tilt * (normFreq - 0.5f)) * 0.05f);

                    // Puerta espectral (reduce el ruido eliminando bins de baja energía)
                    if (magL < gateThresholdLinear) freqL[k] *= 0.05f;
                    else freqL[k] *= tiltGain;

                    if (magR < gateThresholdLinear) freqR[k] *= 0.05f;
                    else freqR[k] *= tiltGain;

                    // Simetría conjugada para la mitad superior
                    if (k > 0 && k < numBins) {
                        freqL[fftSize - k] = std::conj(freqL[k]);
                        freqR[fftSize - k] = std::conj(freqR[k]);
                    }
                }

                // 3. Resíntesis inversa a tiempo
                fftEngineL_.inverse(timeOutL_.data());
                fftEngineR_.inverse(timeOutR_.data());
            }

            if (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) {
                context.outputChannels[0][s] = inL * (1.0f - mix) + wetL * mix;
            }
            if (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) {
                context.outputChannels[1][s] = inR * (1.0f - mix) + wetR * mix;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case SpectralGateDb: targetGateDb_ = std::clamp(value, -80.0f, 0.0f); break;
            case SpectralTilt: targetTilt_ = std::clamp(value, -6.0f, 6.0f); break;
            case DryWet: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case SpectralGateDb: return targetGateDb_;
            case SpectralTilt: return targetTilt_;
            case DryWet: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Custom; }
    const char* getName() const override { return "Spectral Processor"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    FFTEngine fftEngineL_;
    FFTEngine fftEngineR_;

    std::vector<float> timeInL_;
    std::vector<float> timeInR_;
    std::vector<float> timeOutL_;
    std::vector<float> timeOutR_;
    size_t bufferPos_{ 0 };

    float targetGateDb_{ -50.0f };
    float targetTilt_{ 0.0f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 3> params_;
};

inline AutoRegisterNode<SpectralProcessorNode> registerSpectralProcessor(NodeType::Custom, "spectral_processor", "Spectral");

} // namespace audio_graph
