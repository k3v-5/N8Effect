#pragma once

#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Extractor de Descriptores Acústicos Espectrales en Tiempo Real (Reglas 1, 7, 9, 32, 46, 47).
 * Analiza el audio en el dominio de la frecuencia mediante FFT con ventana Hann:
 * - Centroide Espectral (brillo tímbrico normalizado 0.0 a 1.0)
 * - Flujo Espectral (tasa de variación o novedad espectral)
 * - Planitud Espectral (proporción tonal vs ruidosa / inarmonicidad)
 * - Energía (RMS de magnitudes espectrales)
 * Todo el cálculo de FFT y extracción de descriptores se delega al núcleo Rust optimizado
 * con cero alocaciones en el hilo de audio.
 */
class SpectralFeatureExtractor {
public:
    static constexpr size_t DefaultFFTSize = 1024;
    static constexpr size_t DefaultHopSize = 256;

    explicit SpectralFeatureExtractor(RustSpectralFeatureExtractor* existing = nullptr, bool owns = true)
        : rustCore_(existing ? existing : n8_rust_spectral_feature_extractor_create()), ownsCore_(owns)
    {
    }

    ~SpectralFeatureExtractor() {
        if (rustCore_ && ownsCore_) {
            n8_rust_spectral_feature_extractor_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    SpectralFeatureExtractor(const SpectralFeatureExtractor&) = delete;
    SpectralFeatureExtractor& operator=(const SpectralFeatureExtractor&) = delete;

    SpectralFeatureExtractor(SpectralFeatureExtractor&& other) noexcept
        : rustCore_(other.rustCore_), ownsCore_(other.ownsCore_)
    {
        other.rustCore_ = nullptr;
        other.ownsCore_ = false;
    }

    SpectralFeatureExtractor& operator=(SpectralFeatureExtractor&& other) noexcept {
        if (this != &other) {
            if (rustCore_ && ownsCore_) {
                n8_rust_spectral_feature_extractor_destroy(rustCore_);
            }
            rustCore_ = other.rustCore_;
            ownsCore_ = other.ownsCore_;
            other.rustCore_ = nullptr;
            other.ownsCore_ = false;
        }
        return *this;
    }

    void prepare(double sampleRate, size_t fftSize = DefaultFFTSize, size_t hopSize = DefaultHopSize) noexcept {
        if (rustCore_) {
            n8_rust_spectral_feature_extractor_prepare(rustCore_, sampleRate, fftSize, hopSize);
        }
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_spectral_feature_extractor_reset(rustCore_);
        }
    }

    void process(const float* input, uint32_t numSamples, bool isSilent = false) noexcept {
        if (rustCore_) {
            n8_rust_spectral_feature_extractor_process(rustCore_, input, numSamples, isSilent);
        }
    }

    float getSpectralCentroid() const noexcept {
        if (rustCore_) {
            return n8_rust_spectral_feature_extractor_get_centroid(rustCore_);
        }
        return 0.2f;
    }

    float getSpectralFlux() const noexcept {
        if (rustCore_) {
            return n8_rust_spectral_feature_extractor_get_flux(rustCore_);
        }
        return 0.0f;
    }

    float getSpectralFlatness() const noexcept {
        if (rustCore_) {
            return n8_rust_spectral_feature_extractor_get_flatness(rustCore_);
        }
        return 0.0f;
    }

    float getEnergy() const noexcept {
        if (rustCore_) {
            return n8_rust_spectral_feature_extractor_get_energy(rustCore_);
        }
        return 0.0f;
    }

private:
    RustSpectralFeatureExtractor* rustCore_{ nullptr };
    bool ownsCore_{ true };
};

} // namespace audio_graph
