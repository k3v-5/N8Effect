#pragma once

#include "AudioProcessorNode.h"

namespace audio_graph {

/**
 * @brief Base CRTP (Curiously Recurring Template Pattern) para despacho estático y loop fusion (Reglas 5, 14, 46).
 * Permite que los procesadores DSP participen en contenedores de enlace estático (StaticSerialChain)
 * eliminando por completo la sobrecarga de saltos de tabla virtual (vtable lookup)
 * y posibilitando inlining agresivo y optimizaciones SIMD en tiempo de compilación.
 */
template <typename Derived>
class CRTPProcessorNode {
public:
    void prepareStatic(const ProcessSpec& spec) {
        static_cast<Derived*>(this)->prepare(spec);
    }

    void resetStatic() noexcept {
        static_cast<Derived*>(this)->reset();
    }

    void processStatic(ProcessContext& context) {
        static_cast<Derived*>(this)->process(context);
    }

    template <typename... Args>
    float processSampleStatic(Args&&... args) noexcept {
        return static_cast<Derived*>(this)->processSample(std::forward<Args>(args)...);
    }
};

} // namespace audio_graph
