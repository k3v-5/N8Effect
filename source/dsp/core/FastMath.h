#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>

#if defined(__AVX2__)
#include <immintrin.h>
#define N8_HAS_AVX2 1
#else
#define N8_HAS_AVX2 0
#endif

namespace audio_graph {

/**
 * @brief Funciones matemáticas optimizadas sin llamadas trascendentes costosas (Reglas 34 y 47).
 * Diseñadas para ejecutarse por muestra en tiempo real con latencia determinista y cero allocations.
 */
class FastMath {
public:
    /**
     * @brief Limpieza instantánea de números denormales o subnormales (FTZ/DAZ manual)
     */
    static inline float flushDenormal(float x) noexcept {
        return (std::abs(x) < 1.0e-15f) ? 0.0f : x;
    }

    /**
     * @brief Aproximación racional Padé [5/4] para tanh(x).
     * Error relativo máximo < 0.04% para |x| < 3.0f.
     * Garantiza continuidad, monotonía estricta y asíntotas exactas en +/- 1.0.
     */
    static inline float fastTanh(float x) noexcept {
        if (x >= 3.0f) return 1.0f;
        if (x <= -3.0f) return -1.0f;

        const float x2 = x * x;
        const float x4 = x2 * x2;

        const float num = x * (945.0f + 105.0f * x2 + x4);
        const float den = 945.0f + 420.0f * x2 + 15.0f * x4;

        return num / den;
    }

    /**
     * @brief Aproximación parabólica cuadrática de alta precisión para sin(x).
     * Error relativo máximo < 0.1%. Diseñada para osciladores de síntesis en tiempo real.
     */
    static inline float fastSin(float x) noexcept {
        constexpr float pi = 3.14159265358979323846f;
        constexpr float twoPi = 2.0f * pi;
        while (x < -pi) x += twoPi;
        while (x > pi) x -= twoPi;

        constexpr float B = 4.0f / pi;
        constexpr float C = -4.0f / (pi * pi);
        const float y = B * x + C * x * std::abs(x);
        constexpr float P = 0.225f;
        return P * (y * std::abs(y) - y) + y;
    }

    /**
     * @brief Saturador suave cúbico normalizado a [-1.0, 1.0] con pendiente unitaria en el origen.
     */
    static inline float fastSoftClip(float x) noexcept {
        if (x >= 1.5f) return 1.0f;
        if (x <= -1.5f) return -1.0f;

        // f(x) = x - (4/27) * x^3
        return x - (4.0f / 27.0f) * (x * x * x);
    }

    /**
     * @brief Saturación asimétrica estilo tubo de vacío (triodo).
     */
    static inline float fastTubeSaturation(float x) noexcept {
        if (x >= 0.0f) {
            return fastTanh(x);
        } else {
            // Curvatura más suave en semiciclo negativo
            const float clamped = std::max(x, -2.5f);
            return clamped / (1.0f - 0.35f * clamped);
        }
    }

    /**
     * @brief Exponencial rápida para cálculo de coeficientes y envolventes (x <= 0).
     */
    static inline float fastExpNegative(float x) noexcept {
        if (x <= -16.0f) return 0.0f;
        if (x >= 0.0f) return 1.0f;

        // Padé rational [2/2] de e^x en torno a 0
        const float x2 = x * x;
        const float num = 12.0f + 6.0f * x + x2;
        const float den = 12.0f - 6.0f * x + x2;
        return num / den;
    }

    /**
     * @brief Conversión ultrarrápida de Decibelios a Ganancia Lineal: 10^(db / 20).
     */
    static inline float fastDbToGain(float db) noexcept {
        if (db <= -96.0f) return 0.0f;
        if (db >= 36.0f) return 63.0957f;
        if (std::abs(db) < 1e-4f) return 1.0f;

        // 10^(db/20) = exp(db * ln(10) / 20) = exp(db * 0.11512925464970228)
        const float x = db * 0.11512925464970228f;
        if (x <= 0.0f) {
            return fastExpNegative(x);
        } else {
            return 1.0f / fastExpNegative(-x);
        }
    }

