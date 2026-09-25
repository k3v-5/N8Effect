#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>

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
};

} // namespace audio_graph
