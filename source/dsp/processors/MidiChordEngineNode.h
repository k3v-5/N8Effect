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
 * @brief Generador polifónico de acordes musicales con rasgueo (Strum) y espacialidad (Reglas 5, 8, 9, 14, 34, 46, 47).
 * Procesamiento acelerado en Rust con osciladores polifónicos no lineales y síntesis estéreo con paneo acústico.
 */
class MidiChordEngineNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        ChordType = 1,  // 0: Octave, 1: Fifth, 2: Major, 3: Minor, 4: Sus2, 5: Sus4, 6: Dom7, 7: Maj7, 8: Min7, 9: Min9, 10: Dim
        Inversion = 2,  // 0: Root, 1: 1st Inv, 2: 2nd Inv
        StrumDelay = 3, // 0 a 80 ms entre notas del acorde
        Spread = 4,     // 0% a 100% dispersión estéreo de voces
        RootNote = 5,   // 36 a 72 (C2 a C5)
        SynthMix = 6    // 0.0 a 1.0
    };

    MidiChordEngineNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { ChordType, "Chord Type", 7.0f, 0.0f, 10.0f, false }; // Maj7 por defecto
        params_[1] = { Inversion, "Inversion", 0.0f, 0.0f, 2.0f, false };
        params_[2] = { StrumDelay, "Strum ms", 15.0f, 0.0f, 80.0f, true };
        params_[3] = { Spread, "Spread", 0.5f, 0.0f, 1.0f, true };
        params_[4] = { RootNote, "Root Note", 48.0f, 36.0f, 72.0f, false }; // C3
        params_[5] = { SynthMix, "Synth Mix", 0.8f, 0.0f, 1.0f, true };

        rustCore_ = n8_rust_midi_chord_engine_create();
        updateParameters();
    }

    ~MidiChordEngineNode() override {
        if (rustCore_) {
            n8_rust_midi_chord_engine_free(rustCore_);
            rustCore_ = nullptr;
        }
    }

    MidiChordEngineNode(const MidiChordEngineNode& other)
        : AudioProcessorNode(other)
        , spec_(other.spec_)
        , targetChordType_(other.targetChordType_)
        , targetInversion_(other.targetInversion_)
        , targetStrumDelay_(other.targetStrumDelay_)
        , targetSpread_(other.targetSpread_)
        , targetRootNote_(other.targetRootNote_)
        , targetSynthMix_(other.targetSynthMix_)
        , pins_(other.pins_)
        , params_(other.params_)
    {
        rustCore_ = n8_rust_midi_chord_engine_create();
        if (rustCore_ && spec_.sampleRate > 0) {
            n8_rust_midi_chord_engine_prepare(rustCore_, static_cast<float>(spec_.sampleRate));
        }
        updateParameters();
    }

    MidiChordEngineNode& operator=(const MidiChordEngineNode& other) {
        if (this != &other) {
            AudioProcessorNode::operator=(other);
            spec_ = other.spec_;
            targetChordType_ = other.targetChordType_;
            targetInversion_ = other.targetInversion_;
            targetStrumDelay_ = other.targetStrumDelay_;
            targetSpread_ = other.targetSpread_;
            targetRootNote_ = other.targetRootNote_;
            targetSynthMix_ = other.targetSynthMix_;
            pins_ = other.pins_;
            params_ = other.params_;
            if (rustCore_) {
                n8_rust_midi_chord_engine_free(rustCore_);
            }
            rustCore_ = n8_rust_midi_chord_engine_create();
            if (rustCore_ && spec_.sampleRate > 0) {
                n8_rust_midi_chord_engine_prepare(rustCore_, static_cast<float>(spec_.sampleRate));
            }
            updateParameters();
        }
        return *this;
    }

    MidiChordEngineNode(MidiChordEngineNode&& other) noexcept
        : AudioProcessorNode(std::move(other))
        , spec_(other.spec_)
        , targetChordType_(other.targetChordType_)
        , targetInversion_(other.targetInversion_)
        , targetStrumDelay_(other.targetStrumDelay_)
        , targetSpread_(other.targetSpread_)
        , targetRootNote_(other.targetRootNote_)
        , targetSynthMix_(other.targetSynthMix_)
        , pins_(other.pins_)
        , params_(other.params_)
        , rustCore_(other.rustCore_)
    {
        other.rustCore_ = nullptr;
    }

    MidiChordEngineNode& operator=(MidiChordEngineNode&& other) noexcept {
        if (this != &other) {
            AudioProcessorNode::operator=(std::move(other));
            if (rustCore_) {
                n8_rust_midi_chord_engine_free(rustCore_);
            }
            spec_ = other.spec_;
            targetChordType_ = other.targetChordType_;
            targetInversion_ = other.targetInversion_;
            targetStrumDelay_ = other.targetStrumDelay_;
            targetSpread_ = other.targetSpread_;
            targetRootNote_ = other.targetRootNote_;
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
            n8_rust_midi_chord_engine_prepare(rustCore_, static_cast<float>(spec.sampleRate));
        }
        updateParameters();
    }

    void reset() override {
        if (rustCore_) {
            n8_rust_midi_chord_engine_reset(rustCore_);
        }
    }

    void triggerChord() noexcept {
        if (rustCore_) {
            n8_rust_midi_chord_engine_trigger_chord(rustCore_);
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

        n8_rust_midi_chord_engine_process(rustCore_, inL, inR, outL, outR, numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case ChordType:  targetChordType_ = std::clamp(value, 0.0f, 10.0f); break;
            case Inversion:  targetInversion_ = std::clamp(value, 0.0f, 2.0f); break;
            case StrumDelay: targetStrumDelay_ = std::clamp(value, 0.0f, 80.0f); break;
            case Spread:     targetSpread_ = std::clamp(value, 0.0f, 1.0f); break;
            case RootNote:   targetRootNote_ = std::clamp(value, 36.0f, 72.0f); break;
            case SynthMix:   targetSynthMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
        updateParameters();
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case ChordType:  return targetChordType_;
            case Inversion:  return targetInversion_;
            case StrumDelay: return targetStrumDelay_;
            case Spread:     return targetSpread_;
            case RootNote:   return targetRootNote_;
            case SynthMix:   return targetSynthMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::MidiChordEngine; }
    const char* getName() const override { return "MIDI Chord Engine"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateParameters() {
        if (rustCore_) {
            n8_rust_midi_chord_engine_set_parameters(
                rustCore_,
                targetChordType_,
                targetInversion_,
                targetStrumDelay_,
                targetSpread_,
                targetRootNote_,
                targetSynthMix_
            );
        }
    }

    ProcessSpec spec_;

    float targetChordType_{ 7.0f };
    float targetInversion_{ 0.0f };
    float targetStrumDelay_{ 15.0f };
    float targetSpread_{ 0.5f };
    float targetRootNote_{ 48.0f };
    float targetSynthMix_{ 0.8f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;

    RustMidiChordEngine* rustCore_{ nullptr };
};

inline AutoRegisterNode<MidiChordEngineNode> registerMidiChordEngine(NodeType::MidiChordEngine, "midi_chord_engine", "MIDI FX");

} // namespace audio_graph
