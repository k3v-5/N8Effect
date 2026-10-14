#pragma once

#include <unordered_map>
#include <unordered_set>
#include <array>
#include <cmath>
#include <algorithm>
#include <vector>
#include "../core/Types.h"
#include "../graph/Graph.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Snapshot acústico de una escena (Regla 7, 21, 35).
 */
struct SceneSnapshot {
    std::unordered_map<NodeId, std::unordered_map<ParameterId, float>> nodeParameters;
    std::array<float, 8> macros{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    bool isCaptured{ false };

    void clear() {
        nodeParameters.clear();
        macros.fill(0.5f);
        isCaptured = false;
    }
};

/**
 * @brief Gestor de Escenas y Morphing continuo 1D / 2D con suavizado Anti-Click (Reglas 7, 21, 35, 46, 47).
 * Soporta interpolación bilineal lock-free entre 4 escenas acústicas A, B, C, D (Pad X/Y)
 * y delegación optimizada al motor Rust con cero dynamic allocations en tiempo real.
 */
class SceneManager {
public:
    SceneManager() {
        rustEngine_ = n8_rust_scene_morph_create();
    }

    ~SceneManager() {
        if (rustEngine_ != nullptr) {
            n8_rust_scene_morph_destroy(rustEngine_);
            rustEngine_ = nullptr;
        }
    }

    SceneManager(const SceneManager& other)
        : scenes_(other.scenes_),
          targetMorphFactor_(other.targetMorphFactor_),
          currentMorphFactor_(other.currentMorphFactor_),
          targetMorphX_(other.targetMorphX_),
          targetMorphY_(other.targetMorphY_),
          currentMorphX_(other.currentMorphX_),
          currentMorphY_(other.currentMorphY_),
          use2DMorph_(other.use2DMorph_)
    {
        rustEngine_ = n8_rust_scene_morph_create();
        syncRustScenes();
    }

    SceneManager& operator=(const SceneManager& other) {
        if (this != &other) {
            scenes_ = other.scenes_;
            targetMorphFactor_ = other.targetMorphFactor_;
            currentMorphFactor_ = other.currentMorphFactor_;
            targetMorphX_ = other.targetMorphX_;
            targetMorphY_ = other.targetMorphY_;
            currentMorphX_ = other.currentMorphX_;
            currentMorphY_ = other.currentMorphY_;
            use2DMorph_ = other.use2DMorph_;
            syncRustScenes();
        }
        return *this;
    }

    SceneManager(SceneManager&& other) noexcept
        : scenes_(std::move(other.scenes_)),
          targetMorphFactor_(other.targetMorphFactor_),
          currentMorphFactor_(other.currentMorphFactor_),
          targetMorphX_(other.targetMorphX_),
          targetMorphY_(other.targetMorphY_),
          currentMorphX_(other.currentMorphX_),
          currentMorphY_(other.currentMorphY_),
          use2DMorph_(other.use2DMorph_),
          rustEngine_(other.rustEngine_)
    {
        other.rustEngine_ = nullptr;
    }

    SceneManager& operator=(SceneManager&& other) noexcept {
        if (this != &other) {
            if (rustEngine_ != nullptr) {
                n8_rust_scene_morph_destroy(rustEngine_);
            }
            scenes_ = std::move(other.scenes_);
            targetMorphFactor_ = other.targetMorphFactor_;
            currentMorphFactor_ = other.currentMorphFactor_;
            targetMorphX_ = other.targetMorphX_;
            targetMorphY_ = other.targetMorphY_;
            currentMorphX_ = other.currentMorphX_;
            currentMorphY_ = other.currentMorphY_;
            use2DMorph_ = other.use2DMorph_;
            rustEngine_ = other.rustEngine_;
            other.rustEngine_ = nullptr;
        }
        return *this;
    }

    // --- Captura de Escenas ---

    void captureScene(size_t sceneIdx, const Graph& graph, const std::array<float, 8>& macros) {
        if (sceneIdx >= 4) return;
        captureSnapshot(graph, macros, scenes_[sceneIdx], static_cast<uint32_t>(sceneIdx));
    }

    void captureSceneA(const Graph& graph, const std::array<float, 8>& macros) {
        captureScene(0, graph, macros);
    }

    void captureSceneB(const Graph& graph, const std::array<float, 8>& macros) {
        captureScene(1, graph, macros);
    }

    void captureSceneC(const Graph& graph, const std::array<float, 8>& macros) {
        captureScene(2, graph, macros);
    }

    void captureSceneD(const Graph& graph, const std::array<float, 8>& macros) {
        captureScene(3, graph, macros);
    }

