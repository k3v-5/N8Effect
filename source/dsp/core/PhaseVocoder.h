#pragma once

#include <vector>
#include <complex>
#include <numbers>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include "FFTEngine.h"
#include "FastMath.h"

namespace audio_graph {

/**
 * @brief Motor STFT Phase Vocoder en tiempo real con Identity Phase Locking (Laroche & Dolson, 1999)
 * para transposición polifónica prístina sin artefactos de phasiness (Reglas 3, 5, 8, 14, 17, 34, 47).
 */
class PhaseVocoder {
public:
    PhaseVocoder() = default;

    void prepare(size_t fftSize = 1024, size_t hopSize = 256, float sampleRate = 44100.0f) {
        fftSize_ = 1;
        while (fftSize_ < fftSize) fftSize_ <<= 1;
        hopSize_ = (hopSize > 0 && hopSize < fftSize_) ? hopSize : (fftSize_ / 4);
        sampleRate_ = sampleRate > 0.0f ? sampleRate : 44100.0f;

        fft_.prepare(fftSize_, hopSize_);

        // Tamaño de ring buffer como potencia de dos >= 4 * fftSize_
        ringSize_ = 1;
        while (ringSize_ < fftSize_ * 4) ringSize_ <<= 1;
        ringMask_ = ringSize_ - 1;

        inputRing_.assign(ringSize_, 0.0f);
        outputRing_.assign(ringSize_, 0.0f);

        analysisFrame_.assign(fftSize_, 0.0f);
        resynthFrame_.assign(fftSize_, 0.0f);
        olaBuffer_.assign(fftSize_ * 2, 0.0f);

        // Ventana Hann periódica simétrica
        window_.resize(fftSize_);
        const float twoPi = 2.0f * std::numbers::pi_v<float>;
        for (size_t i = 0; i < fftSize_; ++i) {
            window_[i] = 0.5f * (1.0f - std::cos(twoPi * static_cast<float>(i) / static_cast<float>(fftSize_)));
        }

        // Para ventana Hann con solapamiento al 75% (hop = N/4), la suma de w^2 es 1.5
        olaNorm_ = 1.0f / 1.5f;

        const size_t numBins = (fftSize_ / 2) + 1;
        currentMag_.assign(numBins, 0.0f);
        currentPhase_.assign(numBins, 0.0f);
        prevInputPhase_.assign(numBins, 0.0f);
        synthPhase_.assign(numBins, 0.0f);
        synthPeakPhase_.assign(numBins, 0.0f);
        omega_.assign(numBins, 0.0f);
        targetMag_.assign(numBins, 0.0f);
        targetPhase_.assign(numBins, 0.0f);

        peaks_.assign(numBins, 0);
        peakForBin_.assign(numBins, 0);

        reset();
    }

    void reset() noexcept {
        fft_.reset();
        std::fill(inputRing_.begin(), inputRing_.end(), 0.0f);
        std::fill(outputRing_.begin(), outputRing_.end(), 0.0f);
        std::fill(analysisFrame_.begin(), analysisFrame_.end(), 0.0f);
        std::fill(resynthFrame_.begin(), resynthFrame_.end(), 0.0f);
        std::fill(olaBuffer_.begin(), olaBuffer_.end(), 0.0f);

        std::fill(currentMag_.begin(), currentMag_.end(), 0.0f);
        std::fill(currentPhase_.begin(), currentPhase_.end(), 0.0f);
        std::fill(prevInputPhase_.begin(), prevInputPhase_.end(), 0.0f);
        std::fill(synthPhase_.begin(), synthPhase_.end(), 0.0f);
        std::fill(synthPeakPhase_.begin(), synthPeakPhase_.end(), 0.0f);
        std::fill(omega_.begin(), omega_.end(), 0.0f);
        std::fill(targetMag_.begin(), targetMag_.end(), 0.0f);
        std::fill(targetPhase_.begin(), targetPhase_.end(), 0.0f);

        std::fill(peaks_.begin(), peaks_.end(), 0);
        std::fill(peakForBin_.begin(), peakForBin_.end(), 0);

        inputWritePos_ = 0;
        outputReadPos_ = 0;
        outputWritePos_ = 0;
        outputAvailable_ = 0;
        hopCounter_ = 0;
    }

