#pragma once

#include <unordered_map>
#include <array>
#include <cmath>
#include <algorithm>
#include "../core/Types.h"
#include "../graph/Graph.h"

namespace audio_graph {

/**
 * @brief Snapshot acústico de una escena (Regla 7, 21, 35).
 */
struct SceneSnapshot {
    std::unordered_map<NodeId, std::unordered_map<ParameterId, float>> nodeParameters;
    std::array<float, 8> macros{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    bool isCaptured{ false };
};

/**
 * @brief Gestor de Escenas y Morphing continuo con suavizado Anti-Click (Regla 7, 21, 35).
 */
class SceneManager {
public:
    SceneManager() = default;

    void captureSceneA(const Graph& graph, const std::array<float, 8>& macros) {
        captureSnapshot(graph, macros, sceneA_);
    }

    void captureSceneB(const Graph& graph, const std::array<float, 8>& macros) {
        captureSnapshot(graph, macros, sceneB_);
    }

    bool hasSceneA() const noexcept { return sceneA_.isCaptured; }
    bool hasSceneB() const noexcept { return sceneB_.isCaptured; }

    void setMorphFactor(float t) noexcept {
        targetMorphFactor_ = std::clamp(t, 0.0f, 1.0f);
    }

    float getMorphFactor() const noexcept {
        return targetMorphFactor_;
    }

    float getCurrentSmoothedMorph() const noexcept {
        return currentMorphFactor_;
    }

    /**
     * @brief Aplica la interpolación entre Escena A y Escena B a los parámetros del Grafo.
     * Incluye suavizado exponencial para prevenir clicks audibles (Regla 35).
     */
    void updateAndApply(Graph& graph, std::array<float, 8>& inOutMacros, float smoothingSpeed = 0.08f) {
        if (!sceneA_.isCaptured && !sceneB_.isCaptured) return;

        // Suavizado anti-click del factor de morphing
        currentMorphFactor_ += smoothingSpeed * (targetMorphFactor_ - currentMorphFactor_);
        const float t = currentMorphFactor_;

        // Si solo se ha capturado una de las escenas, t actúa como escala o se usa la disponible
        if (sceneA_.isCaptured && !sceneB_.isCaptured) {
            applySnapshotDirect(graph, inOutMacros, sceneA_);
            return;
        }
        if (!sceneA_.isCaptured && sceneB_.isCaptured) {
            applySnapshotDirect(graph, inOutMacros, sceneB_);
            return;
        }

        // Interpolación de Macros (0 a 7)
        for (size_t i = 0; i < 8; ++i) {
            inOutMacros[i] = (1.0f - t) * sceneA_.macros[i] + t * sceneB_.macros[i];
        }

        // Interpolación de Parámetros de Nodo
        for (const auto& [nodeId, paramsA] : sceneA_.nodeParameters) {
            auto* nodeInst = graph.getNode(nodeId);
            if (!nodeInst || !nodeInst->processor) continue;

            auto itB = sceneB_.nodeParameters.find(nodeId);
            for (const auto& [paramId, valA] : paramsA) {
                float valB = valA;
                if (itB != sceneB_.nodeParameters.end()) {
                    auto paramItB = itB->second.find(paramId);
                    if (paramItB != itB->second.end()) {
                        valB = paramItB->second;
                    }
                }

                // Interpolación lineal continua
                const float interpolatedVal = (1.0f - t) * valA + t * valB;
                nodeInst->processor->setParameter(paramId, interpolatedVal);
            }
        }
    }

private:
    void captureSnapshot(const Graph& graph, const std::array<float, 8>& macros, SceneSnapshot& outScene) {
        outScene.nodeParameters.clear();
        outScene.macros = macros;

        for (const auto& [nodeId, inst] : graph.getNodes()) {
            if (!inst || !inst->processor) continue;

            const auto params = inst->processor->getParameters();
            for (const auto& p : params) {
                outScene.nodeParameters[nodeId][p.id] = inst->processor->getParameter(p.id);
            }
        }
        outScene.isCaptured = true;
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

    SceneSnapshot sceneA_{};
    SceneSnapshot sceneB_{};
    float targetMorphFactor_{ 0.0f };
    float currentMorphFactor_{ 0.0f };
};

} // namespace audio_graph
