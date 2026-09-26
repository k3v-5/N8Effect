#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

#if defined(__AVX2__)
#include <immintrin.h>
#endif

namespace audio_graph {

/**
 * @brief Seguidor de Tono / Pitch Tracker en Tiempo Real (Reglas 1, 7, 9, 34, 46, 47).
 * Utiliza el algoritmo YIN / Función de Diferencia Acumulativa Media Normalizada (CMNDF)
 * con aceleración SIMD AVX2 FMA, terminación temprana y refinamiento parabólico sub-sample.
 */
class PitchTracker {
public:
    static constexpr size_t WindowSize = 2048; // Bounded ring buffer prealocado (Regla 47)

    PitchTracker() = default;

    void prepare(double sampleRate) {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;

        buffer_.assign(WindowSize * 3, 0.0f);
        yinBuffer_.assign(WindowSize, 0.0f);
        writeIndex_ = 0;
        bufferedSamples_ = 0;

        minLag_ = static_cast<size_t>(sampleRate_ / 2000.0); // 2000 Hz
        maxLag_ = std::min(WindowSize - 1, static_cast<size_t>(sampleRate_ / 40.0)); // 40 Hz
        if (minLag_ < 2) minLag_ = 2;

        reset();
    }

    void reset() noexcept {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        std::fill(yinBuffer_.begin(), yinBuffer_.end(), 0.0f);
        writeIndex_ = 0;
        bufferedSamples_ = 0;
        currentPitchHz_ = 440.0f;
        targetPitchHz_ = 440.0f;
        clarity_ = 0.0f;
    }

    void process(const float* input, uint32_t numSamples) noexcept {
        if (input == nullptr || numSamples == 0) return;

        // Escribir en el buffer circular con triple espejo para lectura contigua sin saltos
        for (uint32_t s = 0; s < numSamples; ++s) {
            const float val = input[s];
            buffer_[writeIndex_] = val;
            buffer_[writeIndex_ + WindowSize] = val;
            buffer_[writeIndex_ + WindowSize * 2] = val;
            writeIndex_ = (writeIndex_ + 1) % WindowSize;
        }

        bufferedSamples_ += numSamples;
        // Analizar periódicamente (cada ~256 muestras para eficiencia de CPU, Regla 47)
        if (bufferedSamples_ >= 256) {
            bufferedSamples_ = 0;
            estimatePitch();
        }

        // Suavizado anti-click del tono estimado (Regla 35)
        const float alpha = 0.1f;
        currentPitchHz_ += alpha * (targetPitchHz_ - currentPitchHz_);
    }

    float getFundamentalHz() const noexcept { return currentPitchHz_; }
    float getClarity() const noexcept { return clarity_; }

    float getMidiNote() const noexcept {
        if (currentPitchHz_ <= 10.0f) return 0.0f;
        return 69.0f + 12.0f * std::log2(currentPitchHz_ / 440.0f);
    }

    // Retorna el tono normalizado de 0.0 (A0 = 27.5 Hz) a 1.0 (C8 = 4186 Hz) para modulación
    float getPitchNormalized() const noexcept {
        const float midi = getMidiNote();
        return std::clamp((midi - 21.0f) / (108.0f - 21.0f), 0.0f, 1.0f);
    }

private:
    static inline float computeDifferenceForLag(const float* window, size_t tau, size_t W) noexcept {
#if defined(__AVX2__)
        const float* p0 = window;
        const float* pTau = window + tau;

        __m256 acc0 = _mm256_setzero_ps();
        __m256 acc1 = _mm256_setzero_ps();
        __m256 acc2 = _mm256_setzero_ps();
        __m256 acc3 = _mm256_setzero_ps();

        for (size_t j = 0; j < W; j += 32) {
            __m256 w0 = _mm256_loadu_ps(p0 + j);
            __m256 wt0 = _mm256_loadu_ps(pTau + j);
            __m256 d0 = _mm256_sub_ps(w0, wt0);
#if defined(__FMA__) || (defined(_MSC_VER) && defined(__AVX2__))
            acc0 = _mm256_fmadd_ps(d0, d0, acc0);
#else
            acc0 = _mm256_add_ps(acc0, _mm256_mul_ps(d0, d0));
#endif

            __m256 w1 = _mm256_loadu_ps(p0 + j + 8);
            __m256 wt1 = _mm256_loadu_ps(pTau + j + 8);
            __m256 d1 = _mm256_sub_ps(w1, wt1);
#if defined(__FMA__) || (defined(_MSC_VER) && defined(__AVX2__))
            acc1 = _mm256_fmadd_ps(d1, d1, acc1);
#else
            acc1 = _mm256_add_ps(acc1, _mm256_mul_ps(d1, d1));
#endif

            __m256 w2 = _mm256_loadu_ps(p0 + j + 16);
            __m256 wt2 = _mm256_loadu_ps(pTau + j + 16);
            __m256 d2 = _mm256_sub_ps(w2, wt2);
#if defined(__FMA__) || (defined(_MSC_VER) && defined(__AVX2__))
            acc2 = _mm256_fmadd_ps(d2, d2, acc2);
#else
            acc2 = _mm256_add_ps(acc2, _mm256_mul_ps(d2, d2));
#endif

            __m256 w3 = _mm256_loadu_ps(p0 + j + 24);
            __m256 wt3 = _mm256_loadu_ps(pTau + j + 24);
            __m256 d3 = _mm256_sub_ps(w3, wt3);
#if defined(__FMA__) || (defined(_MSC_VER) && defined(__AVX2__))
            acc3 = _mm256_fmadd_ps(d3, d3, acc3);
#else
            acc3 = _mm256_add_ps(acc3, _mm256_mul_ps(d3, d3));
#endif
        }

        __m256 sum01 = _mm256_add_ps(acc0, acc1);
        __m256 sum23 = _mm256_add_ps(acc2, acc3);
        __m256 total = _mm256_add_ps(sum01, sum23);

        __m128 lo = _mm256_castps256_ps128(total);
        __m128 hi = _mm256_extractf128_ps(total, 1);
        __m128 sum4 = _mm_add_ps(lo, hi);
        __m128 sum2 = _mm_add_ps(sum4, _mm_movehl_ps(sum4, sum4));
        __m128 sum1 = _mm_add_ss(sum2, _mm_shuffle_ps(sum2, sum2, 1));
        return _mm_cvtss_f32(sum1);
#else
        const float* p0 = window;
        const float* pTau = window + tau;
        float sum0 = 0.0f, sum1 = 0.0f, sum2 = 0.0f, sum3 = 0.0f;
        for (size_t j = 0; j < W; j += 4) {
            const float d0 = p0[j] - pTau[j];
            const float d1 = p0[j + 1] - pTau[j + 1];
            const float d2 = p0[j + 2] - pTau[j + 2];
            const float d3 = p0[j + 3] - pTau[j + 3];
            sum0 += d0 * d0;
            sum1 += d1 * d1;
            sum2 += d2 * d2;
            sum3 += d3 * d3;
        }
        return (sum0 + sum1) + (sum2 + sum3);
#endif
    }

