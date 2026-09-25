#pragma once

#include <cmath>
#include <algorithm>
#include "ModulationTypes.h"

namespace audio_graph {

/**
 * @brief Generador Polirrítmico Euclidiano de Bjorklund (Reglas 7, 8, 37, 46, 47)
 * Distribuye K pulsos uniformemente en N pasos generando envolventes dinámicas de modulación.
 */
class EuclideanModulator {
public:
    EuclideanModulator() = default;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;
        updateDecayCoeff();
        reset();
    }

    void reset() noexcept {
        currentEnv_ = 0.0f;
        lastStep_ = -1;
        phase_ = 0.0;
    }

    void setSteps(int steps) noexcept {
        steps_ = std::clamp(steps, 1, 32);
    }

    void setPulses(int pulses) noexcept {
        pulses_ = std::clamp(pulses, 0, steps_);
    }

    void setRotation(int rotation) noexcept {
        rotation_ = (rotation % steps_ + steps_) % steps_;
    }

    void setDecay(float seconds) noexcept {
        decaySeconds_ = std::clamp(seconds, 0.01f, 2.0f);
        updateDecayCoeff();
    }

    void setSync(SyncDivision div, float freeBpm = 120.0f) noexcept {
        syncDiv_ = div;
        freeBpm_ = std::clamp(freeBpm, 20.0f, 300.0f);
    }

    bool hasPulseAtStep(int step) const noexcept {
        if (pulses_ <= 0) return false;
        if (pulses_ >= steps_) return true;

        const int rotated = (step + rotation_) % steps_;
        // Algoritmo matemático directo O(1) de Bjorklund / Bresenham
        return ((rotated * pulses_) % steps_) < pulses_;
    }

    void processSample(double bpm, double ppq, bool isPlaying) noexcept {
        int currentStep = 0;

        if (syncDiv_ != SyncDivision::FreeHz && isPlaying && bpm > 1.0) {
            // Sincronizado a subdivisión métrica del DAW
            double stepsPerBeat = 4.0; // 1/16 por defecto
            switch (syncDiv_) {
                case SyncDivision::Quarter:     stepsPerBeat = 1.0; break;
                case SyncDivision::Eighth:      stepsPerBeat = 2.0; break;
                case SyncDivision::Sixteenth:   stepsPerBeat = 4.0; break;
                case SyncDivision::ThirtySecond:stepsPerBeat = 8.0; break;
                case SyncDivision::Triplet_Eighth: stepsPerBeat = 3.0; break;
                default: stepsPerBeat = 4.0; break;
            }

            double totalSteps = ppq * stepsPerBeat;
            currentStep = static_cast<int>(std::floor(totalSteps)) % steps_;
            if (currentStep < 0) currentStep += steps_;
        } else {
            // Reloj interno autónomo
            double beatRate = (freeBpm_ / 60.0) * 4.0; // 1/16th notes
            phase_ += beatRate / sampleRate_;
            if (phase_ >= 1.0) {
                phase_ = std::fmod(phase_, 1.0);
                internalStep_ = (internalStep_ + 1) % steps_;
            }
            currentStep = internalStep_;
        }

        // Detección de nuevo paso
        if (currentStep != lastStep_) {
            lastStep_ = currentStep;
            if (hasPulseAtStep(currentStep)) {
                // Disparo de pulso
                currentEnv_ = 1.0f;
            }
        }

        // Decaimiento exponencial analógico suave
        currentEnv_ *= decayCoeff_;
        if (currentEnv_ < 1e-5f) currentEnv_ = 0.0f;
    }

    float getCurrentValue() const noexcept { return currentEnv_; }

private:
    void updateDecayCoeff() noexcept {
        decayCoeff_ = std::exp(-1.0f / (decaySeconds_ * static_cast<float>(sampleRate_)));
    }

    double sampleRate_{ 44100.0 };
    int steps_{ 16 };
    int pulses_{ 7 }; // Típico ritmo euclidiano (7 en 16)
    int rotation_{ 0 };
    float decaySeconds_{ 0.15f };
    float decayCoeff_{ 0.999f };

    SyncDivision syncDiv_{ SyncDivision::Sixteenth };
    float freeBpm_{ 120.0f };

    float currentEnv_{ 0.0f };
    int lastStep_{ -1 };
    int internalStep_{ 0 };
    double phase_{ 0.0 };
};

} // namespace audio_graph
