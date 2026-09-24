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

namespace audio_graph {

/**
 * @brief Procesador Espectral Avanzado: Freeze (congelamiento de magnitudes) y Smear (difusión temporal) (Reglas 5, 8, 14, 18, 19, 32, 46)
 */
class SpectralFreezeNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Freeze = 1,
        Smear = 2,
        Brightness = 3,
        DryWet = 4
    };

    SpectralFreezeNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Freeze, "Freeze", 0.0f, 0.0f, 1.0f, false };
        params_[1] = { Smear, "Smear", 0.0f, 0.0f, 0.98f, true };
        params_[2] = { Brightness, "Brightness", 0.0f, -1.0f, 1.0f, true };
        params_[3] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };

        timeInL_.assign(512, 0.0f);
        timeInR_.assign(512, 0.0f);
        timeOutL_.assign(512, 0.0f);
        timeOutR_.assign(512, 0.0f);
        prevMagsL_.assign(256, 0.0f);
        prevMagsR_.assign(256, 0.0f);
        frozenMagsL_.assign(256, 0.0f);
        frozenMagsR_.assign(256, 0.0f);
        phaseAccumL_.assign(256, 0.0f);
        phaseAccumR_.assign(256, 0.0f);
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        constexpr size_t fftSize = 512;
        fftEngineL_.prepare(fftSize, fftSize / 4);
        fftEngineR_.prepare(fftSize, fftSize / 4);

        timeInL_.assign(fftSize, 0.0f);
        timeInR_.assign(fftSize, 0.0f);
        timeOutL_.assign(fftSize, 0.0f);
        timeOutR_.assign(fftSize, 0.0f);

        const size_t numBins = fftSize / 2;
        prevMagsL_.assign(numBins, 0.0f);
        prevMagsR_.assign(numBins, 0.0f);
        frozenMagsL_.assign(numBins, 0.0f);
        frozenMagsR_.assign(numBins, 0.0f);
        phaseAccumL_.assign(numBins, 0.0f);
        phaseAccumR_.assign(numBins, 0.0f);

        bufferPos_ = 0;
        wasFrozen_ = false;
        reset();
    }

    void reset() override {
        fftEngineL_.reset();
        fftEngineR_.reset();
        std::fill(timeInL_.begin(), timeInL_.end(), 0.0f);
        std::fill(timeInR_.begin(), timeInR_.end(), 0.0f);
        std::fill(timeOutL_.begin(), timeOutL_.end(), 0.0f);
        std::fill(timeOutR_.begin(), timeOutR_.end(), 0.0f);
        std::fill(prevMagsL_.begin(), prevMagsL_.end(), 0.0f);
        std::fill(prevMagsR_.begin(), prevMagsR_.end(), 0.0f);
        std::fill(frozenMagsL_.begin(), frozenMagsL_.end(), 0.0f);
        std::fill(frozenMagsR_.begin(), frozenMagsR_.end(), 0.0f);
        std::fill(phaseAccumL_.begin(), phaseAccumL_.end(), 0.0f);
        std::fill(phaseAccumR_.begin(), phaseAccumR_.end(), 0.0f);
        bufferPos_ = 0;
        wasFrozen_ = false;
    }

    void process(ProcessContext& context) override {
        if (context.numSamples == 0 || context.numInputChannels == 0 || context.numOutputChannels == 0) return;

        const bool isFrozen = (targetFreeze_ >= 0.5f);
        const float smear = std::clamp(targetSmear_, 0.0f, 0.98f);
        const float brightness = std::clamp(targetBrightness_, -1.0f, 1.0f);
        const float dryWet = std::clamp(targetMix_, 0.0f, 1.0f);

        const size_t fftSize = fftEngineL_.getFFTSize();
        const size_t numBins = fftSize / 2;

        const float* inL = context.inputChannels[0];
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : context.inputChannels[0];

        float* outL = context.outputChannels[0];
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : context.outputChannels[0];

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            const float drySampleL = inL[s];
            const float drySampleR = inR[s];

            timeInL_[bufferPos_] = drySampleL;
            timeInR_[bufferPos_] = drySampleR;

            const float wetL = timeOutL_[bufferPos_];
            const float wetR = timeOutR_[bufferPos_];

            bufferPos_++;
            if (bufferPos_ >= fftSize) {
                bufferPos_ = 0;

                fftEngineL_.forward(timeInL_.data());
                fftEngineR_.forward(timeInR_.data());

                auto& freqL = fftEngineL_.getFrequencyBuffer();
                auto& freqR = fftEngineR_.getFrequencyBuffer();

                for (size_t k = 0; k < numBins; ++k) {
                    float magL = std::abs(freqL[k]);
                    float magR = std::abs(freqR[k]);

                    if (isFrozen) {
                        if (!wasFrozen_) {
                            frozenMagsL_[k] = magL;
                            frozenMagsR_[k] = magR;
                        }
                        magL = frozenMagsL_[k];
                        magR = frozenMagsR_[k];

                        const float binFreq = static_cast<float>(k) * 0.13f + 0.07f;
                        phaseAccumL_[k] += binFreq;
                        phaseAccumR_[k] += binFreq * 1.01f;
                    } else {
                        frozenMagsL_[k] = magL;
                        frozenMagsR_[k] = magR;

                        if (smear > 0.0f) {
                            magL = (1.0f - smear) * magL + smear * prevMagsL_[k];
                            magR = (1.0f - smear) * magR + smear * prevMagsR_[k];
                            prevMagsL_[k] = magL;
                            prevMagsR_[k] = magR;
                        }

                        phaseAccumL_[k] = std::arg(freqL[k]);
                        phaseAccumR_[k] = std::arg(freqR[k]);
                    }

                    const float normFreq = static_cast<float>(k) / static_cast<float>(numBins);
                    const float tiltGain = std::pow(10.0f, (brightness * (normFreq - 0.5f) * 18.0f) * 0.05f);

                    magL *= tiltGain;
                    magR *= tiltGain;

                    freqL[k] = std::polar(magL, phaseAccumL_[k]);
                    freqR[k] = std::polar(magR, phaseAccumR_[k]);

                    if (k > 0 && k < numBins) {
                        freqL[fftSize - k] = std::conj(freqL[k]);
                        freqR[fftSize - k] = std::conj(freqR[k]);
                    }
                }

                wasFrozen_ = isFrozen;

                fftEngineL_.inverse(timeOutL_.data());
                fftEngineR_.inverse(timeOutR_.data());
            }

            outL[s] = drySampleL * (1.0f - dryWet) + wetL * dryWet;
            outR[s] = drySampleR * (1.0f - dryWet) + wetR * dryWet;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Freeze: targetFreeze_ = (value >= 0.5f) ? 1.0f : 0.0f; break;
            case Smear: targetSmear_ = std::clamp(value, 0.0f, 0.98f); break;
            case Brightness: targetBrightness_ = std::clamp(value, -1.0f, 1.0f); break;
            case DryWet: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Freeze: return targetFreeze_;
            case Smear: return targetSmear_;
            case Brightness: return targetBrightness_;
            case DryWet: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Spectral; }
    const char* getName() const override { return "Spectral Freeze"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 3.0);
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 4> params_;

    FFTEngine fftEngineL_{};
    FFTEngine fftEngineR_{};

    std::vector<float> timeInL_;
    std::vector<float> timeInR_;
    std::vector<float> timeOutL_;
    std::vector<float> timeOutR_;

    std::vector<float> prevMagsL_;
    std::vector<float> prevMagsR_;
    std::vector<float> frozenMagsL_;
    std::vector<float> frozenMagsR_;
    std::vector<float> phaseAccumL_;
    std::vector<float> phaseAccumR_;

    size_t bufferPos_{ 0 };
    bool wasFrozen_{ false };

    float targetFreeze_{ 0.0f };
    float targetSmear_{ 0.0f };
    float targetBrightness_{ 0.0f };
    float targetMix_{ 0.5f };
};

inline AutoRegisterNode<SpectralFreezeNode> registerSpectralFreeze(NodeType::Spectral, "spectral_freeze", "Spectral");

} // namespace audio_graph
