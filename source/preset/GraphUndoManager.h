#pragma once

#include <vector>
#include <string>
#include <chrono>
#include "GraphSerializer.h"

namespace audio_graph {

/**
 * @brief Categorías semánticas de acciones en el grafo para el historial visual interactivo.
 */
enum class ActionCategory : uint8_t {
    NodeAdd = 0,
    NodeRemove = 1,
    Connection = 2,
    Disconnection = 3,
    ParameterChange = 4,
    PresetLoad = 5,
    Randomize = 6,
    General = 7
};

/**
 * @brief Checkpoint histórico individual en la línea de tiempo de Undo/Redo (Reglas 21, 22, 47).
 */
struct UndoActionCheckpoint {
    std::string description;
    ActionCategory category{ ActionCategory::General };
    int64_t timestampMs{ 0 };
    std::string serializedState;
};

/**
 * @brief Gestor de Historial Gráfico Undo / Redo con Timeline de Acciones y Time-Travel (Reglas 21, 22, 47).
 * Mantiene una pila acotada de checkpoints cronológicos y permite saltar directamente a cualquier punto
 * del historial sin requerir desacer paso por paso.
 */
class GraphUndoManager {
public:
    explicit GraphUndoManager(size_t maxHistorySteps = 32)
        : maxSteps_(maxHistorySteps) {}

    void pushAction(std::string description,
                    ActionCategory category,
                    const Graph& graph,
                    const PresetMetadata& meta = PresetMetadata{},
                    const std::array<float, 8>& macros = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f })
    {
        std::string serialized = GraphSerializer::serialize(graph, meta, macros);
        const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // Si estábamos en el pasado y se realiza una nueva acción, truncar el futuro
        if (!timeline_.empty() && currentIndex_ + 1 < timeline_.size()) {
            timeline_.erase(timeline_.begin() + static_cast<ptrdiff_t>(currentIndex_ + 1), timeline_.end());
        }

        timeline_.push_back({
            .description = std::move(description),
            .category = category,
            .timestampMs = now,
            .serializedState = std::move(serialized)
        });

        // Control estricto de memoria acotada (Regla 47)
        if (timeline_.size() > maxSteps_) {
            timeline_.erase(timeline_.begin());
        }

        currentIndex_ = timeline_.size() - 1;
    }

    void pushState(const Graph& graph,
                   const PresetMetadata& meta = PresetMetadata{},
                   const std::array<float, 8>& macros = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f })
    {
        pushAction("Graph Modified", ActionCategory::General, graph, meta, macros);
    }

    [[nodiscard]] bool canUndo() const noexcept {
        return !timeline_.empty() && currentIndex_ > 0;
    }

    [[nodiscard]] bool canRedo() const noexcept {
        return !timeline_.empty() && (currentIndex_ + 1 < timeline_.size());
    }

    bool undo(Graph& outGraph, PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
        if (!canUndo()) return false;

        currentIndex_--;
        const std::string& prevState = timeline_[currentIndex_].serializedState;
        std::string err;
        return GraphSerializer::deserialize(prevState, outGraph, outMeta, outMacros, err);
    }

    bool redo(Graph& outGraph, PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
        if (!canRedo()) return false;

        currentIndex_++;
        const std::string& nextState = timeline_[currentIndex_].serializedState;
        std::string err;
        return GraphSerializer::deserialize(nextState, outGraph, outMeta, outMacros, err);
    }

    bool jumpToStep(size_t targetIndex, Graph& outGraph, PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
        if (targetIndex >= timeline_.size() || targetIndex == currentIndex_) return false;

        currentIndex_ = targetIndex;
        const std::string& targetState = timeline_[currentIndex_].serializedState;
        std::string err;
        return GraphSerializer::deserialize(targetState, outGraph, outMeta, outMacros, err);
    }

    [[nodiscard]] size_t getCurrentIndex() const noexcept {
        return currentIndex_;
    }

    [[nodiscard]] size_t getHistorySize() const noexcept {
        return timeline_.size();
    }

    [[nodiscard]] const std::vector<UndoActionCheckpoint>& getTimeline() const noexcept {
        return timeline_;
    }

    [[nodiscard]] std::string getNextUndoDescription() const {
        if (!canUndo()) return "";
        return timeline_[currentIndex_].description;
    }

    [[nodiscard]] std::string getNextRedoDescription() const {
        if (!canRedo()) return "";
        return timeline_[currentIndex_ + 1].description;
    }

    void clear() noexcept {
        timeline_.clear();
        currentIndex_ = 0;
    }

private:
    size_t maxSteps_{ 32 };
    std::vector<UndoActionCheckpoint> timeline_;
    size_t currentIndex_{ 0 };
};

} // namespace audio_graph
