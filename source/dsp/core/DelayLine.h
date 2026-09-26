#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include "../../core/RealtimePools.h"

namespace audio_graph {

/**
 * @brief Línea de retardo fraccional circular con capacidad de potencia de dos (2^N),
 * indexación ultrarrápida por máscara de bits (index & mask) e interpolación cúbica (Hermite)
 * con buffer alineado a 64 bytes para aceleración AVX2/AVX-512 (Reglas 11, 12, 32, 34, 46, 47).
 */
class DelayLine {
public:
    DelayLine() = default;

    void prepare(size_t maxDelaySamples) {
        maxDelaySamples_ = std::max<size_t>(16, maxDelaySamples);
        // Calcular la siguiente potencia de 2 >= maxDelaySamples + 4
        size_t cap = 16;
        while (cap < maxDelaySamples_ + 4) {
            cap <<= 1;
        }
        capacity_ = cap;
        mask_ = capacity_ - 1;
        buffer_.assign(capacity_, 0.0f);
        writeIndex_ = 0;
    }

    void reset() noexcept {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        writeIndex_ = 0;
    }

    void write(float sample) noexcept {
        if (capacity_ == 0) return;
        buffer_[writeIndex_] = sample;
        writeIndex_ = (writeIndex_ + 1) & mask_;
    }

    // Lectura con interpolación lineal ultra-optimizada sin divisiones ni bucles
    float readLinear(float delaySamples) const noexcept {
        if (capacity_ == 0) return 0.0f;

        float clampedDelay = std::clamp(delaySamples, 0.0f, static_cast<float>(maxDelaySamples_));
        float readPos = static_cast<float>(writeIndex_) - clampedDelay;
        int iPos = static_cast<int>(std::floor(readPos));
        float frac = readPos - static_cast<float>(iPos);

        size_t idx0 = static_cast<size_t>(iPos) & mask_;
        size_t idx1 = (idx0 + 1) & mask_;

        return buffer_[idx0] + frac * (buffer_[idx1] - buffer_[idx0]);
    }

    // Lectura con interpolación cúbica Hermite de 4 puntos optimizada por máscara de bits (Reglas 34 y 47)
    float readCubic(float delaySamples) const noexcept {
        if (capacity_ == 0) return 0.0f;
        if (capacity_ < 4) return readLinear(delaySamples);

        float clampedDelay = std::clamp(delaySamples, 1.0f, static_cast<float>(maxDelaySamples_));
        float readPos = static_cast<float>(writeIndex_) - clampedDelay;
        int iPos = static_cast<int>(std::floor(readPos));
        float frac = readPos - static_cast<float>(iPos);

        size_t idx0 = static_cast<size_t>(iPos - 1) & mask_;
        size_t idx1 = static_cast<size_t>(iPos) & mask_;
        size_t idx2 = (idx1 + 1) & mask_;
        size_t idx3 = (idx1 + 2) & mask_;

        float xm1 = buffer_[idx0];
        float x0  = buffer_[idx1];
        float x1  = buffer_[idx2];
        float x2  = buffer_[idx3];

        float c0 = x0;
        float c1 = 0.5f * (x1 - xm1);
        float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    size_t getMaxDelaySamples() const noexcept { return maxDelaySamples_; }
    size_t getCapacity() const noexcept { return capacity_; }
    size_t getMask() const noexcept { return mask_; }

private:
    size_t maxDelaySamples_{ 0 };
    size_t capacity_{ 0 };
    size_t mask_{ 0 };
    size_t writeIndex_{ 0 };
    std::vector<float, AlignedAllocator<float, 64>> buffer_;
};

} // namespace audio_graph
