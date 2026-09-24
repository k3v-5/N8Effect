#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DelayLine.h"
#include "../core/BiquadFilter.h"

namespace audio_graph {

/**
 * @brief Reverb Algorítmica FDN (Feedback Delay Network) de 4 líneas con matriz ortogonal y Tails (Reglas 5, 8, 14, 15, 18, 34)
 */
class ReverbNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        RoomSize = 1,
        DecayTime = 2,
        Damping = 3,
        Predelay = 4,
        DryWet = 5
    };

    ReverbNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { RoomSize, "Size", 0.7f, 0.1f, 1.0f, true };
        params_[1] = { DecayTime, "Decay", 2.5f, 0.2f, 10.0f, true };
        params_[2] = { Damping, "Damping", 5000.0f, 500.0f, 20000.0f, true };
        params_[3] = { Predelay, "Predelay", 20.0f, 0.0f, 100.0f, true };
        params_[4] = { DryWet, "Mix", 0.4f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;

        // Longitudes de delay base mutuamente primas para máxima densidad modal (Regla 34)
        baseLengthsMs_ = { 29.7f, 37.1f, 41.3f, 47.9f };

        predelayLine_.prepare(static_cast<size_t>(spec.sampleRate * 0.2)); // Hasta 200ms

        for (size_t i = 0; i < 4; ++i) {
            size_t maxDelay = static_cast<size_t>(spec.sampleRate * 0.5);
            fdnLines_[i].prepare(maxDelay);
            dampFilters_[i].reset();
        }

        reset();
        updateFilters();
    }

    void reset() override {
        predelayLine_.reset();
        for (size_t i = 0; i < 4; ++i) {
            fdnLines_[i].reset();
            dampFilters_[i].reset();
        }
    }

    void process(ProcessContext& context) override {
        updateFilters();

        const float sr = static_cast<float>(spec_.sampleRate);
        const float predelaySamples = (targetPredelay_ * 0.001f) * sr;
        const float mix = targetMix_;

        // Coeficiente de feedback derivado del tiempo de decaimiento T60 (RT60)
        // g = 10^(-3 * delayTime / T60)
        const float t60 = std::max(0.1f, targetDecay_);

        std::array<float, 4> delaySamples;
        std::array<float, 4> feedbackGains;
        for (size_t i = 0; i < 4; ++i) {
            float dMs = baseLengthsMs_[i] * targetSize_;
            delaySamples[i] = (dMs * 0.001f) * sr;
            feedbackGains[i] = std::pow(10.0f, -3.0f * (dMs * 0.001f) / t60);
        }

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            float inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0][s] : 0.0f;
            float inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1][s] : inL;
            float monoIn = 0.5f * (inL + inR);

            // 1. Predelay
            predelayLine_.write(monoIn);
            float predelayed = predelayLine_.readLinear(predelaySamples);

            // 2. Lectura y amortiguación de las 4 líneas FDN
            std::array<float, 4> lineOutputs;
            for (size_t i = 0; i < 4; ++i) {
                float readVal = fdnLines_[i].readCubic(delaySamples[i]);
                lineOutputs[i] = dampFilters_[i].processSample(readVal);
            }

            // 3. Matriz de mezcla ortogonal de Householder (difusión sin pérdidas): H = I - 2/N * (1 1 1 1)^T * (1 1 1 1)
            // Para N=4, H * y = y - 0.5 * sum(y)
            float sumY = lineOutputs[0] + lineOutputs[1] + lineOutputs[2] + lineOutputs[3];
            float halfSum = 0.5f * sumY;

            std::array<float, 4> diffused;
            for (size_t i = 0; i < 4; ++i) {
                diffused[i] = lineOutputs[i] - halfSum;
                // Escribir de vuelta con feedback y entrada predelay
                fdnLines_[i].write(predelayed + diffused[i] * feedbackGains[i]);
            }

            // 4. Decodificación estéreo
            float wetL = 0.5f * (diffused[0] + diffused[2]);
            float wetR = 0.5f * (diffused[1] + diffused[3]);

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
            case RoomSize: targetSize_ = std::clamp(value, 0.1f, 1.0f); break;
            case DecayTime: targetDecay_ = std::clamp(value, 0.2f, 10.0f); break;
            case Damping: targetDamping_ = std::clamp(value, 500.0f, 20000.0f); break;
            case Predelay: targetPredelay_ = std::clamp(value, 0.0f, 100.0f); break;
            case DryWet: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case RoomSize: return targetSize_;
            case DecayTime: return targetDecay_;
            case Damping: return targetDamping_;
            case Predelay: return targetPredelay_;
            case DryWet: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Reverb; }
    const char* getName() const override { return "Algorithmic Reverb"; }
    bool supportsTail() const override { return true; } // Soporte explícito de tails (Regla 18)
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 10.0); // 10 segundos de tail máximo
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateFilters() noexcept {
        for (size_t i = 0; i < 4; ++i) {
            dampFilters_[i].setCoefficients(BiquadFilter::Type::Lowpass, spec_.sampleRate, targetDamping_, 0.707f);
        }
    }

    ProcessSpec spec_;
    std::array<float, 4> baseLengthsMs_{ 29.7f, 37.1f, 41.3f, 47.9f };
    DelayLine predelayLine_;
    std::array<DelayLine, 4> fdnLines_;
    std::array<BiquadFilter, 4> dampFilters_;

    float targetSize_{ 0.7f };
    float targetDecay_{ 2.5f };
    float targetDamping_{ 5000.0f };
    float targetPredelay_{ 20.0f };
    float targetMix_{ 0.4f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
};

inline AutoRegisterNode<ReverbNode> registerReverb(NodeType::Reverb, "reverb", "Space");

} // namespace audio_graph
