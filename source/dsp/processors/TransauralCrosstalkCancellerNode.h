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
 * @brief Transaural Crosstalk Canceller (RACE 3D Holographic Speaker Widener) (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Expansor estereofÃ³nico 3D para monitores fÃ­sicos basado en el algoritmo RACE (Recursive Ambiophonics Crosstalk Elimination).
 * Cancela el derrame acÃºstico contralateral entre altavoces y oÃ­dos opuestos con retardo fraccional de 180 microsegundos,
 * filtrado de sombra de cabeza y preservaciÃ³n del canal central vocal.
 */
class TransauralCrosstalkCancellerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        DelayUs = 1,
        CrosstalkCancellation = 2,
        HeadShadowDamping = 3,
        CenterPreserve = 4,
        Mix = 5
    };

    TransauralCrosstalkCancellerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { DelayUs, "Crosstalk Delay (us)", 180.0f, 60.0f, 320.0f, true };
        params_[1] = { CrosstalkCancellation, "Cancellation Depth", 0.65f, 0.0f, 1.0f, true };
        params_[2] = { HeadShadowDamping, "Head Shadow Freq (Hz)", 6500.0f, 1500.0f, 12000.0f, true };
        params_[3] = { CenterPreserve, "Center Phantom Preserve", 0.7f, 0.0f, 1.0f, true };
        params_[4] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_transaural_crosstalk_canceller_create(44100.0f);
    }

    ~TransauralCrosstalkCancellerNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_transaural_crosstalk_canceller_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::TransauralCrosstalkCanceller; }
    const char* getName() const override { return "Transaural Crosstalk Canceller"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_transaural_crosstalk_canceller_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_transaural_crosstalk_canceller_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_transaural_crosstalk_canceller_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_transaural_crosstalk_canceller_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case DelayUs: return params_[0].defaultValue;
            case CrosstalkCancellation: return params_[1].defaultValue;
            case HeadShadowDamping: return params_[2].defaultValue;
            case CenterPreserve: return params_[3].defaultValue;
            case Mix: return params_[4].defaultValue;
            default: return 0.0f;
        }
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (outL == nullptr || rustInstance_ == nullptr || inL == nullptr) return;

        n8_rust_transaural_crosstalk_canceller_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustTransauralCrosstalkCanceller* rustInstance_{ nullptr };
    ProcessSpec spec_{};
    std::array<PinDescriptor, 2> pins_{};
    std::array<ParameterInfo, 5> params_{};
};

inline AutoRegisterNode<TransauralCrosstalkCancellerNode> registerTransauralCrosstalkCanceller(
    NodeType::TransauralCrosstalkCanceller, "transauralcrosstalkcanceller", "Spatial");

} // namespace audio_graph
