#pragma once

#include <array>
#include <vector>
#include <algorithm>
#include <cstdint>
#include "ModulationTypes.h"
#include "../graph/Graph.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Matriz Centralizada de Modulación Universal (Reglas 7, 8, 25, 46)
 * Contiene hasta 64 rutas activas prealocadas para interconectar cualquier fuente
 * con cualquier parámetro del sistema sin asignación dinámica en tiempo real.
 * Delega el enrutamiento y cálculo de offsets netos al núcleo Rust.
 */
class ModulationMatrix {
public:
    static constexpr size_t MaxRoutes = 64;

    explicit ModulationMatrix(RustModulationMatrix* existing = nullptr, bool owns = true)
        : rustCore_(existing ? existing : n8_rust_modulation_matrix_create()), ownsCore_(owns)
    {
    }

    ~ModulationMatrix() {
        if (rustCore_ && ownsCore_) {
            n8_rust_modulation_matrix_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    ModulationMatrix(const ModulationMatrix&) = delete;
    ModulationMatrix& operator=(const ModulationMatrix&) = delete;

    ModulationMatrix(ModulationMatrix&& other) noexcept
        : rustCore_(other.rustCore_), ownsCore_(other.ownsCore_)
    {
        other.rustCore_ = nullptr;
        other.ownsCore_ = false;
    }

    ModulationMatrix& operator=(ModulationMatrix&& other) noexcept {
        if (this != &other) {
            if (rustCore_ && ownsCore_) {
                n8_rust_modulation_matrix_destroy(rustCore_);
            }
            rustCore_ = other.rustCore_;
            ownsCore_ = other.ownsCore_;
            other.rustCore_ = nullptr;
            other.ownsCore_ = false;
        }
        return *this;
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_modulation_matrix_reset(rustCore_);
        }
    }

    int addRoute(ModSourceType source, NodeId targetNode, ParameterId targetParam, float amount, bool bipolar = true) noexcept {
        if (rustCore_) {
            return n8_rust_modulation_matrix_add_route(
                rustCore_,
                static_cast<uint16_t>(source),
                targetNode,
                targetParam,
                amount,
                bipolar
            );
        }
        return -1;
    }

    void removeRoute(size_t index) noexcept {
        if (rustCore_) {
            n8_rust_modulation_matrix_remove_route(rustCore_, index);
        }
    }

    void setRouteAmount(size_t index, float amount) noexcept {
        if (rustCore_) {
            n8_rust_modulation_matrix_set_route_amount(rustCore_, index, amount);
        }
    }

    size_t getActiveRouteCount() const noexcept {
        if (rustCore_) {
            return n8_rust_modulation_matrix_get_active_route_count(rustCore_);
        }
        return 0;
    }

    const ModulationRoute& getRoute(size_t index) const noexcept {
        if (rustCore_) {
            RustModulationRoute rRoute;
            n8_rust_modulation_matrix_get_route(rustCore_, index, &rRoute);
            cachedRoute_.source = static_cast<ModSourceType>(rRoute.source);
            cachedRoute_.targetNodeId = rRoute.target_node_id;
            cachedRoute_.targetParamId = rRoute.target_param_id;
            cachedRoute_.amount = rRoute.amount;
            cachedRoute_.bipolar = rRoute.bipolar;
            cachedRoute_.isActive = rRoute.is_active;
        }
        return cachedRoute_;
    }

    size_t getMaxRoutes() const noexcept { return MaxRoutes; }

    template <size_t N>
    float calculateModulationOffset(NodeId targetNode, ParameterId targetParam, const std::array<float, N>& sourceValues) const noexcept {
        if (rustCore_) {
            return n8_rust_modulation_matrix_calculate_offset(
                rustCore_,
                targetNode,
                targetParam,
                sourceValues.data(),
                sourceValues.size()
            );
        }
        return 0.0f;
    }

private:
    RustModulationMatrix* rustCore_{ nullptr };
    bool ownsCore_{ true };
    mutable ModulationRoute cachedRoute_{};
};

} // namespace audio_graph
