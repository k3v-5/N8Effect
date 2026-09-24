#pragma once

#include <cmath>
#include <algorithm>
#include "FastMath.h"

namespace audio_graph {

/**
 * @brief Algoritmos y funciones matemáticas de saturación y distorsión aceleradas (Reglas 32, 34 y 47)
 */
struct Saturation {
    // 1. Soft Clip hiperbólico (Tanh) suave acelerado vía aproximante Padé (error < 0.04%)
    static inline float softClipTanh(float in, float drive = 1.0f) noexcept {
        const float x = in * drive;
        return FastMath::fastTanh(x);
    }

    // 2. Hard Clip digital
    static inline float hardClip(float in, float threshold = 1.0f) noexcept {
        return std::clamp(in, -threshold, threshold);
    }

    // 3. Wavefolder simétrico para armónicos densos
    static inline float wavefold(float in, float drive = 1.0f) noexcept {
        float x = in * drive;
        // Plegado continuo acotado
        while (x > 1.0f || x < -1.0f) {
            if (x > 1.0f) x = 2.0f - x;
            else if (x < -1.0f) x = -2.0f - x;
        }
        return x;
    }

    // 4. Saturación asimétrica estilo válvula (Tube) acelerada
    static inline float tube(float in, float drive = 1.0f) noexcept {
        const float x = in * drive;
        if (x >= 0.0f) {
            return 1.0f - FastMath::fastExpNegative(-x);
        } else {
            return - (1.0f - FastMath::fastExpNegative(x)) * 0.8f;
        }
    }

    // 5. Reducción de bits (Bitcrusher)
    static inline float bitcrush(float in, float bits = 8.0f) noexcept {
        const float numLevels = std::ldexp(1.0f, static_cast<int>(std::clamp(bits, 1.0f, 24.0f)));
        const float scaled = in * 0.5f + 0.5f;
        const float quantized = std::round(scaled * numLevels) / numLevels;
        return (quantized - 0.5f) * 2.0f;
    }
};

} // namespace audio_graph
