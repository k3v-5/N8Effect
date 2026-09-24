#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cassert>
#include "../core/Types.h"

namespace audio_graph {

/**
 * @brief Buffer circular prealocado para captura y extracción de fragmentos de audio (Reglas 9, 10, 46)
 * Permite registrar el audio entrante y leer rebanadas (slices/fragments) hacia adelante o en reversa.
 */
class EventCaptureBuffer {
public:
    EventCaptureBuffer() = default;

    void prepare(double sampleRate, double maxHistorySeconds = 5.0, uint32_t numChannels = 2) {
        numChannels_ = numChannels;
        capacitySamples_ = static_cast<size_t>(std::max(1.0, sampleRate) * maxHistorySeconds);
        
        channelData_.clear();
        channelData_.resize(numChannels_);
        for (uint32_t ch = 0; ch < numChannels_; ++ch) {
            channelData_[ch].assign(capacitySamples_, 0.0f);
        }
        writePos_ = 0;
    }

    void reset() noexcept {
        for (auto& ch : channelData_) {
            std::fill(ch.begin(), ch.end(), 0.0f);
        }
        writePos_ = 0;
    }

    // Escribe un bloque entrante en el buffer circular (Regla 9: sin malloc)
    void write(const float* const* input, uint32_t numChannels, uint32_t numSamples) noexcept {
        if (capacitySamples_ == 0 || input == nullptr) return;

        const uint32_t channelsToCopy = std::min(numChannels_, numChannels);
        for (uint32_t s = 0; s < numSamples; ++s) {
            for (uint32_t ch = 0; ch < channelsToCopy; ++ch) {
                if (input[ch] != nullptr) {
                    channelData_[ch][writePos_] = input[ch][s];
                }
            }
            writePos_ = (writePos_ + 1) % capacitySamples_;
        }
    }

    // Lee una muestra con interpolación lineal a un offset relativo respecto a la posición de captura
    float readSample(uint32_t channel, double sampleOffsetFromWritePos) const noexcept {
        if (capacitySamples_ == 0 || channel >= numChannels_) return 0.0f;

        // Offset positivo = atrás en el tiempo
        double readIndex = static_cast<double>(writePos_) - sampleOffsetFromWritePos;
        while (readIndex < 0.0) readIndex += static_cast<double>(capacitySamples_);
        while (readIndex >= static_cast<double>(capacitySamples_)) readIndex -= static_cast<double>(capacitySamples_);

        const size_t idx0 = static_cast<size_t>(readIndex) % capacitySamples_;
        const size_t idx1 = (idx0 + 1) % capacitySamples_;
        const float frac = static_cast<float>(readIndex - std::floor(readIndex));

        const float s0 = channelData_[channel][idx0];
        const float s1 = channelData_[channel][idx1];
        return s0 + frac * (s1 - s0);
    }

    size_t getCapacitySamples() const noexcept { return capacitySamples_; }
    size_t getWritePosition() const noexcept { return writePos_; }
    uint32_t getNumChannels() const noexcept { return numChannels_; }

private:
    uint32_t numChannels_{ 2 };
    size_t capacitySamples_{ 0 };
    size_t writePos_{ 0 };
    std::vector<std::vector<float>> channelData_;
};

} // namespace audio_graph
