#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Limitador de Pico Brickwall y Maximizador Profesional (Reglas 5, 8, 9, 14, 34, 46, 47)
 * Proporciona limitación transparente con búfer de lookahead circular de capacidad acotada,
 * detector de picos adelantado, envolvente de decaimiento suave y techo de salida inexpugnable.
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class BrickwallLimiterNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Threshold = 1,  // -24.0 a 0.0 dB (Drive / Umbral)
        Ceiling = 2,    // -12.0 a 0.0 dB (Techo absoluto)
        Release = 3,    // 2.0 a 500.0 ms (Tiempo de recuperación)
        Lookahead = 4,  // 0.2 a 5.0 ms (Anticipación en lookahead)
        AutoMakeup = 5  // 0.0 o 1.0 (Compensación automática de ganancia)
    };

    BrickwallLimiterNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Threshold, "Threshold", -1.0f, -24.0f, 0.0f, true };
        params_[1] = { Ceiling, "Ceiling", -0.1f, -12.0f, 0.0f, true };
        params_[2] = { Release, "Release", 40.0f, 2.0f, 500.0f, true };
        params_[3] = { Lookahead, "Lookahead", 2.0f, 0.2f, 5.0f, true };
        params_[4] = { AutoMakeup, "Auto Makeup", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_brickwall_limiter_create(44100.0f);
    }

    ~BrickwallLimiterNode() override {
        if (rustInstance_) {
            n8_rust_brickwall_limiter_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    BrickwallLimiterNode(const BrickwallLimiterNode&) = delete;
    BrickwallLimiterNode& operator=(const BrickwallLimiterNode&) = delete;

    BrickwallLimiterNode(BrickwallLimiterNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    BrickwallLimiterNode& operator=(BrickwallLimiterNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_brickwall_limiter_destroy(rustInstance_);
            }
            pins_ = other.pins_;
            params_ = other.params_;
            spec_ = other.spec_;
            rustInstance_ = other.rustInstance_;
            other.rustInstance_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        const float sr = spec.sampleRate > 0.0 ? static_cast<float>(spec.sampleRate) : 44100.0f;
        if (rustInstance_) {
            n8_rust_brickwall_limiter_prepare(rustInstance_, sr);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_brickwall_limiter_reset(rustInstance_);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;
        if (!rustInstance_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0]) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1]) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0]) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1]) ? context.outputChannels[1] : nullptr;

        if (!inL || !outL) return;

        n8_rust_brickwall_limiter_process(
            rustInstance_,
            inL,
            inR,
            outL,
            outR,
            static_cast<uint32_t>(context.numSamples)
        );
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_) {
            n8_rust_brickwall_limiter_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_brickwall_limiter_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::BrickwallLimiter; }
    const char* getName() const override { return "Brickwall Limiter"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
    RustBrickwallLimiter* rustInstance_{ nullptr };
};

inline AutoRegisterNode<BrickwallLimiterNode> registerLimiter(NodeType::BrickwallLimiter, "brickwall_limiter", "Dynamics");

} // namespace audio_graph