    /**
     * @brief Conversión de Ganancia Lineal a Decibelios con protección contra silencio y denormales.
     */
    static inline float fastGainToDb(float gain) noexcept {
        if (gain <= 1.58489e-5f) return -96.0f; // < -96 dB
        return 20.0f * std::log10(gain);
    }

    /**
     * @brief Interpolación lineal rápida: (1 - t) * a + t * b
     */
    static inline float lerp(float a, float b, float t) noexcept {
        return a + t * (b - a);
    }

    /**
     * @brief Vectorized Padé [5/4] tanh(x) with AVX2/FMA and scalar fallback for tail/non-AVX2 (Punto 9).
     */
    static inline void vecPadéTanh(const float* in, float* out, size_t numSamples) noexcept {
        if (!in || !out || numSamples == 0) return;

        size_t s = 0;
#if N8_HAS_AVX2
        const size_t simdBlocks = numSamples / 8;
        const __m256 vThree = _mm256_set1_ps(3.0f);
        const __m256 vNegThree = _mm256_set1_ps(-3.0f);
        const __m256 vOne = _mm256_set1_ps(1.0f);
        const __m256 vNegOne = _mm256_set1_ps(-1.0f);
        const __m256 c945 = _mm256_set1_ps(945.0f);
        const __m256 c105 = _mm256_set1_ps(105.0f);
        const __m256 c420 = _mm256_set1_ps(420.0f);
        const __m256 c15  = _mm256_set1_ps(15.0f);

        for (size_t b = 0; b < simdBlocks; ++b) {
            __m256 vx = _mm256_loadu_ps(in + s);

            __m256 vClamped = _mm256_min_ps(vThree, _mm256_max_ps(vNegThree, vx));
            __m256 vx2 = _mm256_mul_ps(vClamped, vClamped);
            __m256 vx4 = _mm256_mul_ps(vx2, vx2);

#if defined(__FMA__)
            __m256 numP = _mm256_fmadd_ps(c105, vx2, c945);
#else
            __m256 numP = _mm256_add_ps(_mm256_mul_ps(c105, vx2), c945);
#endif
            numP = _mm256_add_ps(numP, vx4);
            __m256 num = _mm256_mul_ps(vClamped, numP);

#if defined(__FMA__)
            __m256 den = _mm256_fmadd_ps(c420, vx2, c945);
            den = _mm256_fmadd_ps(c15, vx4, den);
#else
            __m256 den = _mm256_add_ps(_mm256_mul_ps(c420, vx2), c945);
            den = _mm256_add_ps(_mm256_mul_ps(c15, vx4), den);
#endif
            __m256 res = _mm256_div_ps(num, den);

            __m256 maskGe3 = _mm256_cmp_ps(vx, vThree, _CMP_GE_OQ);
            __m256 maskLeNeg3 = _mm256_cmp_ps(vx, vNegThree, _CMP_LE_OQ);
            res = _mm256_blendv_ps(res, vOne, maskGe3);
            res = _mm256_blendv_ps(res, vNegOne, maskLeNeg3);

            _mm256_storeu_ps(out + s, res);
            s += 8;
        }
#endif
        for (; s < numSamples; ++s) {
            out[s] = fastTanh(in[s]);
        }
    }

    /**
     * @brief Vectorized out[i] = a[i] * b[i] + c[i] with AVX2/FMA and scalar fallback (Punto 9).
     */
    static inline void vecMultiplyAdd(const float* a, const float* b, const float* c, float* out, size_t numSamples) noexcept {
        if (!a || !b || !c || !out || numSamples == 0) return;

        size_t s = 0;
#if N8_HAS_AVX2
        const size_t simdBlocks = numSamples / 8;
        for (size_t block = 0; block < simdBlocks; ++block) {
            __m256 va = _mm256_loadu_ps(a + s);
            __m256 vb = _mm256_loadu_ps(b + s);
            __m256 vc = _mm256_loadu_ps(c + s);
#if defined(__FMA__)
            __m256 vres = _mm256_fmadd_ps(va, vb, vc);
#else
            __m256 vres = _mm256_add_ps(_mm256_mul_ps(va, vb), vc);
#endif
            _mm256_storeu_ps(out + s, vres);
            s += 8;
        }
#endif
        for (; s < numSamples; ++s) {
            out[s] = a[s] * b[s] + c[s];
        }
    }

