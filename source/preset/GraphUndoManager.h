#pragma once

#include <vector>
#include <string>
#include <chrono>
#include "GraphSerializer.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

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
 * Acelerado por n8_rust_core con capacidad acotada, saltos en el tiempo directos y cero fugas de memoria.
 */
class GraphUndoManager {
public:
    explicit GraphUndoManager(size_t maxHistorySteps = 32)
        : maxSteps_(maxHistorySteps)
    {
        rustCore_ = n8_rust_graph_undo_manager_create(maxHistorySteps);
    }

    ~GraphUndoManager() {
        if (rustCore_) {
            n8_rust_graph_undo_manager_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    GraphUndoManager(const GraphUndoManager&) = delete;
    GraphUndoManager& operator=(const GraphUndoManager&) = delete;

    GraphUndoManager(GraphUndoManager&& other) noexcept
        : maxSteps_(other.maxSteps_),
          rustCore_(other.rustCore_),
          cachedTimeline_(std::move(other.cachedTimeline_))
    {
        other.rustCore_ = nullptr;
    }

    GraphUndoManager& operator=(GraphUndoManager&& other) noexcept {
        if (this != &other) {
            if (rustCore_) {
                n8_rust_graph_undo_manager_destroy(rustCore_);
            }
            maxSteps_ = other.maxSteps_;
            rustCore_ = other.rustCore_;
            cachedTimeline_ = std::move(other.cachedTimeline_);
            other.rustCore_ = nullptr;
        }
        return *this;
    }

    void pushAction(std::string description,
                    ActionCategory category,
                    const Graph& graph,
                    const PresetMetadata& meta = PresetMetadata{},
                    const std::array<float, 8>& macros = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f })
    {
        std::string serialized = GraphSerializer::serialize(graph, meta, macros);
        const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        if (rustCore_) {
            n8_rust_graph_undo_manager_push_action(
                rustCore_,
                description.c_str(),
                static_cast<uint8_t>(category),
                serialized.c_str(),
                now
            );
        }
    }

    void pushState(const Graph& graph,
                   const PresetMetadata& meta = PresetMetadata{},
                   const std::array<float, 8>& macros = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f })
    {
        pushAction("Graph Modified", ActionCategory::General, graph, meta, macros);
    }

    [[nodiscard]] bool canUndo() const noexcept {
        return rustCore_ && n8_rust_graph_undo_manager_can_undo(rustCore_);
    }

    [[nodiscard]] bool canRedo() const noexcept {
        return rustCore_ && n8_rust_graph_undo_manager_can_redo(rustCore_);
    }

    bool undo(Graph& outGraph, PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
        if (!canUndo()) return false;

        size_t currentIdx = n8_rust_graph_undo_manager_get_current_index(rustCore_);
        if (currentIdx == 0) return false;
        size_t stateLen = n8_rust_graph_undo_manager_get_state_len(rustCore_, currentIdx - 1);
        std::string stateBuf;
        stateBuf.resize(stateLen + 1);

        if (!n8_rust_graph_undo_manager_undo(rustCore_, stateBuf.data(), stateBuf.size())) {
            return false;
        }
        stateBuf.resize(stateLen);

        std::string err;
        return GraphSerializer::deserialize(stateBuf, outGraph, outMeta, outMacros, err);
    }

    bool redo(Graph& outGraph, PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
        if (!canRedo()) return false;

        size_t currentIdx = n8_rust_graph_undo_manager_get_current_index(rustCore_);
        size_t stateLen = n8_rust_graph_undo_manager_get_state_len(rustCore_, currentIdx + 1);
        std::string stateBuf;
        stateBuf.resize(stateLen + 1);

        if (!n8_rust_graph_undo_manager_redo(rustCore_, stateBuf.data(), stateBuf.size())) {
            return false;
        }
        stateBuf.resize(stateLen);

        std::string err;
        return GraphSerializer::deserialize(stateBuf, outGraph, outMeta, outMacros, err);
    }

    bool jumpToStep(size_t targetIndex, Graph& outGraph, PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
        if (!rustCore_) return false;
        size_t total = n8_rust_graph_undo_manager_get_history_size(rustCore_);
        size_t cur = n8_rust_graph_undo_manager_get_current_index(rustCore_);
        if (targetIndex >= total || targetIndex == cur) return false;

        size_t stateLen = n8_rust_graph_undo_manager_get_state_len(rustCore_, targetIndex);
        std::string stateBuf;
        stateBuf.resize(stateLen + 1);

        if (!n8_rust_graph_undo_manager_jump_to_step(rustCore_, targetIndex, stateBuf.data(), stateBuf.size())) {
            return false;
        }
        stateBuf.resize(stateLen);

        std::string err;
        return GraphSerializer::deserialize(stateBuf, outGraph, outMeta, outMacros, err);
    }

    [[nodiscard]] size_t getCurrentIndex() const noexcept {
        return rustCore_ ? n8_rust_graph_undo_manager_get_current_index(rustCore_) : 0;
    }

    [[nodiscard]] size_t getHistorySize() const noexcept {
        return rustCore_ ? n8_rust_graph_undo_manager_get_history_size(rustCore_) : 0;
    }

    [[nodiscard]] const std::vector<UndoActionCheckpoint>& getTimeline() const {
        if (!rustCore_) {
            cachedTimeline_.clear();
            return cachedTimeline_;
        }
        const size_t count = n8_rust_graph_undo_manager_get_history_size(rustCore_);
        cachedTimeline_.resize(count);
        char descBuf[256];
        for (size_t i = 0; i < count; ++i) {
            uint8_t cat = 0;
            int64_t ts = 0;
            size_t stateLen = 0;
            n8_rust_graph_undo_manager_get_checkpoint_info(
                rustCore_, i, descBuf, sizeof(descBuf), &cat, &ts, &stateLen
            );
            cachedTimeline_[i].description = descBuf;
            cachedTimeline_[i].category = static_cast<ActionCategory>(cat);
            cachedTimeline_[i].timestampMs = ts;
            cachedTimeline_[i].serializedState.resize(stateLen + 1);
            if (stateLen > 0) {
                n8_rust_graph_undo_manager_get_checkpoint_state(
                    rustCore_, i, cachedTimeline_[i].serializedState.data(), stateLen + 1
                );
            }
            cachedTimeline_[i].serializedState.resize(stateLen);
        }
        return cachedTimeline_;
    }

    [[nodiscard]] std::string getNextUndoDescription() const {
        if (!canUndo()) return "";
        char buf[256] = { 0 };
        n8_rust_graph_undo_manager_get_next_undo_description(rustCore_, buf, sizeof(buf));
        return std::string(buf);
    }

    [[nodiscard]] std::string getNextRedoDescription() const {
        if (!canRedo()) return "";
        char buf[256] = { 0 };
        n8_rust_graph_undo_manager_get_next_redo_description(rustCore_, buf, sizeof(buf));
        return std::string(buf);
    }

    void clear() noexcept {
        if (rustCore_) {
            n8_rust_graph_undo_manager_clear(rustCore_);
        }
        cachedTimeline_.clear();
    }

    [[nodiscard]] RustGraphUndoManager* getRustCore() noexcept { return rustCore_; }
    [[nodiscard]] const RustGraphUndoManager* getRustCore() const noexcept { return rustCore_; }

private:
    size_t maxSteps_{ 32 };
    RustGraphUndoManager* rustCore_{ nullptr };
    mutable std::vector<UndoActionCheckpoint> cachedTimeline_;
};

} // namespace audio_graph
