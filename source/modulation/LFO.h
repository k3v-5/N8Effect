#pragma once

#include <cmath>
#include <numbers>
#include <algorithm>
#include "ModulationTypes.h"

namespace audio_graph {

/**
 * @brief Oscilador de Baja Frecuencia (LFO) multi-forma sincronizable (Reglas 7, 28, 35, 37)
 */
class LFO {
public:
    LFO() = default;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;
        phase_ = 0.0;
        currentOutput_ = 0.0f;
    }

    void reset() noexcept {
        phase_ = 0.0;
        currentOutput_ = 0.0f;
        shHoldValue_ = 0.0f;
    }

    void setShape(LFOShape shape) noexcept { shape_ = shape; }
    void setSyncDivision(SyncDivision div) noexcept { syncDiv_ = div; }
    void setFrequencyHz(float freq) noexcept { frequencyHz_ = std::clamp(freq, 0.001f, 100.0f); }
    void setPulseWidth(float pw) noexcept { pulseWidth_ = std::clamp(pw, 0.05f, 0.95f); }

    // Avanza el LFO un paso de muestra y retorna el valor en rango [-1.0, +1.0]
    float processSample(double bpm, double ppqPosition, bool isHostPlaying) noexcept {
        // 1. Determinar el incremento de fase por muestra
        double phaseInc = 0.0;
        if (syncDiv_ == SyncDivision::FreeHz || !isHostPlaying) {
            phaseInc = frequencyHz_ / sampleRate_;
            phase_ += phaseInc;
            if (phase_ >= 1.0) phase_ -= 1.0;
        } else {
            // Sincronización rítmica con el DAW (Regla 37)
            const double beatsPerCycle = getBeatsForDivision(syncDiv_);
            if (beatsPerCycle > 0.0) {
                // Posición de fase derivada directamente del PPQ del host
                double cyclePos = ppqPosition / beatsPerCycle;
                phase_ = cyclePos - std::floor(cyclePos);
            }
        }

        // 2. Generar la forma de onda
        float rawValue = 0.0f;
        const float p = static_cast<float>(phase_);

        switch (shape_) {
            case LFOShape::Sine:
                rawValue = std::sin(p * 2.0f * std::numbers::pi_v<float>);
                break;

            case LFOShape::Triangle:
                rawValue = (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);
                break;

            case LFOShape::SawUp:
                rawValue = 2.0f * p - 1.0f;
                break;

            case LFOShape::SawDown:
                rawValue = 1.0f - 2.0f * p;
                break;

            case LFOShape::Square:
                rawValue = (p < 0.5f) ? 1.0f : -1.0f;
                break;

            case LFOShape::Pulse:
                rawValue = (p < pulseWidth_) ? 1.0f : -1.0f;
                break;

            case LFOShape::SampleAndHold:
                if (p < lastPhase_) { // Nuevo ciclo iniciado
                    // PRNG LCG simple y determinista para tiempo real (Regla 9)
                    rngState_ = rngState_ * 1664525u + 1013904223u;
                    shHoldValue_ = (static_cast<float>(rngState_ & 0xFFFF) / 32767.5f) - 1.0f;
                }
                rawValue = shHoldValue_;
                break;

            case LFOShape::SmoothRandom:
                if (p < lastPhase_) {
                    targetRandom_ = ((rngState_ = rngState_ * 1664525u + 1013904223u) / 2147483648.0f) - 1.0f;
                }
                currentRandom_ += 0.01f * (targetRandom_ - currentRandom_);
                rawValue = currentRandom_;
                break;
        }

        lastPhase_ = p;

        // 3. Suavizado anti-click (Regla 35)
        currentOutput_ += 0.05f * (rawValue - currentOutput_);
        return currentOutput_;
    }

    float getCurrentValue() const noexcept { return currentOutput_; }

private:
    static double getBeatsForDivision(SyncDivision div) noexcept {
        switch (div) {
            case SyncDivision::Bar_1: return 4.0;
            case SyncDivision::Half: return 2.0;
            case SyncDivision::Quarter: return 1.0;
            case SyncDivision::Eighth: return 0.5;
            case SyncDivision::Sixteenth: return 0.25;
            case SyncDivision::ThirtySecond: return 0.125;
            case SyncDivision::Triplet_Quarter: return 1.0 * (2.0 / 3.0);
            case SyncDivision::Triplet_Eighth: return 0.5 * (2.0 / 3.0);
            case SyncDivision::Dotted_Quarter: return 1.5;
            case SyncDivision::Dotted_Eighth: return 0.75;
            default: return 1.0;
        }
    }

    double sampleRate_{ 44100.0 };
    LFOShape shape_{ LFOShape::Sine };
    SyncDivision syncDiv_{ SyncDivision::FreeHz };
    float frequencyHz_{ 1.0f };
    float pulseWidth_{ 0.5f };

    double phase_{ 0.0 };
    float lastPhase_{ 0.0f };
    float currentOutput_{ 0.0f };
    float shHoldValue_{ 0.0f };
    float targetRandom_{ 0.0f };
    float currentRandom_{ 0.0f };
    uint32_t rngState_{ 123456789u };
};

} // namespace audio_graph
