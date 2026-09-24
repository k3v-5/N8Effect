#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/SaturationFunctions.h"
#include "../core/BiquadFilter.h"

namespace audio_graph {

/**
 * @brief Nodo de Saturación y Distorsión Multimodelo (Reglas 5, 8, 13, 34, 35)
 */
class DistortionNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Drive = 1,
        Mode = 2, // 0: SoftClip, 1: HardClip, 2: Wavefold, 3: Tube, 4: Bitcrush
        ToneHz = 3,
        DryWet = 4
    };

    DistortionNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Drive, "Drive", 2.0f, 1.0f, 50.0f, true };
        params_[1] = { Mode, "Mode", 0.0f, 0.0f, 4.0f, false };
        params_[2] = { ToneHz, "Tone", 10000.0f, 500.0f, 20000.0f, true };
        params_[3] = { DryWet, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        reset();
        updateTone();
    }

    void reset() override {
        toneFilter_[0].reset();
        toneFilter_[1].reset();
    }

    void process(ProcessContext& context) override {
        updateTone();

        const float drive = targetDrive_;
        const int mode = targetMode_;
        const float mix = targetMix_;
        const uint32_t channels = std::min(context.numOutputChannels, static_cast<uint32_t>(2));

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            for (uint32_t ch = 0; ch < channels; ++ch) {
                float in = (ch < context.numInputChannels && context.inputChannels[ch] != nullptr)
                    ? context.inputChannels[ch][s]
                    : 0.0f;

                float wet = 0.0f;
                switch (mode) {
                    case 0: wet = Saturation::softClipTanh(in, drive); break;
                    case 1: wet = Saturation::hardClip(in * drive, 1.0f); break;
                    case 2: wet = Saturation::wavefold(in, drive); break;
                    case 3: wet = Saturation::tube(in, drive); break;
                    case 4: wet = Saturation::bitcrush(in * drive, 4.0f); break;
                    default: wet = Saturation::softClipTanh(in, drive); break;
                }

                // Filtro de tono post-saturación para control de armónicos ásperos
                wet = toneFilter_[ch].processSample(wet);

                context.outputChannels[ch][s] = in * (1.0f - mix) + wet * mix;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Drive: targetDrive_ = std::clamp(value, 1.0f, 50.0f); break;
            case Mode: targetMode_ = std::clamp(static_cast<int>(std::round(value)), 0, 4); break;
            case ToneHz: targetToneHz_ = std::clamp(value, 500.0f, 20000.0f); break;
            case DryWet: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Drive: return targetDrive_;
            case Mode: return static_cast<float>(targetMode_);
            case ToneHz: return targetToneHz_;
            case DryWet: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Distortion; }
    const char* getName() const override { return "Distortion"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateTone() noexcept {
        toneFilter_[0].setCoefficients(BiquadFilter::Type::Lowpass, spec_.sampleRate, targetToneHz_, 0.707f);
        toneFilter_[1].setCoefficients(BiquadFilter::Type::Lowpass, spec_.sampleRate, targetToneHz_, 0.707f);
    }

    ProcessSpec spec_;
    std::array<BiquadFilter, 2> toneFilter_;

    float targetDrive_{ 2.0f };
    int targetMode_{ 0 };
    float targetToneHz_{ 10000.0f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 4> params_;
};

inline AutoRegisterNode<DistortionNode> registerDistortion(NodeType::Distortion, "distortion", "Distortion");

} // namespace audio_graph