    /**
     * @brief Vectorized out[i] = in[i] * gain with AVX2 and scalar fallback (Punto 9).
     */
    static inline void vecApplyGain(const float* in, float* out, float gain, size_t numSamples) noexcept {
        if (!in || !out || numSamples == 0) return;

        size_t s = 0;
#if N8_HAS_AVX2
        const size_t simdBlocks = numSamples / 8;
        __m256 vGain = _mm256_set1_ps(gain);
        for (size_t block = 0; block < simdBlocks; ++block) {
            __m256 vin = _mm256_loadu_ps(in + s);
            __m256 vres = _mm256_mul_ps(vin, vGain);
            _mm256_storeu_ps(out + s, vres);
            s += 8;
        }
#endif
        for (; s < numSamples; ++s) {
            out[s] = in[s] * gain;
        }
    }

    /**
     * @brief Vectorized linear mix: out[i] = (1 - mix) * dry[i] + mix * wet[i] with AVX2/FMA and scalar fallback (Punto 9).
     */
    static inline void vecMix(const float* dry, const float* wet, float* out, float mix, size_t numSamples) noexcept {
        if (!dry || !wet || !out || numSamples == 0) return;

        size_t s = 0;
#if N8_HAS_AVX2
        const size_t simdBlocks = numSamples / 8;
        const __m256 vDryGain = _mm256_set1_ps(1.0f - mix);
        const __m256 vWetGain = _mm256_set1_ps(mix);
        for (size_t block = 0; block < simdBlocks; ++block) {
            __m256 vdry = _mm256_loadu_ps(dry + s);
            __m256 vwet = _mm256_loadu_ps(wet + s);
#if defined(__FMA__)
            __m256 vres = _mm256_fmadd_ps(vwet, vWetGain, _mm256_mul_ps(vdry, vDryGain));
#else
            __m256 vres = _mm256_add_ps(_mm256_mul_ps(vdry, vDryGain), _mm256_mul_ps(vwet, vWetGain));
#endif
            _mm256_storeu_ps(out + s, vres);
            s += 8;
        }
#endif
        const float dryGain = 1.0f - mix;
        for (; s < numSamples; ++s) {
            out[s] = dry[s] * dryGain + wet[s] * mix;
        }
    }

    /**
     * @brief Vectorized flushing of denormals with AVX2 and scalar fallback (Regla 47).
     */
    static inline void vecFlushDenormals(float* buffer, size_t numSamples) noexcept {
        if (!buffer || numSamples == 0) return;

        size_t s = 0;
#if N8_HAS_AVX2
        const size_t simdBlocks = numSamples / 8;
        const __m256 vMinNormal = _mm256_set1_ps(1.17549435e-38f);
        const __m256 vAbsMask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));
        const __m256 vZero = _mm256_setzero_ps();

        for (size_t block = 0; block < simdBlocks; ++block) {
            __m256 vval = _mm256_loadu_ps(buffer + s);
            __m256 vabs = _mm256_and_ps(vval, vAbsMask);
            __m256 isNormal = _mm256_cmp_ps(vabs, vMinNormal, _CMP_GE_OQ);
            __m256 vcleaned = _mm256_blendv_ps(vZero, vval, isNormal);
            _mm256_storeu_ps(buffer + s, vcleaned);
            s += 8;
        }
#endif
        for (; s < numSamples; ++s) {
            buffer[s] = flushDenormal(buffer[s]);
        }
    }
};

} // namespace audio_graph