    void clearScene(size_t sceneIdx) {
        if (sceneIdx >= 4) return;
        scenes_[sceneIdx].clear();
        if (rustEngine_ != nullptr) {
            n8_rust_scene_morph_clear_scene(rustEngine_, static_cast<uint32_t>(sceneIdx));
        }
    }

    [[nodiscard]] bool hasScene(size_t sceneIdx) const noexcept {
        return (sceneIdx < 4) && scenes_[sceneIdx].isCaptured;
    }

    [[nodiscard]] bool hasSceneA() const noexcept { return hasScene(0); }
    [[nodiscard]] bool hasSceneB() const noexcept { return hasScene(1); }
    [[nodiscard]] bool hasSceneC() const noexcept { return hasScene(2); }
    [[nodiscard]] bool hasSceneD() const noexcept { return hasScene(3); }

    [[nodiscard]] const SceneSnapshot& getScene(size_t sceneIdx) const noexcept {
        return scenes_[std::min(sceneIdx, size_t(3))];
    }

    // --- Controles de Morphing 1D ---

    void setMorphFactor(float t) noexcept {
        targetMorphFactor_ = std::clamp(t, 0.0f, 1.0f);
        use2DMorph_ = false;
        if (rustEngine_ != nullptr) {
            n8_rust_scene_morph_set_factor_1d(rustEngine_, targetMorphFactor_);
        }
    }

    [[nodiscard]] float getMorphFactor() const noexcept {
        return targetMorphFactor_;
    }

    [[nodiscard]] float getCurrentSmoothedMorph() const noexcept {
        return currentMorphFactor_;
    }

    // --- Controles de Morphing 2D (Pad X/Y) ---

    void setMorphXY(float x, float y) noexcept {
        targetMorphX_ = std::clamp(x, 0.0f, 1.0f);
        targetMorphY_ = std::clamp(y, 0.0f, 1.0f);
        use2DMorph_ = true;
        if (rustEngine_ != nullptr) {
            n8_rust_scene_morph_set_xy(rustEngine_, targetMorphX_, targetMorphY_);
        }
    }

    [[nodiscard]] float getMorphX() const noexcept { return targetMorphX_; }
    [[nodiscard]] float getMorphY() const noexcept { return targetMorphY_; }

    [[nodiscard]] float getCurrentSmoothedMorphX() const noexcept { return currentMorphX_; }
    [[nodiscard]] float getCurrentSmoothedMorphY() const noexcept { return currentMorphY_; }

    [[nodiscard]] bool is2DMorphActive() const noexcept { return use2DMorph_; }
    void set2DMorphActive(bool active) noexcept { use2DMorph_ = active; }

    /**
     * @brief Aplica la interpolación continua 2D (Pad X/Y) entre Escenas A, B, C y D a los parámetros del Grafo.
     * Incluye suavizado exponencial anti-click (Regla 35).
     */
    void updateAndApplyXY(Graph& graph, std::array<float, 8>& inOutMacros, float smoothingSpeed = 0.08f) {
        bool anyCaptured = false;
        for (const auto& s : scenes_) {
            if (s.isCaptured) { anyCaptured = true; break; }
        }
        if (!anyCaptured) return;

        if (rustEngine_ != nullptr) {
            // Suavizado anti-click exponencial acelerado en Rust para XY
            n8_rust_scene_morph_step_smoothing_xy(rustEngine_, smoothingSpeed, &currentMorphX_, &currentMorphY_);

            // Interpolación de los 8 macros globales
            n8_rust_scene_morph_interpolate_macros_xy(rustEngine_, inOutMacros.data());

            // Recolectar parámetros de todos los nodos en todas las escenas capturadas
            for (size_t sIdx = 0; sIdx < 4; ++sIdx) {
                if (!scenes_[sIdx].isCaptured) continue;

                for (const auto& [nodeId, params] : scenes_[sIdx].nodeParameters) {
                    auto* nodeInst = graph.getNode(nodeId);
                    if (!nodeInst || !nodeInst->processor) continue;

                    for (const auto& [paramId, val] : params) {
                        const float curVal = nodeInst->processor->getParameter(paramId);
                        const float interpolatedVal = n8_rust_scene_morph_interpolate_param_xy(
                            rustEngine_,
                            nodeId,
                            paramId,
                            curVal
                        );
                        nodeInst->processor->setParameter(paramId, interpolatedVal);
                    }
                }
            }
            return;
        }

        // Fallback escalar en C++
        currentMorphX_ += smoothingSpeed * (targetMorphX_ - currentMorphX_);
        currentMorphY_ += smoothingSpeed * (targetMorphY_ - currentMorphY_);

        const float x = currentMorphX_;
        const float y = currentMorphY_;

        float wA = scenes_[0].isCaptured ? (1.0f - x) * (1.0f - y) : 0.0f;
        float wB = scenes_[1].isCaptured ? x * (1.0f - y) : 0.0f;
        float wC = scenes_[2].isCaptured ? (1.0f - x) * y : 0.0f;
        float wD = scenes_[3].isCaptured ? x * y : 0.0f;

        const float totalW = wA + wB + wC + wD;
        if (totalW > 1e-6f) {
            wA /= totalW;
            wB /= totalW;
            wC /= totalW;
            wD /= totalW;
        }

        for (size_t m = 0; m < 8; ++m) {
            inOutMacros[m] = wA * scenes_[0].macros[m] +
                             wB * scenes_[1].macros[m] +
                             wC * scenes_[2].macros[m] +
                             wD * scenes_[3].macros[m];
        }

        for (size_t sIdx = 0; sIdx < 4; ++sIdx) {
            if (!scenes_[sIdx].isCaptured) continue;
            for (const auto& [nodeId, params] : scenes_[sIdx].nodeParameters) {
                auto* nodeInst = graph.getNode(nodeId);
                if (!nodeInst || !nodeInst->processor) continue;

                for (const auto& [paramId, _] : params) {
                    float valA = getParamValOrFallback(0, nodeId, paramId, nodeInst->processor->getParameter(paramId));
                    float valB = getParamValOrFallback(1, nodeId, paramId, nodeInst->processor->getParameter(paramId));
                    float valC = getParamValOrFallback(2, nodeId, paramId, nodeInst->processor->getParameter(paramId));
                    float valD = getParamValOrFallback(3, nodeId, paramId, nodeInst->processor->getParameter(paramId));

                    float interpolatedVal = wA * valA + wB * valB + wC * valC + wD * valD;
                    nodeInst->processor->setParameter(paramId, interpolatedVal);
                }
            }
        }
    }

