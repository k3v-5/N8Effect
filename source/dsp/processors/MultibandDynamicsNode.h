#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/LinkwitzRileyFilter.h"
#include "../core/EnvelopeDetector.h"

namespace audio_graph {

/**
 * @brief Procesador de Dinámica Multibanda (3 bandas) estilo OTT con compresión Upward y Downward (Reglas 5, 8, 32, 33, 34, 46)
 */
class MultibandDynamicsNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        CrossoverLowMid = 1,
        CrossoverMidHigh = 2,
        LowDynamics = 3,
        MidDynamics = 4,
        HighDynamics = 5,
        DryWet = 6
    };

    MultibandDynamicsNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { CrossoverLowMid, "Xover Low-Mid", 250.0f, 60.0f, 1000.0f, true };
        params_[1] = { CrossoverMidHigh, "Xover Mid-High", 2500.0f, 800.0f, 12000.0f, true };
        params_[2] = { LowDynamics, "Low Dyn", 1.0f, 0.0f, 2.0f, true };
        params_[3] = { MidDynamics, "Mid Dyn", 1.0f, 0.0f, 2.0f, true };
        params_[4] = { HighDynamics, "High Dyn", 1.0f, 0.0f, 2.0f, true };
        params_[5] = { DryWet, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        crossover_.prepare(spec.sampleRate);
        updateCrossovers();

        for (int ch = 0; ch < 2; ++ch) {
            envLow_[ch].prepare(spec.sampleRate);
            envLow_[ch].setAttackMs(20.0f);
            envLow_[ch].setReleaseMs(150.0f);

            envMid_[ch].prepare(spec.sampleRate);
            envMid_[ch].setAttackMs(10.0f);
            envMid_[ch].setReleaseMs(100.0f);

            envHigh_[ch].prepare(spec.sampleRate);
            envHigh_[ch].setAttackMs(5.0f);
            envHigh_[ch].setReleaseMs(60.0f);
        }
    }

    void reset() override {
        crossover_.reset();
        for (int ch = 0; ch < 2; ++ch) {
            envLow_[ch].reset();
            envMid_[ch].reset();
            envHigh_[ch].reset();
        }
    }

    void process(ProcessContext& context) override {
        if (context.numSamples == 0 || context.numInputChannels == 0 || context.numOutputChannels == 0) return;

        updateCrossovers();

        const float lowDyn = std::clamp(targetLowDynamics_, 0.0f, 2.0f);
        const float midDyn = std::clamp(targetMidDynamics_, 0.0f, 2.0f);
        const float highDyn = std::clamp(targetHighDynamics_, 0.0f, 2.0f);
        const float dryWet = std::clamp(targetMix_, 0.0f, 1.0f);

        const float* inL = context.inputChannels[0];
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : context.inputChannels[0];

        float* outL = context.outputChannels[0];
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : context.outputChannels[0];

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            const float drySampleL = inL[s];
            const float drySampleR = inR[s];

            const auto bandsL = crossover_.processSample(0, drySampleL);
            const auto bandsR = crossover_.processSample(1, drySampleR);

            const float procLowL = processBandDynamics(bandsL.low, envLow_[0], lowDyn, -20.0f, -40.0f);
            const float procLowR = processBandDynamics(bandsR.low, envLow_[1], lowDyn, -20.0f, -40.0f);

            const float procMidL = processBandDynamics(bandsL.mid, envMid_[0], midDyn, -18.0f, -36.0f);
            const float procMidR = processBandDynamics(bandsR.mid, envMid_[1], midDyn, -18.0f, -36.0f);

            const float procHighL = processBandDynamics(bandsL.high, envHigh_[0], highDyn, -16.0f, -34.0f);
            const float procHighR = processBandDynamics(bandsR.high, envHigh_[1], highDyn, -16.0f, -34.0f);

            const float wetL = procLowL + procMidL + procHighL;
            const float wetR = procLowR + procMidR + procHighR;

            outL[s] = drySampleL * (1.0f - dryWet) + wetL * dryWet;
            outR[s] = drySampleR * (1.0f - dryWet) + wetR * dryWet;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case CrossoverLowMid: targetLowMid_ = std::clamp(value, 60.0f, 1000.0f); break;
            case CrossoverMidHigh: targetMidHigh_ = std::clamp(value, 800.0f, 12000.0f); break;
            case LowDynamics: targetLowDynamics_ = std::clamp(value, 0.0f, 2.0f); break;
            case MidDynamics: targetMidDynamics_ = std::clamp(value, 0.0f, 2.0f); break;
            case HighDynamics: targetHighDynamics_ = std::clamp(value, 0.0f, 2.0f); break;
            case DryWet: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case CrossoverLowMid: return targetLowMid_;
            case CrossoverMidHigh: return targetMidHigh_;
            case LowDynamics: return targetLowDynamics_;
            case MidDynamics: return targetMidDynamics_;
            case HighDynamics: return targetHighDynamics_;
            case DryWet: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Multiband; }
    const char* getName() const override { return "Multiband Dynamics"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateCrossovers() noexcept {
        crossover_.setCrossoverFrequencies(targetLowMid_, targetMidHigh_);
    }

    inline float processBandDynamics(float sample, EnvelopeDetector& detector, float dynamicsAmount, float downThreshDb, float upThreshDb) noexcept {
        const float envLinear = detector.processSample(sample);
        const float envDb = EnvelopeDetector::linearToDb(envLinear);

        float gainDb = 0.0f;

        if (envDb > downThreshDb) {
            const float overDb = envDb - downThreshDb;
            const float ratio = 1.0f + 3.0f * dynamicsAmount;
            const float compressedOver = overDb / ratio;
            gainDb -= (overDb - compressedOver);
        } else if (envDb < upThreshDb && envDb > -70.0f) {
            const float underDb = upThreshDb - envDb;
            const float boostFactor = std::clamp(underDb * 0.4f * dynamicsAmount, 0.0f, 15.0f);
            gainDb += boostFactor;
        }

        const float linearGain = EnvelopeDetector::dbToLinear(gainDb);
        return sample * linearGain;
    }

    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;

    LinkwitzRileyCrossover3Way crossover_{};
    std::array<EnvelopeDetector, 2> envLow_{};
    std::array<EnvelopeDetector, 2> envMid_{};
    std::array<EnvelopeDetector, 2> envHigh_{};

    float targetLowMid_{ 250.0f };
    float targetMidHigh_{ 2500.0f };
    float targetLowDynamics_{ 1.0f };
    float targetMidDynamics_{ 1.0f };
    float targetHighDynamics_{ 1.0f };
    float targetMix_{ 1.0f };
};

inline AutoRegisterNode<MultibandDynamicsNode> registerMultibandDynamics(NodeType::Multiband, "multiband_dynamics", "Dynamics");

} // namespace audio_graph
