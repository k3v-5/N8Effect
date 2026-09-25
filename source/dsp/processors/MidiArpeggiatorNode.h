#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include <numbers>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../core/FastMath.h"

namespace audio_graph {

/**
 * @brief Arpegiador rítmico sincronizado por PPQ con generador sonoro integrado (Reglas 5, 8, 9, 14, 34, 37, 46, 47).
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
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        oscPhase_ = 0.0f;
        envLevel_ = 0.0f;
        lastStep_ = -1;
        currentNote_ = activeNotes_[0];
    }

    void reset() override {
        oscPhase_ = 0.0f;
        envLevel_ = 0.0f;
        lastStep_ = -1;
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

        const double sampleRate = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        const double beatsPerStep = getBeatsPerStep();
        const int totalOctaves = std::clamp(static_cast<int>(targetOctaves_), 1, 4);
        const float gateRatio = std::clamp(targetGate_ * 0.01f, 0.05f, 0.99f);
        const float mix = targetSynthMix_;

        // Posición PPQ continua del bloque
        double currentPpq = context.ppqPosition;
        const double ppqPerSample = (context.bpm / 60.0) / sampleRate;

        for (uint32_t s = 0; s < numSamples; ++s) {
            const double posInStep = std::fmod(currentPpq, beatsPerStep) / beatsPerStep;
            const int64_t stepIdx = static_cast<int64_t>(std::floor(currentPpq / beatsPerStep));

            if (stepIdx != lastStep_) {
                lastStep_ = stepIdx;
                currentNote_ = computeArpNote(stepIdx, totalOctaves);
                envLevel_ = 1.0f; // Disparo de ataque
            }

            // Envolvente rápida percusiva (decay / gate)
            if (posInStep > gateRatio) {
                envLevel_ *= 0.992f; // Release rápido al terminar el gate
            } else {
                envLevel_ *= 0.9995f; // Decay suave
            }

            // Generador tímbrico (onda analógica suave enriquecida)
            const float freq = 440.0f * std::pow(2.0f, static_cast<float>(currentNote_ - 69) / 12.0f);
            const float phaseInc = freq / static_cast<float>(sampleRate);

            oscPhase_ += phaseInc;
            if (oscPhase_ >= 1.0f) oscPhase_ -= 1.0f;

            // Onda Saw saturada y filtrada para timbre musical agradable
            float synthSample = (2.0f * oscPhase_ - 1.0f) * envLevel_;
            synthSample = std::tanh(synthSample * 1.5f);

            // Mezcla con audio de entrada si existe
            const float dryL = inL ? inL[s] : 0.0f;
            const float dryR = inR ? inR[s] : dryL;

            outL[s] = dryL * (1.0f - mix) + synthSample * mix;
            if (outR != nullptr) outR[s] = dryR * (1.0f - mix) + synthSample * mix;

            currentPpq += ppqPerSample;
        }
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
    double getBeatsPerStep() const noexcept {
        const int r = static_cast<int>(std::round(targetRate_));
        switch (r) {
            case 0: return 0.125;  // 1/32
            case 1: return 0.25;   // 1/16
            case 2: return 0.5;    // 1/8
            case 3: return 1.0;    // 1/4
            case 4: return 2.0;    // 1/2
            case 5: return 0.5 * (2.0 / 3.0); // 1/8T
            case 6: return 0.25 * 1.5;         // 1/16D
            default: return 0.25;
        }
    }

    int computeArpNote(int64_t stepIdx, int totalOctaves) noexcept {
        if (numActiveNotes_ == 0) return 60;

        const size_t totalNotes = numActiveNotes_ * static_cast<size_t>(totalOctaves);
        const int pattern = static_cast<int>(std::round(targetPattern_));
        size_t noteIdx = 0;

        switch (pattern) {
            case 0: // Up
                noteIdx = static_cast<size_t>((stepIdx % totalNotes + totalNotes) % totalNotes);
                break;
            case 1: { // Down
                const size_t pos = static_cast<size_t>((stepIdx % totalNotes + totalNotes) % totalNotes);
                noteIdx = totalNotes - 1 - pos;
                break;
            }
            case 2: { // UpDown
                const size_t cycle = (totalNotes > 1) ? (totalNotes * 2 - 2) : 1;
                const size_t pos = static_cast<size_t>((stepIdx % cycle + cycle) % cycle);
                noteIdx = (pos < totalNotes) ? pos : (cycle - pos);
                break;
            }
            case 3: { // Random
                // Generador pseudo-aleatorio lineal determinista
                const uint32_t r = static_cast<uint32_t>(stepIdx * 1103515245u + 12345u);
                noteIdx = (r >> 16) % totalNotes;
                break;
            }
            case 4: // Chord root / As-Played
            default:
                noteIdx = static_cast<size_t>((stepIdx % numActiveNotes_ + numActiveNotes_) % numActiveNotes_);
                break;
        }

        const size_t baseNoteIdx = noteIdx % numActiveNotes_;
        const int octaveOffset = static_cast<int>(noteIdx / numActiveNotes_) * 12;
        return activeNotes_[baseNoteIdx] + octaveOffset;
    }

    ProcessSpec spec_;
    std::array<int, 16> activeNotes_{ 48, 51, 55, 58, 62 };
    size_t numActiveNotes_{ 5 };

    int currentNote_{ 48 };
    int64_t lastStep_{ -1 };
    float oscPhase_{ 0.0f };
    float envLevel_{ 0.0f };

    float targetRate_{ 1.0f };
    float targetPattern_{ 0.0f };
    float targetOctaves_{ 2.0f };
    float targetGate_{ 75.0f };
    float targetSwing_{ 0.0f };
    float targetSynthMix_{ 0.8f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<MidiArpeggiatorNode> registerMidiArpeggiator(NodeType::MidiArpeggiator, "midi_arpeggiator", "MIDI FX");

} // namespace audio_graph
