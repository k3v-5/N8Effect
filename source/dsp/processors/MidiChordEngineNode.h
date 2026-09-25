#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include <numbers>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"

namespace audio_graph {

/**
 * @brief Generador polifónico de acordes musicales con rasgueo (Strum) y espacialidad (Reglas 5, 8, 9, 14, 34, 46, 47).
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

    static constexpr size_t MaxVoices = 5;

    struct ChordVoice {
        float phase{ 0.0f };
        float phaseInc{ 0.0f };
        float env{ 0.0f };
        uint32_t delayRemaining{ 0 };
        float panL{ 0.707f };
        float panR{ 0.707f };
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
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        recalculateChord();
        triggerChord();
    }

    void reset() override {
        for (auto& v : voices_) {
            v.phase = 0.0f;
            v.env = 0.0f;
        }
    }

    void triggerChord() noexcept {
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        const uint32_t strumSamples = static_cast<uint32_t>((targetStrumDelay_ * 0.001) * sr);
        const float spread = targetSpread_;

        for (size_t i = 0; i < currentVoiceCount_; ++i) {
            voices_[i].delayRemaining = static_cast<uint32_t>(i) * (strumSamples / static_cast<uint32_t>(std::max<size_t>(currentVoiceCount_, 1)));
            voices_[i].env = 1.0f;

            // Paneo estereofónico alternado para el acorde
            const float pan = (currentVoiceCount_ > 1)
                ? (static_cast<float>(i) / static_cast<float>(currentVoiceCount_ - 1) * 2.0f - 1.0f) * spread
                : 0.0f;
            voices_[i].panL = std::cos((pan + 1.0f) * 0.25f * std::numbers::pi_v<float>);
            voices_[i].panR = std::sin((pan + 1.0f) * 0.25f * std::numbers::pi_v<float>);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const uint32_t numSamples = context.numSamples;
        const uint32_t numOut = context.numOutputChannels;
        float* outL = (numOut > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (numOut > 1) ? context.outputChannels[1] : outL;
        const float* inL = (context.numInputChannels > 0) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : inL;

        if (outL == nullptr || numSamples == 0) return;

        const float mix = targetSynthMix_;

        for (uint32_t s = 0; s < numSamples; ++s) {
            float chordSampleL = 0.0f;
            float chordSampleR = 0.0f;

            for (size_t v = 0; v < currentVoiceCount_; ++v) {
                auto& voice = voices_[v];
                if (voice.delayRemaining > 0) {
                    --voice.delayRemaining;
                    continue;
                }

                voice.phase += voice.phaseInc;
                if (voice.phase >= 1.0f) voice.phase -= 1.0f;

                // Envolvente de decaimiento natural
                voice.env *= 0.99985f;

                // Forma de onda híbrida suave (senoide con segundo armónico cálido)
                const float wave = (std::sin(voice.phase * 2.0f * std::numbers::pi_v<float>) * 0.7f +
                                    std::sin(voice.phase * 4.0f * std::numbers::pi_v<float>) * 0.3f) * voice.env;

                chordSampleL += wave * voice.panL;
                chordSampleR += wave * voice.panR;
            }

            // Normalización suave y saturación analógica
            chordSampleL = std::tanh(chordSampleL * 0.6f);
            chordSampleR = std::tanh(chordSampleR * 0.6f);

            const float dryL = inL ? inL[s] : 0.0f;
            const float dryR = inR ? inR[s] : dryL;

            outL[s] = dryL * (1.0f - mix) + chordSampleL * mix;
            if (outR != nullptr) outR[s] = dryR * (1.0f - mix) + chordSampleR * mix;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case ChordType:  targetChordType_ = std::clamp(value, 0.0f, 10.0f); recalculateChord(); break;
            case Inversion:  targetInversion_ = std::clamp(value, 0.0f, 2.0f); recalculateChord(); break;
            case StrumDelay: targetStrumDelay_ = std::clamp(value, 0.0f, 80.0f); break;
            case Spread:     targetSpread_ = std::clamp(value, 0.0f, 1.0f); break;
            case RootNote:   targetRootNote_ = std::clamp(value, 36.0f, 72.0f); recalculateChord(); break;
            case SynthMix:   targetSynthMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
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
    void recalculateChord() noexcept {
        const int type = static_cast<int>(std::round(targetChordType_));
        const int root = static_cast<int>(std::round(targetRootNote_));
        const int inv = static_cast<int>(std::round(targetInversion_));

        std::array<int, MaxVoices> intervals{};
        size_t count = 0;

        switch (type) {
            case 0: intervals = { 0, 12, 24, 0, 0 }; count = 3; break;       // Octave
            case 1: intervals = { 0, 7, 12, 19, 0 }; count = 4; break;       // Fifth
            case 2: intervals = { 0, 4, 7, 12, 0 }; count = 4; break;        // Major
            case 3: intervals = { 0, 3, 7, 12, 0 }; count = 4; break;        // Minor
            case 4: intervals = { 0, 2, 7, 12, 0 }; count = 4; break;        // Sus2
            case 5: intervals = { 0, 5, 7, 12, 0 }; count = 4; break;        // Sus4
            case 6: intervals = { 0, 4, 7, 10, 0 }; count = 4; break;        // Dom7
            case 7: intervals = { 0, 4, 7, 11, 0 }; count = 4; break;        // Maj7
            case 8: intervals = { 0, 3, 7, 10, 0 }; count = 4; break;        // Min7
            case 9: intervals = { 0, 3, 7, 10, 14 }; count = 5; break;       // Min9
            case 10: intervals = { 0, 3, 6, 12, 0 }; count = 4; break;       // Diminished
            default: intervals = { 0, 4, 7, 11, 0 }; count = 4; break;
        }

        // Aplicar inversión
        for (int i = 0; i < inv && count > 1; ++i) {
            intervals[0] += 12;
            std::rotate(intervals.begin(), intervals.begin() + 1, intervals.begin() + count);
        }

        currentVoiceCount_ = count;
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;

        for (size_t i = 0; i < count; ++i) {
            const int note = root + intervals[i];
            const float freq = 440.0f * std::pow(2.0f, static_cast<float>(note - 69) / 12.0f);
            voices_[i].phaseInc = freq / static_cast<float>(sr);
        }
    }

    ProcessSpec spec_;
    std::array<ChordVoice, MaxVoices> voices_{};
    size_t currentVoiceCount_{ 4 };

    float targetChordType_{ 7.0f };
    float targetInversion_{ 0.0f };
    float targetStrumDelay_{ 15.0f };
    float targetSpread_{ 0.5f };
    float targetRootNote_{ 48.0f };
    float targetSynthMix_{ 0.8f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<MidiChordEngineNode> registerMidiChordEngine(NodeType::MidiChordEngine, "midi_chord_engine", "MIDI FX");

} // namespace audio_graph
