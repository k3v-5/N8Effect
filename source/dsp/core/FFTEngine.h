#pragma once

#include <vector>
#include <complex>
#include <numbers>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include "FastMath.h"

namespace audio_graph {

/**
 * @brief Motor FFT / STFT reutilizable en tiempo real con ventana Hann, tablas LUT de factores de giro (twiddles),
 * aceleración AVX2 FMA y transformaciones R2C / C2R optimizadas (Reglas 13, 19, 32, 46, 47).
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

        // Ventana Hann
        window_.resize(fftSize_);
        const float twoPi = 2.0f * std::numbers::pi_v<float>;
        for (size_t i = 0; i < fftSize_; ++i) {
            window_[i] = 0.5f * (1.0f - std::cos(twoPi * static_cast<float>(i) / static_cast<float>(fftSize_ - 1)));
        }

        // Precomputar tabla de inversión de bits (Bit-Reversal Permutation)
        bitRevTable_.resize(fftSize_);
        for (size_t i = 0; i < fftSize_; ++i) {
            size_t rev = 0;
            size_t temp = i;
            for (size_t bit = 1; bit < fftSize_; bit <<= 1) {
                rev = (rev << 1) | (temp & 1);
                temp >>= 1;
            }
            bitRevTable_[i] = rev;
        }

        // Precomputar tablas de factores de giro (Twiddle Factors LUT) para Forward e Inverse
        const size_t halfSize = fftSize_ / 2;
        twiddleFwd_.resize(halfSize);
        twiddleInv_.resize(halfSize);
        for (size_t k = 0; k < halfSize; ++k) {
            const float angle = twoPi * static_cast<float>(k) / static_cast<float>(fftSize_);
            const float cosVal = std::cos(angle);
            const float sinVal = std::sin(angle);
            twiddleFwd_[k] = { cosVal, -sinVal }; // e^{-j 2*pi*k / N}
            twiddleInv_[k] = { cosVal, sinVal };  // e^{+j 2*pi*k / N}
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

    // FFT Cooley-Tukey Radix-2 optimizada con LUT precalculada y aceleración AVX2
    void computeFFT(std::vector<std::complex<float>>& buffer, bool inverse) const noexcept {
        const size_t n = buffer.size();
        if (n <= 1) return;

        if (n != fftSize_) {
            // Fallback si el buffer no coincide con el tamaño preparado
            computeRadix2FFT(buffer, inverse);
            return;
        }

        // 1. Bit-reversal permutation vía LUT O(N) sin operaciones de bits por muestra
        for (size_t i = 0; i < n; ++i) {
            const size_t j = bitRevTable_[i];
            if (i < j) {
                std::swap(buffer[i], buffer[j]);
            }
        }

        // 2. Etapa len = 2 (sin multiplicaciones, w = 1)
        for (size_t i = 0; i < n; i += 2) {
            const auto u = buffer[i];
            const auto v = buffer[i + 1];
            buffer[i] = u + v;
            buffer[i + 1] = u - v;
        }

        // 3. Etapa len = 4 (w = 1 y w = -j, rotaciones directas sin multiplicar)
        if (n >= 4) {
            for (size_t i = 0; i < n; i += 4) {
                // j = 0: w = 1
                const auto u0 = buffer[i];
                const auto v0 = buffer[i + 2];
                buffer[i] = u0 + v0;
                buffer[i + 2] = u0 - v0;

                // j = 1: w = -j (fwd) o +j (inv)
                const auto u1 = buffer[i + 1];
                const auto v1 = buffer[i + 3];
                const std::complex<float> v1_rot = inverse
                    ? std::complex<float>(-v1.imag(), v1.real())
                    : std::complex<float>(v1.imag(), -v1.real());
                buffer[i + 1] = u1 + v1_rot;
                buffer[i + 3] = u1 - v1_rot;
            }
        }

        // 4. Etapas len = 8 hasta n con aceleración AVX2 y Twiddle LUT
        const auto* twiddles = inverse ? twiddleInv_.data() : twiddleFwd_.data();
        for (size_t len = 8; len <= n; len <<= 1) {
            const size_t halfLen = len >> 1;
            const size_t stride = n / len;

            for (size_t i = 0; i < n; i += len) {
#if N8_HAS_AVX2
                size_t j = 0;
                // Mariposa SIMD procesando 4 números complejos (8 floats) por iteración
                for (; j + 3 < halfLen; j += 4) {
                    __m256 u_vec = _mm256_loadu_ps(reinterpret_cast<const float*>(&buffer[i + j]));
                    __m256 v_vec = _mm256_loadu_ps(reinterpret_cast<const float*>(&buffer[i + j + halfLen]));

                    __m256 w_vec;
                    if (stride == 1) {
                        w_vec = _mm256_loadu_ps(reinterpret_cast<const float*>(&twiddles[j]));
                    } else {
                        alignas(32) float tw[8] = {
                            twiddles[j * stride].real(), twiddles[j * stride].imag(),
                            twiddles[(j + 1) * stride].real(), twiddles[(j + 1) * stride].imag(),
                            twiddles[(j + 2) * stride].real(), twiddles[(j + 2) * stride].imag(),
                            twiddles[(j + 3) * stride].real(), twiddles[(j + 3) * stride].imag()
                        };
                        w_vec = _mm256_load_ps(tw);
                    }

                    // Multiplicación compleja: (vr + j vi) * (wr + j wi) = (vr*wr - vi*wi) + j(vr*wi + vi*wr)
                    __m256 v_r = _mm256_moveldup_ps(v_vec);
                    __m256 v_i = _mm256_movehdup_ps(v_vec);
                    __m256 w_swap = _mm256_permute_ps(w_vec, _MM_SHUFFLE(2, 3, 0, 1));

                    __m256 prod1 = _mm256_mul_ps(v_r, w_vec);
                    __m256 prod2 = _mm256_mul_ps(v_i, w_swap);
                    __m256 v_rot = _mm256_addsub_ps(prod1, prod2);

                    __m256 out_u = _mm256_add_ps(u_vec, v_rot);
                    __m256 out_v = _mm256_sub_ps(u_vec, v_rot);

                    _mm256_storeu_ps(reinterpret_cast<float*>(&buffer[i + j]), out_u);
                    _mm256_storeu_ps(reinterpret_cast<float*>(&buffer[i + j + halfLen]), out_v);
                }
                // Cola escalar para los elementos restantes
                for (; j < halfLen; ++j) {
                    const auto w = twiddles[j * stride];
                    const auto u = buffer[i + j];
                    const auto v = buffer[i + j + halfLen] * w;
                    buffer[i + j] = u + v;
                    buffer[i + j + halfLen] = u - v;
                }
#else
                for (size_t j = 0; j < halfLen; ++j) {
                    const auto w = twiddles[j * stride];
                    const auto u = buffer[i + j];
                    const auto v = buffer[i + j + halfLen] * w;
                    buffer[i + j] = u + v;
                    buffer[i + j + halfLen] = u - v;
                }
#endif
            }
        }

        // 5. Normalización 1/N para IFFT
        if (inverse) {
            const float invN = 1.0f / static_cast<float>(n);
#if N8_HAS_AVX2
            const __m256 vInvN = _mm256_set1_ps(invN);
            float* ptr = reinterpret_cast<float*>(buffer.data());
            const size_t totalFloats = n * 2;
            size_t k = 0;
            for (; k + 7 < totalFloats; k += 8) {
                __m256 data = _mm256_loadu_ps(ptr + k);
                _mm256_storeu_ps(ptr + k, _mm256_mul_ps(data, vInvN));
            }
            for (; k < totalFloats; ++k) {
                ptr[k] *= invN;
            }
#else
            for (auto& x : buffer) {
                x *= invN;
            }
#endif
        }
    }

    // FFT Cooley-Tukey Radix-2 estática para compatibilidad retrospectiva
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
            const float angle = 2.0f * std::numbers::pi_v<float> / static_cast<float>(len) * (inverse ? 1.0f : -1.0f);
            const std::complex<float> wlen(std::cos(angle), std::sin(angle));
            for (size_t i = 0; i < n; i += len) {
                std::complex<float> w(1.0f, 0.0f);
                for (size_t j = 0; j < len / 2; ++j) {
                    const std::complex<float> u = buffer[i + j];
                    const std::complex<float> v = buffer[i + j + len / 2] * w;
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
    void forward(const float* timeInput, bool applyWindow = true) noexcept {
        if (applyWindow) {
            for (size_t i = 0; i < fftSize_; ++i) {
                fftBuffer_[i] = { timeInput[i] * window_[i], 0.0f };
            }
        } else {
            for (size_t i = 0; i < fftSize_; ++i) {
                fftBuffer_[i] = { timeInput[i], 0.0f };
            }
        }
        computeFFT(fftBuffer_, false);
    }

    // Ejecuta síntesis inversa hacia el dominio de tiempo
    void inverse(float* timeOutput, bool applyWindow = false) noexcept {
        computeFFT(fftBuffer_, true);
        if (applyWindow) {
            for (size_t i = 0; i < fftSize_; ++i) {
                timeOutput[i] = fftBuffer_[i].real() * window_[i];
            }
        } else {
            for (size_t i = 0; i < fftSize_; ++i) {
                timeOutput[i] = fftBuffer_[i].real();
            }
        }
    }

    // Transformación Real-to-Complex (R2C) empaquetada: devuelve espectro de bins [0, N/2]
    void forwardR2C(const float* timeInput, std::complex<float>* halfSpectrum, bool applyWindow = true) noexcept {
        forward(timeInput, applyWindow);
        const size_t numBins = (fftSize_ / 2) + 1;
        for (size_t k = 0; k < numBins; ++k) {
            halfSpectrum[k] = fftBuffer_[k];
        }
    }

    // Transformación Complex-to-Real (C2R) hermítica: reconstruye N muestras temporales a partir de [0, N/2]
    void inverseC2R(const std::complex<float>* halfSpectrum, float* timeOutput, bool applyWindow = false) noexcept {
        const size_t half = fftSize_ / 2;
        for (size_t k = 0; k <= half; ++k) {
            fftBuffer_[k] = halfSpectrum[k];
        }
        for (size_t k = 1; k < half; ++k) {
            fftBuffer_[fftSize_ - k] = std::conj(halfSpectrum[k]);
        }
        inverse(timeOutput, applyWindow);
    }

    std::vector<std::complex<float>>& getFrequencyBuffer() noexcept { return fftBuffer_; }
    const std::vector<std::complex<float>>& getFrequencyBuffer() const noexcept { return fftBuffer_; }

    size_t getFFTSize() const noexcept { return fftSize_; }
    size_t getHopSize() const noexcept { return hopSize_; }
    const std::vector<float>& getWindow() const noexcept { return window_; }

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

    std::vector<size_t> bitRevTable_;
    std::vector<std::complex<float>> twiddleFwd_;
    std::vector<std::complex<float>> twiddleInv_;
};

} // namespace audio_graph
