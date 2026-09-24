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
 * @brief Ecualizador Paramétrico de 3 Bandas (Low Shelf, Bell, High Shelf) (Reglas 5, 8, 12, 14, 34)
 */
class ParametricEQNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        LowFreq = 1,
        LowGain = 2,
        MidFreq = 3,
        MidQ = 4,
        MidGain = 5,
        HighFreq = 6,
        HighGain = 7
    };

    ParametricEQNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { LowFreq, "Low Freq", 100.0f, 20.0f, 500.0f, true };
        params_[1] = { LowGain, "Low Gain", 0.0f, -18.0f, 18.0f, true };
        params_[2] = { MidFreq, "Mid Freq", 1000.0f, 100.0f, 8000.0f, true };
        params_[3] = { MidQ, "Mid Q", 1.0f, 0.1f, 10.0f, true };
        params_[4] = { MidGain, "Mid Gain", 0.0f, -18.0f, 18.0f, true };
        params_[5] = { HighFreq, "High Freq", 8000.0f, 1000.0f, 18000.0f, true };
        params_[6] = { HighGain, "High Gain", 0.0f, -18.0f, 18.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        reset();
        updateFilters();
    }

    void reset() override {
        for (auto& f : lowShelf_) f.reset();
        for (auto& f : midPeak_) f.reset();
        for (auto& f : highShelf_) f.reset();
    }

    void process(ProcessContext& context) override {
        updateFilters();

        const uint32_t channels = std::min(context.numOutputChannels, static_cast<uint32_t>(2));
        for (uint32_t s = 0; s < context.numSamples; ++s) {
            for (uint32_t ch = 0; ch < channels; ++ch) {
                float sample = (ch < context.numInputChannels && context.inputChannels[ch] != nullptr)
                    ? context.inputChannels[ch][s]
                    : 0.0f;

                // Cascada de ecualización de 3 bandas
                sample = lowShelf_[ch].processSample(sample);
                sample = midPeak_[ch].processSample(sample);
                sample = highShelf_[ch].processSample(sample);

                context.outputChannels[ch][s] = sample;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case LowFreq: targetLowFreq_ = std::clamp(value, 20.0f, 500.0f); break;
            case LowGain: targetLowGain_ = std::clamp(value, -18.0f, 18.0f); break;
            case MidFreq: targetMidFreq_ = std::clamp(value, 100.0f, 8000.0f); break;
            case MidQ: targetMidQ_ = std::clamp(value, 0.1f, 10.0f); break;
            case MidGain: targetMidGain_ = std::clamp(value, -18.0f, 18.0f); break;
            case HighFreq: targetHighFreq_ = std::clamp(value, 1000.0f, 18000.0f); break;
            case HighGain: targetHighGain_ = std::clamp(value, -18.0f, 18.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case LowFreq: return targetLowFreq_;
            case LowGain: return targetLowGain_;
            case MidFreq: return targetMidFreq_;
            case MidQ: return targetMidQ_;
            case MidGain: return targetMidGain_;
            case HighFreq: return targetHighFreq_;
            case HighGain: return targetHighGain_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Filter; }
    const char* getName() const override { return "Parametric EQ"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateFilters() noexcept {
        const double sr = spec_.sampleRate;
        for (int ch = 0; ch < 2; ++ch) {
            lowShelf_[ch].setCoefficients(BiquadFilter::Type::LowShelf, sr, targetLowFreq_, 0.707f, targetLowGain_);
            midPeak_[ch].setCoefficients(BiquadFilter::Type::Peak, sr, targetMidFreq_, targetMidQ_, targetMidGain_);
            highShelf_[ch].setCoefficients(BiquadFilter::Type::HighShelf, sr, targetHighFreq_, 0.707f, targetHighGain_);
        }
    }

    ProcessSpec spec_;
    std::array<BiquadFilter, 2> lowShelf_;
    std::array<BiquadFilter, 2> midPeak_;
    std::array<BiquadFilter, 2> highShelf_;

    float targetLowFreq_{ 100.0f };
    float targetLowGain_{ 0.0f };
    float targetMidFreq_{ 1000.0f };
    float targetMidQ_{ 1.0f };
    float targetMidGain_{ 0.0f };
    float targetHighFreq_{ 8000.0f };
    float targetHighGain_{ 0.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 7> params_;
};

inline AutoRegisterNode<ParametricEQNode> registerParametricEQ(NodeType::Filter, "parametric_eq", "Filters");

} // namespace audio_graph