    void estimatePitch() noexcept {
        const float* window = buffer_.data() + writeIndex_;
        const size_t W = WindowSize / 2;

        // Comprobación de silencio rápida en la ventana de análisis (< -70 dBFS / 0.0003 RMS)
        float windowEnergy = 0.0f;
        for (size_t j = 0; j < W; ++j) {
            windowEnergy += window[j] * window[j];
        }
        if (windowEnergy < static_cast<float>(W) * (0.0003f * 0.0003f)) {
            clarity_ = 0.0f;
            return;
        }

        const float threshold = 0.15f;
        size_t bestTau = 0;
        float minVal = 100.0f;
        size_t minValTau = 0;
        float runningSum = 0.0f;

        yinBuffer_[0] = 1.0f;

        // Búsqueda progresiva YIN con CMNDF y terminación temprana (Reglas 34 y 47)
        for (size_t tau = minLag_; tau <= maxLag_; ++tau) {
            const float diffSum = computeDifferenceForLag(window, tau, W);
            runningSum += diffSum;
            const float cmndf = (runningSum > 1e-6f)
                ? (diffSum * static_cast<float>(tau) / runningSum)
                : 1.0f;
            yinBuffer_[tau] = cmndf;

            if (cmndf < minVal) {
                minVal = cmndf;
                minValTau = tau;
            }

            // Terminación temprana: si CMNDF cae bajo el umbral YIN (0.15),
            // continuar avanzando mientras descienda para localizar el mínimo local del valle
            if (cmndf < threshold) {
                while (tau + 1 <= maxLag_) {
                    const size_t nextTau = tau + 1;
                    const float nextDiff = computeDifferenceForLag(window, nextTau, W);
                    runningSum += nextDiff;
                    const float nextCmndf = (runningSum > 1e-6f)
                        ? (nextDiff * static_cast<float>(nextTau) / runningSum)
                        : 1.0f;
                    yinBuffer_[nextTau] = nextCmndf;

                    if (nextCmndf < minVal) {
                        minVal = nextCmndf;
                        minValTau = nextTau;
                    }

                    if (nextCmndf < yinBuffer_[tau]) {
                        tau = nextTau;
                    } else {
                        // El valle comenzó a ascender: el mínimo local está en tau.
                        // yinBuffer_[tau + 1] ya está calculado en yinBuffer_[nextTau] para la interpolación.
                        break;
                    }
                }
                bestTau = tau;
                break;
            }
        }

        // Si no se encontró ningún punto por debajo de threshold, usar el mínimo absoluto
        if (bestTau == 0 && minValTau >= minLag_) {
            bestTau = minValTau;
        }

        if (bestTau >= minLag_ && bestTau <= maxLag_) {
            // Paso 4: Interpolación parabólica sub-sample para máxima precisión (Regla 34)
            float refinedTau = static_cast<float>(bestTau);
            if (bestTau > minLag_ && bestTau < maxLag_) {
                const float y0 = yinBuffer_[bestTau - 1];
                const float y1 = yinBuffer_[bestTau];
                const float y2 = yinBuffer_[bestTau + 1];
                const float denom = 2.0f * (y0 - 2.0f * y1 + y2);
                if (std::abs(denom) > 1e-6f) {
                    refinedTau += (y0 - y2) / denom;
                }
            }

            if (refinedTau > 0.0f) {
                const float estimatedHz = static_cast<float>(sampleRate_) / refinedTau;
                const float currentVal = yinBuffer_[bestTau];
                clarity_ = std::clamp(1.0f - currentVal, 0.0f, 1.0f);

                if (clarity_ > 0.25f && estimatedHz >= 40.0f && estimatedHz <= 2200.0f) {
                    targetPitchHz_ = estimatedHz;
                }
            }
        }
    }

    double sampleRate_{ 44100.0 };
    std::vector<float> buffer_;
    std::vector<float> yinBuffer_;
    size_t writeIndex_{ 0 };
    uint32_t bufferedSamples_{ 0 };

    size_t minLag_{ 22 };
    size_t maxLag_{ 1102 };

    float currentPitchHz_{ 440.0f };
    float targetPitchHz_{ 440.0f };
    float clarity_{ 0.0f };
};

} // namespace audio_graph
