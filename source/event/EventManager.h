#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include "EventTypes.h"
#include "EventCaptureBuffer.h"
#include "EventPool.h"
#include "EventTelemetryBuffer.h"

namespace audio_graph {

/**
 * @brief Administrador y Orquestador de Eventos en tiempo real (Reglas 2, 3, 10, 11, 12, 35, 46)
 */
class EventManager {
public:
    static constexpr uint32_t MaxGenerations = 4; // Protección contra multiplicación exponencial (Regla 11 y 35)

    EventManager() = default;

    void prepare(double sampleRate, uint32_t maxBlockSize, size_t poolCapacity = EventPool::DefaultMaxEvents) {
        sampleRate_ = sampleRate;
        maxBlockSize_ = maxBlockSize;

        captureBuffer_.prepare(sampleRate, 5.0, 2);
        eventPool_.prepare(poolCapacity);

        activeEvents_.clear();
        activeEvents_.reserve(poolCapacity);

        scratchL_.assign(maxBlockSize, 0.0f);
        scratchR_.assign(maxBlockSize, 0.0f);

        nextEventId_ = 1;
    }

    void reset() noexcept {
        captureBuffer_.reset();
        eventPool_.reset();
        activeEvents_.clear();
        telemetryBuffer_.reset();
    }

    // Registra audio entrante en el buffer de captura (Regla 9)
    void captureInputAudio(const float* const* input, uint32_t numChannels, uint32_t numSamples) noexcept {
        captureBuffer_.write(input, numChannels, numSamples);
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
        Event* event = eventPool_.acquire();
        if (event == nullptr) {
            return nullptr; // Límite de pool alcanzado de forma segura (Regla 11)
        }

        EventAttributes attrs;
        attrs.id = nextEventId_++;
        attrs.type = type;
        attrs.pitchRatio = pitchRatio;
        attrs.gain = gain;
        attrs.pan = pan;
        attrs.sourceFollow = sourceFollow;
        attrs.disappearanceMode = mode;
        attrs.generation = 0;
        attrs.parentId = InvalidEventId;
        attrs.energy = 1.0f;
        attrs.attackSamples = static_cast<uint32_t>(sampleRate_ * 0.005); // 5ms ataque anti-click
        attrs.releaseSamples = static_cast<uint32_t>(sampleRate_ * 0.020); // 20ms release anti-click

        event->initialize(attrs, captureOffsetSamples, durationSamples, reverse);
        event->start();

        activeEvents_.push_back(event);
        return event;
    }

    // Spawning de un evento hijo desde un evento padre (Reglas 35 y 36)
    Event* spawnChildEvent(const Event& parent,
                           double offsetShiftSamples,
                           float pitchMultiplier = 1.0f,
                           float energyDecay = 0.7f) noexcept
    {
        const auto& parentAttrs = parent.getAttributes();
        if (parentAttrs.generation >= MaxGenerations) {
            return nullptr; // Límite de generaciones alcanzado (Regla 11)
        }

        Event* child = eventPool_.acquire();
        if (child == nullptr) {
            return nullptr;
        }

        EventAttributes attrs = parentAttrs;
        attrs.id = nextEventId_++;
        attrs.generation = parentAttrs.generation + 1;
        attrs.parentId = parentAttrs.id;
        attrs.energy = parentAttrs.energy * energyDecay;
        attrs.pitchRatio = parentAttrs.pitchRatio * pitchMultiplier;
        attrs.gain = parentAttrs.gain * attrs.energy;

        child->initialize(attrs, offsetShiftSamples, parentAttrs.lifetimeSamples, false);
        child->start();

        activeEvents_.push_back(child);
        return child;
    }

    // Renderiza todos los eventos activos hacia los buffers estéreo de salida (Regla 9 y 10)
    void render(float* outL, float* outR, uint32_t numSamples, float sourceLevel) noexcept {
        if (outL == nullptr || outR == nullptr || numSamples == 0) return;

        // Limpieza de buffers temporales
        std::fill_n(scratchL_.data(), numSamples, 0.0f);
        std::fill_n(scratchR_.data(), numSamples, 0.0f);

        size_t i = 0;
        while (i < activeEvents_.size()) {
            Event* event = activeEvents_[i];

            if (event != nullptr && event->isActive()) {
                event->render(captureBuffer_, scratchL_.data(), scratchR_.data(), numSamples, sourceLevel);

                // Telemetría lock-free hacia la GUI (Reglas 9, 23, 26)
                const auto& attrs = event->getAttributes();
                EventTelemetryItem tItem;
                tItem.pan = attrs.pan;
                tItem.pitchRatio = attrs.pitchRatio;
                tItem.energy = attrs.energy * attrs.gain;
                tItem.distance = attrs.distance;
                tItem.azimuth = attrs.azimuth;
                tItem.type = attrs.type;
                tItem.generation = static_cast<uint8_t>(attrs.generation);
                tItem.isAlive = true;
                telemetryBuffer_.push(tItem);

                ++i;
            } else {
                // El evento ha muerto o terminado su ciclo de vida: devolver al pool (Regla 10)
                eventPool_.release(event);

                // O(1) swap and pop para remoción segura en tiempo real
                activeEvents_[i] = activeEvents_.back();
                activeEvents_.pop_back();
            }
        }

        // Sumar al buffer final
        for (uint32_t s = 0; s < numSamples; ++s) {
            outL[s] += scratchL_[s];
            outR[s] += scratchR_[s];
        }
    }

    size_t getActiveEventCount() const noexcept { return activeEvents_.size(); }
    size_t getPoolCapacity() const noexcept { return eventPool_.getCapacity(); }
    const EventCaptureBuffer& getCaptureBuffer() const noexcept { return captureBuffer_; }
    EventTelemetryBuffer& getTelemetryBuffer() noexcept { return telemetryBuffer_; }
    const EventTelemetryBuffer& getTelemetryBuffer() const noexcept { return telemetryBuffer_; }

private:
    double sampleRate_{ 44100.0 };
    uint32_t maxBlockSize_{ 512 };
    uint32_t nextEventId_{ 1 };

    EventCaptureBuffer captureBuffer_;
    EventPool eventPool_;
    std::vector<Event*> activeEvents_;
    EventTelemetryBuffer telemetryBuffer_;

    std::vector<float> scratchL_;
    std::vector<float> scratchR_;
};

} // namespace audio_graph
