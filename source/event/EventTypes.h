#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

namespace audio_graph {

using EventId = uint32_t;
constexpr EventId InvalidEventId = 0;

// Tipos de eventos reconocidos por el motor (Regla 2 y Sección 4.1 del Requerimiento Maestro)
enum class EventType : uint16_t {
    Unknown = 0,
    Fragment,
    Grain,
    Transient,
    Note,
    AudioSlice,
    Loop,
    Burst,
    Echo,
    Tail,
    Freeze,
    Reverse,
    PitchEvent,
    SpectralEvent,
    GeneratedEvent
};

// Ciclo de vida explícito obligatorio (Regla 2: Sin lógica implícita)
enum class EventLifecycle : uint8_t {
    Created,
    Playing,
    Releasing,
    Frozen,
    Looping,
    Killed,
    Destroyed
};

// Comportamiento ante la desaparición de la señal fuente (Regla 3)
enum class SourceDisappearanceMode : uint8_t {
    Cut,      // Corte inmediato del evento
    Short,    // Acortar duración restante
    Fade,     // Aplicar fade-out rápido
    Natural,  // Continuar su envolvente normal
    Hold,     // Mantener el nivel actual
    Freeze    // Congelar buffer en bucle infinito
};

// Atributos dinámicos del evento (Regla 37)
struct EventAttributes {
    EventId id{ InvalidEventId };
    EventType type{ EventType::Fragment };
    EventLifecycle state{ EventLifecycle::Created };

    // Grado de dependencia de la fuente (Regla 3: 0.0 = autónomo, 1.0 = dependiente)
    float sourceFollow{ 0.0f };
    SourceDisappearanceMode disappearanceMode{ SourceDisappearanceMode::Fade };

    // Propiedades acústicas y espaciales
    float pitchRatio{ 1.0f };     // 1.0 = pitch original, 2.0 = octava arriba, 0.5 = octava abajo
    float gain{ 1.0f };           // Amplitud base (0.0 a 2.0)
    float pan{ 0.0f };            // -1.0 (Izquierda) a +1.0 (Derecha)
    float azimuth{ 0.0f };        // -180° a +180° (grados)
    float elevation{ 0.0f };      // -90° a +90° (grados)
    float distance{ 1.0f };       // 0.1m a 10.0m
    float energy{ 1.0f };         // Nivel de energía para spawning/feedback (Reglas 11 y 35)

    // Jerarquía de Spawning (Reglas 11, 35 y 36)
    uint32_t generation{ 0 };     // Generación (0 = padre primario)
    EventId parentId{ InvalidEventId };

    // Temporización
    uint64_t ageSamples{ 0 };
    uint64_t lifetimeSamples{ 0 };
    uint32_t attackSamples{ 128 };
    uint32_t releaseSamples{ 512 };
};

} // namespace audio_graph
