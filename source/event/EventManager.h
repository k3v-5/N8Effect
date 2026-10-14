#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <atomic>
#include "EventTypes.h"
#include "EventCaptureBuffer.h"
#include "EventPool.h"
#include "EventTelemetryBuffer.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Administrador y Orquestador de Eventos en tiempo real (Reglas 2, 3, 10, 11, 12, 35, 46)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class EventManager {
public:
    static constexpr uint32_t MaxGenerations = 4; // Protección contra multiplicación exponencial (Regla 11 y 35)

    explicit EventManager(RustEventManager* existing = nullptr, bool owns = true)
        : rustInstance_(existing != nullptr ? existing : n8_rust_event_manager_create()),
          ownsCore_(existing == nullptr || owns),
          captureBuffer_(nullptr, false),
          eventPool_(nullptr, false),
          telemetryBuffer_(nullptr, false)
    {
        syncSubWrappers();
    }

    ~EventManager() {
        if (ownsCore_ && rustInstance_ != nullptr) {
            n8_rust_event_manager_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    EventManager(EventManager&& other) noexcept
        : rustInstance_(other.rustInstance_),
          ownsCore_(other.ownsCore_),
          sampleRate_(other.sampleRate_),
          maxBlockSize_(other.maxBlockSize_),
          captureBuffer_(std::move(other.captureBuffer_)),
          eventPool_(std::move(other.eventPool_)),
          telemetryBuffer_(std::move(other.telemetryBuffer_))
    {
        other.rustInstance_ = nullptr;
        other.ownsCore_ = false;
    }

    EventManager& operator=(EventManager&& other) noexcept {
        if (this != &other) {
            if (ownsCore_ && rustInstance_ != nullptr) {
                n8_rust_event_manager_destroy(rustInstance_);
            }
            rustInstance_ = other.rustInstance_;
            ownsCore_ = other.ownsCore_;
            sampleRate_ = other.sampleRate_;
            maxBlockSize_ = other.maxBlockSize_;
            captureBuffer_ = std::move(other.captureBuffer_);
            eventPool_ = std::move(other.eventPool_);
            telemetryBuffer_ = std::move(other.telemetryBuffer_);
            other.rustInstance_ = nullptr;
            other.ownsCore_ = false;
        }
        return *this;
    }

    void prepare(double sampleRate, uint32_t maxBlockSize, size_t poolCapacity = EventPool::DefaultMaxEvents) {
        sampleRate_ = sampleRate;
        maxBlockSize_ = maxBlockSize;

        if (rustInstance_ != nullptr) {
            n8_rust_event_manager_prepare(rustInstance_, sampleRate, maxBlockSize, poolCapacity);
            syncSubWrappers();
            eventPool_.prepare(poolCapacity);
        }
    }

    void reset() noexcept {
        if (rustInstance_ != nullptr) {
            n8_rust_event_manager_reset(rustInstance_);
            eventPool_.reset();
            telemetryBuffer_.reset();
        }
    }

    // Registra audio entrante en el buffer de captura (Regla 9)
    void captureInputAudio(const float* const* input, uint32_t numChannels, uint32_t numSamples) noexcept {
        if (rustInstance_ == nullptr || input == nullptr || numSamples == 0) return;
        const float* inL = (numChannels > 0) ? input[0] : nullptr;
        const float* inR = (numChannels > 1) ? input[1] : inL;
        n8_rust_event_manager_capture_input(rustInstance_, inL, inR, numSamples);
    }

    // Genera un nuevo evento primario (generación 0)
    Event* spawnEvent(EventType type,
                      double captureOffsetSamples,
                      uint64_t durationSamples,
                      float pitchRatio = 1.0f,
                      float gain = 1.0f,
                      float pan = 0.0f,
                      float sourceFollow = 0.0f,
                      SourceDisappearanceMode mode = SourceDisappearanceMode::Fade,
                      bool reverse = false) noexcept
    {
        if (rustInstance_ == nullptr) return nullptr;

        RustEvent* rEv = n8_rust_event_manager_spawn_event(
            rustInstance_,
            static_cast<uint16_t>(type),
            captureOffsetSamples,
            durationSamples,
            pitchRatio,
            gain,
            pan,
            sourceFollow,
            static_cast<uint8_t>(mode),
            reverse
        );

        if (rEv == nullptr) {
            return nullptr;
        }

        return eventPool_.getWrapperForRustEvent(rEv);
    }

    // Spawning de un evento hijo desde un evento padre (Reglas 35 y 36)
    Event* spawnChildEvent(const Event& parent,
                           double offsetShiftSamples,
                           float pitchMultiplier = 1.0f,
                           float energyDecay = 0.7f) noexcept
    {
        if (rustInstance_ == nullptr || parent.getRustInstance() == nullptr) return nullptr;

        RustEvent* child = n8_rust_event_manager_spawn_child_event(
            rustInstance_,
            parent.getRustInstance(),
            offsetShiftSamples,
            pitchMultiplier,
            energyDecay
        );

        if (child == nullptr) {
            return nullptr;
        }

        return eventPool_.getWrapperForRustEvent(child);
    }

    // Renderiza todos los eventos activos hacia los buffers estéreo de salida (Regla 9 y 10, Punto 27)
    void render(float* outL, float* outR, uint32_t numSamples, float sourceLevel, int sourceSilenceOverride = -1) noexcept {
        if (rustInstance_ != nullptr) {
            n8_rust_event_manager_render(rustInstance_, outL, outR, numSamples, sourceLevel, sourceSilenceOverride);
        }
    }

    void setEditorActive(bool active) noexcept {
        if (rustInstance_ != nullptr) {
            n8_rust_event_manager_set_editor_active(rustInstance_, active);
        }
    }

    [[nodiscard]] bool isEditorActive() const noexcept {
        if (rustInstance_ != nullptr) {
            return n8_rust_event_manager_is_editor_active(rustInstance_);
        }
        return false;
    }

    size_t getActiveEventCount() const noexcept {
        if (rustInstance_ != nullptr) {
            return n8_rust_event_manager_get_active_count(rustInstance_);
        }
        return 0;
    }

    size_t getPoolCapacity() const noexcept {
        if (rustInstance_ != nullptr) {
            return n8_rust_event_manager_get_pool_capacity(rustInstance_);
        }
        return 0;
    }

    const EventCaptureBuffer& getCaptureBuffer() const noexcept { return captureBuffer_; }
    EventCaptureBuffer& getCaptureBuffer() noexcept { return captureBuffer_; }

    EventTelemetryBuffer& getTelemetryBuffer() noexcept { return telemetryBuffer_; }
    const EventTelemetryBuffer& getTelemetryBuffer() const noexcept { return telemetryBuffer_; }

    EventPool& getEventPool() noexcept { return eventPool_; }
    const EventPool& getEventPool() const noexcept { return eventPool_; }

    RustEventManager* getRustInstance() noexcept { return rustInstance_; }
    const RustEventManager* getRustInstance() const noexcept { return rustInstance_; }

private:
    void syncSubWrappers() {
        if (rustInstance_ != nullptr) {
            captureBuffer_ = EventCaptureBuffer(n8_rust_event_manager_get_capture_buffer(rustInstance_), false);
            eventPool_ = EventPool(n8_rust_event_manager_get_event_pool(rustInstance_), false);
            telemetryBuffer_ = EventTelemetryBuffer(n8_rust_event_manager_get_telemetry_buffer(rustInstance_), false);
        }
    }

    RustEventManager* rustInstance_{ nullptr };
    bool ownsCore_{ true };

    double sampleRate_{ 44100.0 };
    uint32_t maxBlockSize_{ 512 };

    EventCaptureBuffer captureBuffer_;
    EventPool eventPool_;
    EventTelemetryBuffer telemetryBuffer_;
};

} // namespace audio_graph
