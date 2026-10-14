#pragma once

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include "EventTypes.h"
#include "EventCaptureBuffer.h"
#include "AudioFragment.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Entidad de Evento individual con ciclo de vida explícito (Reglas 2, 3, 18, 46)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class Event {
public:
    explicit Event(RustEvent* existing = nullptr, bool owns = true)
        : rustInstance_(existing != nullptr ? existing : n8_rust_event_create()),
          ownsCore_(existing == nullptr || owns)
    {
    }

    ~Event() {
        if (ownsCore_ && rustInstance_ != nullptr) {
            n8_rust_event_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    Event(Event&& other) noexcept
        : rustInstance_(other.rustInstance_),
          ownsCore_(other.ownsCore_),
          attrs_(other.attrs_)
    {
        other.rustInstance_ = nullptr;
        other.ownsCore_ = false;
    }

    Event& operator=(Event&& other) noexcept {
        if (this != &other) {
            if (ownsCore_ && rustInstance_ != nullptr) {
                n8_rust_event_destroy(rustInstance_);
            }
            rustInstance_ = other.rustInstance_;
            ownsCore_ = other.ownsCore_;
            attrs_ = other.attrs_;
            other.rustInstance_ = nullptr;
            other.ownsCore_ = false;
        }
        return *this;
    }

    void initialize(const EventAttributes& attrs, double captureOffset, uint64_t durationSamples, bool reverse = false) {
        attrs_ = attrs;
        attrs_.state = EventLifecycle::Created;
        attrs_.ageSamples = 0;
        attrs_.lifetimeSamples = durationSamples;

        if (rustInstance_ != nullptr) {
            RustEventAttributes r = toRust(attrs_);
            n8_rust_event_initialize(rustInstance_, &r, captureOffset, durationSamples, reverse);
        }
    }

    void start() noexcept {
        if (rustInstance_ != nullptr) {
            n8_rust_event_start(rustInstance_);
        }
        attrs_.state = EventLifecycle::Playing;
    }

    void kill() noexcept {
        if (rustInstance_ != nullptr) {
            n8_rust_event_kill(rustInstance_);
        }
        attrs_.state = EventLifecycle::Killed;
    }

    void release() noexcept {
        if (rustInstance_ != nullptr) {
            n8_rust_event_release(rustInstance_);
        }
        if (attrs_.state == EventLifecycle::Playing || attrs_.state == EventLifecycle::Frozen || attrs_.state == EventLifecycle::Looping) {
            attrs_.state = EventLifecycle::Releasing;
        }
    }

    void render(const EventCaptureBuffer& capture, float* outL, float* outR, uint32_t numSamples, float sourceLevel, int sourceSilenceOverride = -1) noexcept {
        if (rustInstance_ != nullptr && capture.getRustInstance() != nullptr) {
            n8_rust_event_render(rustInstance_, capture.getRustInstance(), outL, outR, numSamples, sourceLevel, sourceSilenceOverride);
        }
    }

    bool isActive() const noexcept {
        if (rustInstance_ != nullptr) {
            return n8_rust_event_is_active(rustInstance_);
        }
        return false;
    }

    const EventAttributes& getAttributes() const noexcept {
        if (rustInstance_ != nullptr) {
            RustEventAttributes r{};
            n8_rust_event_get_attributes(rustInstance_, &r);
            attrs_ = fromRust(r);
        }
        return attrs_;
    }

    EventAttributes& getAttributes() noexcept {
        if (rustInstance_ != nullptr) {
            RustEventAttributes r{};
            n8_rust_event_get_attributes(rustInstance_, &r);
            attrs_ = fromRust(r);
        }
        return attrs_;
    }

    EventLifecycle getState() const noexcept {
        if (rustInstance_ != nullptr) {
            return static_cast<EventLifecycle>(n8_rust_event_get_state(rustInstance_));
        }
        return attrs_.state;
    }

    void setState(EventLifecycle state) noexcept {
        attrs_.state = state;
        if (rustInstance_ != nullptr) {
            n8_rust_event_set_state(rustInstance_, static_cast<uint8_t>(state));
        }
    }

    RustEvent* getRustInstance() noexcept { return rustInstance_; }
    const RustEvent* getRustInstance() const noexcept { return rustInstance_; }

    void attachRustInstance(RustEvent* ptr, bool owns = false) noexcept {
        if (ownsCore_ && rustInstance_ != nullptr && rustInstance_ != ptr) {
            n8_rust_event_destroy(rustInstance_);
        }
        rustInstance_ = ptr;
        ownsCore_ = owns;
    }

private:
    RustEvent* rustInstance_{ nullptr };
    bool ownsCore_{ true };
    mutable EventAttributes attrs_{};
};

} // namespace audio_graph
