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
 * @brief Cuantizador de afinación y escalas musicales en tiempo real (Reglas 5, 8, 9, 14, 34, 46, 47).
 * Cuantiza cualquier nota o señal entrante a la escala y tono elegidos mediante pitch-snap musical.
 * Procesamiento acelerado en Rust con buffers circulares y doble cabezal granular.
 */
class MidiScaleQuantizerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        RootKey = 1,        // 0: C, 1: C#, 2: D, ..., 11: B
        Scale = 2,          // 0: Major, 1: Nat Minor, 2: Harm Minor, 3: Dorian, 4: Phrygian, 5: Lydian, 6: Mixolydian, 7: Pent Major, 8: Pent Minor, 9: Blues, 10: Hirajoshi
        Transpose = 3,      // -24 a +24 semitonos
        SnapSpeed = 4,      // 5 a 100 ms de velocidad de ajuste
        Mix = 5             // 0.0 a 1.0
    };

    MidiScaleQuantizerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { RootKey, "Root Key", 0.0f, 0.0f, 11.0f, false };   // C
        params_[1] = { Scale, "Scale", 1.0f, 0.0f, 10.0f, false };        // Menor Natural por defecto
        params_[2] = { Transpose, "Transpose", 0.0f, -24.0f, 24.0f, false };
        params_[3] = { SnapSpeed, "Snap ms", 20.0f, 5.0f, 100.0f, true };
        params_[4] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustCore_ = n8_rust_midi_scale_quantizer_create();
        updateParameters();
    }

    ~MidiScaleQuantizerNode() override {
        if (rustCore_) {
            n8_rust_midi_scale_quantizer_free(rustCore_);
            rustCore_ = nullptr;
        }
    }

    MidiScaleQuantizerNode(const MidiScaleQuantizerNode& other)
        : AudioProcessorNode(other)
        , spec_(other.spec_)
        , targetRootKey_(other.targetRootKey_)
        , targetScale_(other.targetScale_)
        , targetTranspose_(other.targetTranspose_)
        , targetSnapSpeed_(other.targetSnapSpeed_)
        , targetMix_(other.targetMix_)
        , pins_(other.pins_)
        , params_(other.params_)
    {
        rustCore_ = n8_rust_midi_scale_quantizer_create();
        if (rustCore_ && spec_.sampleRate > 0) {
            n8_rust_midi_scale_quantizer_prepare(rustCore_, static_cast<float>(spec_.sampleRate));
        }
        updateParameters();
    }

    MidiScaleQuantizerNode& operator=(const MidiScaleQuantizerNode& other) {
        if (this != &other) {
            AudioProcessorNode::operator=(other);
            spec_ = other.spec_;
            targetRootKey_ = other.targetRootKey_;
            targetScale_ = other.targetScale_;
            targetTranspose_ = other.targetTranspose_;
            targetSnapSpeed_ = other.targetSnapSpeed_;
            targetMix_ = other.targetMix_;
            pins_ = other.pins_;
            params_ = other.params_;
            if (rustCore_) {
                n8_rust_midi_scale_quantizer_free(rustCore_);
            }
            rustCore_ = n8_rust_midi_scale_quantizer_create();
            if (rustCore_ && spec_.sampleRate > 0) {
                n8_rust_midi_scale_quantizer_prepare(rustCore_, static_cast<float>(spec_.sampleRate));
            }
            updateParameters();
        }
        return *this;
    }

    MidiScaleQuantizerNode(MidiScaleQuantizerNode&& other) noexcept
        : AudioProcessorNode(std::move(other))
        , spec_(other.spec_)
        , targetRootKey_(other.targetRootKey_)
        , targetScale_(other.targetScale_)
        , targetTranspose_(other.targetTranspose_)
        , targetSnapSpeed_(other.targetSnapSpeed_)
        , targetMix_(other.targetMix_)
        , pins_(other.pins_)
        , params_(other.params_)
        , rustCore_(other.rustCore_)
    {
        other.rustCore_ = nullptr;
    }

    MidiScaleQuantizerNode& operator=(MidiScaleQuantizerNode&& other) noexcept {
        if (this != &other) {
            AudioProcessorNode::operator=(std::move(other));
            if (rustCore_) {
                n8_rust_midi_scale_quantizer_free(rustCore_);
            }
            spec_ = other.spec_;
            targetRootKey_ = other.targetRootKey_;
            targetScale_ = other.targetScale_;
            targetTranspose_ = other.targetTranspose_;
            targetSnapSpeed_ = other.targetSnapSpeed_;
            targetMix_ = other.targetMix_;
            pins_ = other.pins_;
            params_ = other.params_;
            rustCore_ = other.rustCore_;
            other.rustCore_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustCore_) {
            n8_rust_midi_scale_quantizer_prepare(rustCore_, static_cast<float>(spec.sampleRate));
        }
        updateParameters();
    }

    void reset() override {
        if (rustCore_) {
            n8_rust_midi_scale_quantizer_reset(rustCore_);
        }
    }

    int quantizeNote(int midiNote) const noexcept {
        if (rustCore_) {
            return n8_rust_midi_scale_quantizer_quantize_note(rustCore_, midiNote);
        }
        return midiNote;
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const uint32_t numSamples = context.numSamples;
        const uint32_t numOut = context.numOutputChannels;
        float* outL = (numOut > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (numOut > 1) ? context.outputChannels[1] : nullptr;
        const float* inL = (context.numInputChannels > 0) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : inL;

        if (outL == nullptr || numSamples == 0 || rustCore_ == nullptr) return;

        n8_rust_midi_scale_quantizer_process(rustCore_, inL, inR, outL, outR, numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case RootKey:   targetRootKey_ = std::clamp(value, 0.0f, 11.0f); break;
            case Scale:     targetScale_ = std::clamp(value, 0.0f, 10.0f); break;
            case Transpose: targetTranspose_ = std::clamp(value, -24.0f, 24.0f); break;
            case SnapSpeed: targetSnapSpeed_ = std::clamp(value, 5.0f, 100.0f); break;
            case Mix:       targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
        updateParameters();
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case RootKey:   return targetRootKey_;
            case Scale:     return targetScale_;
            case Transpose: return targetTranspose_;
            case SnapSpeed: return targetSnapSpeed_;
            case Mix:       return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::MidiScaleQuantizer; }
    const char* getName() const override { return "Scale Quantizer"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateParameters() {
        if (rustCore_) {
            n8_rust_midi_scale_quantizer_set_parameters(
                rustCore_,
                targetRootKey_,
                targetScale_,
                targetTranspose_,
                targetSnapSpeed_,
                targetMix_
            );
        }
    }

    ProcessSpec spec_;
    float targetRootKey_{ 0.0f };
    float targetScale_{ 1.0f };
    float targetTranspose_{ 0.0f };
    float targetSnapSpeed_{ 20.0f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;

    RustMidiScaleQuantizer* rustCore_{ nullptr };
};

inline AutoRegisterNode<MidiScaleQuantizerNode> registerMidiScaleQuantizer(NodeType::MidiScaleQuantizer, "midi_scale_quantizer", "MIDI FX");

} // namespace audio_graph
