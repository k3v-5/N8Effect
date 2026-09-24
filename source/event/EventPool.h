#pragma once

#include <vector>
#include <cstdint>
#include <cassert>
#include "Event.h"

namespace audio_graph {

/**
 * @brief Pool de Eventos con capacidad fija preasignada (Reglas 9, 10, 11, 46)
 * Capacidad por defecto: 1024 eventos. Cero allocations en el hilo de audio.
 */
class EventPool {
public:
    static constexpr size_t DefaultMaxEvents = 1024;

    EventPool() = default;

    void prepare(size_t maxEvents = DefaultMaxEvents) {
        maxEvents_ = maxEvents;
        storage_.clear();
        freeIndices_.clear();

        storage_.resize(maxEvents_);
        freeIndices_.resize(maxEvents_);

        for (size_t i = 0; i < maxEvents_; ++i) {
            freeIndices_[i] = static_cast<uint32_t>(i);
        }
        freeCount_ = maxEvents_;
    }

    void reset() noexcept {
        freeCount_ = maxEvents_;
        for (size_t i = 0; i < maxEvents_; ++i) {
            freeIndices_[i] = static_cast<uint32_t>(i);
            storage_[i].kill();
            storage_[i].setState(EventLifecycle::Destroyed);
        }
    }

    // Adquiere un evento del pool sin asignaciones dinámicas (Regla 9 y 10)
    Event* acquire() noexcept {
        if (freeCount_ == 0) {
            // Protección de CPU: límite alcanzado (Regla 11)
            return nullptr;
        }

        --freeCount_;
        uint32_t index = freeIndices_[freeCount_];
        return &storage_[index];
    }

    // Libera un evento devolviéndolo al pool (Regla 10)
    void release(Event* event) noexcept {
        if (event == nullptr) return;

        // Comprobar que pertenece al rango de memoria del pool
        const auto* base = storage_.data();
        const auto* ptr = event;
        if (ptr < base || ptr >= base + maxEvents_) {
            return;
        }

        size_t index = static_cast<size_t>(ptr - base);
        event->kill();
        event->setState(EventLifecycle::Destroyed);

        assert(freeCount_ < maxEvents_);
        freeIndices_[freeCount_] = static_cast<uint32_t>(index);
        ++freeCount_;
    }

    size_t getCapacity() const noexcept { return maxEvents_; }
    size_t getActiveCount() const noexcept { return maxEvents_ - freeCount_; }
    size_t getAvailableCount() const noexcept { return freeCount_; }

private:
    size_t maxEvents_{ DefaultMaxEvents };
    size_t freeCount_{ 0 };
    std::vector<Event> storage_;
    std::vector<uint32_t> freeIndices_;
};

} // namespace audio_graph
