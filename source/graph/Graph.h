#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <queue>
#include "AudioProcessorNode.h"

namespace audio_graph {

/**
 * @brief Conexión dirigida entre un pin de salida y un pin de entrada (Regla 4)
 */
struct Connection {
    ConnectionId id{ 0 };
    NodeId sourceNodeId{ InvalidNodeId };
    PinId sourcePinId{ InvalidPinId };
    NodeId destNodeId{ InvalidNodeId };
    PinId destPinId{ InvalidPinId };
};

/**
 * @brief Nodo instanciado dentro del Grafo de Usuario
 */
struct NodeInstance {
    NodeId id{ InvalidNodeId };
    std::string name;
    NodeType type{ NodeType::Unknown };
    std::unique_ptr<AudioProcessorNode> processor;
    float posX{ 0.0f };
    float posY{ 0.0f };
};

/**
 * @brief Grafo editable por el usuario (Reglas 4, 22, 28, 29).
 * Contiene la descripción de alto nivel de nodos y conexiones.
 */
class Graph {
public:
    Graph() = default;

    NodeId addNode(std::unique_ptr<AudioProcessorNode> processor, std::string_view name = "", float x = 0.0f, float y = 0.0f) {
        if (!processor) return InvalidNodeId;
        
        NodeId id = ++nextNodeId_;
        auto instance = std::make_unique<NodeInstance>();
        instance->id = id;
        instance->name = name.empty() ? processor->getName() : std::string(name);
        instance->type = processor->getType();
        instance->processor = std::move(processor);
        instance->posX = x;
        instance->posY = y;

        nodes_[id] = std::move(instance);
        return id;
    }

    bool removeNode(NodeId id) {
        auto it = nodes_.find(id);
        if (it == nodes_.end()) return false;

        // Remover todas las conexiones asociadas
        std::erase_if(connections_, [id](const Connection& c) {
            return c.sourceNodeId == id || c.destNodeId == id;
        });

        nodes_.erase(it);
        return true;
    }

    ConnectionId connect(NodeId srcNode, PinId srcPin, NodeId destNode, PinId destPin) {
        if (srcNode == destNode) return 0; // Evitar lazo directo trivial
        if (!hasNode(srcNode) || !hasNode(destNode)) return 0;

        // Verificar si la conexión ya existe
        for (const auto& c : connections_) {
            if (c.sourceNodeId == srcNode && c.sourcePinId == srcPin &&
                c.destNodeId == destNode && c.destPinId == destPin) {
                return c.id;
            }
        }

        ConnectionId cid = ++nextConnectionId_;
        connections_.push_back(Connection{
            .id = cid,
            .sourceNodeId = srcNode,
            .sourcePinId = srcPin,
            .destNodeId = destNode,
            .destPinId = destPin
        });

        return cid;
    }

    bool disconnect(ConnectionId cid) {
        auto it = std::remove_if(connections_.begin(), connections_.end(), [cid](const Connection& c) {
            return c.id == cid;
        });
        if (it != connections_.end()) {
            connections_.erase(it, connections_.end());
            return true;
        }
        return false;
    }

    void clear() noexcept {
        connections_.clear();
        nodes_.clear();
        nextNodeId_ = 0;
        nextConnectionId_ = 0;
    }

    NodeInstance* getNode(NodeId id) noexcept {
        auto it = nodes_.find(id);
        return (it != nodes_.end()) ? it->second.get() : nullptr;
    }

    const NodeInstance* getNode(NodeId id) const noexcept {
        auto it = nodes_.find(id);
        return (it != nodes_.end()) ? it->second.get() : nullptr;
    }

    bool hasNode(NodeId id) const noexcept {
        return nodes_.contains(id);
    }

    AudioProcessorNode* getNodeProcessor(NodeId id) const noexcept {
        auto it = nodes_.find(id);
        if (it != nodes_.end()) {
            return it->second->processor.get();
        }
        return nullptr;
    }

    const std::unordered_map<NodeId, std::unique_ptr<NodeInstance>>& getNodes() const noexcept {
        return nodes_;
    }

    const std::vector<Connection>& getConnections() const noexcept {
        return connections_;
    }

    // Validación y ordenamiento topológico acíclico (Regla 29)
    // Retorna true si es un DAG válido y escribe el orden en sortedNodeIds
    bool validateAndTopologicalSort(std::vector<NodeId>& sortedNodeIds, std::string& outErrorMessage) const {
        sortedNodeIds.clear();
        outErrorMessage.clear();

        // Calcular grado de entrada (in-degree)
        std::unordered_map<NodeId, int> inDegree;
        std::unordered_map<NodeId, std::vector<NodeId>> adjList;

        for (const auto& [id, _] : nodes_) {
            inDegree[id] = 0;
            adjList[id] = {};
        }

        for (const auto& conn : connections_) {
            if (!nodes_.contains(conn.sourceNodeId) || !nodes_.contains(conn.destNodeId)) {
                outErrorMessage = "Conexión a un nodo inexistente detectada.";
                return false;
            }
            adjList[conn.sourceNodeId].push_back(conn.destNodeId);
            inDegree[conn.destNodeId]++;
        }

        // Algoritmo de Kahn
        std::queue<NodeId> zeroInDegreeQueue;
        for (const auto& [id, degree] : inDegree) {
            if (degree == 0) {
                zeroInDegreeQueue.push(id);
            }
        }

        while (!zeroInDegreeQueue.empty()) {
            NodeId u = zeroInDegreeQueue.front();
            zeroInDegreeQueue.pop();
            sortedNodeIds.push_back(u);

            for (NodeId v : adjList[u]) {
                inDegree[v]--;
                if (inDegree[v] == 0) {
                    zeroInDegreeQueue.push(v);
                }
            }
        }

        if (sortedNodeIds.size() != nodes_.size()) {
            outErrorMessage = "Ciclo no controlado detectado en el grafo.";
            return false;
        }

        return true;
    }

private:
    NodeId nextNodeId_{ 0 };
    ConnectionId nextConnectionId_{ 0 };
    std::unordered_map<NodeId, std::unique_ptr<NodeInstance>> nodes_;
    std::vector<Connection> connections_;
};

} // namespace audio_graph
