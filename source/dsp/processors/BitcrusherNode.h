#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/BiquadFilter.h"
#include "../core/EnvelopeDetector.h"
#include "../core/DenormalGuards.h"
#include "../core/FastMath.h"

namespace audio_graph {

/**
 * @brief Procesador Bitcrusher y Reductor de Sample Rate / Lo-Fi Vintage (Reglas 5, 8, 9, 14, 34, 46, 47)
 * Permite degradación digital de alta fidelidad: reducción de profundidad de bits (2 a 16 bits),
 * diezma de frecuencia de muestreo (Sample Rate Decimator), fluctuación estocástica de reloj (Jitter),
 * saturación de entrada (Drive) y filtro suavizador post-aliasing.
 */
class BitcrusherNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        BitDepth = 1,       // 2.0 a 16.0 bits (Resolución de cuantización)
        Downsample = 2,     // 1.0 a 50.0 factor (Diezmador de sample rate)
        Jitter = 3,         // 0.0 a 1.0 (Inestabilidad de reloj del convertidor DAC)
        AntiAliasing = 4,   // 0.0 a 1.0 (Filtro pasa-bajos post-cuantización)
        Drive = 5,          // 0.0 a 24.0 dB (Empuje armónico previo a cuantización)
        Mix = 6             // 0.0 a 1.0 (Mezcla Dry / Wet)
    };

    BitcrusherNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { BitDepth, "Bit Depth", 8.0f, 2.0f, 16.0f, true };
        params_[1] = { Downsample, "Downsample", 4.0f, 1.0f, 50.0f, true };
        params_[2] = { Jitter, "Jitter", 0.05f, 0.0f, 1.0f, true };
        params_[3] = { AntiAliasing, "Post Filter", 0.0f, 0.0f, 1.0f, true };
        params_[4] = { Drive, "Drive", 0.0f, 0.0f, 24.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        phaseAcc_ = 0.0f;
        heldSampleL_ = 0.0f;
        heldSampleR_ = 0.0f;
        rngState_ = 0x12345678u;

        updateFilter();
    }

    void reset() override {
        phaseAcc_ = 0.0f;
        heldSampleL_ = 0.0f;
        heldSampleR_ = 0.0f;
        postFilter_[0].reset();
        postFilter_[1].reset();
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const float driveGain = EnvelopeDetector::dbToLinear(targetDrive_);
        const float bits = std::clamp(targetBitDepth_, 2.0f, 16.0f);
        const float numLevels = std::pow(2.0f, bits - 1.0f);
        const float invLevels = 1.0f / numLevels;
        const float downsampleFactor = std::max(1.0f, targetDownsample_);
        const float jitterAmount = targetJitter_;
        const float mix = targetMix_;

        const bool useFilter = targetAntiAliasing_ > 0.02f;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0]) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1]) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0]) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1]) ? context.outputChannels[1] : nullptr;

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            const float rawInL = inL ? inL[s] : 0.0f;
            const float rawInR = inR ? inR[s] : 0.0f;

            // 1. Acumulador de fase para diezmado de sample rate (Sample-and-Hold)
            phaseAcc_ += 1.0f;

            // Jitter estocástico sobre el umbral de disparo
            float jitterOffset = 0.0f;
            if (jitterAmount > 0.001f) {
                rngState_ = rngState_ * 1664525u + 1013904223u;
                jitterOffset = (static_cast<float>(rngState_ & 0xFFFF) / 65535.0f - 0.5f) * jitterAmount * 2.0f;
            }

            if (phaseAcc_ >= (downsampleFactor + jitterOffset)) {
                phaseAcc_ -= downsampleFactor;
                if (phaseAcc_ < 0.0f) phaseAcc_ = 0.0f;

                // 2. Saturación suave de entrada (Drive)
                float drivenL = rawInL * driveGain;
                float drivenR = rawInR * driveGain;
                if (targetDrive_ > 0.1f) {
                    drivenL = FastMath::fastTanh(drivenL);
                    drivenR = FastMath::fastTanh(drivenR);
                }

                // 3. Cuantización de profundidad de bits
                // y = round(x * (2^(B-1))) / (2^(B-1))
                drivenL = std::clamp(drivenL, -1.0f, 1.0f);
                drivenR = std::clamp(drivenR, -1.0f, 1.0f);

                heldSampleL_ = std::round(drivenL * numLevels) * invLevels;
                heldSampleR_ = std::round(drivenR * numLevels) * invLevels;
            }

            float wetL = heldSampleL_;
            float wetR = heldSampleR_;

            // 4. Filtro pasa-bajos opcional post-aliasing
            if (useFilter) {
                wetL = postFilter_[0].processSample(wetL);
                wetR = postFilter_[1].processSample(wetR);
            }

            // 5. Mezcla Dry/Wet
            if (outL) outL[s] = (1.0f - mix) * rawInL + mix * wetL;
            if (outR) outR[s] = (1.0f - mix) * rawInR + mix * wetR;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case BitDepth: targetBitDepth_ = std::clamp(value, 2.0f, 16.0f); break;
            case Downsample: targetDownsample_ = std::clamp(value, 1.0f, 50.0f); break;
            case Jitter: targetJitter_ = std::clamp(value, 0.0f, 1.0f); break;
            case AntiAliasing: {
                targetAntiAliasing_ = std::clamp(value, 0.0f, 1.0f);
                updateFilter();
                break;
            }
            case Drive: targetDrive_ = std::clamp(value, 0.0f, 24.0f); break;
            case Mix: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case BitDepth: return targetBitDepth_;
            case Downsample: return targetDownsample_;
            case Jitter: return targetJitter_;
            case AntiAliasing: return targetAntiAliasing_;
            case Drive: return targetDrive_;
            case Mix: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Bitcrusher; }
    const char* getName() const override { return "Bitcrusher"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateFilter() noexcept {
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        // Filtro pasa-bajos suave: mapea AntiAliasing 0..1 a frecuencia de 18kHz bajando a 1.5kHz
        const float cutoff = 18000.0f * std::pow(1500.0f / 18000.0f, targetAntiAliasing_);
        for (auto& f : postFilter_) {
            f.setCoefficients(BiquadFilter::Type::Lowpass, sr, cutoff, 0.707f);
        }
    }

    ProcessSpec spec_;
    float phaseAcc_{ 0.0f };
    float heldSampleL_{ 0.0f };
    float heldSampleR_{ 0.0f };
    uint32_t rngState_{ 0x12345678u };

    std::array<BiquadFilter, 2> postFilter_;

    float targetBitDepth_{ 8.0f };
    float targetDownsample_{ 4.0f };
    float targetJitter_{ 0.05f };
    float targetAntiAliasing_{ 0.0f };
    float targetDrive_{ 0.0f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<BitcrusherNode> registerBitcrusher(NodeType::Bitcrusher, "bitcrusher", "Distortion");

} // namespace audio_graph
