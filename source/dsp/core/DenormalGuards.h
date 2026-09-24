#pragma once

#include <cstdint>
#include <cmath>

#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)
    #define N8_HAS_SSE_INTRINSICS 1
    #include <xmmintrin.h>
    #include <pmmintrin.h>
#else
    #define N8_HAS_SSE_INTRINSICS 0
#endif

namespace audio_graph {

/**
 * @brief Guardia RAII para erradicar números denormales a nivel de hardware (Reglas 34 y 47).
 * Activa FTZ (Flush-To-Zero) y DAZ (Denormals-Are-Zero) en el registro MXCSR para evitar
 * caídas de rendimiento de hasta 100x en la CPU durante silencios o colas de filtros/reverbs.
 */
class ScopedDenormalGuard {
public:
    ScopedDenormalGuard() noexcept {
#if N8_HAS_SSE_INTRINSICS
        oldMxcsr_ = _mm_getcsr();
        // Activar Flush-To-Zero (0x8000) y Denormals-Are-Zero (0x0040)
        _mm_setcsr(oldMxcsr_ | 0x8040);
#endif
    }

    ~ScopedDenormalGuard() noexcept {
#if N8_HAS_SSE_INTRINSICS
        _mm_setcsr(oldMxcsr_);
#endif
    }

    // No copiable ni movible
    ScopedDenormalGuard(const ScopedDenormalGuard&) = delete;
    ScopedDenormalGuard& operator=(const ScopedDenormalGuard&) = delete;
    ScopedDenormalGuard(ScopedDenormalGuard&&) = delete;
    ScopedDenormalGuard& operator=(ScopedDenormalGuard&&) = delete;

    /**
     * @brief Comprueba si los bits DAZ y FTZ están actualmente activos en el registro de hardware.
     */
    static bool areDenormalsDisabled() noexcept {
#if N8_HAS_SSE_INTRINSICS
        const unsigned int mxcsr = _mm_getcsr();
        return (mxcsr & 0x8040) == 0x8040;
#else
        return false;
#endif
    }

    /**
     * @brief Utilidad para detectar si un valor de punto flotante es denormal.
     */
    static bool isDenormal(float value) noexcept {
        return std::fpclassify(value) == FP_SUBNORMAL;
    }

private:
#if N8_HAS_SSE_INTRINSICS
    unsigned int oldMxcsr_{ 0 };
#endif
};

} // namespace audio_graph
