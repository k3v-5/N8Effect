#pragma once

#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>
#include "../dsp/core/FFTEngine.h"

namespace audio_graph {

/**
 * @brief Extractor de Descriptores Acústicos Espectrales en Tiempo Real (Reglas 1, 7, 9, 32, 46, 47).
 * Analiza el audio en el dominio de la frecuencia mediante FFT con ventana Hann:
 * - Centroide Espectral (brillo tímbrico normalizado 0.0 a 1.0)
 * - Flujo Espectral (tasa de variación o novedad espectral)
 * - Planitud Espectral (proporción tonal vs ruidosa / inarmonicidad)
 */
class SpectralFeatureExtractor {
public:
    static constexpr size_t DefaultFFTSize = 1024;
    static constexpr size_t DefaultHopSize = 256;

    SpectralFeatureExtractor() = default;

    void prepare(double sampleRate, size_t fftSize = DefaultFFTSize, size_t hopSize = DefaultHopSize) {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;
        fftSize_ = 1;
        while (fftSize_ < fftSize) fftSize_ <<= 1;
        hopSize_ = hopSize > 0 ? hopSize : (fftSize_ / 4);

        fftEngine_.prepare(fftSize_, hopSize_);

        const size_t numBins = fftSize_ / 2 + 1;
        currentMagnitudes_.assign(numBins, 0.0f);
        prevMagnitudes_.assign(numBins, 0.0f);
        timeBuffer_.assign(fftSize_, 0.0f);
        fftComplexBuffer_.assign(fftSize_, { 0.0f, 0.0f });

        window_.resize(fftSize_);
        for (size_t i = 0; i < fftSize_; ++i) {
            window_[i] = 0.5f * (1.0f - std::cos(2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(fftSize_ - 1)));
        }

        hopCounter_ = 0;
        reset();
    }

    void reset() noexcept {
        std::fill(currentMagnitudes_.begin(), currentMagnitudes_.end(), 0.0f);
        std::fill(prevMagnitudes_.begin(), prevMagnitudes_.end(), 0.0f);
        std::fill(timeBuffer_.begin(), timeBuffer_.end(), 0.0f);
        hopCounter_ = 0;
        centroid_ = 0.2f;
        targetCentroid_ = 0.2f;
        flux_ = 0.0f;
        targetFlux_ = 0.0f;
        flatness_ = 0.0f;
        energy_ = 0.0f;
    }

    void process(const float* input, uint32_t numSamples) noexcept {
        if (input == nullptr || numSamples == 0) return;

        for (uint32_t s = 0; s < numSamples; ++s) {
            // Desplazar buffer temporal circular
            timeBuffer_[hopCounter_] = input[s];
            ++hopCounter_;

            if (hopCounter_ >= fftSize_) {
                hopCounter_ = 0;
                computeSpectralDescriptors();
            }
        }

        // Suavizado anti-click de los descriptores espectrales (Regla 35)
        const float alpha = 0.15f;
        centroid_ += alpha * (targetCentroid_ - centroid_);
        flux_ += alpha * (targetFlux_ - flux_);
    }

    float getSpectralCentroid() const noexcept { return centroid_; }
    float getSpectralFlux() const noexcept { return flux_; }
    float getSpectralFlatness() const noexcept { return flatness_; }
    float getEnergy() const noexcept { return energy_; }

private:
    void computeSpectralDescriptors() noexcept {
        const size_t numBins = fftSize_ / 2 + 1;

        // 1. Aplicar ventana Hann y llenar buffer complejo
        for (size_t i = 0; i < fftSize_; ++i) {
            fftComplexBuffer_[i] = { timeBuffer_[i] * window_[i], 0.0f };
        }

        // 2. Ejecutar FFT Radix-2 rápida
        FFTEngine::computeRadix2FFT(fftComplexBuffer_, false);

        // 3. Extraer espectro de magnitud
        float sumMagnitude = 0.0f;
        float weightedSum = 0.0f;
        float fluxDiffSum = 0.0f;
        float logPowerSum = 0.0f;
        float powerSum = 0.0f;

        for (size_t k = 0; k < numBins; ++k) {
            const float real = fftComplexBuffer_[k].real();
            const float imag = fftComplexBuffer_[k].imag();
            const float mag = std::sqrt(real * real + imag * imag);
            currentMagnitudes_[k] = mag;

            sumMagnitude += mag;
            weightedSum += static_cast<float>(k) * mag;

            // Flujo espectral (rectified spectral difference)
            const float diff = std::max(0.0f, mag - prevMagnitudes_[k]);
            fluxDiffSum += diff * diff;

            // Planitud espectral (Power spectrum)
            const float power = mag * mag + 1e-12f;
            logPowerSum += std::log(power);
            powerSum += power;
        }

        // 4. Centroide espectral normalizado (0.0 = DC, 1.0 = Nyquist)
        if (sumMagnitude > 1e-5f) {
            targetCentroid_ = std::clamp((weightedSum / sumMagnitude) / static_cast<float>(numBins - 1), 0.0f, 1.0f);
        }

        // 5. Flujo espectral normalizado
        const float rawFlux = std::sqrt(fluxDiffSum);
        targetFlux_ = std::clamp(rawFlux * 0.1f, 0.0f, 1.0f);

        // 6. Planitud espectral (Wiener entropy)
        const float geomMean = std::exp(logPowerSum / static_cast<float>(numBins));
        const float arithMean = powerSum / static_cast<float>(numBins);
        flatness_ = std::clamp(geomMean / (arithMean + 1e-9f), 0.0f, 1.0f);

        // Energía total RMS del frame
        energy_ = std::clamp(std::sqrt(powerSum / static_cast<float>(numBins)), 0.0f, 1.0f);

        // Guardar para la siguiente iteración
        prevMagnitudes_ = currentMagnitudes_;
    }

    double sampleRate_{ 44100.0 };
    size_t fftSize_{ DefaultFFTSize };
    size_t hopSize_{ DefaultHopSize };
    size_t hopCounter_{ 0 };

    FFTEngine fftEngine_;
    std::vector<float> timeBuffer_;
    std::vector<float> window_;
    std::vector<std::complex<float>> fftComplexBuffer_;
    std::vector<float> currentMagnitudes_;
    std::vector<float> prevMagnitudes_;

    float centroid_{ 0.2f };
    float targetCentroid_{ 0.2f };
    float flux_{ 0.0f };
    float targetFlux_{ 0.0f };
    float flatness_{ 0.0f };
    float energy_{ 0.0f };
};

} // namespace audio_graph
