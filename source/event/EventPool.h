#pragma once

#include <vector>
#include <cstdint>
#include <cassert>
#include "Event.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Pool de Eventos con capacidad fija preasignada (Reglas 9, 10, 11, 46)
 * Capacidad por defecto: 1024 eventos. Cero allocations en el hilo de audio.
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class EventPool {
public:
    static constexpr size_t DefaultMaxEvents = 1024;

    explicit EventPool(RustEventPool* existing = nullptr, bool owns = true)
        : rustInstance_(existing != nullptr ? existing : n8_rust_event_pool_create()),
          ownsCore_(existing == nullptr || owns)
    {
    }

    ~EventPool() {
        if (ownsCore_ && rustInstance_ != nullptr) {
            n8_rust_event_pool_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    EventPool(const EventPool&) = delete;
    EventPool& operator=(const EventPool&) = delete;

    EventPool(EventPool&& other) noexcept
        : rustInstance_(other.rustInstance_),
          ownsCore_(other.ownsCore_),
          eventWrappers_(std::move(other.eventWrappers_))
    {
        other.rustInstance_ = nullptr;
        other.ownsCore_ = false;
    }

    EventPool& operator=(EventPool&& other) noexcept {
        if (this != &other) {
            if (ownsCore_ && rustInstance_ != nullptr) {
                n8_rust_event_pool_destroy(rustInstance_);
            }
            rustInstance_ = other.rustInstance_;
            ownsCore_ = other.ownsCore_;
            eventWrappers_ = std::move(other.eventWrappers_);
            other.rustInstance_ = nullptr;
            other.ownsCore_ = false;
        }
        return *this;
    }

    void prepare(size_t maxEvents = DefaultMaxEvents) {
        if (rustInstance_ != nullptr) {
            n8_rust_event_pool_prepare(rustInstance_, maxEvents);
            eventWrappers_.clear();
            eventWrappers_.reserve(maxEvents);
            for (size_t i = 0; i < maxEvents; ++i) {
                RustEvent* rEv = n8_rust_event_pool_get_event_at(rustInstance_, i);
                eventWrappers_.emplace_back(rEv, false);
            }
        }
    }

    void reset() noexcept {
        if (rustInstance_ != nullptr) {
            n8_rust_event_pool_reset(rustInstance_);
        }
    }

    Event* acquire() noexcept {
        if (rustInstance_ == nullptr) return nullptr;
        RustEvent* rEv = n8_rust_event_pool_acquire(rustInstance_);
        if (rEv == nullptr) return nullptr;

        size_t index = n8_rust_event_pool_get_event_index(rustInstance_, rEv);
        if (index < eventWrappers_.size()) {
            return &eventWrappers_[index];
        }
        return nullptr;
    }

    void release(Event* event) noexcept {
        if (event == nullptr || rustInstance_ == nullptr) return;
        n8_rust_event_pool_release(rustInstance_, event->getRustInstance());
    }

    size_t getCapacity() const noexcept {
        if (rustInstance_ != nullptr) {
            return n8_rust_event_pool_get_capacity(rustInstance_);
        }
        return 0;
    }

    size_t getActiveCount() const noexcept {
        if (rustInstance_ != nullptr) {
            return n8_rust_event_pool_get_active_count(rustInstance_);
        }
        return 0;
    }

    size_t getAvailableCount() const noexcept {
        if (rustInstance_ != nullptr) {
            return n8_rust_event_pool_get_available_count(rustInstance_);
        }
        return 0;
    }

    Event* getWrapperForRustEvent(RustEvent* rEv) noexcept {
        if (rEv == nullptr || rustInstance_ == nullptr) return nullptr;
        size_t index = n8_rust_event_pool_get_event_index(rustInstance_, rEv);
        if (index < eventWrappers_.size()) {
            return &eventWrappers_[index];
        }
        return nullptr;
    }

    RustEventPool* getRustInstance() noexcept { return rustInstance_; }
    const RustEventPool* getRustInstance() const noexcept { return rustInstance_; }

private:
    RustEventPool* rustInstance_{ nullptr };
    bool ownsCore_{ true };
    std::vector<Event> eventWrappers_;
};

} // namespace audio_graph
