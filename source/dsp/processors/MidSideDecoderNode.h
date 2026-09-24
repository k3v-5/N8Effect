#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include <numbers>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/BiquadFilter.h"
#include "../core/DenormalGuards.h"

namespace audio_graph {

/**
 * @brief Decodificador Mid/Side a Estéreo con Mono Bass Maker (Reglas 5, 8, 13, 16, 34, 46, 47).
 * Reconstruye la imagen estéreo L/R a partir de componentes Mid/Side, proporcionando control
 * continuo de ancho estéreo (Width) y filtro Mono Bass Maker (filtro pasa-altos Butterworth en Side)
 * para garantizar subgraves monofónicos sin desfases destructivos.
 */
class MidSideDecoderNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Width = 1,
        MonoBassFreq = 2,
        MonoBassActive = 3,
        OutputGain = 4
    };

    MidSideDecoderNode() {
        pins_[0] = { 1, "Mid/Side In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Stereo Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Width, "Width", 1.0f, 0.0f, 2.0f, true };
        params_[1] = { MonoBassFreq, "Mono Bass Freq", 120.0f, 20.0f, 400.0f, true };
        params_[2] = { MonoBassActive, "Mono Bass Active", 1.0f, 0.0f, 1.0f, false };
        params_[3] = { OutputGain, "Gain", 1.0f, 0.0f, 2.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        updateFilter();
        reset();
        currentWidth_ = targetWidth_;
        currentGain_ = targetGain_;
    }

    void reset() override {
        sideHighpass_.reset();
        currentWidth_ = targetWidth_;
        currentGain_ = targetGain_;
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Reglas 34 y 47
        const float alpha = 0.005f;        // Anti-click (Regla 35)
        constexpr float invSqrt2 = 0.70710678f;

        const bool monoBassEnabled = (targetMonoBassActive_ > 0.5f);

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            currentWidth_ += alpha * (targetWidth_ - currentWidth_);
            currentGain_ += alpha * (targetGain_ - currentGain_);

            const float mid = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr)
                ? context.inputChannels[0][s] : 0.0f;
            const float sideRaw = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr)
                ? context.inputChannels[1][s] : 0.0f;

            // 1. Filtrado Mono Bass Maker en el canal Side si está activo
            const float side = monoBassEnabled ? sideHighpass_.processSample(sideRaw) : sideRaw;

            // 2. Ancho estéreo continuo: S' = S * Width
            const float sideScaled = side * currentWidth_;

            // 3. Matriz inversa: L = (M + S') * 0.7071, R = (M - S') * 0.7071
            const float outL = (mid + sideScaled) * invSqrt2 * currentGain_;
            const float outR = (mid - sideScaled) * invSqrt2 * currentGain_;

            if (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) {
                context.outputChannels[0][s] = outL;
            }
            if (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) {
                context.outputChannels[1][s] = outR;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Width:          targetWidth_ = std::clamp(value, 0.0f, 2.0f); break;
            case MonoBassFreq:   targetMonoBassFreq_ = std::clamp(value, 20.0f, 400.0f); updateFilter(); break;
            case MonoBassActive: targetMonoBassActive_ = value; break;
            case OutputGain:     targetGain_ = std::clamp(value, 0.0f, 2.0f); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Width:          return targetWidth_;
            case MonoBassFreq:   return targetMonoBassFreq_;
            case MonoBassActive: return targetMonoBassActive_;
            case OutputGain:     return targetGain_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::MidSideDecoder; }
    const char* getName() const override { return "M/S Decoder"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateFilter() noexcept {
        if (spec_.sampleRate > 0.0) {
            // Filtro pasa-altos Butterworth Q=0.7071 en el canal Side
            sideHighpass_.setCoefficients(BiquadFilter::Type::Highpass, spec_.sampleRate, targetMonoBassFreq_, 0.7071f);
        }
    }

    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    BiquadFilter sideHighpass_;

    float targetWidth_{ 1.0f };
    float currentWidth_{ 1.0f };
    float targetMonoBassFreq_{ 120.0f };
    float targetMonoBassActive_{ 1.0f };
    float targetGain_{ 1.0f };
    float currentGain_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 4> params_;
};

inline AutoRegisterNode<MidSideDecoderNode> registerMidSideDecoderNode(
    NodeType::MidSideDecoder, "M/S Decoder", "Spatial"
);

} // namespace audio_graph
