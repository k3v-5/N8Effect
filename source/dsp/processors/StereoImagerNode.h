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
 * @brief Modelador Estéreo Multibanda y Procesador de Imagen M/S (Reglas 5, 8, 9, 14, 16, 34, 46, 47)
 * Proporciona ensanchamiento Mid/Side, micro-retardo Haas y filtro elíptico mono de graves.
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class StereoImagerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Width = 1,
        HaasDelayMs = 2,
        MonoBassFreq = 3,
        Balance = 4,
        Mix = 5
    };

    StereoImagerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Width, "Width", 1.0f, 0.0f, 2.0f, true };
        params_[1] = { HaasDelayMs, "Haas Delay", 0.0f, 0.0f, 20.0f, true };
        params_[2] = { MonoBassFreq, "Mono Bass", 120.0f, 20.0f, 350.0f, true };
        params_[3] = { Balance, "Balance", 0.0f, -1.0f, 1.0f, true };
        params_[4] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_stereo_imager_create(44100.0f);
    }

    ~StereoImagerNode() override {
        if (rustInstance_) {
            n8_rust_stereo_imager_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    StereoImagerNode(const StereoImagerNode&) = delete;
    StereoImagerNode& operator=(const StereoImagerNode&) = delete;

    StereoImagerNode(StereoImagerNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    StereoImagerNode& operator=(StereoImagerNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_stereo_imager_destroy(rustInstance_);
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
            n8_rust_stereo_imager_prepare(rustInstance_, sr);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_stereo_imager_reset(rustInstance_);
        }
    }

    NodeType getType() const override { return NodeType::StereoImager; }
    const char* getName() const override { return "Stereo Imager"; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;
        if (!rustInstance_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (!inL || !outL) return;

        n8_rust_stereo_imager_process(
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
            n8_rust_stereo_imager_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_stereo_imager_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    float getPhaseCorrelation() const noexcept {
        return rustInstance_ ? n8_rust_stereo_imager_get_phase_correlation(rustInstance_) : 1.0f;
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
    RustStereoImager* rustInstance_{ nullptr };
};

inline AutoRegisterNode<StereoImagerNode> registerStereoImager(NodeType::StereoImager, "stereoimager", "Spatial");

} // namespace audio_graph

