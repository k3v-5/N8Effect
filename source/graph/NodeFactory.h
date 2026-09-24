#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <functional>
#include "AudioProcessorNode.h"

namespace audio_graph {

/**
 * @brief Registro y Fábrica Extensible de Procesadores (Reglas 19 y 20).
 * Permite registrar nuevos procesadores en tiempo de compilación o inicio
 * sin modificar el núcleo del Graph Engine ni usar sentencias if/switch gigantescas.
 */
class NodeFactory {
public:
    using CreatorFunc = std::function<std::unique_ptr<AudioProcessorNode>()>;

    struct RegistryEntry {
        NodeType type;
        std::string name;
        std::string category;
        CreatorFunc creator;
    };

    static NodeFactory& getInstance() {
        static NodeFactory instance;
        return instance;
    }

    // Registro de un nuevo procesador (Regla 19)
    void registerNodeType(NodeType type, std::string_view name, std::string_view category, CreatorFunc creator) {
        entriesByType_[type] = RegistryEntry{
            .type = type,
            .name = std::string(name),
            .category = std::string(category),
            .creator = std::move(creator)
        };
        typeByName_[std::string(name)] = type;
    }

    // Creación por NodeType
    std::unique_ptr<AudioProcessorNode> create(NodeType type) const {
        auto it = entriesByType_.find(type);
        if (it != entriesByType_.end() && it->second.creator) {
            return it->second.creator();
        }
        return nullptr;
    }

    // Creación por Nombre textual (Regla 20: createProcessor("delay"))
    std::unique_ptr<AudioProcessorNode> create(std::string_view name) const {
        auto it = typeByName_.find(std::string(name));
        if (it != typeByName_.end()) {
            return create(it->second);
        }
        return nullptr;
    }

    // Obtener catálogo para UI / Preset Browser
    const std::unordered_map<NodeType, RegistryEntry>& getRegistry() const noexcept {
        return entriesByType_;
    }

private:
    NodeFactory() = default;
    std::unordered_map<NodeType, RegistryEntry> entriesByType_;
    std::unordered_map<std::string, NodeType> typeByName_;
};

/**
 * @brief Macro auxiliar para auto-registro de procesadores en tiempo estático
 */
template <typename TNode>
struct AutoRegisterNode {
    AutoRegisterNode(NodeType type, std::string_view name, std::string_view category) {
        NodeFactory::getInstance().registerNodeType(type, name, category, []() -> std::unique_ptr<AudioProcessorNode> {
            return std::make_unique<TNode>();
        });
    }
};

} // namespace audio_graph
