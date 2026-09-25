#pragma once

#include <array>
#include <cmath>
#include <algorithm>
#include "ModulationTypes.h"

namespace audio_graph {

/**
 * @brief Generador de Envolvente Multi-Segmento (MSEG - Multi-Segment Envelope Generator)
 * con curvas Bézier/tensión configurables y sincronización rítmica (Reglas 7, 8, 25, 37, 46, 47)
 */
class MSEGModulator {
public:
    enum class PlayMode : uint8_t {
        Loop = 0,
        OneShot = 1,
        PingPong = 2
    };

    struct MSEGPoint {
        float time{ 0.0f };  // Tiempo normalizado (0.0 a 1.0)
        float level{ 0.0f }; // Nivel de amplitud (-1.0 a +1.0 o 0.0 a 1.0)
        float curve{ 0.0f }; // Tensión de la curva (-1.0 cóncava, 0.0 lineal, +1.0 convexa)
    };

    static constexpr size_t MaxPoints = 16;

    MSEGModulator() {
        setDefaultEnvelope();
    }

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;
        reset();
    }

    void reset() noexcept {
        phase_ = 0.0;
        currentValue_ = points_[0].level;
        pingPongForward_ = true;
        finished_ = false;
    }

    void setDefaultEnvelope() noexcept {
        numPoints_ = 4;
        points_[0] = { 0.0f,  0.0f,  0.0f };  // Inicio
        points_[1] = { 0.25f, 1.0f,  0.25f }; // Ataque
        points_[2] = { 0.75f, 0.4f, -0.2f };  // Caída / Sustain
        points_[3] = { 1.0f,  0.0f, -0.4f };  // Release
    }

    void setPoint(size_t index, float time, float level, float curve) noexcept {
        if (index < MaxPoints) {
            points_[index].time = std::clamp(time, 0.0f, 1.0f);
            points_[index].level = std::clamp(level, -1.0f, 1.0f);
            points_[index].curve = std::clamp(curve, -1.0f, 1.0f);
        }
    }

    void setNumPoints(size_t count) noexcept {
        numPoints_ = std::clamp<size_t>(count, 2, MaxPoints);
    }

    void setMode(PlayMode mode) noexcept { mode_ = mode; }
    void setSync(SyncDivision div, float freeHz = 1.0f) noexcept {
        syncDiv_ = div;
        freeHz_ = std::clamp(freeHz, 0.01f, 40.0f);
    }

    void trigger() noexcept {
        phase_ = 0.0;
        pingPongForward_ = true;
        finished_ = false;
    }

    void processSample(double bpm, double ppq, bool isPlaying) noexcept {
        if (finished_) return;

        // 1. Cálculo del incremento de fase según tempo o Hz libres
        if (syncDiv_ != SyncDivision::FreeHz && isPlaying && bpm > 1.0) {
            // Sincronizado al PPQ del DAW
            double beatsPerCycle = 1.0;
            switch (syncDiv_) {
                case SyncDivision::Bar_1:       beatsPerCycle = 4.0; break;
                case SyncDivision::Half:        beatsPerCycle = 2.0; break;
                case SyncDivision::Quarter:     beatsPerCycle = 1.0; break;
                case SyncDivision::Eighth:      beatsPerCycle = 0.5; break;
                case SyncDivision::Sixteenth:   beatsPerCycle = 0.25; break;
                case SyncDivision::ThirtySecond:beatsPerCycle = 0.125; break;
                case SyncDivision::Triplet_Quarter: beatsPerCycle = 4.0 / 3.0; break;
                case SyncDivision::Triplet_Eighth:  beatsPerCycle = 2.0 / 3.0; break;
                default: beatsPerCycle = 1.0; break;
            }

            double normalizedPos = std::fmod(ppq, beatsPerCycle) / beatsPerCycle;
            if (normalizedPos < 0.0) normalizedPos += 1.0;
            phase_ = normalizedPos;
        } else {
            // Frecuencia libre
            const double phaseIncrement = static_cast<double>(freeHz_) / sampleRate_;
            if (mode_ == PlayMode::PingPong) {
                if (pingPongForward_) {
                    phase_ += phaseIncrement;
                    if (phase_ >= 1.0) {
                        phase_ = 1.0;
                        pingPongForward_ = false;
                    }
                } else {
                    phase_ -= phaseIncrement;
                    if (phase_ <= 0.0) {
                        phase_ = 0.0;
                        pingPongForward_ = true;
                    }
                }
            } else {
                phase_ += phaseIncrement;
                if (phase_ >= 1.0) {
                    if (mode_ == PlayMode::Loop) {
                        phase_ = std::fmod(phase_, 1.0);
                    } else {
                        phase_ = 1.0;
                        finished_ = true;
                    }
                }
            }
        }

        // 2. Evaluar valor del MSEG en la fase actual
        currentValue_ = evaluateAt(static_cast<float>(phase_));
    }

    float getCurrentValue() const noexcept { return currentValue_; }

private:
    float evaluateAt(float t) const noexcept {
        if (numPoints_ < 2) return 0.0f;

        // Si t está antes del primer punto o después del último
        if (t <= points_[0].time) return points_[0].level;
        if (t >= points_[numPoints_ - 1].time) return points_[numPoints_ - 1].level;

        // Encontrar segmento activo [i, i+1]
        size_t segIdx = 0;
        for (size_t i = 0; i < numPoints_ - 1; ++i) {
            if (t >= points_[i].time && t <= points_[i + 1].time) {
                segIdx = i;
                break;
            }
        }

        const auto& p0 = points_[segIdx];
        const auto& p1 = points_[segIdx + 1];

        const float segDuration = p1.time - p0.time;
        if (segDuration <= 1e-6f) return p0.level;

        const float localT = std::clamp((t - p0.time) / segDuration, 0.0f, 1.0f);

        // Curvatura / Tensión no lineal
        float curvedT = localT;
        const float c = p0.curve;
        if (std::abs(c) > 0.001f) {
            if (c > 0.0f) {
                // Convexa (rápido al principio, lento al final)
                const float exponent = 1.0f + 3.0f * c;
                curvedT = 1.0f - std::pow(1.0f - localT, exponent);
            } else {
                // Cóncava (lento al principio, rápido al final)
                const float exponent = 1.0f - 3.0f * c;
                curvedT = std::pow(localT, exponent);
            }
        }

        return (1.0f - curvedT) * p0.level + curvedT * p1.level;
    }

    double sampleRate_{ 44100.0 };
    std::array<MSEGPoint, MaxPoints> points_;
    size_t numPoints_{ 4 };

    PlayMode mode_{ PlayMode::Loop };
    SyncDivision syncDiv_{ SyncDivision::Bar_1 };
    float freeHz_{ 1.0f };

    double phase_{ 0.0 };
    float currentValue_{ 0.0f };
    bool pingPongForward_{ true };
    bool finished_{ false };
};

} // namespace audio_graph
