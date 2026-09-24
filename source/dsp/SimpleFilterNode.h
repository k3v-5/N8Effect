#pragma once

#include <cmath>
#include <array>
#include <algorithm>
#include <span>
#include <numbers>
#include "../graph/AudioProcessorNode.h"
#include "../graph/NodeFactory.h"

namespace audio_graph {

/**
 * @brief Filtro State-Variable de 2 polos (SVF) (Regla 5, 8, 14, 34, 38)
 * Implementa Lowpass, Highpass y Bandpass matemáticamente exactos con TPT (Topology-Preserving Transform).
 */
class SimpleFilterNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        CutoffHz = 1,
        Resonance = 2,
        Mode = 3 // 0: LP, 1: HP, 2: BP
    };

    SimpleFilterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        params_[0] = { CutoffHz, "Cutoff", 1000.0f, 20.0f, 20000.0f, true };
        params_[1] = { Resonance, "Resonance", 0.707f, 0.1f, 10.0f, true };
        params_[2] = { Mode, "Mode", 0.0f, 0.0f, 2.0f, false };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        reset();
        updateCoefficients();
    }

    void reset() override {
        s1_.fill(0.0f);
        s2_.fill(0.0f);
    }

    void process(ProcessContext& context) override {
        updateCoefficients();

        const uint32_t channels = std::min(context.numOutputChannels, static_cast<uint32_t>(2));
        for (uint32_t s = 0; s < context.numSamples; ++s) {
            for (uint32_t ch = 0; ch < channels; ++ch) {
                const float in = (ch < context.numInputChannels && context.inputChannels[ch] != nullptr)
                    ? context.inputChannels[ch][s]
                    : 0.0f;

                // TPT State Variable Filter equations
                const float hp = (in - (2.0f * r_ + g_) * s1_[ch] - s2_[ch]) / h_;
                const float bp = g_ * hp + s1_[ch];
                const float lp = g_ * bp + s2_[ch];

                s1_[ch] = 2.0f * bp - s1_[ch];
                s2_[ch] = 2.0f * lp - s2_[ch];

                // Protección contra NaN / Inf (Regla 38)
                if (std::isnan(s1_[ch]) || std::isinf(s1_[ch])) s1_[ch] = 0.0f;
                if (std::isnan(s2_[ch]) || std::isinf(s2_[ch])) s2_[ch] = 0.0f;

                float out = lp;
                if (mode_ == 1) out = hp;
                else if (mode_ == 2) out = bp;

                context.outputChannels[ch][s] = out;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case CutoffHz: targetCutoff_ = std::clamp(value, 20.0f, 20000.0f); break;
            case Resonance: targetResonance_ = std::clamp(value, 0.1f, 10.0f); break;
            case Mode: mode_ = static_cast<int>(std::round(value)); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case CutoffHz: return targetCutoff_;
            case Resonance: return targetResonance_;
            case Mode: return static_cast<float>(mode_);
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Filter; }
    const char* getName() const override { return "SVF Filter"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateCoefficients() noexcept {
        const float sampleRate = static_cast<float>(spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0);
        const float wd = 2.0f * std::numbers::pi_v<float> * targetCutoff_;
        const float T = 1.0f / sampleRate;
        const float wa = (2.0f / T) * std::tan(wd * T * 0.5f);
        g_ = wa * T * 0.5f;
        r_ = 1.0f / (2.0f * targetResonance_);
        h_ = 1.0f + 2.0f * r_ * g_ + g_ * g_;
    }

    ProcessSpec spec_;
    float targetCutoff_{ 1000.0f };
    float targetResonance_{ 0.707f };
    int mode_{ 0 };

    float g_{ 0.0f };
    float r_{ 0.707f };
    float h_{ 1.0f };

    std::array<float, 2> s1_{ 0.0f, 0.0f };
    std::array<float, 2> s2_{ 0.0f, 0.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 3> params_;
};

inline AutoRegisterNode<SimpleFilterNode> registerSimpleFilter(NodeType::Filter, "filter", "Filters");

} // namespace audio_graph
