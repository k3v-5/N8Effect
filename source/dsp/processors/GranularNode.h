#pragma once

#include <cmath>
#include <array>
#include <vector>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/GrainPool.h"

namespace audio_graph {

/**
 * @brief Nodo Procesador Granular en tiempo real con nubes de hasta 128 granos (Reglas 5, 8, 9, 10, 11, 18, 35)
 */
class GranularNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        GrainSizeMs = 1,
        Density = 2,
        PositionSprayMs = 3,
        PitchSemitones = 4,
        PitchSpray = 5,
        PanSpray = 6,
        DryWet = 7,
        ScrubPosition = 8,
        Freeze = 9
    };

    GranularNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { GrainSizeMs, "Grain Size", 80.0f, 10.0f, 500.0f, true };
        params_[1] = { Density, "Density", 15.0f, 1.0f, 60.0f, true };
        params_[2] = { PositionSprayMs, "Pos Spray", 20.0f, 0.0f, 200.0f, true };
        params_[3] = { PitchSemitones, "Pitch", 0.0f, -24.0f, 24.0f, true };
        params_[4] = { PitchSpray, "Pitch Spray", 0.0f, 0.0f, 12.0f, true };
        params_[5] = { PanSpray, "Pan Spray", 0.5f, 0.0f, 1.0f, true };
        params_[6] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };
        params_[7] = { ScrubPosition, "Scrub Pos", 0.0f, 0.0f, 1.0f, true };
        params_[8] = { Freeze, "Freeze", 0.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        grainPool_.reset();

        bufferCapacity_ = static_cast<size_t>(spec.sampleRate * 2.0);
        if (bufferCapacity_ > MaxRingBufferSize) {
            bufferCapacity_ = MaxRingBufferSize;
        }
        ringBufferL_.assign(bufferCapacity_, 0.0f);
        ringBufferR_.assign(bufferCapacity_, 0.0f);
        writePos_ = 0;
        samplesUntilNextGrain_ = 0.0f;
        totalSamplesRecorded_ = 0;
    }

    void reset() override {
        grainPool_.reset();
        writePos_ = 0;
        samplesUntilNextGrain_ = 0.0f;
        totalSamplesRecorded_ = 0;
        std::fill(ringBufferL_.begin(), ringBufferL_.end(), 0.0f);
        std::fill(ringBufferR_.begin(), ringBufferR_.end(), 0.0f);
    }

    void process(ProcessContext& context) override {
        if (ringBufferL_.empty() || bufferCapacity_ == 0 || context.numSamples == 0 || context.numInputChannels == 0 || context.numOutputChannels == 0) return;

        const float grainSizeMs = targetGrainSizeMs_;
        const float density = std::clamp(targetDensity_, 1.0f, 60.0f);
        const float posSprayMs = std::max(0.0f, targetPositionSprayMs_);
        const float pitchSemi = targetPitchSemitones_;
        const float pitchSpray = std::max(0.0f, targetPitchSpray_);
        const float panSpray = std::clamp(targetPanSpray_, 0.0f, 1.0f);
        const float dryWet = std::clamp(targetMix_, 0.0f, 1.0f);

        const float spawnIntervalSamples = static_cast<float>(spec_.sampleRate) / density;
        const int baseGrainDuration = std::max(16, static_cast<int>(grainSizeMs * 0.001f * static_cast<float>(spec_.sampleRate)));

        const float* inL = context.inputChannels[0];
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : context.inputChannels[0];

        float* outL = context.outputChannels[0];
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : context.outputChannels[0];

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            const float drySampleL = inL[s];
            const float drySampleR = inR[s];

            if (targetFreeze_ <= 0.5f) {
                ringBufferL_[writePos_] = drySampleL;
                ringBufferR_[writePos_] = drySampleR;
                writePos_ = (writePos_ + 1 < bufferCapacity_) ? (writePos_ + 1) : 0;
                totalSamplesRecorded_++;
            }
            const size_t currentWrite = writePos_;

            samplesUntilNextGrain_ -= 1.0f;
            if (samplesUntilNextGrain_ <= 0.0f) {
                samplesUntilNextGrain_ += spawnIntervalSamples;

                Grain* grain = grainPool_.acquire();
                if (grain != nullptr) {
                    const float rPos = nextRandomFloat();
                    const float rPitch = nextRandomFloat() * 2.0f - 1.0f;
                    const float rPan = nextRandomFloat() * 2.0f - 1.0f;

                    const float offsetSamples = posSprayMs * 0.001f * static_cast<float>(spec_.sampleRate) * rPos;
                    float startPos = 0.0f;
                    if (targetScrubPos_ > 0.0001f) {
                        float centerPos = targetScrubPos_ * static_cast<float>(bufferCapacity_);
                        startPos = centerPos - offsetSamples;
                        while (startPos < 0.0f) startPos += static_cast<float>(bufferCapacity_);
                        while (startPos >= static_cast<float>(bufferCapacity_)) startPos -= static_cast<float>(bufferCapacity_);
                    } else if (totalSamplesRecorded_ < bufferCapacity_) {
                        // Al inicio, no leer más allá de lo grabado
                        const float maxBack = static_cast<float>(currentWrite);
                        startPos = std::max(0.0f, maxBack - offsetSamples - static_cast<float>(baseGrainDuration) * 0.5f);
                    } else {
                        startPos = static_cast<float>(currentWrite) - offsetSamples - static_cast<float>(baseGrainDuration);
                        while (startPos < 0.0f) startPos += static_cast<float>(bufferCapacity_);
                    }

                    const float finalPitchSemitones = pitchSemi + (pitchSpray * rPitch);
                    const float rate = std::pow(2.0f, finalPitchSemitones / 12.0f);

                    const float pan = std::clamp(0.5f + 0.5f * (rPan * panSpray), 0.0f, 1.0f);

                    grain->sourceStartPos = startPos;
                    grain->currentPlayhead = startPos;
                    grain->playbackRate = std::clamp(rate, 0.25f, 4.0f);
                    grain->durationSamples = baseGrainDuration;
                    grain->ageSamples = 0;
                    grain->panL = std::cos(pan * 1.5707963f);
                    grain->panR = std::sin(pan * 1.5707963f);
                    grain->amplitude = 1.0f;
                }
            }

            float grainSumL = 0.0f;
            float grainSumR = 0.0f;

            for (size_t g = 0; g < GrainPool::MaxGrains; ++g) {
                Grain& grain = grainPool_.getGrain(g);
                if (!grain.active) continue;

                const float env = grain.getWindowEnvelope();

                const float ph = grain.currentPlayhead;
                const size_t idx0 = static_cast<size_t>(ph) % bufferCapacity_;
                const size_t idx1 = (idx0 + 1 < bufferCapacity_) ? (idx0 + 1) : 0;
                const float frac = ph - std::floor(ph);

                const float gSampL = ringBufferL_[idx0] + frac * (ringBufferL_[idx1] - ringBufferL_[idx0]);
                const float gSampR = ringBufferR_[idx0] + frac * (ringBufferR_[idx1] - ringBufferR_[idx0]);

                const float winSampL = gSampL * env;
                const float winSampR = gSampR * env;

                grainSumL += winSampL * grain.panL;
                grainSumR += winSampR * grain.panR;

                grain.currentPlayhead += grain.playbackRate;
                if (grain.currentPlayhead >= static_cast<float>(bufferCapacity_)) {
                    grain.currentPlayhead -= static_cast<float>(bufferCapacity_);
                }
                grain.ageSamples++;

                if (grain.ageSamples >= grain.durationSamples) {
                    grainPool_.release(&grain);
                }
            }

            constexpr float grainGain = 0.707f;
            const float wetL = grainSumL * grainGain;
            const float wetR = grainSumR * grainGain;

            outL[s] = drySampleL * (1.0f - dryWet) + wetL * dryWet;
            outR[s] = drySampleR * (1.0f - dryWet) + wetR * dryWet;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case GrainSizeMs: targetGrainSizeMs_ = std::clamp(value, 10.0f, 500.0f); break;
            case Density: targetDensity_ = std::clamp(value, 1.0f, 60.0f); break;
            case PositionSprayMs: targetPositionSprayMs_ = std::clamp(value, 0.0f, 200.0f); break;
            case PitchSemitones: targetPitchSemitones_ = std::clamp(value, -24.0f, 24.0f); break;
            case PitchSpray: targetPitchSpray_ = std::clamp(value, 0.0f, 12.0f); break;
            case PanSpray: targetPanSpray_ = std::clamp(value, 0.0f, 1.0f); break;
            case DryWet: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            case ScrubPosition: targetScrubPos_ = std::clamp(value, 0.0f, 1.0f); break;
            case Freeze: targetFreeze_ = (value > 0.5f) ? 1.0f : 0.0f; break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case GrainSizeMs: return targetGrainSizeMs_;
            case Density: return targetDensity_;
            case PositionSprayMs: return targetPositionSprayMs_;
            case PitchSemitones: return targetPitchSemitones_;
            case PitchSpray: return targetPitchSpray_;
            case PanSpray: return targetPanSpray_;
            case DryWet: return targetMix_;
            case ScrubPosition: return targetScrubPos_;
            case Freeze: return targetFreeze_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Granular; }
    const char* getName() const override { return "Granular"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 0.5);
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    struct GrainCloudPoint {
        float normPosition{ 0.0f }; // 0.0 to 1.0
        float pitchRatio{ 1.0f };
        float pan{ 0.5f };
        float envelope{ 0.0f };
        bool active{ false };
    };

    size_t copyWaveformOverview(float* outMinMax, size_t numPairs) const noexcept {
        if (ringBufferL_.empty() || bufferCapacity_ == 0 || outMinMax == nullptr || numPairs == 0) return 0;
        const size_t step = std::max<size_t>(1, bufferCapacity_ / numPairs);
        for (size_t p = 0; p < numPairs; ++p) {
            float minVal = 0.0f;
            float maxVal = 0.0f;
            const size_t startIdx = p * step;
            const size_t endIdx = std::min(startIdx + step, bufferCapacity_);
            for (size_t i = startIdx; i < endIdx; ++i) {
                float s = 0.5f * (ringBufferL_[i] + ringBufferR_[i]);
                if (s < minVal) minVal = s;
                if (s > maxVal) maxVal = s;
            }
            outMinMax[p * 2] = minVal;
            outMinMax[p * 2 + 1] = maxVal;
        }
        return numPairs;
    }

    size_t copyActiveGrainsSnapshot(std::span<GrainCloudPoint> outPoints) const noexcept {
        if (outPoints.empty() || bufferCapacity_ == 0) return 0;
        size_t written = 0;
        for (size_t g = 0; g < GrainPool::MaxGrains && written < outPoints.size(); ++g) {
            const Grain& grain = grainPool_.getGrain(g);
            if (!grain.active) continue;
            auto& pt = outPoints[written++];
            pt.normPosition = std::clamp(grain.currentPlayhead / static_cast<float>(bufferCapacity_), 0.0f, 1.0f);
            pt.pitchRatio = grain.playbackRate;
            pt.pan = std::clamp(std::atan2(grain.panR, grain.panL) / 1.5707963f, 0.0f, 1.0f);
            pt.envelope = grain.getWindowEnvelope();
            pt.active = true;
        }
        return written;
    }

    float getWritePositionNormalized() const noexcept {
        return (bufferCapacity_ > 0) ? (static_cast<float>(writePos_) / static_cast<float>(bufferCapacity_)) : 0.0f;
    }

private:
    [[nodiscard]] float nextRandomFloat() noexcept {
        rngState_ = rngState_ * 1664525u + 1013904223u;
        return static_cast<float>(rngState_ & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
    }

    static constexpr size_t MaxRingBufferSize = 192000;
    ProcessSpec spec_;

    GrainPool grainPool_{};
    std::vector<float> ringBufferL_;
    std::vector<float> ringBufferR_;
    size_t bufferCapacity_{ 88200 };
    size_t writePos_{ 0 };
    size_t totalSamplesRecorded_{ 0 };
    float samplesUntilNextGrain_{ 0.0f };
    uint32_t rngState_{ 123456789 };

    float targetGrainSizeMs_{ 80.0f };
    float targetDensity_{ 15.0f };
    float targetPositionSprayMs_{ 20.0f };
    float targetPitchSemitones_{ 0.0f };
    float targetPitchSpray_{ 0.0f };
    float targetPanSpray_{ 0.5f };
    float targetMix_{ 0.5f };
    float targetScrubPos_{ 0.0f };
    float targetFreeze_{ 0.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 9> params_;
};

inline AutoRegisterNode<GranularNode> registerGranular(NodeType::Granular, "granular", "Granular");

} // namespace audio_graph
