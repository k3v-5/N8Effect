#pragma once

#include <cstdint>
#include <atomic>
#include <array>
#include "EventTypes.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Instantánea atómica ligera de un evento para visualización en GUI (Reglas 9, 23, 26).
 */
struct EventTelemetryItem {
    float pan{ 0.0f };            // -1.0 (L) a +1.0 (R)
    float pitchRatio{ 1.0f };     // 0.25 a 4.0
    float energy{ 0.0f };         // 0.0 a 1.0
    float distance{ 1.0f };       // 0.1 a 10.0 m
    float azimuth{ 0.0f };        // -180° a +180°
    EventType type{ EventType::Fragment };
    uint8_t generation{ 0 };
    bool isAlive{ true };
};

inline RustEventTelemetryItem toRust(const EventTelemetryItem& it) noexcept {
    RustEventTelemetryItem r{};
    r.pan = it.pan;
    r.pitch_ratio = it.pitchRatio;
    r.energy = it.energy;
    r.distance = it.distance;
    r.azimuth = it.azimuth;
    r.event_type = static_cast<uint16_t>(it.type);
    r.generation = it.generation;
    r.is_alive = it.isAlive;
    return r;
}

inline EventTelemetryItem fromRust(const RustEventTelemetryItem& r) noexcept {
    EventTelemetryItem it{};
    it.pan = r.pan;
    it.pitchRatio = r.pitch_ratio;
    it.energy = r.energy;
    it.distance = r.distance;
    it.azimuth = r.azimuth;
    it.type = static_cast<EventType>(r.event_type);
    it.generation = r.generation;
    it.isAlive = r.is_alive;
    return it;
}

/**
 * @brief Búfer circular SPSC lock-free para telemetría de eventos de audio en tiempo real (Reglas 9, 26, 47).
 * Cero alocaciones dinámicas en el audio thread, cero bloqueos (lock-free), capacidad finita acotada.
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class EventTelemetryBuffer {
public:
    static constexpr size_t Capacity = 1024;
    static constexpr size_t Mask = Capacity - 1;

    explicit EventTelemetryBuffer(RustEventTelemetryBuffer* existing = nullptr, bool owns = true)
        : rustInstance_(existing != nullptr ? existing : n8_rust_event_telemetry_create()),
          ownsCore_(existing == nullptr || owns)
    {
    }

    ~EventTelemetryBuffer() {
        if (ownsCore_ && rustInstance_ != nullptr) {
            n8_rust_event_telemetry_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    EventTelemetryBuffer(const EventTelemetryBuffer&) = delete;
    EventTelemetryBuffer& operator=(const EventTelemetryBuffer&) = delete;

    EventTelemetryBuffer(EventTelemetryBuffer&& other) noexcept
        : rustInstance_(other.rustInstance_),
          ownsCore_(other.ownsCore_)
    {
        other.rustInstance_ = nullptr;
        other.ownsCore_ = false;
    }

    EventTelemetryBuffer& operator=(EventTelemetryBuffer&& other) noexcept {
        if (this != &other) {
            if (ownsCore_ && rustInstance_ != nullptr) {
                n8_rust_event_telemetry_destroy(rustInstance_);
            }
            rustInstance_ = other.rustInstance_;
            ownsCore_ = other.ownsCore_;
            other.rustInstance_ = nullptr;
            other.ownsCore_ = false;
        }
        return *this;
    }

    void reset() noexcept {
        if (rustInstance_ != nullptr) {
            n8_rust_event_telemetry_reset(rustInstance_);
        }
    }

    bool push(const EventTelemetryItem& item) noexcept {
        if (rustInstance_ != nullptr) {
            RustEventTelemetryItem r = toRust(item);
            return n8_rust_event_telemetry_push(rustInstance_, &r);
        }
        return false;
    }

    bool pop(EventTelemetryItem& outItem) noexcept {
        if (rustInstance_ != nullptr) {
            RustEventTelemetryItem r{};
            if (n8_rust_event_telemetry_pop(rustInstance_, &r)) {
                outItem = fromRust(r);
                return true;
            }
        }
        return false;
    }

    size_t size() const noexcept {
        if (rustInstance_ != nullptr) {
            return n8_rust_event_telemetry_size(rustInstance_);
        }
        return 0;
    }

    RustEventTelemetryBuffer* getRustInstance() noexcept { return rustInstance_; }
    const RustEventTelemetryBuffer* getRustInstance() const noexcept { return rustInstance_; }

private:
    RustEventTelemetryBuffer* rustInstance_{ nullptr };
    bool ownsCore_{ true };
};

} // namespace audio_graph
