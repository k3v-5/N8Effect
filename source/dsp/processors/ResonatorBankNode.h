#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/BiquadFilter.h"

namespace audio_graph {

/**
 * @brief Banco de 6 Resonadores Modales y Armónicos en paralelo con amortiguación y Tails (Reglas 5, 8, 12, 14, 18, 32, 46)
 */
class ResonatorBankNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        FundamentalFreq = 1,
        DecayTime = 2,
        HarmonicSpread = 3,
        Brightness = 4,
        DryWet = 5
    };

    static constexpr size_t NumResonators = 6;

    ResonatorBankNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { FundamentalFreq, "Freq", 220.0f, 20.0f, 2000.0f, true };
        params_[1] = { DecayTime, "Decay", 1.5f, 0.05f, 10.0f, true };
        params_[2] = { HarmonicSpread, "Spread", 0.0f, 0.0f, 1.0f, true };
        params_[3] = { Brightness, "Brightness", 0.0f, -1.0f, 1.0f, true };
        params_[4] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        reset();
        updateResonators();
    }

    void reset() override {
        for (size_t i = 0; i < NumResonators; ++i) {
            filtersL_[i].reset();
            filtersR_[i].reset();
        }
    }

    void process(ProcessContext& context) override {
        if (context.numSamples == 0 || context.numInputChannels == 0 || context.numOutputChannels == 0) return;

        updateResonators();

        const float dryWet = std::clamp(targetMix_, 0.0f, 1.0f);
        const float* inL = context.inputChannels[0];
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : context.inputChannels[0];

        float* outL = context.outputChannels[0];
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : context.outputChannels[0];

        constexpr float normGain = 0.45f;

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            const float dryL = inL[s];
            const float dryR = inR[s];

            float resSumL = 0.0f;
            float resSumR = 0.0f;

            for (size_t i = 0; i < NumResonators; ++i) {
                const float gain = gains_[i];
                resSumL += filtersL_[i].processSample(dryL) * gain;
                resSumR += filtersR_[i].processSample(dryR) * gain;
            }

            const float wetL = resSumL * normGain;
            const float wetR = resSumR * normGain;

            outL[s] = dryL * (1.0f - dryWet) + wetL * dryWet;
            outR[s] = dryR * (1.0f - dryWet) + wetR * dryWet;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case FundamentalFreq: targetFundamental_ = std::clamp(value, 20.0f, 2000.0f); break;
            case DecayTime: targetDecayTime_ = std::clamp(value, 0.05f, 10.0f); break;
            case HarmonicSpread: targetSpread_ = std::clamp(value, 0.0f, 1.0f); break;
            case Brightness: targetBrightness_ = std::clamp(value, -1.0f, 1.0f); break;
            case DryWet: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case FundamentalFreq: return targetFundamental_;
            case DecayTime: return targetDecayTime_;
            case HarmonicSpread: return targetSpread_;
            case Brightness: return targetBrightness_;
            case DryWet: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Resonator; }
    const char* getName() const override { return "Resonator Bank"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * targetDecayTime_);
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateResonators() noexcept {
        const float f0 = std::clamp(targetFundamental_, 20.0f, 2000.0f);
        const float decay = std::clamp(targetDecayTime_, 0.05f, 10.0f);
        const float spread = std::clamp(targetSpread_, 0.0f, 1.0f);
        const float bright = std::clamp(targetBrightness_, -1.0f, 1.0f);

        const double nyquist = spec_.sampleRate * 0.49;
        constexpr std::array<float, NumResonators> modalMultipliers = { 1.0f, 2.756f, 5.404f, 8.933f, 13.345f, 18.64f };

        for (size_t i = 0; i < NumResonators; ++i) {
            const float harmonicMul = static_cast<float>(i + 1);
            const float modalMul = modalMultipliers[i];
            const float mul = harmonicMul * (1.0f - spread) + modalMul * spread;

            float freq = f0 * mul;
            if (freq > nyquist) {
                freq = static_cast<float>(nyquist);
            }

            float q = std::clamp(static_cast<float>(3.14159265 * freq * decay * 0.1), 2.0f, 180.0f);

            filtersL_[i].setCoefficients(BiquadFilter::Type::Bandpass, spec_.sampleRate, freq, q);
            filtersR_[i].setCoefficients(BiquadFilter::Type::Bandpass, spec_.sampleRate, freq * 1.002f, q);

            const float harmonicRatio = static_cast<float>(i) / static_cast<float>(NumResonators - 1);
            const float tiltDb = bright * harmonicRatio * 18.0f;
            gains_[i] = std::pow(10.0f, tiltDb * 0.05f);
        }
    }

    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;

    std::array<BiquadFilter, NumResonators> filtersL_{};
    std::array<BiquadFilter, NumResonators> filtersR_{};
    std::array<float, NumResonators> gains_{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

    float targetFundamental_{ 220.0f };
    float targetDecayTime_{ 1.5f };
    float targetSpread_{ 0.0f };
    float targetBrightness_{ 0.0f };
    float targetMix_{ 0.5f };
};

inline AutoRegisterNode<ResonatorBankNode> registerResonatorBank(NodeType::Resonator, "resonator_bank", "Filter");

} // namespace audio_graph
