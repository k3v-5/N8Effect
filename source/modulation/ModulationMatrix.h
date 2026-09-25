#pragma once

#include <array>
#include <vector>
#include <algorithm>
#include <cstdint>
#include "ModulationTypes.h"
#include "../graph/Graph.h"

namespace audio_graph {

/**
 * @brief Matriz Centralizada de Modulación Universal (Reglas 7, 8, 25, 46)
 * Contiene hasta 64 rutas activas prealocadas para interconectar cualquier fuente
 * con cualquier parámetro del sistema sin asignación dinámica en tiempo real.
 */
class ModulationMatrix {
public:
    static constexpr size_t MaxRoutes = 64;

    ModulationMatrix() {
        for (auto& r : routes_) r.isActive = false;
    }

    void reset() noexcept {
        for (auto& r : routes_) r.isActive = false;
    }

    // Configura o añade una ruta de modulación
    int addRoute(ModSourceType source, NodeId targetNode, ParameterId targetParam, float amount, bool bipolar = true) noexcept {
        for (size_t i = 0; i < MaxRoutes; ++i) {
            if (!routes_[i].isActive) {
                routes_[i] = {
                    .source = source,
                    .targetNodeId = targetNode,
                    .targetParamId = targetParam,
                    .amount = std::clamp(amount, -1.0f, 1.0f),
                    .bipolar = bipolar,
                    .isActive = true
                };
                return static_cast<int>(i);
            }
        }
        return -1; // Matriz llena
    }

    void removeRoute(size_t index) noexcept {
        if (index < MaxRoutes) {
            routes_[index].isActive = false;
        }
    }

    void setRouteAmount(size_t index, float amount) noexcept {
        if (index < MaxRoutes && routes_[index].isActive) {
            routes_[index].amount = std::clamp(amount, -1.0f, 1.0f);
        }
    }

    size_t getActiveRouteCount() const noexcept {
        size_t count = 0;
        for (const auto& r : routes_) {
            if (r.isActive) ++count;
        }
        return count;
    }

    const ModulationRoute& getRoute(size_t index) const noexcept {
        return routes_[index < MaxRoutes ? index : 0];
    }

    size_t getMaxRoutes() const noexcept { return MaxRoutes; }

    // Calcula el offset neto de modulación para un nodo y parámetro específico
    float calculateModulationOffset(NodeId targetNode, ParameterId targetParam, const std::array<float, static_cast<size_t>(ModSourceType::Count)>& sourceValues) const noexcept {
        float netOffset = 0.0f;

        for (const auto& r : routes_) {
            if (r.isActive && r.targetNodeId == targetNode && r.targetParamId == targetParam) {
                size_t srcIdx = static_cast<size_t>(r.source);
                if (srcIdx < sourceValues.size()) {
                    float srcVal = sourceValues[srcIdx];
                    if (!r.bipolar) {
                        // Unipolar: mapear [-1.0, 1.0] a [0.0, 1.0]
                        srcVal = (srcVal + 1.0f) * 0.5f;
                    }
                    netOffset += srcVal * r.amount;
                }
            }
        }

        return netOffset;
    }

private:
    std::array<ModulationRoute, MaxRoutes> routes_;
};

} // namespace audio_graph
