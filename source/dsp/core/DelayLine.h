#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace audio_graph {

/**
 * @brief Línea de retardo fraccional circular con interpolación cúbica (Hermite) (Reglas 32 y 46)
 */
class DelayLine {
public:
    DelayLine() = default;

    void prepare(size_t maxDelaySamples) {
        maxDelaySamples_ = std::max<size_t>(16, maxDelaySamples);
        buffer_.assign(maxDelaySamples_, 0.0f);
        writeIndex_ = 0;
    }

    void reset() noexcept {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        writeIndex_ = 0;
    }

    void write(float sample) noexcept {
        if (maxDelaySamples_ == 0) return;
        buffer_[writeIndex_] = sample;
        writeIndex_ = (writeIndex_ + 1) % maxDelaySamples_;
    }

    // Lectura con interpolación lineal
    float readLinear(float delaySamples) const noexcept {
        if (maxDelaySamples_ == 0) return 0.0f;

        float clampedDelay = std::clamp(delaySamples, 0.0f, static_cast<float>(maxDelaySamples_ - 2));
        float readPos = static_cast<float>(writeIndex_) - clampedDelay;
        while (readPos < 0.0f) readPos += static_cast<float>(maxDelaySamples_);

        size_t idx0 = static_cast<size_t>(readPos) % maxDelaySamples_;
        size_t idx1 = (idx0 + 1) % maxDelaySamples_;
        float frac = readPos - std::floor(readPos);

        return buffer_[idx0] + frac * (buffer_[idx1] - buffer_[idx0]);
    }

    // Lectura con interpolación cúbica Hermite de 4 puntos para máxima calidad acústica (Regla 34)
    float readCubic(float delaySamples) const noexcept {
        if (maxDelaySamples_ < 4) return readLinear(delaySamples);

        float clampedDelay = std::clamp(delaySamples, 1.0f, static_cast<float>(maxDelaySamples_ - 3));
        float readPos = static_cast<float>(writeIndex_) - clampedDelay;
        while (readPos < 0.0f) readPos += static_cast<float>(maxDelaySamples_);

        int idx1 = static_cast<int>(readPos);
        int idx0 = idx1 - 1;
        int idx2 = idx1 + 1;
        int idx3 = idx1 + 2;

        auto wrap = [this](int idx) -> size_t {
            while (idx < 0) idx += static_cast<int>(maxDelaySamples_);
            return static_cast<size_t>(idx) % maxDelaySamples_;
        };

        float xm1 = buffer_[wrap(idx0)];
        float x0  = buffer_[wrap(idx1)];
        float x1  = buffer_[wrap(idx2)];
        float x2  = buffer_[wrap(idx3)];

        float frac = readPos - std::floor(readPos);
        float c0 = x0;
        float c1 = 0.5f * (x1 - xm1);
        float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    size_t getMaxDelaySamples() const noexcept { return maxDelaySamples_; }

private:
    size_t maxDelaySamples_{ 0 };
    size_t writeIndex_{ 0 };
    std::vector<float> buffer_;
};

} // namespace audio_graph
