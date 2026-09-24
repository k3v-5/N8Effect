#pragma once

#include <vector>
#include <complex>
#include <numbers>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace audio_graph {

/**
 * @brief Motor FFT / STFT reutilizable en tiempo real con ventana Hann y síntesis Overlap-Add (Reglas 19, 32, 46)
 */
class FFTEngine {
public:
    FFTEngine() = default;

    void prepare(size_t fftSize = 1024, size_t hopSize = 256) {
        fftSize_ = 1;
        while (fftSize_ < fftSize) fftSize_ <<= 1;
        hopSize_ = hopSize > 0 ? hopSize : (fftSize_ / 4);

        timeDomainInput_.assign(fftSize_, 0.0f);
        timeDomainOutput_.assign(fftSize_, 0.0f);
        fftBuffer_.assign(fftSize_, { 0.0f, 0.0f });

        window_.resize(fftSize_);
        for (size_t i = 0; i < fftSize_; ++i) {
            // Ventana Hann
            window_[i] = 0.5f * (1.0f - std::cos(2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(fftSize_ - 1)));
        }

        olaBuffer_.assign(fftSize_ * 2, 0.0f);
        inputBuffer_.assign(fftSize_, 0.0f);
        inputPos_ = 0;
        outputPos_ = 0;
    }

    void reset() noexcept {
        std::fill(timeDomainInput_.begin(), timeDomainInput_.end(), 0.0f);
        std::fill(timeDomainOutput_.begin(), timeDomainOutput_.end(), 0.0f);
        std::fill(fftBuffer_.begin(), fftBuffer_.end(), std::complex<float>(0.0f, 0.0f));
        std::fill(olaBuffer_.begin(), olaBuffer_.end(), 0.0f);
        std::fill(inputBuffer_.begin(), inputBuffer_.end(), 0.0f);
        inputPos_ = 0;
        outputPos_ = 0;
    }

    // FFT Cooley-Tukey Radix-2 en sitio (in-place)
    static void computeRadix2FFT(std::vector<std::complex<float>>& buffer, bool inverse) noexcept {
        const size_t n = buffer.size();
        if (n <= 1) return;

        // Bit-reversal permutation
        for (size_t i = 1, j = 0; i < n; ++i) {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1) {
                j ^= bit;
            }
            j ^= bit;
            if (i < j) {
                std::swap(buffer[i], buffer[j]);
            }
        }

        // Cooley-Tukey butterfly
        for (size_t len = 2; len <= n; len <<= 1) {
            float angle = 2.0f * std::numbers::pi_v<float> / static_cast<float>(len) * (inverse ? 1.0f : -1.0f);
            std::complex<float> wlen(std::cos(angle), std::sin(angle));
            for (size_t i = 0; i < n; i += len) {
                std::complex<float> w(1.0f, 0.0f);
                for (size_t j = 0; j < len / 2; ++j) {
                    std::complex<float> u = buffer[i + j];
                    std::complex<float> v = buffer[i + j + len / 2] * w;
                    buffer[i + j] = u + v;
                    buffer[i + j + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }

        if (inverse) {
            const float invN = 1.0f / static_cast<float>(n);
            for (auto& x : buffer) {
                x *= invN;
            }
        }
    }

    // Ejecuta análisis hacia el dominio de frecuencia
    void forward(const float* timeInput) noexcept {
        for (size_t i = 0; i < fftSize_; ++i) {
            fftBuffer_[i] = { timeInput[i] * window_[i], 0.0f };
        }
        computeRadix2FFT(fftBuffer_, false);
    }

    // Ejecuta síntesis inversa hacia el dominio de tiempo
    void inverse(float* timeOutput) noexcept {
        computeRadix2FFT(fftBuffer_, true);
        for (size_t i = 0; i < fftSize_; ++i) {
            timeOutput[i] = fftBuffer_[i].real() * window_[i];
        }
    }

    std::vector<std::complex<float>>& getFrequencyBuffer() noexcept { return fftBuffer_; }
    const std::vector<std::complex<float>>& getFrequencyBuffer() const noexcept { return fftBuffer_; }

    size_t getFFTSize() const noexcept { return fftSize_; }
    size_t getHopSize() const noexcept { return hopSize_; }

private:
    size_t fftSize_{ 1024 };
    size_t hopSize_{ 256 };
    size_t inputPos_{ 0 };
    size_t outputPos_{ 0 };

    std::vector<float> timeDomainInput_;
    std::vector<float> timeDomainOutput_;
    std::vector<std::complex<float>> fftBuffer_;
    std::vector<float> window_;
    std::vector<float> olaBuffer_;
    std::vector<float> inputBuffer_;
};

} // namespace audio_graph
