#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>
#include <queue>
#include <algorithm>
#include <array>
#include <cstdint>
#include "AudioProcessorNode.h"
#include "../modulation/NodeAutomationSequencer.h"

namespace audio_graph {

using GroupId = uint32_t;
constexpr GroupId InvalidGroupId = 0;

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
 * @brief Mapeo de control macro de grupo a un parámetro de nodo interno (Reglas 4, 8, R2).
 */
struct GroupMacroMapping {
    NodeId targetNodeId{ InvalidNodeId };
    ParameterId targetParamId{ InvalidParameterId };
    float depth{ 0.0f };      // Bipolar depth [-1.0f .. +1.0f]
    float baseValue{ 0.0f };  // Base parameter value
};

/**
 * @brief Control macro de grupo con nombre, valor normalizado y mapeos hacia nodos internos.
 */
struct GroupMacro {
    std::string name{ "CTRL" };
    float value{ 0.5f };      // Normalized [0.0f .. 1.0f], default centered
    std::vector<GroupMacroMapping> mappings;
};

/**
 * @brief Caja de Agrupación Visual de Nodos en el Grafo (Reglas 4, 8, 22, R2).
 */
struct NodeGroup {
    GroupId id{ InvalidGroupId };
    std::string name{ "Group" };
    uint32_t colorRgba{ 0x00d4ffff }; // Default neon cyan (RGBA)
    bool isBypassed{ false };
    bool isCollapsed{ false };
    std::vector<NodeId> memberNodeIds;
    std::array<GroupMacro, 3> macros{
        GroupMacro{ "CTRL 1", 0.5f, {} },
        GroupMacro{ "CTRL 2", 0.5f, {} },
        GroupMacro{ "CTRL 3", 0.5f, {} }
    };

    bool containsNode(NodeId nid) const noexcept {
        return std::find(memberNodeIds.begin(), memberNodeIds.end(), nid) != memberNodeIds.end();
    }
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
    bool isBypassed{ false };
    NodeAutomationBank sequencer; // Secuenciador personal de automatización rítmica por efecto
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

        // Limpiar membresía en grupos y mapeos de macro (Regla R2)
        for (auto& [gid, grp] : groups_) {
            std::erase(grp->memberNodeIds, id);
            for (auto& macro : grp->macros) {
                std::erase_if(macro.mappings, [id](const GroupMacroMapping& m) {
                    return m.targetNodeId == id;
                });
            }
        }

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
        groups_.clear();
        nextNodeId_ = 0;
        nextConnectionId_ = 0;
        nextGroupId_ = 0;
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

    bool setNodeBypassed(NodeId id, bool bypassed) noexcept {
        auto it = nodes_.find(id);
        if (it == nodes_.end()) return false;
        it->second->isBypassed = bypassed;
        return true;
    }

