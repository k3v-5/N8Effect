#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../core/DelayLine.h"

namespace audio_graph {

/**
 * @brief Cuantizador de afinación y escalas musicales en tiempo real (Reglas 5, 8, 9, 14, 34, 46, 47).
 * Cuantiza cualquier nota o señal entrante a la escala y tono elegidos mediante pitch-snap musical.
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

        updateScaleMask();
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        const size_t maxDelay = static_cast<size_t>(spec.sampleRate * 0.1); // 100 ms buffer
        delayLineL_.prepare(maxDelay);
        delayLineR_.prepare(maxDelay);
        phase_ = 0.0f;
        smoothedShift_ = 1.0f;
        updateScaleMask();
    }

    void reset() override {
        delayLineL_.reset();
        delayLineR_.reset();
        phase_ = 0.0f;
        smoothedShift_ = 1.0f;
    }

    int quantizeNote(int midiNote) const noexcept {
        const int transposed = midiNote + static_cast<int>(std::round(targetTranspose_));
        const int root = static_cast<int>(std::round(targetRootKey_));

        int bestNote = transposed;
        int minDistance = 999;

        // Buscar la nota más cercana permitida en la escala activa
        for (int note = transposed - 6; note <= transposed + 6; ++note) {
            const int pitchClass = ((note - root) % 12 + 12) % 12;
            if (activeScaleMask_[pitchClass]) {
                const int dist = std::abs(note - transposed);
                if (dist < minDistance) {
                    minDistance = dist;
                    bestNote = note;
                }
            }
        }

        return bestNote;
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
        const float grainSize = static_cast<float>(sampleRate * 0.04); // 40 ms grain window
        const float invGrain = 1.0f / grainSize;

        const int semitoneShift = static_cast<int>(std::round(targetTranspose_));
        const float targetShiftRatio = std::pow(2.0f, static_cast<float>(semitoneShift) / 12.0f);
        const float rate = 1.0f - targetShiftRatio;

        const float alpha = 1.0f / (static_cast<float>(targetSnapSpeed_ * 0.001 * sampleRate));
        const float mix = targetMix_;

        for (uint32_t s = 0; s < numSamples; ++s) {
            const float rawL = inL ? inL[s] : 0.0f;
            const float rawR = inR ? inR[s] : rawL;

            delayLineL_.write(rawL);
            delayLineR_.write(rawR);

            smoothedShift_ += alpha * (rate - smoothedShift_);

            // Avance del doble cabezal de pitch shifting granular
            phase_ += smoothedShift_;
            if (phase_ >= grainSize) phase_ -= grainSize;
            if (phase_ < 0.0f) phase_ += grainSize;

            const float phase2 = std::fmod(phase_ + grainSize * 0.5f, grainSize);

            // Ventana triangular para crossfade sin clips
            const float w1 = 1.0f - std::abs(2.0f * phase_ * invGrain - 1.0f);
            const float w2 = 1.0f - std::abs(2.0f * phase2 * invGrain - 1.0f);

            const float s1L = delayLineL_.readCubic(phase_);
            const float s2L = delayLineL_.readCubic(phase2);
            const float shiftedL = s1L * w1 + s2L * w2;

            const float s1R = delayLineR_.readCubic(phase_);
            const float s2R = delayLineR_.readCubic(phase2);
            const float shiftedR = s1R * w1 + s2R * w2;

            outL[s] = rawL * (1.0f - mix) + shiftedL * mix;
            if (outR != nullptr) outR[s] = rawR * (1.0f - mix) + shiftedR * mix;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case RootKey:   targetRootKey_ = std::clamp(value, 0.0f, 11.0f); updateScaleMask(); break;
            case Scale:     targetScale_ = std::clamp(value, 0.0f, 10.0f); updateScaleMask(); break;
            case Transpose: targetTranspose_ = std::clamp(value, -24.0f, 24.0f); break;
            case SnapSpeed: targetSnapSpeed_ = std::clamp(value, 5.0f, 100.0f); break;
            case Mix:       targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
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
    void updateScaleMask() noexcept {
        activeScaleMask_.fill(false);
        const int s = static_cast<int>(std::round(targetScale_));

        switch (s) {
            case 0: // Major [0, 2, 4, 5, 7, 9, 11]
                for (int i : { 0, 2, 4, 5, 7, 9, 11 }) activeScaleMask_[i] = true;
                break;
            case 1: // Natural Minor [0, 2, 3, 5, 7, 8, 10]
                for (int i : { 0, 2, 3, 5, 7, 8, 10 }) activeScaleMask_[i] = true;
                break;
            case 2: // Harmonic Minor [0, 2, 3, 5, 7, 8, 11]
                for (int i : { 0, 2, 3, 5, 7, 8, 11 }) activeScaleMask_[i] = true;
                break;
            case 3: // Dorian [0, 2, 3, 5, 7, 9, 10]
                for (int i : { 0, 2, 3, 5, 7, 9, 10 }) activeScaleMask_[i] = true;
                break;
            case 4: // Phrygian [0, 1, 3, 5, 7, 8, 10]
                for (int i : { 0, 1, 3, 5, 7, 8, 10 }) activeScaleMask_[i] = true;
                break;
            case 5: // Lydian [0, 2, 4, 6, 7, 9, 11]
                for (int i : { 0, 2, 4, 6, 7, 9, 11 }) activeScaleMask_[i] = true;
                break;
            case 6: // Mixolydian [0, 2, 4, 5, 7, 9, 10]
                for (int i : { 0, 2, 4, 5, 7, 9, 10 }) activeScaleMask_[i] = true;
                break;
            case 7: // Pentatonic Major [0, 2, 4, 7, 9]
                for (int i : { 0, 2, 4, 7, 9 }) activeScaleMask_[i] = true;
                break;
            case 8: // Pentatonic Minor [0, 3, 5, 7, 10]
                for (int i : { 0, 3, 5, 7, 10 }) activeScaleMask_[i] = true;
                break;
            case 9: // Blues [0, 3, 5, 6, 7, 10]
                for (int i : { 0, 3, 5, 6, 7, 10 }) activeScaleMask_[i] = true;
                break;
            case 10: // Japanese Hirajoshi [0, 2, 3, 7, 8]
                for (int i : { 0, 2, 3, 7, 8 }) activeScaleMask_[i] = true;
                break;
            default:
                for (int i : { 0, 2, 4, 5, 7, 9, 11 }) activeScaleMask_[i] = true;
                break;
        }
    }

    ProcessSpec spec_;
    DelayLine delayLineL_;
    DelayLine delayLineR_;

    std::array<bool, 12> activeScaleMask_{};
    float phase_{ 0.0f };
    float smoothedShift_{ 1.0f };

    float targetRootKey_{ 0.0f };
    float targetScale_{ 1.0f };
    float targetTranspose_{ 0.0f };
    float targetSnapSpeed_{ 20.0f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
};

inline AutoRegisterNode<MidiScaleQuantizerNode> registerMidiScaleQuantizer(NodeType::MidiScaleQuantizer, "midi_scale_quantizer", "MIDI FX");

} // namespace audio_graph
