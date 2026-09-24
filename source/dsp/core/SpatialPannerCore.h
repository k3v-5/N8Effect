#pragma once

#include <cmath>
#include <numbers>
#include <algorithm>
#include "DelayLine.h"
#include "BiquadFilter.h"

namespace audio_graph {

/**
 * @brief Núcleo DSP de Posicionamiento Espacial 3D y Binaural (Reglas 13, 16, 32, 34, 46, 47).
 * Modela con precisión psicoacústica:
 * - ITD (Interaural Time Difference): Modelo esférico de Woodworth con retardo fraccional Hermite
 * - ILD (Interaural Level Difference): Sombra acústica de la cabeza humana (Head Shadowing)
 * - Atenuación por distancia y absorción atmosférica de alta frecuencia
 * - Claves espectrales de elevación (Pinna notch filtering)
 */
class SpatialPannerCore {
public:
    SpatialPannerCore() = default;

    void prepare(double sampleRate) {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;

        // Bounded preallocated delay lines: 256 samples máx (~5.8 ms a 44.1 kHz, muy superior al ITD máx de 0.7 ms)
        itdDelayLine_[0].prepare(256);
        itdDelayLine_[1].prepare(256);

        updateFilters();
        reset();
    }

    void reset() noexcept {
        itdDelayLine_[0].reset();
        itdDelayLine_[1].reset();
        headShadowFilter_[0].reset();
        headShadowFilter_[1].reset();
        airAbsorptionFilter_[0].reset();
        airAbsorptionFilter_[1].reset();
        pinnaFilter_[0].reset();
        pinnaFilter_[1].reset();

        currentAzimuth_ = targetAzimuth_;
        currentElevation_ = targetElevation_;
        currentDistance_ = targetDistance_;
    }

    void setCoordinates(float azimuthDeg, float elevationDeg, float distanceMeters) noexcept {
        targetAzimuth_ = std::clamp(azimuthDeg, -180.0f, 180.0f);
        targetElevation_ = std::clamp(elevationDeg, -90.0f, 90.0f);
        targetDistance_ = std::clamp(distanceMeters, 0.1f, 10.0f);
        updateFilters();
    }

    float getAzimuth() const noexcept { return targetAzimuth_; }
    float getElevation() const noexcept { return targetElevation_; }
    float getDistance() const noexcept { return targetDistance_; }