    /**
     * @brief Procesa un bloque de audio en tiempo real sin alocaciones dinámicas.
     * @param input Puntero a muestras de entrada (puede ser nulo para silencio).
     * @param output Puntero a muestras de salida.
     * @param numSamples Cantidad de muestras a procesar en el bloque actual.
     * @param pitchRatio Factor multiplicador de frecuencia (e.g. 2.0 = +1 octava, 0.5 = -1 octava).
     * @param phaseLocking Cantidad de Identity Phase Locking [0.0 = suelto/vintage, 1.0 = rígido Laroche-Dolson].
     */
    void process(const float* input, float* output, uint32_t numSamples, float pitchRatio, float phaseLocking = 1.0f) noexcept {
        if (output == nullptr || numSamples == 0 || fftSize_ == 0) return;

        const float clampedPitch = std::clamp(pitchRatio, 0.25f, 4.0f); // +/- 2 octavas
        const float clampedLocking = std::clamp(phaseLocking, 0.0f, 1.0f);
        const size_t numBins = (fftSize_ / 2) + 1;
        const float twoPi = 2.0f * std::numbers::pi_v<float>;
        const float binFreqFactor = twoPi * static_cast<float>(hopSize_) / static_cast<float>(fftSize_);

        for (uint32_t s = 0; s < numSamples; ++s) {
            const float inSample = input != nullptr ? input[s] : 0.0f;
            inputRing_[inputWritePos_] = inSample;
            inputWritePos_ = (inputWritePos_ + 1) & ringMask_;
            hopCounter_++;

            if (hopCounter_ >= hopSize_) {
                hopCounter_ = 0;

                // 1. Extraer ventana temporal de análisis
                const size_t readStart = (inputWritePos_ + ringSize_ - fftSize_) & ringMask_;
                for (size_t i = 0; i < fftSize_; ++i) {
                    analysisFrame_[i] = inputRing_[(readStart + i) & ringMask_] * window_[i];
                }

                // 2. Transformada directa hacia frecuencia
                fft_.forward(analysisFrame_.data(), false);
                auto& spec = fft_.getFrequencyBuffer();

                // 3. Extraer magnitudes y fases
                for (size_t k = 0; k < numBins; ++k) {
                    currentMag_[k] = std::abs(spec[k]);
                    currentPhase_[k] = std::arg(spec[k]);
                }

                // 4. Detección de picos espectrales prominentes
                size_t numPeaks = 0;
                for (size_t k = 1; k + 1 < numBins; ++k) {
                    if (currentMag_[k] > currentMag_[k - 1] &&
                        currentMag_[k] > currentMag_[k + 1] &&
                        currentMag_[k] > 1.0e-5f) {
                        peaks_[numPeaks++] = static_cast<int>(k);
                    }
                }

                // 5. Asignación de regiones de influencia de cada pico
                if (numPeaks == 0) {
                    for (size_t k = 0; k < numBins; ++k) {
                        peakForBin_[k] = static_cast<int>(k);
                    }
                } else {
                    for (size_t p = 0; p < numPeaks; ++p) {
                        const size_t start = (p == 0) ? 0 : (static_cast<size_t>(peaks_[p - 1] + peaks_[p]) / 2 + 1);
                        const size_t end = (p + 1 == numPeaks) ? (numBins - 1) : (static_cast<size_t>(peaks_[p] + peaks_[p + 1]) / 2);
                        for (size_t k = start; k <= end; ++k) {
                            peakForBin_[k] = peaks_[p];
                        }
                    }
                }

                // 6. Propagación de fases para picos (Frecuencia Instantánea Desenroscada)
                for (size_t p = 0; p < numPeaks; ++p) {
                    const int pk = peaks_[p];
                    const float deltaPhi = currentPhase_[pk] - prevInputPhase_[pk];
                    const float nominalOmega = static_cast<float>(pk) * binFreqFactor;
                    const float dev = std::remainder(deltaPhi - nominalOmega, twoPi);
                    const float trueOmega = nominalOmega + dev;
                    omega_[pk] = trueOmega;

                    const float synthAdvance = clampedPitch * trueOmega;
                    synthPeakPhase_[pk] = std::remainder(synthPeakPhase_[pk] + synthAdvance, twoPi);
                }

                // 7. Mapeo espectral con Identity Phase Locking
                for (size_t k = 0; k < numBins; ++k) {
                    const float srcBin = static_cast<float>(k) / clampedPitch;
                    if (srcBin >= static_cast<float>(numBins - 1)) {
                        targetMag_[k] = 0.0f;
                        targetPhase_[k] = 0.0f;
                        continue;
                    }

                    const size_t s0 = static_cast<size_t>(srcBin);
                    const float frac = srcBin - static_cast<float>(s0);
                    targetMag_[k] = (1.0f - frac) * currentMag_[s0] + frac * currentMag_[s0 + 1];

                    // Bloqueo de fase rígido respecto al pico espectral regente
                    const int pk = peakForBin_[s0];
                    const float lockedPhase = std::remainder(synthPeakPhase_[pk] + (currentPhase_[s0] - currentPhase_[pk]), twoPi);

                    // Avance de fase independiente como fallback
                    const float deltaPhiS = currentPhase_[s0] - prevInputPhase_[s0];
                    const float nominalOmegaS = static_cast<float>(s0) * binFreqFactor;
                    const float devS = std::remainder(deltaPhiS - nominalOmegaS, twoPi);
                    const float indepPhase = std::remainder(synthPhase_[k] + clampedPitch * (nominalOmegaS + devS), twoPi);

                    // Mezcla entre bloqueo de fase estricto y suelto
                    targetPhase_[k] = (1.0f - clampedLocking) * indepPhase + clampedLocking * lockedPhase;
                    synthPhase_[k] = targetPhase_[k];
                }

                // Almacenar fase previa para el próximo salto
                std::copy(currentPhase_.begin(), currentPhase_.end(), prevInputPhase_.begin());

                // 8. Reconstrucción espectral simétrica
                for (size_t k = 0; k < numBins; ++k) {
                    spec[k] = std::polar(targetMag_[k], targetPhase_[k]);
                }
                for (size_t k = 1; k < numBins - 1; ++k) {
                    spec[fftSize_ - k] = std::conj(spec[k]);
                }

                // 9. IFFT y síntesis con solapamiento y suma (Overlap-Add)
                fft_.inverse(resynthFrame_.data(), false);

                for (size_t i = 0; i < fftSize_; ++i) {
                    olaBuffer_[i] += resynthFrame_[i] * window_[i] * olaNorm_;
                }

                // 10. Despachar bloque a la cola de salida circular
                for (size_t i = 0; i < hopSize_; ++i) {
                    outputRing_[outputWritePos_] = FastMath::flushDenormal(olaBuffer_[i]);
                    outputWritePos_ = (outputWritePos_ + 1) & ringMask_;
                }
                outputAvailable_ = std::min(ringSize_, outputAvailable_ + hopSize_);

                // Desplazar el buffer OLA por hopSize_
                std::copy(olaBuffer_.begin() + hopSize_, olaBuffer_.begin() + (fftSize_ * 2), olaBuffer_.begin());
                std::fill(olaBuffer_.begin() + (fftSize_ * 2 - hopSize_), olaBuffer_.end(), 0.0f);
            }

            // Lectura de salida sin bloqueos
            if (outputAvailable_ > 0) {
                output[s] = outputRing_[outputReadPos_];
                outputReadPos_ = (outputReadPos_ + 1) & ringMask_;
                --outputAvailable_;
            } else {
                output[s] = 0.0f;
            }
        }
    }

