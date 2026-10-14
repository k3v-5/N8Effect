#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Línea de retardo fraccional circular con capacidad de potencia de dos (2^N),
 * indexación ultrarrápida por máscara de bits (index & mask) e interpolación cúbica (Hermite)
 * delegada al Rust Core n8_rust_core (Reglas 11, 12, 32, 34, 46, 47).
 */
class DelayLine {
public:
    DelayLine() noexcept {
        rustCore_ = n8_rust_delay_line_create();
    }

    ~DelayLine() noexcept {
        if (rustCore_) {
            n8_rust_delay_line_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    DelayLine(const DelayLine&) = delete;
    DelayLine& operator=(const DelayLine&) = delete;

    DelayLine(DelayLine&& other) noexcept : rustCore_(other.rustCore_) {
        other.rustCore_ = nullptr;
    }

    DelayLine& operator=(DelayLine&& other) noexcept {
        if (this != &other) {
            if (rustCore_) n8_rust_delay_line_destroy(rustCore_);
            rustCore_ = other.rustCore_;
            other.rustCore_ = nullptr;
        }
        return *this;
    }

    void prepare(size_t maxDelaySamples) {
        if (rustCore_) {
            n8_rust_delay_line_prepare(rustCore_, maxDelaySamples);
        }
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_delay_line_reset(rustCore_);
        }
    }

    void write(float sample) noexcept {
        if (rustCore_) {
            n8_rust_delay_line_write(rustCore_, sample);
        }
    }

    float readLinear(float delaySamples) const noexcept {
        if (rustCore_) {
            return n8_rust_delay_line_read_linear(rustCore_, delaySamples);
        }
        return 0.0f;
    }

    float readCubic(float delaySamples) const noexcept {
        if (rustCore_) {
            return n8_rust_delay_line_read_cubic(rustCore_, delaySamples);
        }
        return 0.0f;
    }

    size_t getMaxDelaySamples() const noexcept {
        if (rustCore_) return n8_rust_delay_line_get_max_delay_samples(rustCore_);
        return 0;
    }

    size_t getCapacity() const noexcept {
        if (rustCore_) return n8_rust_delay_line_get_capacity(rustCore_);
        return 0;
    }

    size_t getMask() const noexcept {
        if (rustCore_) return n8_rust_delay_line_get_mask(rustCore_);
        return 0;
    }

    RustDelayLine* getRustDelayLine() noexcept { return rustCore_; }
    const RustDelayLine* getRustDelayLine() const noexcept { return rustCore_; }

private:
    RustDelayLine* rustCore_{ nullptr };
};

} // namespace audio_graph
