#pragma once

#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <span>
#include "../../core/Types.h"
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/PolyphaseOversampler.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Nodo de Acondicionamiento y Saturación con Sobremuestreo Polifásico (OversamplerNode)
 * Permite procesar con sobremuestreo 2x, 4x u 8x y saturación hiper-limpia libre de aliasing
 * con compensación de fase (PDC) (Reglas 5, 8, 9, 14, 34, 46, 47).
 * Delegado al motor SIMD de alto rendimiento en Rust (n8_rust_oversampler).
 */
class OversamplerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Factor = 1,       // 0: 1x (Bypass), 1: 2x, 2: 4x, 3: 8x
        DriveDb = 2,      // 0.0 a 24.0 dB de empuje armónico
        Saturation = 3,   // 0: Off, 1: Soft Tape, 2: Hard Clip, 3: Tube Wavefold
        LowCutHz = 4,     // 20.0 a 500.0 Hz
        HighCutHz = 5,    // 2000.0 a 20000.0 Hz
        Mix = 6           // 0.0 a 1.0 (Dry / Wet)
    };

    OversamplerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Factor, "Factor", 1.0f, 0.0f, 3.0f, false };
        params_[1] = { DriveDb, "Drive dB", 0.0f, 0.0f, 24.0f, true };
        params_[2] = { Saturation, "Mode", 1.0f, 0.0f, 3.0f, false };
        params_[3] = { LowCutHz, "Low Cut", 30.0f, 20.0f, 500.0f, true };
        params_[4] = { HighCutHz, "High Cut", 18000.0f, 2000.0f, 20000.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustNode_ = n8_rust_oversampler_create();
    }

    ~OversamplerNode() override {
        if (rustNode_) {
            n8_rust_oversampler_destroy(rustNode_);
            rustNode_ = nullptr;
        }
    }

    OversamplerNode(const OversamplerNode&) = delete;
    OversamplerNode& operator=(const OversamplerNode&) = delete;

    OversamplerNode(OversamplerNode&& other) noexcept
        : spec_(other.spec_),
          pins_(other.pins_),
          params_(other.params_),
          rustNode_(other.rustNode_) {
        other.rustNode_ = nullptr;
    }

    OversamplerNode& operator=(OversamplerNode&& other) noexcept {
        if (this != &other) {
            if (rustNode_) {
                n8_rust_oversampler_destroy(rustNode_);
            }
            spec_ = other.spec_;
            pins_ = other.pins_;
            params_ = other.params_;
            rustNode_ = other.rustNode_;
            other.rustNode_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustNode_) {
            n8_rust_oversampler_prepare(rustNode_, spec.sampleRate, spec.maximumBlockSize);
        }
    }

    void reset() noexcept override {
        if (rustNode_) {
            n8_rust_oversampler_reset(rustNode_);
        }
    }

    void process(ProcessContext& ctx) noexcept override {
        if (!rustNode_ || ctx.numSamples == 0 || ctx.outputChannels == nullptr) return;

        const float* inL = (ctx.numInputChannels > 0 && ctx.inputChannels[0]) ? ctx.inputChannels[0] : nullptr;
        const float* inR = (ctx.numInputChannels > 1 && ctx.inputChannels[1]) ? ctx.inputChannels[1] : inL;
        float* outL = ctx.outputChannels[0];
        float* outR = (ctx.numOutputChannels > 1) ? ctx.outputChannels[1] : nullptr;

        n8_rust_oversampler_process(
            rustNode_,
            inL,
            inR,
            outL,
            outR,
            static_cast<uint32_t>(ctx.numSamples)
        );
    }

    std::span<const PinDescriptor> getPins() const noexcept override { return pins_; }
    std::span<const ParameterInfo> getParameters() const noexcept override { return params_; }

    NodeType getType() const noexcept override { return NodeType::Oversampler; }
    const char* getName() const noexcept override { return "Oversampler"; }

    float getParameter(ParameterId id) const override {
        if (rustNode_) {
            return n8_rust_oversampler_get_parameter(rustNode_, id);
        }
        return 0.0f;
    }

    void setParameter(ParameterId id, float value) override {
        if (rustNode_) {
            n8_rust_oversampler_set_parameter(rustNode_, id, value);
        }
    }

    size_t getPdcLatencySamples() const noexcept {
        if (rustNode_) {
            return n8_rust_oversampler_get_latency(rustNode_);
        }
        return 0;
    }

private:
    ProcessSpec spec_{ 48000.0, 256, 2, 2 };
    RustOversampler* rustNode_{ nullptr };
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<OversamplerNode> registerOversampler(NodeType::Oversampler, "oversampler", "Dynamics");

} // namespace audio_graph
