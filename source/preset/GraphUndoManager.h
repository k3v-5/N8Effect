#pragma once

#include <vector>
#include <string>
#include "GraphSerializer.h"

namespace audio_graph {

/**
 * @brief Gestor de Pila Undo / Redo para ediciones de topología y parámetros del Grafo (Regla 22).
 */
class GraphUndoManager {
public:
    explicit GraphUndoManager(size_t maxHistorySteps = 32)
        : maxSteps_(maxHistorySteps) {}

    void pushState(const Graph& graph,
                   const PresetMetadata& meta = PresetMetadata{},
                   const std::array<float, 8>& macros = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f }) {
        std::string serialized = GraphSerializer::serialize(graph, meta, macros);
        undoStack_.push_back(std::move(serialized));

        if (undoStack_.size() > maxSteps_) {
            undoStack_.erase(undoStack_.begin());
        }

        redoStack_.clear(); // Nueva acción invalida el historial de Redo
    }

    bool canUndo() const noexcept {
        return undoStack_.size() > 1; // Debe haber al menos un estado previo al actual
    }

    bool canRedo() const noexcept {
        return !redoStack_.empty();
    }

    bool undo(Graph& outGraph, PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
        if (!canUndo()) return false;

        // Mover estado actual a la pila de Redo
        std::string current = std::move(undoStack_.back());
        undoStack_.pop_back();
        redoStack_.push_back(std::move(current));

        // Restaurar estado anterior
        const std::string& prevState = undoStack_.back();
        std::string err;
        return GraphSerializer::deserialize(prevState, outGraph, outMeta, outMacros, err);
    }

    bool redo(Graph& outGraph, PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
        if (!canRedo()) return false;

        std::string nextState = std::move(redoStack_.back());
        redoStack_.pop_back();

        std::string err;
        bool ok = GraphSerializer::deserialize(nextState, outGraph, outMeta, outMacros, err);
        if (ok) {
            undoStack_.push_back(std::move(nextState));
        }
        return ok;
    }

    void clear() noexcept {
        undoStack_.clear();
        redoStack_.clear();
    }

private:
    size_t maxSteps_{ 32 };
    std::vector<std::string> undoStack_;
    std::vector<std::string> redoStack_;
};

} // namespace audio_graph
