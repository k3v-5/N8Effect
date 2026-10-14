#pragma once

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include "EventTypes.h"
#include "EventCaptureBuffer.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Representación y reproducción de un fragmento de audio con envolvente y pitch (Reglas 2, 4.1, 46)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class AudioFragment {
public:
    explicit AudioFragment(RustAudioFragment* existing = nullptr, bool owns = true)
        : rustInstance_(existing != nullptr ? existing : n8_rust_audio_fragment_create()),
          ownsCore_(existing == nullptr || owns)
    {
    }

    ~AudioFragment() {
        if (ownsCore_ && rustInstance_ != nullptr) {
            n8_rust_audio_fragment_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    AudioFragment(const AudioFragment&) = delete;
    AudioFragment& operator=(const AudioFragment&) = delete;

    AudioFragment(AudioFragment&& other) noexcept
        : rustInstance_(other.rustInstance_),
          ownsCore_(other.ownsCore_)
    {
        other.rustInstance_ = nullptr;
        other.ownsCore_ = false;
    }

    AudioFragment& operator=(AudioFragment&& other) noexcept {
        if (this != &other) {
            if (ownsCore_ && rustInstance_ != nullptr) {
                n8_rust_audio_fragment_destroy(rustInstance_);
            }
            rustInstance_ = other.rustInstance_;
            ownsCore_ = other.ownsCore_;
            other.rustInstance_ = nullptr;
            other.ownsCore_ = false;
        }
        return *this;
    }

    void configure(double captureOffsetSamples, uint64_t durationSamples, float pitchRatio, bool reverse = false) noexcept {
        if (rustInstance_ != nullptr) {
            n8_rust_audio_fragment_configure(rustInstance_, captureOffsetSamples, durationSamples, pitchRatio, reverse);
        }
    }

    void reset() noexcept {
        if (rustInstance_ != nullptr) {
            n8_rust_audio_fragment_reset(rustInstance_);
        }
    }

    // Genera el siguiente par de muestras estéreo leyendo desde el buffer de captura
    bool getNextSample(const EventCaptureBuffer& capture, float& outL, float& outR, float envelopeGain) noexcept {
        if (rustInstance_ != nullptr && capture.getRustInstance() != nullptr) {
            return n8_rust_audio_fragment_get_next_sample(rustInstance_, capture.getRustInstance(), envelopeGain, &outL, &outR);
        }
        outL = 0.0f;
        outR = 0.0f;
        return false;
    }

    double getCurrentPhase() const noexcept {
        if (rustInstance_ != nullptr) {
            return n8_rust_audio_fragment_get_current_phase(rustInstance_);
        }
        return 0.0;
    }

    uint64_t getDurationSamples() const noexcept {
        if (rustInstance_ != nullptr) {
            return n8_rust_audio_fragment_get_duration_samples(rustInstance_);
        }
        return 0;
    }

    bool isFinished() const noexcept {
        if (rustInstance_ != nullptr) {
            return n8_rust_audio_fragment_is_finished(rustInstance_);
        }
        return true;
    }

    RustAudioFragment* getRustInstance() noexcept { return rustInstance_; }
    const RustAudioFragment* getRustInstance() const noexcept { return rustInstance_; }

private:
    RustAudioFragment* rustInstance_{ nullptr };
    bool ownsCore_{ true };
};

} // namespace audio_graph
