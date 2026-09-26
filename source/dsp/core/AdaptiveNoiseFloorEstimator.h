#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>
#include "FastMath.h"

namespace audio_graph {

/**
 * @brief Estimador Adaptativo de Piso de Ruido y Disparador Schmitt de Histéresis (Reglas 1, 3, 9, 27, 47).
 * - Realiza seguimiento continuo de mínimos estadísticos del ruido ambiente de entrada.
 * - Elimina el "chattering" y cortes espurios en colas de notas y decaimientos naturales.
 * - Cero alocaciones dinámicas en el audio thread.
 */
class AdaptiveNoiseFloorEstimator {
public:
    static constexpr float MinNoiseFloor = 1.58489e-5f; // -96 dBFS
    static constexpr float MaxNoiseFloor = 0.01f;       // -40 dBFS
    static constexpr float DefaultFloor  = 1.0e-4f;      // -80 dBFS

    AdaptiveNoiseFloorEstimator() noexcept {
        reset();
    }

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 44100.0;
        // Ataque lento hacia arriba (1.5 segundos) para no seguir la música
        attackCoeff_ = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate_ * 1.5)));
        // Decaimiento rápido hacia mínimos (0.06 segundos) para adquirir pausas
        decayCoeff_  = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate_ * 0.06)));
        reset();
    }

    void reset() noexcept {
        currentFloor_ = DefaultFloor;
        smoothedLevel_ = 0.0f;
        sourceActive_ = false;
        silentBlockCounter_ = 0;
    }

    /**
     * @brief Actualiza la estimación del piso de ruido a partir de la energía RMS del bloque.
     */
    void updateBlock(float blockRms, uint32_t numSamples = 128) noexcept {
        FastMath::flushDenormal(blockRms);

        // Suavizado del nivel para rechazar picos transitorios inmediatos
        const float blockCoeff = static_cast<float>(numSamples) / static_cast<float>(sampleRate_ * 0.05);
        smoothedLevel_ += (blockRms - smoothedLevel_) * std::clamp(blockCoeff, 0.01f, 1.0f);
        FastMath::flushDenormal(smoothedLevel_);

        // Actualizar estimador de mínimos (asymmetric tracker)
        if (smoothedLevel_ < currentFloor_) {
            // Decaimiento rápido hacia el mínimo detectado
            const float alpha = std::clamp(decayCoeff_ * static_cast<float>(numSamples), 0.001f, 0.5f);
            currentFloor_ += (smoothedLevel_ - currentFloor_) * alpha;
        } else {
            // Subida ultra-lenta para adaptarse a cambios en el ambiente acústico
            const float alpha = std::clamp(attackCoeff_ * static_cast<float>(numSamples), 0.00001f, 0.01f);
            currentFloor_ += (smoothedLevel_ - currentFloor_) * alpha;
        }

        // Acotar estrictamente dentro del presupuesto físico
        currentFloor_ = std::clamp(currentFloor_, MinNoiseFloor, MaxNoiseFloor);

        // Máquina de estados de Histéresis Schmitt de dos umbrales:
        // High Threshold = Piso + 8.0 dB (factor 2.5x)
        // Low Threshold  = Piso + 2.0 dB (factor 1.25x)
        const float highThreshold = currentFloor_ * 2.5f;
        const float lowThreshold  = currentFloor_ * 1.25f;

        if (blockRms > highThreshold) {
            sourceActive_ = true;
            silentBlockCounter_ = 0;
        } else if (blockRms < lowThreshold) {
            if (++silentBlockCounter_ >= DebounceBlocks) {
                sourceActive_ = false;
            }
        }
        // En la zona de histéresis [lowThreshold, highThreshold], sourceActive_ mantiene su estado previo
    }

    bool isSourceActive() const noexcept { return sourceActive_; }
    bool isSourceSilent() const noexcept { return !sourceActive_; }
    float getNoiseFloor() const noexcept { return currentFloor_; }
    float getSmoothedLevel() const noexcept { return smoothedLevel_; }

    void setDebounceBlocks(uint32_t blocks) noexcept {
        DebounceBlocks = std::clamp<uint32_t>(blocks, 1, 32);
    }

private:
    double sampleRate_{ 44100.0 };
    float attackCoeff_{ 0.00001f };
    float decayCoeff_{ 0.0005f };
    float currentFloor_{ DefaultFloor };
    float smoothedLevel_{ 0.0f };
    bool sourceActive_{ false };
    uint32_t silentBlockCounter_{ 0 };
    uint32_t DebounceBlocks{ 4 }; // Requiere 4 bloques consecutivos de silencio (~12ms a 128 muestras)
};

} // namespace audio_graph
