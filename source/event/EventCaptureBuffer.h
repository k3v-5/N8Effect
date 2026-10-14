#pragma once

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include "../core/Types.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Buffer circular prealocado para captura y extracción de fragmentos de audio (Reglas 9, 10, 46)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class EventCaptureBuffer {
public:
    explicit EventCaptureBuffer(RustEventCaptureBuffer* existing = nullptr, bool owns = true)
        : rustInstance_(existing != nullptr ? existing : n8_rust_event_capture_buffer_create()),
          ownsCore_(existing == nullptr || owns)
    {
    }

    ~EventCaptureBuffer() {
        if (ownsCore_ && rustInstance_ != nullptr) {
            n8_rust_event_capture_buffer_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    EventCaptureBuffer(const EventCaptureBuffer&) = delete;
    EventCaptureBuffer& operator=(const EventCaptureBuffer&) = delete;

    EventCaptureBuffer(EventCaptureBuffer&& other) noexcept
        : rustInstance_(other.rustInstance_),
          ownsCore_(other.ownsCore_)
    {
        other.rustInstance_ = nullptr;
        other.ownsCore_ = false;
    }

    EventCaptureBuffer& operator=(EventCaptureBuffer&& other) noexcept {
        if (this != &other) {
            if (ownsCore_ && rustInstance_ != nullptr) {
                n8_rust_event_capture_buffer_destroy(rustInstance_);
            }
            rustInstance_ = other.rustInstance_;
            ownsCore_ = other.ownsCore_;
            other.rustInstance_ = nullptr;
            other.ownsCore_ = false;
        }
        return *this;
    }

    void prepare(double sampleRate, double maxHistorySeconds = 5.0, uint32_t numChannels = 2) {
        if (rustInstance_ != nullptr) {
            n8_rust_event_capture_buffer_prepare(rustInstance_, sampleRate, maxHistorySeconds, numChannels);
        }
    }

    void reset() noexcept {
        if (rustInstance_ != nullptr) {
            n8_rust_event_capture_buffer_reset(rustInstance_);
        }
    }

    void write(const float* const* input, uint32_t numChannels, uint32_t numSamples) noexcept {
        if (rustInstance_ == nullptr || input == nullptr || numSamples == 0) return;
        const float* inL = (numChannels > 0) ? input[0] : nullptr;
        const float* inR = (numChannels > 1) ? input[1] : inL;
        n8_rust_event_capture_buffer_write(rustInstance_, inL, inR, numSamples);
    }

    float readSample(uint32_t channel, double sampleOffsetFromWritePos) const noexcept {
        if (rustInstance_ != nullptr) {
            return n8_rust_event_capture_buffer_read_sample(rustInstance_, channel, sampleOffsetFromWritePos);
        }
        return 0.0f;
    }

    size_t getCapacitySamples() const noexcept {
        if (rustInstance_ != nullptr) {
            return n8_rust_event_capture_buffer_get_capacity(rustInstance_);
        }
        return 0;
    }

    size_t getWritePosition() const noexcept {
        if (rustInstance_ != nullptr) {
            return n8_rust_event_capture_buffer_get_write_pos(rustInstance_);
        }
        return 0;
    }

    uint32_t getNumChannels() const noexcept {
        if (rustInstance_ != nullptr) {
            return static_cast<uint32_t>(n8_rust_event_capture_buffer_get_num_channels(rustInstance_));
        }
        return 0;
    }

    RustEventCaptureBuffer* getRustInstance() noexcept { return rustInstance_; }
    const RustEventCaptureBuffer* getRustInstance() const noexcept { return rustInstance_; }

private:
    RustEventCaptureBuffer* rustInstance_{ nullptr };
    bool ownsCore_{ true };
};

} // namespace audio_graph