    bool isNodeBypassed(NodeId id) const noexcept {
        auto it = nodes_.find(id);
        if (it == nodes_.end()) return false;
        return it->second->isBypassed;
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

    std::vector<NodeId> getNodeIds() const {
        std::vector<NodeId> ids;
        ids.reserve(nodes_.size());
        for (const auto& [id, _] : nodes_) {
            ids.push_back(id);
        }
        return ids;
    }

    const std::vector<Connection>& getConnections() const noexcept {
        return connections_;
    }

    // ==============================================================================
    // Gestión de Grupos de Nodos y Macros de Grupo (Reglas 4, 8, 21, 22, R2)
    // ==============================================================================
    GroupId addGroup(std::string_view name = "Group", uint32_t colorRgba = 0x00d4ffff, const std::vector<NodeId>& memberNodeIds = {}) {
        GroupId gid = ++nextGroupId_;
        auto grp = std::make_unique<NodeGroup>();
        grp->id = gid;
        grp->name = name.empty() ? ("Group " + std::to_string(gid)) : std::string(name);
        grp->colorRgba = colorRgba;
        grp->memberNodeIds = memberNodeIds;
        groups_[gid] = std::move(grp);
        return gid;
    }

    GroupId addGroup(std::unique_ptr<NodeGroup> group) {
        if (!group) return InvalidGroupId;
        if (group->id == InvalidGroupId) {
            group->id = ++nextGroupId_;
        } else {
            nextGroupId_ = std::max(nextGroupId_, group->id);
        }
        GroupId gid = group->id;
        groups_[gid] = std::move(group);
        return gid;
    }

    bool removeGroup(GroupId id) {
        return groups_.erase(id) > 0;
    }

    bool addNodeToGroup(GroupId gid, NodeId nid) {
        auto* grp = getGroup(gid);
        if (!grp || !hasNode(nid)) return false;

        // Garantizar pertenencia a un único grupo
        for (auto& [otherId, otherGrp] : groups_) {
            if (otherId != gid) {
                std::erase(otherGrp->memberNodeIds, nid);
            }
        }

        if (!grp->containsNode(nid)) {
            grp->memberNodeIds.push_back(nid);
        }
        return true;
    }

    bool removeNodeFromGroup(GroupId gid, NodeId nid) {
        auto* grp = getGroup(gid);
        if (!grp) return false;

        auto it = std::find(grp->memberNodeIds.begin(), grp->memberNodeIds.end(), nid);
        if (it != grp->memberNodeIds.end()) {
            grp->memberNodeIds.erase(it);
            for (auto& macro : grp->macros) {
                std::erase_if(macro.mappings, [nid](const GroupMacroMapping& m) {
                    return m.targetNodeId == nid;
                });
            }
            return true;
        }
        return false;
    }

    void setGroupBypassed(GroupId gid, bool bypassed) {
        auto* grp = getGroup(gid);
        if (!grp) return;
        grp->isBypassed = bypassed;
        for (NodeId nid : grp->memberNodeIds) {
            setNodeBypassed(nid, bypassed);
        }
    }

    bool isGroupBypassed(GroupId gid) const noexcept {
        auto it = groups_.find(gid);
        return (it != groups_.end()) ? it->second->isBypassed : false;
    }

    void setGroupCollapsed(GroupId gid, bool collapsed) {
        auto* grp = getGroup(gid);
        if (!grp) return;
        grp->isCollapsed = collapsed;
    }

    bool isGroupCollapsed(GroupId gid) const noexcept {
        auto it = groups_.find(gid);
        return (it != groups_.end()) ? it->second->isCollapsed : false;
    }

    void applyGroupMacroValue(GroupId gid, size_t macroIndex, float macroValue) {
        auto* grp = getGroup(gid);
        if (!grp || macroIndex >= grp->macros.size()) return;

        macroValue = std::clamp(macroValue, 0.0f, 1.0f);
        grp->macros[macroIndex].value = macroValue;

        for (const auto& mapping : grp->macros[macroIndex].mappings) {
            auto* node = getNode(mapping.targetNodeId);
            if (!node || !node->processor) continue;

            float minValue = 0.0f;
            float maxValue = 1.0f;
            bool found = false;
            for (const auto& pInfo : node->processor->getParameters()) {
                if (pInfo.id == mapping.targetParamId) {
                    minValue = pInfo.minValue;
                    maxValue = pInfo.maxValue;
                    found = true;
                    break;
                }
            }
            if (!found) continue;

            float val = std::clamp(mapping.baseValue + (macroValue - 0.5f) * mapping.depth * (maxValue - minValue), minValue, maxValue);
            node->processor->setParameter(mapping.targetParamId, val);
        }
    }

    const std::unordered_map<GroupId, std::unique_ptr<NodeGroup>>& getGroups() const noexcept {
        return groups_;
    }

    NodeGroup* getGroup(GroupId id) noexcept {
        auto it = groups_.find(id);
        return (it != groups_.end()) ? it->second.get() : nullptr;
    }

    const NodeGroup* getGroup(GroupId id) const noexcept {
        auto it = groups_.find(id);
        return (it != groups_.end()) ? it->second.get() : nullptr;
    }

    std::vector<GroupId> getGroupIds() const {
        std::vector<GroupId> ids;
        ids.reserve(groups_.size());
        for (const auto& [id, _] : groups_) {
            ids.push_back(id);
        }
        return ids;
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

    /**
     * @brief Valida predictivamente si agregar una conexion dirigida (srcNode -> destNode)
     * crearia un ciclo en el grafo DAG (Reglas 4, 15, 28, 29).
     * Busqueda en anchura (BFS) rapida desde destNode para comprobar si puede alcanzar srcNode.
     */
    bool wouldCreateCycle(NodeId srcNode, NodeId destNode) const noexcept {
        if (srcNode == destNode || srcNode == InvalidNodeId || destNode == InvalidNodeId) {
            return true;
        }
        if (!hasNode(srcNode) || !hasNode(destNode)) {
            return true;
        }

        std::vector<NodeId> queue;
        queue.reserve(nodes_.size());
        queue.push_back(destNode);

        std::unordered_set<NodeId> visited;
        visited.reserve(nodes_.size());
        visited.insert(destNode);

        size_t head = 0;
        while (head < queue.size()) {
            NodeId current = queue[head++];
            if (current == srcNode) {
                return true;
            }

            for (const auto& conn : connections_) {
                if (conn.sourceNodeId == current) {
                    if (!visited.contains(conn.destNodeId)) {
                        visited.insert(conn.destNodeId);
                        queue.push_back(conn.destNodeId);
                    }
                }
            }
        }

        return false;
    }

private:
    NodeId nextNodeId_{ 0 };
    ConnectionId nextConnectionId_{ 0 };
    GroupId nextGroupId_{ 0 };
    std::unordered_map<NodeId, std::unique_ptr<NodeInstance>> nodes_;
    std::vector<Connection> connections_;
    std::unordered_map<GroupId, std::unique_ptr<NodeGroup>> groups_;
};

} // namespace audio_graph
