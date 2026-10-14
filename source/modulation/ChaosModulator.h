#pragma once

#include <cmath>
#include <algorithm>
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Modulador de Caos No Lineal Continuo basado en el Atractor de Lorenz (Reglas 7, 8, 25, 46, 47)
 * Delega toda la integración numérica Runge-Kutta RK4 al núcleo Rust de alto rendimiento.
 */
class ChaosModulator {
public:
    explicit ChaosModulator(RustChaosModulator* existing = nullptr, bool owns = true)
        : rustCore_(existing ? existing : n8_rust_chaos_modulator_create()), ownsCore_(owns)
    {
    }

    ~ChaosModulator() {
        if (rustCore_ && ownsCore_) {
            n8_rust_chaos_modulator_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    ChaosModulator(const ChaosModulator&) = delete;
    ChaosModulator& operator=(const ChaosModulator&) = delete;

    ChaosModulator(ChaosModulator&& other) noexcept
        : rustCore_(other.rustCore_), ownsCore_(other.ownsCore_)
    {
        other.rustCore_ = nullptr;
        other.ownsCore_ = false;
    }

    ChaosModulator& operator=(ChaosModulator&& other) noexcept {
        if (this != &other) {
            if (rustCore_ && ownsCore_) {
                n8_rust_chaos_modulator_destroy(rustCore_);
            }
            rustCore_ = other.rustCore_;
            ownsCore_ = other.ownsCore_;
            other.rustCore_ = nullptr;
            other.ownsCore_ = false;
        }
        return *this;
    }

    void prepare(double sampleRate) noexcept {
        if (rustCore_) {
            n8_rust_chaos_modulator_prepare(rustCore_, sampleRate);
        }
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_chaos_modulator_reset(rustCore_);
        }
    }

    void setSpeed(float hz) noexcept {
        if (rustCore_) {
            n8_rust_chaos_modulator_set_speed(rustCore_, hz);
        }
    }

    void processSample() noexcept {
        if (rustCore_) {
            n8_rust_chaos_modulator_process_sample(rustCore_);
        }
    }

    float getNormalizedX() const noexcept {
        if (rustCore_) {
            return n8_rust_chaos_modulator_get_normalized_x(rustCore_);
        }
        return 0.0f;
    }

    float getNormalizedY() const noexcept {
        if (rustCore_) {
            return n8_rust_chaos_modulator_get_normalized_y(rustCore_);
        }
        return 0.0f;
    }

    float getNormalizedZ() const noexcept {
        if (rustCore_) {
            return n8_rust_chaos_modulator_get_normalized_z(rustCore_);
        }
        return 0.0f;
    }

private:
    RustChaosModulator* rustCore_{ nullptr };
    bool ownsCore_{ true };
};

} // namespace audio_graph