    size_t getFFTSize() const noexcept { return fftSize_; }
    size_t getHopSize() const noexcept { return hopSize_; }
    uint32_t getLatencySamples() const noexcept { return static_cast<uint32_t>(fftSize_); }

private:
    size_t fftSize_{ 1024 };
    size_t hopSize_{ 256 };
    float sampleRate_{ 44100.0f };
    float olaNorm_{ 0.6666667f };

    size_t ringSize_{ 4096 };
    size_t ringMask_{ 4095 };
    size_t inputWritePos_{ 0 };
    size_t outputReadPos_{ 0 };
    size_t outputWritePos_{ 0 };
    size_t outputAvailable_{ 0 };
    size_t hopCounter_{ 0 };

    FFTEngine fft_;
    std::vector<float> inputRing_;
    std::vector<float> outputRing_;
    std::vector<float> analysisFrame_;
    std::vector<float> resynthFrame_;
    std::vector<float> olaBuffer_;
    std::vector<float> window_;

    std::vector<float> currentMag_;
    std::vector<float> currentPhase_;
    std::vector<float> prevInputPhase_;
    std::vector<float> synthPhase_;
    std::vector<float> synthPeakPhase_;
    std::vector<float> omega_;
    std::vector<float> targetMag_;
    std::vector<float> targetPhase_;

    std::vector<int> peaks_;
    std::vector<int> peakForBin_;
};

} // namespace audio_graph
