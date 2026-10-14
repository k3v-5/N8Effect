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
 * @brief Arpegiador rítmico sincronizado por PPQ con generador sonoro integrado (Reglas 5, 8, 9, 14, 34, 37, 46, 47).
 * Procesamiento acelerado en Rust con oscilador analógico no lineal y envolvente rítmica ultra-precisa.
 */
class MidiArpeggiatorNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Rate = 1,       // 0: 1/32, 1: 1/16, 2: 1/8, 3: 1/4, 4: 1/2, 5: 1/8T, 6: 1/16D
        Pattern = 2,    // 0: Up, 1: Down, 2: UpDown, 3: Random, 4: Chord
        Octaves = 3,    // 1 a 4
        Gate = 4,       // 10% a 100%
        Swing = 5,      // 0% a 75%
        SynthMix = 6    // 0.0 a 1.0 (Mezcla de generador de arpegio)
    };

    MidiArpeggiatorNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Rate, "Rate", 1.0f, 0.0f, 6.0f, false };       // 1/16 por defecto
        params_[1] = { Pattern, "Pattern", 0.0f, 0.0f, 4.0f, false }; // Up por defecto
        params_[2] = { Octaves, "Octaves", 2.0f, 1.0f, 4.0f, false };
        params_[3] = { Gate, "Gate", 75.0f, 10.0f, 100.0f, true };
        params_[4] = { Swing, "Swing", 0.0f, 0.0f, 75.0f, true };
        params_[5] = { SynthMix, "Synth Mix", 0.8f, 0.0f, 1.0f, true };

        // Acorde base predeterminado (C Minor 9: C3, Eb3, G3, Bb3, D4)
        activeNotes_ = { 48, 51, 55, 58, 62 };
        numActiveNotes_ = 5;

        rustCore_ = n8_rust_midi_arpeggiator_create();
        if (rustCore_) {
            n8_rust_midi_arpeggiator_set_notes(rustCore_, activeNotes_.data(), numActiveNotes_);
        }
        updateParameters();
    }

    ~MidiArpeggiatorNode() override {
        if (rustCore_) {
            n8_rust_midi_arpeggiator_free(rustCore_);
            rustCore_ = nullptr;
        }
    }

    MidiArpeggiatorNode(const MidiArpeggiatorNode& other)
        : AudioProcessorNode(other)
        , spec_(other.spec_)
        , activeNotes_(other.activeNotes_)
        , numActiveNotes_(other.numActiveNotes_)
        , targetRate_(other.targetRate_)
        , targetPattern_(other.targetPattern_)
        , targetOctaves_(other.targetOctaves_)
        , targetGate_(other.targetGate_)
        , targetSwing_(other.targetSwing_)
        , targetSynthMix_(other.targetSynthMix_)
        , pins_(other.pins_)
        , params_(other.params_)
    {
        rustCore_ = n8_rust_midi_arpeggiator_create();
        if (rustCore_) {
            if (spec_.sampleRate > 0) {
                n8_rust_midi_arpeggiator_prepare(rustCore_, static_cast<float>(spec_.sampleRate));
            }
            n8_rust_midi_arpeggiator_set_notes(rustCore_, activeNotes_.data(), numActiveNotes_);
        }
        updateParameters();
    }

    MidiArpeggiatorNode& operator=(const MidiArpeggiatorNode& other) {
        if (this != &other) {
            AudioProcessorNode::operator=(other);
            spec_ = other.spec_;
            activeNotes_ = other.activeNotes_;
            numActiveNotes_ = other.numActiveNotes_;
            targetRate_ = other.targetRate_;
            targetPattern_ = other.targetPattern_;
            targetOctaves_ = other.targetOctaves_;
            targetGate_ = other.targetGate_;
            targetSwing_ = other.targetSwing_;
            targetSynthMix_ = other.targetSynthMix_;
            pins_ = other.pins_;
            params_ = other.params_;
            if (rustCore_) {
                n8_rust_midi_arpeggiator_free(rustCore_);
            }
            rustCore_ = n8_rust_midi_arpeggiator_create();
            if (rustCore_) {
                if (spec_.sampleRate > 0) {
                    n8_rust_midi_arpeggiator_prepare(rustCore_, static_cast<float>(spec_.sampleRate));
                }
                n8_rust_midi_arpeggiator_set_notes(rustCore_, activeNotes_.data(), numActiveNotes_);
            }
            updateParameters();
        }
        return *this;
    }

    MidiArpeggiatorNode(MidiArpeggiatorNode&& other) noexcept
        : AudioProcessorNode(std::move(other))
        , spec_(other.spec_)
        , activeNotes_(other.activeNotes_)
        , numActiveNotes_(other.numActiveNotes_)
        , targetRate_(other.targetRate_)
        , targetPattern_(other.targetPattern_)
        , targetOctaves_(other.targetOctaves_)
        , targetGate_(other.targetGate_)
        , targetSwing_(other.targetSwing_)
        , targetSynthMix_(other.targetSynthMix_)
        , pins_(other.pins_)
        , params_(other.params_)
        , rustCore_(other.rustCore_)
    {
        other.rustCore_ = nullptr;
    }

    MidiArpeggiatorNode& operator=(MidiArpeggiatorNode&& other) noexcept {
        if (this != &other) {
            AudioProcessorNode::operator=(std::move(other));
            if (rustCore_) {
                n8_rust_midi_arpeggiator_free(rustCore_);
            }
            spec_ = other.spec_;
            activeNotes_ = other.activeNotes_;
            numActiveNotes_ = other.numActiveNotes_;
            targetRate_ = other.targetRate_;
            targetPattern_ = other.targetPattern_;
            targetOctaves_ = other.targetOctaves_;
            targetGate_ = other.targetGate_;
            targetSwing_ = other.targetSwing_;
            targetSynthMix_ = other.targetSynthMix_;
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
            n8_rust_midi_arpeggiator_prepare(rustCore_, static_cast<float>(spec.sampleRate));
            n8_rust_midi_arpeggiator_set_notes(rustCore_, activeNotes_.data(), numActiveNotes_);
        }
        updateParameters();
    }

    void reset() override {
        if (rustCore_) {
            n8_rust_midi_arpeggiator_reset(rustCore_);
        }
    }

    void setNotes(const int* notes, size_t count) noexcept {
        numActiveNotes_ = std::min(count, activeNotes_.size());
        for (size_t i = 0; i < numActiveNotes_; ++i) {
            activeNotes_[i] = notes[i];
        }
        if (numActiveNotes_ == 0) {
            activeNotes_[0] = 48;
            numActiveNotes_ = 1;
        }
        if (rustCore_) {
            n8_rust_midi_arpeggiator_set_notes(rustCore_, activeNotes_.data(), numActiveNotes_);
        }
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

        n8_rust_midi_arpeggiator_process(
            rustCore_,
            inL,
            inR,
            outL,
            outR,
            numSamples,
            context.ppqPosition,
            context.bpm
        );
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Rate: targetRate_ = std::clamp(value, 0.0f, 6.0f); break;
            case Pattern: targetPattern_ = std::clamp(value, 0.0f, 4.0f); break;
            case Octaves: targetOctaves_ = std::clamp(value, 1.0f, 4.0f); break;
            case Gate: targetGate_ = std::clamp(value, 10.0f, 100.0f); break;
            case Swing: targetSwing_ = std::clamp(value, 0.0f, 75.0f); break;
            case SynthMix: targetSynthMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
        updateParameters();
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Rate: return targetRate_;
            case Pattern: return targetPattern_;
            case Octaves: return targetOctaves_;
            case Gate: return targetGate_;
            case Swing: return targetSwing_;
            case SynthMix: return targetSynthMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::MidiArpeggiator; }
    const char* getName() const override { return "MIDI Arpeggiator"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateParameters() {
        if (rustCore_) {
            n8_rust_midi_arpeggiator_set_parameters(
                rustCore_,
                targetRate_,
                targetPattern_,
                targetOctaves_,
                targetGate_,
                targetSwing_,
                targetSynthMix_
            );
        }
    }

    ProcessSpec spec_;
    std::array<int, 16> activeNotes_{ 48, 51, 55, 58, 62 };
    size_t numActiveNotes_{ 5 };

    float targetRate_{ 1.0f };
    float targetPattern_{ 0.0f };
    float targetOctaves_{ 2.0f };
    float targetGate_{ 75.0f };
    float targetSwing_{ 0.0f };
    float targetSynthMix_{ 0.8f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;

    RustMidiArpeggiator* rustCore_{ nullptr };
};

inline AutoRegisterNode<MidiArpeggiatorNode> registerMidiArpeggiator(NodeType::MidiArpeggiator, "midi_arpeggiator", "MIDI FX");

} // namespace audio_graph
