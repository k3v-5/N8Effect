#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/EnvelopeDetector.h"

namespace audio_graph {

/**
 * @brief Nodo de Compresor VCA profesional con Soft-Knee (Reglas 5, 8, 11, 14, 34, 35)
 */
class CompressorNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Threshold = 1,
        Ratio = 2,
        Attack = 3,
        Release = 4,
        Makeup = 5,
        Knee = 6
    };

    CompressorNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { SidechainPinId, "Sidechain", PinType::AudioInput, PinDataType::AudioStereo };

        params_[0] = { Threshold, "Threshold", -18.0f, -60.0f, 0.0f, true };
        params_[1] = { Ratio, "Ratio", 4.0f, 1.0f, 20.0f, true };
        params_[2] = { Attack, "Attack", 15.0f, 0.1f, 200.0f, true };
        params_[3] = { Release, "Release", 100.0f, 5.0f, 1000.0f, true };
        params_[4] = { Makeup, "Makeup", 0.0f, 0.0f, 30.0f, true };
        params_[5] = { Knee, "Knee", 4.0f, 0.0f, 20.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        detector_.prepare(spec.sampleRate);
        detector_.setAttackMs(targetAttack_);
        detector_.setReleaseMs(targetRelease_);
        currentGainReduction_ = 1.0f;
    }

    void reset() override {
        detector_.reset();
        currentGainReduction_ = 1.0f;
    }

    void process(ProcessContext& context) override {
        detector_.setAttackMs(targetAttack_);
        detector_.setReleaseMs(targetRelease_);

        const float makeupLinear = EnvelopeDetector::dbToLinear(targetMakeup_);
        const float threshold = targetThreshold_;
        const float ratio = targetRatio_;
        const float knee = targetKnee_;
        const float halfKnee = knee * 0.5f;

        const float* scLPtr = (context.numSidechainChannels > 0 && context.sidechainChannels != nullptr && context.sidechainChannels[0] != nullptr)
            ? context.sidechainChannels[0] : ((context.numInputChannels > 0) ? context.inputChannels[0] : nullptr);
        const float* scRPtr = (context.numSidechainChannels > 1 && context.sidechainChannels != nullptr && context.sidechainChannels[1] != nullptr)
            ? context.sidechainChannels[1] : scLPtr;

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            float inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0][s] : 0.0f;
            float inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1][s] : inL;

            float scL = (scLPtr != nullptr) ? scLPtr[s] : inL;
            float scR = (scRPtr != nullptr) ? scRPtr[s] : inR;

            // Detector sidechain estéreo (Reglas 6, 13 y 34)
            float maxSample = std::max(std::abs(scL), std::abs(scR));
            float env = detector_.processSample(maxSample);
            float envDb = EnvelopeDetector::linearToDb(env);

            // Curva de compresión con Soft-Knee
            float targetGrDb = 0.0f;
            if (knee > 0.0f && envDb > (threshold - halfKnee) && envDb < (threshold + halfKnee)) {
                float x = envDb - threshold + halfKnee;
                targetGrDb = ((1.0f / ratio - 1.0f) * (x * x)) / (2.0f * knee);
            } else if (envDb >= (threshold + halfKnee)) {
                targetGrDb = (threshold - envDb) * (1.0f - 1.0f / ratio);
            }

            float targetGrLinear = EnvelopeDetector::dbToLinear(targetGrDb);

            // Suavizado anti-click del Gain Reduction (Regla 35)
            currentGainReduction_ += 0.05f * (targetGrLinear - currentGainReduction_);

            float finalGain = currentGainReduction_ * makeupLinear;

            if (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) {
                context.outputChannels[0][s] = inL * finalGain;
            }
            if (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) {
                context.outputChannels[1][s] = inR * finalGain;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Threshold: targetThreshold_ = std::clamp(value, -60.0f, 0.0f); break;
            case Ratio: targetRatio_ = std::clamp(value, 1.0f, 20.0f); break;
            case Attack: targetAttack_ = std::clamp(value, 0.1f, 200.0f); break;
            case Release: targetRelease_ = std::clamp(value, 5.0f, 1000.0f); break;
            case Makeup: targetMakeup_ = std::clamp(value, 0.0f, 30.0f); break;
            case Knee: targetKnee_ = std::clamp(value, 0.0f, 20.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Threshold: return targetThreshold_;
            case Ratio: return targetRatio_;
            case Attack: return targetAttack_;
            case Release: return targetRelease_;
            case Makeup: return targetMakeup_;
            case Knee: return targetKnee_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Compressor; }
    const char* getName() const override { return "Compressor"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    EnvelopeDetector detector_;

    float targetThreshold_{ -18.0f };
    float targetRatio_{ 4.0f };
    float targetAttack_{ 15.0f };
    float targetRelease_{ 100.0f };
    float targetMakeup_{ 0.0f };
    float targetKnee_{ 4.0f };

    float currentGainReduction_{ 1.0f };

    std::array<PinDescriptor, 3> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<CompressorNode> registerCompressor(NodeType::Compressor, "compressor", "Dynamics");

} // namespace audio_graph
