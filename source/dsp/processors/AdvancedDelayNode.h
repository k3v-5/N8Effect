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
 * @brief Nodo de Delay Estéreo Avanzado con Ping-Pong, amortiguación y soporte de Tails (Reglas 5, 8, 12, 14, 18)
 */
class AdvancedDelayNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        TimeLeft = 1,
        TimeRight = 2,
        Feedback = 3,
        Damping = 4,
        PingPong = 5,
        DryWet = 6
    };

    AdvancedDelayNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { TimeLeft, "Time L", 300.0f, 1.0f, 2500.0f, true };
        params_[1] = { TimeRight, "Time R", 450.0f, 1.0f, 2500.0f, true };
        params_[2] = { Feedback, "Feedback", 0.45f, 0.0f, 0.95f, true };
        params_[3] = { Damping, "Damping", 6000.0f, 1000.0f, 20000.0f, true };
        params_[4] = { PingPong, "Ping Pong", 0.0f, 0.0f, 1.0f, false };
        params_[5] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        size_t maxDelay = static_cast<size_t>(spec.sampleRate * 3.0); // 3 segundos máximos
        delayL_.prepare(maxDelay);
        delayR_.prepare(maxDelay);
        reset();
        updateDamping();
    }

    void reset() override {
        delayL_.reset();
        delayR_.reset();
        dampFilter_[0].reset();
        dampFilter_[1].reset();
    }

    void process(ProcessContext& context) override {
        updateDamping();

        const float delaySamplesL = (targetTimeL_ * 0.001f) * static_cast<float>(spec_.sampleRate);
        const float delaySamplesR = (targetTimeR_ * 0.001f) * static_cast<float>(spec_.sampleRate);
        const float fb = std::clamp(targetFeedback_, 0.0f, 0.95f); // Protección runaway feedback (Regla 12)
        const bool pingPong = (targetPingPong_ > 0.5f);
        const float mix = targetMix_;

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            float inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0][s] : 0.0f;
            float inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1][s] : inL;

            // Lectura con interpolación cúbica (Hermite)
            float delayedL = delayL_.readCubic(delaySamplesL);
            float delayedR = delayR_.readCubic(delaySamplesR);

            // Filtrado de amortiguación en el lazo de feedback
            delayedL = dampFilter_[0].processSample(delayedL);
            delayedR = dampFilter_[1].processSample(delayedR);

            // Modos Normal vs Ping-Pong
            float fbInL = pingPong ? (inL + delayedR * fb) : (inL + delayedL * fb);
            float fbInR = pingPong ? (inR + delayedL * fb) : (inR + delayedR * fb);

            delayL_.write(fbInL);
            delayR_.write(fbInR);

            if (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) {
                context.outputChannels[0][s] = inL * (1.0f - mix) + delayedL * mix;
            }
            if (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) {
                context.outputChannels[1][s] = inR * (1.0f - mix) + delayedR * mix;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case TimeLeft: targetTimeL_ = std::clamp(value, 1.0f, 2500.0f); break;
            case TimeRight: targetTimeR_ = std::clamp(value, 1.0f, 2500.0f); break;
            case Feedback: targetFeedback_ = std::clamp(value, 0.0f, 0.95f); break;
            case Damping: targetDamping_ = std::clamp(value, 1000.0f, 20000.0f); break;
            case PingPong: targetPingPong_ = value > 0.5f ? 1.0f : 0.0f; break;
            case DryWet: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case TimeLeft: return targetTimeL_;
            case TimeRight: return targetTimeR_;
            case Feedback: return targetFeedback_;
            case Damping: return targetDamping_;
            case PingPong: return targetPingPong_;
            case DryWet: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Delay; }
    const char* getName() const override { return "Advanced Delay"; }
    bool supportsTail() const override { return true; } // Soporte explícito de tails (Regla 18)
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 5.0); // 5 segundos de tail
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateDamping() noexcept {
        dampFilter_[0].setCoefficients(BiquadFilter::Type::Lowpass, spec_.sampleRate, targetDamping_, 0.707f);
        dampFilter_[1].setCoefficients(BiquadFilter::Type::Lowpass, spec_.sampleRate, targetDamping_, 0.707f);
    }

    ProcessSpec spec_;
    DelayLine delayL_;
    DelayLine delayR_;
    std::array<BiquadFilter, 2> dampFilter_;

    float targetTimeL_{ 300.0f };
    float targetTimeR_{ 450.0f };
    float targetFeedback_{ 0.45f };
    float targetDamping_{ 6000.0f };
    float targetPingPong_{ 0.0f };
    float targetMix_{ 0.5f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<AdvancedDelayNode> registerAdvancedDelay(NodeType::Delay, "advanced_delay", "Time");

} // namespace audio_graph