    // Procesa un par de muestras estéreo en tiempo real con cero alocaciones (Reglas 9 y 16)
    void processSample(float inL, float inR, float& outL, float& outR) noexcept {
        const float alpha = 0.005f; // Suavizado anti-click de trayectoria (Regla 35)
        currentAzimuth_ += alpha * (targetAzimuth_ - currentAzimuth_);
        currentElevation_ += alpha * (targetElevation_ - currentElevation_);
        currentDistance_ += alpha * (targetDistance_ - currentDistance_);

        // 1. Mono sum / centro espacial de la fuente
        const float srcMono = 0.5f * (inL + inR);

        // 2. Cálculo de ITD (Modelo de Woodworth)
        // Azimut en radianes [-pi, pi]
        const float azRad = currentAzimuth_ * (std::numbers::pi_v<float> / 180.0f);
        const float sinAz = std::sin(std::abs(azRad));
        const float absAz = std::abs(azRad);

        // Retardo Woodworth: deltaT = (r / c) * (sin|theta| + |theta|)
        // r = 0.0875m (radio promedio cabeza), c = 343 m/s -> r/c approx 0.000255 s
        const float maxItdSec = 0.000255f * (sinAz + absAz);
        const float itdSamples = maxItdSec * static_cast<float>(sampleRate_);

        // Escribir en las líneas de retardo interaural
        itdDelayLine_[0].write(srcMono);
        itdDelayLine_[1].write(srcMono);

        // Si azRad > 0: fuente a la derecha -> oído izquierdo es contralateral (recibe retardo)
        // Si azRad < 0: fuente a la izquierda -> oído derecho es contralateral (recibe retardo)
        const float delayL = (azRad > 0.0f) ? itdSamples : 0.0f;
        const float delayR = (azRad < 0.0f) ? itdSamples : 0.0f;

        float sigL = itdDelayLine_[0].readCubic(delayL);
        float sigR = itdDelayLine_[1].readCubic(delayR);

        // 3. ILD (Head Shadowing) en oído contralateral
        sigL = headShadowFilter_[0].processSample(sigL);
        sigR = headShadowFilter_[1].processSample(sigR);

        // 4. Claves de Elevación (Pinna notch filtering)
        sigL = pinnaFilter_[0].processSample(sigL);
        sigR = pinnaFilter_[1].processSample(sigR);

        // 5. Absorción de aire y atenuación por distancia (1/d)
        sigL = airAbsorptionFilter_[0].processSample(sigL);
        sigR = airAbsorptionFilter_[1].processSample(sigR);

        const float distanceGain = 1.0f / std::sqrt(currentDistance_);
        outL = sigL * distanceGain;
        outR = sigR * distanceGain;
    }

private:
    void updateFilters() noexcept {
        if (sampleRate_ <= 0.0) return;

        const float azRad = std::abs(targetAzimuth_ * (std::numbers::pi_v<float> / 180.0f));
        const float sinAz = std::sin(azRad);

        // Frecuencia de corte de sombra de cabeza: cae a medida que aumenta el azimut lateral
        // Oído contralateral atenúa agudos (> 2.5 kHz)
        const float shadowCutoff = 20000.0f * (1.0f - 0.70f * sinAz);

        if (targetAzimuth_ > 0.0f) {
            // Fuente a la derecha: Oído izquierdo (0) recibe sombra
            headShadowFilter_[0].setCoefficients(BiquadFilter::Type::Lowpass, sampleRate_, shadowCutoff, 0.707f);
            headShadowFilter_[1].setCoefficients(BiquadFilter::Type::Allpass, sampleRate_, 1000.0f, 0.707f);
        } else if (targetAzimuth_ < 0.0f) {
            // Fuente a la izquierda: Oído derecho (1) recibe sombra
            headShadowFilter_[0].setCoefficients(BiquadFilter::Type::Allpass, sampleRate_, 1000.0f, 0.707f);
            headShadowFilter_[1].setCoefficients(BiquadFilter::Type::Lowpass, sampleRate_, shadowCutoff, 0.707f);
        } else {
            // Frente: sin sombra
            headShadowFilter_[0].setCoefficients(BiquadFilter::Type::Allpass, sampleRate_, 1000.0f, 0.707f);
            headShadowFilter_[1].setCoefficients(BiquadFilter::Type::Allpass, sampleRate_, 1000.0f, 0.707f);
        }

        // Filtro de absorción atmosférica en función de la distancia (pasa-bajos progresivo)
        const float airCutoff = std::clamp(20000.0f / std::sqrt(targetDistance_), 3000.0f, 20000.0f);
        airAbsorptionFilter_[0].setCoefficients(BiquadFilter::Type::Lowpass, sampleRate_, airCutoff, 0.707f);
        airAbsorptionFilter_[1].setCoefficients(BiquadFilter::Type::Lowpass, sampleRate_, airCutoff, 0.707f);

        // Filtro pinna de elevación (notch centrado entre 6 kHz y 10 kHz según elevación)
        const float pinnaFreq = 8000.0f + targetElevation_ * 25.0f;
        pinnaFilter_[0].setCoefficients(BiquadFilter::Type::Peak, sampleRate_, pinnaFreq, 2.0f, -3.0f);
        pinnaFilter_[1].setCoefficients(BiquadFilter::Type::Peak, sampleRate_, pinnaFreq, 2.0f, -3.0f);
    }

    double sampleRate_{ 44100.0 };
    DelayLine itdDelayLine_[2];
    BiquadFilter headShadowFilter_[2];
    BiquadFilter airAbsorptionFilter_[2];
    BiquadFilter pinnaFilter_[2];

    float targetAzimuth_{ 0.0f };
    float currentAzimuth_{ 0.0f };
    float targetElevation_{ 0.0f };
    float currentElevation_{ 0.0f };
    float targetDistance_{ 1.0f };
    float currentDistance_{ 1.0f };
};

} // namespace audio_graph
