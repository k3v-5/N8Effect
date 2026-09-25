#pragma once

#include <cstdint>
#include <atomic>
#include <array>
#include "EventTypes.h"

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

/**
 * @brief Búfer circular SPSC lock-free para telemetría de eventos de audio en tiempo real (Reglas 9, 26, 47).
 * Cero alocaciones dinámicas en el audio thread, cero bloqueos (lock-free), capacidad finita acotada.
 */
class EventTelemetryBuffer {
public:
    static constexpr size_t Capacity = 1024;
    static constexpr size_t Mask = Capacity - 1;

    EventTelemetryBuffer() {
        writeIndex_.store(0, std::memory_order_relaxed);
        readIndex_.store(0, std::memory_order_relaxed);
    }

    void reset() noexcept {
        writeIndex_.store(0, std::memory_order_relaxed);
        readIndex_.store(0, std::memory_order_relaxed);
    }

    bool push(const EventTelemetryItem& item) noexcept {
        const size_t w = writeIndex_.load(std::memory_order_relaxed);
        const size_t r = readIndex_.load(std::memory_order_acquire);

        if ((w - r) >= Capacity) {
            // Desbordamiento controlado: descartar sin bloquear (Regla 11)
            return false;
        }

        buffer_[w & Mask] = item;
        writeIndex_.store(w + 1, std::memory_order_release);
        return true;
    }

    bool pop(EventTelemetryItem& outItem) noexcept {
        const size_t r = readIndex_.load(std::memory_order_relaxed);
        const size_t w = writeIndex_.load(std::memory_order_acquire);

        if (r == w) {
            return false; // Cola vacía
        }

        outItem = buffer_[r & Mask];
        readIndex_.store(r + 1, std::memory_order_release);
        return true;
    }

    size_t size() const noexcept {
        const size_t w = writeIndex_.load(std::memory_order_relaxed);
        const size_t r = readIndex_.load(std::memory_order_relaxed);
        return (w >= r) ? (w - r) : 0;
    }

private:
    std::array<EventTelemetryItem, Capacity> buffer_{};
    std::atomic<size_t> writeIndex_{ 0 };
    std::atomic<size_t> readIndex_{ 0 };
};

} // namespace audio_graph
