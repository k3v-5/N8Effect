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

#include "../rust_core/include/n8_rust_core.h"

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
        oldMxcsr_ = n8_rust_opt_enable_ftz_daz();
#endif
    }

    ~ScopedDenormalGuard() noexcept {
#if N8_HAS_SSE_INTRINSICS
        n8_rust_opt_restore_mxcsr(oldMxcsr_);
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
        return n8_rust_opt_is_ftz_daz_active();
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
