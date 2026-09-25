#pragma once

#include <cmath>
#include <array>
#include <vector>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"

namespace audio_graph {

/**
 * @brief Procesador de Glitch, Stutter y Rebanado rítmico con suavizado Anti-Click (Reglas 5, 8, 9, 35, 37)
 */
class GlitchNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Division = 1,
        RepeatProbability = 2,
        ReverseProbability = 3,
        StutterGate = 4,
        DryWet = 5
    };

    GlitchNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Division, "Division", 2.0f, 0.0f, 4.0f, false };
        params_[1] = { RepeatProbability, "Repeat Prob", 0.5f, 0.0f, 1.0f, true };
        params_[2] = { ReverseProbability, "Reverse Prob", 0.2f, 0.0f, 1.0f, true };
        params_[3] = { StutterGate, "Stutter Gate", 1.0f, 0.05f, 1.0f, true };
        params_[4] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        bufferCapacity_ = static_cast<size_t>(spec.sampleRate * 2.0);
        if (bufferCapacity_ > MaxBufferSize) {
            bufferCapacity_ = MaxBufferSize;
        }
        bufferL_.assign(bufferCapacity_, 0.0f);
        bufferR_.assign(bufferCapacity_, 0.0f);
        reset();
    }

    void reset() override {
        writePos_ = 0;
        sliceStartPos_ = 0;
        sliceReadPos_ = 0;
        sliceSampleCounter_ = 0;
        isGlitching_ = false;
        isReverse_ = false;
        std::fill(bufferL_.begin(), bufferL_.end(), 0.0f);
        std::fill(bufferR_.begin(), bufferR_.end(), 0.0f);
    }

    void process(ProcessContext& context) override {
        if (bufferL_.empty() || bufferCapacity_ == 0 || context.numSamples == 0 || context.numInputChannels == 0 || context.numOutputChannels == 0) return;

        const float divVal = targetDivision_;
        const float repeatProb = std::clamp(targetRepeatProb_, 0.0f, 1.0f);
        const float revProb = std::clamp(targetReverseProb_, 0.0f, 1.0f);
        const float stutterGate = std::clamp(targetStutterGate_, 0.05f, 1.0f);
        const float dryWet = std::clamp(targetMix_, 0.0f, 1.0f);

        const int divIndex = std::clamp(static_cast<int>(divVal), 0, 4);
        constexpr std::array<float, 5> divisionMultipliers = { 0.5f, 0.25f, 0.125f, 0.0625f, 0.03125f };
        const size_t sliceLen = std::max(size_t{ 64 }, static_cast<size_t>(spec_.sampleRate * divisionMultipliers[divIndex]));
        const size_t activeGateLen = static_cast<size_t>(static_cast<float>(sliceLen) * stutterGate);

        const float* inL = context.inputChannels[0];
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : context.inputChannels[0];

        float* outL = context.outputChannels[0];
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : context.outputChannels[0];

        constexpr size_t fadeLen = 32;

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            const float dryL = inL[s];
            const float dryR = inR[s];

            bufferL_[writePos_] = dryL;
            bufferR_[writePos_] = dryR;
            const size_t currentWrite = writePos_;
            writePos_ = (writePos_ + 1 < bufferCapacity_) ? (writePos_ + 1) : 0;

            if (sliceSampleCounter_ >= sliceLen) {
                sliceSampleCounter_ = 0;

                const float rRep = nextRandomFloat();
                if (rRep < repeatProb) {
                    isGlitching_ = true;
                    sliceStartPos_ = (currentWrite >= sliceLen) ? (currentWrite - sliceLen) : (bufferCapacity_ + currentWrite - sliceLen);
                    isReverse_ = (nextRandomFloat() < revProb);
                } else {
                    isGlitching_ = false;
                }
            }

            float wetL = 0.0f;
            float wetR = 0.0f;

            if (isGlitching_) {
                if (sliceSampleCounter_ < activeGateLen) {
                    size_t readOffset = isReverse_ ? (sliceLen - 1 - sliceSampleCounter_) : sliceSampleCounter_;
                    size_t readIdx = (sliceStartPos_ + readOffset) % bufferCapacity_;

                    float env = 1.0f;
                    if (sliceSampleCounter_ < fadeLen) {
                        env = static_cast<float>(sliceSampleCounter_) / static_cast<float>(fadeLen);
                    } else if (sliceSampleCounter_ + fadeLen >= activeGateLen) {
                        env = static_cast<float>(activeGateLen - sliceSampleCounter_) / static_cast<float>(fadeLen);
                    }

                    wetL = bufferL_[readIdx] * env;
                    wetR = bufferR_[readIdx] * env;
                }
            } else {
                wetL = dryL;
                wetR = dryR;
            }

            sliceSampleCounter_++;

            outL[s] = dryL * (1.0f - dryWet) + wetL * dryWet;
            outR[s] = dryR * (1.0f - dryWet) + wetR * dryWet;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Division: targetDivision_ = std::clamp(value, 0.0f, 4.0f); break;
            case RepeatProbability: targetRepeatProb_ = std::clamp(value, 0.0f, 1.0f); break;
            case ReverseProbability: targetReverseProb_ = std::clamp(value, 0.0f, 1.0f); break;
            case StutterGate: targetStutterGate_ = std::clamp(value, 0.05f, 1.0f); break;
            case DryWet: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Division: return targetDivision_;
            case RepeatProbability: return targetRepeatProb_;
            case ReverseProbability: return targetReverseProb_;
            case StutterGate: return targetStutterGate_;
            case DryWet: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Glitch; }
    const char* getName() const override { return "Glitch"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    [[nodiscard]] float nextRandomFloat() noexcept {
        rngState_ = rngState_ * 1664525u + 1013904223u;
        return static_cast<float>(rngState_ & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
    }

    static constexpr size_t MaxBufferSize = 192000;
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;

    std::vector<float> bufferL_;
    std::vector<float> bufferR_;
    size_t bufferCapacity_{ 88200 };
    size_t writePos_{ 0 };

    size_t sliceStartPos_{ 0 };
    size_t sliceReadPos_{ 0 };
    size_t sliceSampleCounter_{ 0 };
    bool isGlitching_{ false };
    bool isReverse_{ false };
    uint32_t rngState_{ 543210987 };

    float targetDivision_{ 2.0f };
    float targetRepeatProb_{ 0.5f };
    float targetReverseProb_{ 0.2f };
    float targetStutterGate_{ 1.0f };
    float targetMix_{ 0.5f };
};

inline AutoRegisterNode<GlitchNode> registerGlitch(NodeType::Glitch, "glitch", "Glitch");

} // namespace audio_graph