    /**
     * @brief Aplica la interpolación (1D o 2D según modo activo) a los parámetros del Grafo.
     */
    void updateAndApply(Graph& graph, std::array<float, 8>& inOutMacros, float smoothingSpeed = 0.08f) {
        if (use2DMorph_ || scenes_[2].isCaptured || scenes_[3].isCaptured) {
            updateAndApplyXY(graph, inOutMacros, smoothingSpeed);
            return;
        }

        if (!scenes_[0].isCaptured && !scenes_[1].isCaptured) return;

        if (rustEngine_ != nullptr) {
            currentMorphFactor_ = n8_rust_scene_morph_step_smoothing_1d(rustEngine_, smoothingSpeed);
            n8_rust_scene_morph_interpolate_macros_1d(rustEngine_, inOutMacros.data());

            for (const auto& [nodeId, paramsA] : scenes_[0].nodeParameters) {
                auto* nodeInst = graph.getNode(nodeId);
                if (!nodeInst || !nodeInst->processor) continue;

                for (const auto& [paramId, valA] : paramsA) {
                    const float interpolatedVal = n8_rust_scene_morph_interpolate_param_1d(
                        rustEngine_,
                        nodeId,
                        paramId,
                        valA
                    );
                    nodeInst->processor->setParameter(paramId, interpolatedVal);
                }
            }

            if (scenes_[1].isCaptured) {
                for (const auto& [nodeId, paramsB] : scenes_[1].nodeParameters) {
                    if (scenes_[0].nodeParameters.contains(nodeId)) continue;
                    auto* nodeInst = graph.getNode(nodeId);
                    if (!nodeInst || !nodeInst->processor) continue;

                    for (const auto& [paramId, valB] : paramsB) {
                        const float interpolatedVal = n8_rust_scene_morph_interpolate_param_1d(
                            rustEngine_,
                            nodeId,
                            paramId,
                            valB
                        );
                        nodeInst->processor->setParameter(paramId, interpolatedVal);
                    }
                }
            }
            return;
        }

        // Fallback 1D escalar
        currentMorphFactor_ += smoothingSpeed * (targetMorphFactor_ - currentMorphFactor_);
        const float t = currentMorphFactor_;

        if (scenes_[0].isCaptured && !scenes_[1].isCaptured) {
            applySnapshotDirect(graph, inOutMacros, scenes_[0]);
            return;
        }
        if (!scenes_[0].isCaptured && scenes_[1].isCaptured) {
            applySnapshotDirect(graph, inOutMacros, scenes_[1]);
            return;
        }

        for (size_t i = 0; i < 8; ++i) {
            inOutMacros[i] = (1.0f - t) * scenes_[0].macros[i] + t * scenes_[1].macros[i];
        }

        for (const auto& [nodeId, paramsA] : scenes_[0].nodeParameters) {
            auto* nodeInst = graph.getNode(nodeId);
            if (!nodeInst || !nodeInst->processor) continue;

            auto itB = scenes_[1].nodeParameters.find(nodeId);
            for (const auto& [paramId, valA] : paramsA) {
                float valB = valA;
                if (itB != scenes_[1].nodeParameters.end()) {
                    auto paramItB = itB->second.find(paramId);
                    if (paramItB != itB->second.end()) {
                        valB = paramItB->second;
                    }
                }
                const float interpolatedVal = (1.0f - t) * valA + t * valB;
                nodeInst->processor->setParameter(paramId, interpolatedVal);
            }
        }
    }

private:
    float getParamValOrFallback(size_t sceneIdx, NodeId nodeId, ParameterId paramId, float fallback) const {
        if (sceneIdx >= 4 || !scenes_[sceneIdx].isCaptured) return fallback;
        auto nit = scenes_[sceneIdx].nodeParameters.find(nodeId);
        if (nit != scenes_[sceneIdx].nodeParameters.end()) {
            auto pit = nit->second.find(paramId);
            if (pit != nit->second.end()) return pit->second;
        }
        return fallback;
    }

