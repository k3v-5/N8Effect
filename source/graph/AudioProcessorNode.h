#pragma once

#include <string_view>
#include <vector>
#include <span>
#include <cstdint>
#include "../core/Types.h"

namespace audio_graph {

/**
 * @brief Metadatos descriptivos de un parámetro (Regla 8)
 */
struct ParameterInfo {
    ParameterId id{ 0 };
    const char* name{ "" };
    float defaultValue{ 0.0f };
    float minValue{ 0.0f };
    float maxValue{ 1.0f };
    bool isSmoothed{ true };
};

/**
 * @brief Descripción de un puerto o pin del nodo (Regla 4 y 24)
 */
struct PinDescriptor {
    PinId id{ 0 };
    const char* name{ "" };
    PinType type{ PinType::AudioInput };
    PinDataType dataType{ PinDataType::AudioStereo };
};

/**
 * @brief Interfaz Universal de Nodo de Procesamiento (Reglas 5, 10, 13)
 * Todos los efectos, generadores, contenedores y utilidades implementan esta interfaz.
 */
class AudioProcessorNode {
public:
    virtual ~AudioProcessorNode() = default;

    // Inicialización y preparación para el sample rate y block size actuales (Reglas 14 y 15)
    virtual void prepare(const ProcessSpec& spec) = 0;

    // Procesamiento en tiempo real (Regla 9: Cero alocaciones)
    virtual void process(ProcessContext& context) = 0;

    // Reseteo de buffers internos / fase / estados de filtro (Regla 38)
    virtual void reset() = 0;

    // Control de Parámetros por ID numérico (Regla 8)
    virtual void setParameter(ParameterId id, float value) = 0;
    virtual float getParameter(ParameterId id) const = 0;

    // Metadatos e información de capacidades
    virtual NodeType getType() const = 0;
    virtual const char* getName() const = 0;
    virtual uint32_t getLatencySamples() const { return 0; } // Compensación de latencia (Regla 46)
    virtual bool supportsTail() const { return false; }      // Manejo de Tails (Regla 18)
    virtual uint32_t getTailSamples() const { return 0; }

    // Catálogo de pines y parámetros
    virtual std::span<const PinDescriptor> getPins() const = 0;
    virtual std::span<const ParameterInfo> getParameters() const = 0;
};

} // namespace audio_graph
