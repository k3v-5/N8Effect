#pragma once

#include <cmath>
#include <algorithm>
#include <numbers>
#include "EventTypes.h"
#include "EventCaptureBuffer.h"

namespace audio_graph {

/**
 * @brief Representación y reproducción de un fragmento de audio con envolvente y pitch (Reglas 2, 4.1, 46)
 */
class AudioFragment {
public:
    AudioFragment() = default;

    void configure(double captureOffsetSamples, uint64_t durationSamples, float pitchRatio, bool reverse = false) noexcept {
        captureOffsetSamples_ = captureOffsetSamples;
        durationSamples_ = std::max<uint64_t>(durationSamples, 16);
        pitchRatio_ = std::clamp(pitchRatio, 0.125f, 8.0f);
        reverse_ = reverse;
        currentPhase_ = 0.0;
        isFinished_ = false;
    }

    void reset() noexcept {
        currentPhase_ = 0.0;
        isFinished_ = false;
    }

    // Genera el siguiente par de muestras estéreo leyendo desde el buffer de captura
    bool getNextSample(const EventCaptureBuffer& capture, float& outL, float& outR, float envelopeGain) noexcept {
        if (isFinished_ || currentPhase_ >= static_cast<double>(durationSamples_)) {
            outL = 0.0f;
            outR = 0.0f;
            isFinished_ = true;
            return false;
        }

        // Posición dentro del fragmento respecto al inicio de la captura en el pasado
        double playOffset = reverse_ ? (static_cast<double>(durationSamples_) - currentPhase_) : currentPhase_;
        double sampleOffset = captureOffsetSamples_ - playOffset;

        outL = capture.readSample(0, sampleOffset) * envelopeGain;
        outR = capture.readSample(1, sampleOffset) * envelopeGain;

        currentPhase_ += pitchRatio_;
        if (currentPhase_ >= static_cast<double>(durationSamples_)) {
            isFinished_ = true;
        }

        return true;
    }

    double getCurrentPhase() const noexcept { return currentPhase_; }
    uint64_t getDurationSamples() const noexcept { return durationSamples_; }
    bool isFinished() const noexcept { return isFinished_; }

private:
    double captureOffsetSamples_{ 0.0 };
    uint64_t durationSamples_{ 1024 };
    float pitchRatio_{ 1.0f };
    double currentPhase_{ 0.0 };
    bool reverse_{ false };
    bool isFinished_{ true };
};

} // namespace audio_graph