    void captureSnapshot(const Graph& graph, const std::array<float, 8>& macros, SceneSnapshot& outScene, uint32_t rustSceneIdx) {
        outScene.nodeParameters.clear();
        outScene.macros = macros;

        if (rustEngine_ != nullptr) {
            n8_rust_scene_morph_clear_scene(rustEngine_, rustSceneIdx);
            n8_rust_scene_morph_set_macros(rustEngine_, rustSceneIdx, macros.data());
        }

        for (const auto& [nodeId, inst] : graph.getNodes()) {
            if (!inst || !inst->processor) continue;

            const auto params = inst->processor->getParameters();
            std::vector<RustParamEntry> rustParams;
            rustParams.reserve(params.size());

            for (const auto& p : params) {
                float v = inst->processor->getParameter(p.id);
                outScene.nodeParameters[nodeId][p.id] = v;
                rustParams.push_back(RustParamEntry{ .param_id = p.id, .value = v });
            }

            if (rustEngine_ != nullptr) {
                n8_rust_scene_morph_set_node_params(
                    rustEngine_,
                    rustSceneIdx,
                    nodeId,
                    rustParams.data(),
                    static_cast<uint32_t>(rustParams.size())
                );
            }
        }
        outScene.isCaptured = true;
    }

    void syncRustScenes() {
        if (rustEngine_ == nullptr) return;
        for (uint32_t i = 0; i < 4; ++i) {
            if (scenes_[i].isCaptured) {
                n8_rust_scene_morph_clear_scene(rustEngine_, i);
                n8_rust_scene_morph_set_macros(rustEngine_, i, scenes_[i].macros.data());
                for (const auto& [nodeId, params] : scenes_[i].nodeParameters) {
                    std::vector<RustParamEntry> rParams;
                    rParams.reserve(params.size());
                    for (const auto& [pid, val] : params) {
                        rParams.push_back(RustParamEntry{ .param_id = pid, .value = val });
                    }
                    n8_rust_scene_morph_set_node_params(rustEngine_, i, nodeId, rParams.data(), static_cast<uint32_t>(rParams.size()));
                }
            } else {
                n8_rust_scene_morph_clear_scene(rustEngine_, i);
            }
        }
        n8_rust_scene_morph_set_factor_1d(rustEngine_, targetMorphFactor_);
        n8_rust_scene_morph_set_xy(rustEngine_, targetMorphX_, targetMorphY_);
    }

    void applySnapshotDirect(Graph& graph, std::array<float, 8>& inOutMacros, const SceneSnapshot& scene) {
        inOutMacros = scene.macros;
        for (const auto& [nodeId, params] : scene.nodeParameters) {
            auto* nodeInst = graph.getNode(nodeId);
            if (!nodeInst || !nodeInst->processor) continue;

            for (const auto& [paramId, val] : params) {
                nodeInst->processor->setParameter(paramId, val);
            }
        }
    }

    std::array<SceneSnapshot, 4> scenes_{};
    float targetMorphFactor_{ 0.0f };
    float currentMorphFactor_{ 0.0f };

    float targetMorphX_{ 0.0f };
    float targetMorphY_{ 0.0f };
    float currentMorphX_{ 0.0f };
    float currentMorphY_{ 0.0f };
    bool use2DMorph_{ false };

    RustSceneMorphEngine* rustEngine_{ nullptr };
};

} // namespace audio_graph
