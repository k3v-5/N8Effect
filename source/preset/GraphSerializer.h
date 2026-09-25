#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <charconv>
#include <algorithm>
#include <memory>
#include "../core/Types.h"
#include "../graph/Graph.h"
#include "../graph/NodeFactory.h"

namespace audio_graph {

/**
 * @brief Metadatos descriptivos de un Preset (Regla 21).
 */
struct PresetMetadata {
    uint32_t schemaVersion{ 1 };
    std::string name{ "Default Preset" };
    std::string author{ "N8Audio" };
    std::string category{ "General" };
    std::string description{ "" };
    float dryLevel{ 1.0f };
    float wetLevel{ 1.0f };
};

/**
 * @brief Serializador y Deserializador JSON independiente de la GUI con control de versión (Reglas 21 y 22).
 */
class GraphSerializer {
public:
    static constexpr uint32_t CurrentSchemaVersion = 1;

    /**
     * @brief Serializa el grafo, parámetros y metadatos a formato JSON estructurado.
     */
    static std::string serialize(const Graph& graph,
                                const PresetMetadata& meta = PresetMetadata{},
                                const std::array<float, 8>& macros = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f }) {
        std::ostringstream ss;
        ss << "{\n";
        ss << "  \"schemaVersion\": " << CurrentSchemaVersion << ",\n";
        ss << "  \"name\": \"" << escapeJson(meta.name) << "\",\n";
        ss << "  \"author\": \"" << escapeJson(meta.author) << "\",\n";
        ss << "  \"category\": \"" << escapeJson(meta.category) << "\",\n";
        ss << "  \"description\": \"" << escapeJson(meta.description) << "\",\n";
        ss << "  \"dryLevel\": " << meta.dryLevel << ",\n";
        ss << "  \"wetLevel\": " << meta.wetLevel << ",\n";

        // Macros
        ss << "  \"macros\": [";
        for (size_t i = 0; i < macros.size(); ++i) {
            ss << macros[i] << (i + 1 < macros.size() ? ", " : "");
        }
        ss << "],\n";

        // Nodos
        ss << "  \"nodes\": [\n";
        const auto& nodes = graph.getNodes();
        size_t nIdx = 0;
        for (const auto& [nodeId, inst] : nodes) {
            if (!inst || !inst->processor) continue;
            ss << "    {\n";
            ss << "      \"id\": " << inst->id << ",\n";
            ss << "      \"name\": \"" << escapeJson(inst->name) << "\",\n";
            ss << "      \"type\": " << static_cast<uint32_t>(inst->type) << ",\n";
            ss << "      \"x\": " << inst->posX << ",\n";
            ss << "      \"y\": " << inst->posY << ",\n";
            ss << "      \"params\": {\n";

            const auto params = inst->processor->getParameters();
            for (size_t pIdx = 0; pIdx < params.size(); ++pIdx) {
                const auto& p = params[pIdx];
                const float val = inst->processor->getParameter(p.id);
                ss << "        \"" << p.id << "\": " << val;
                if (pIdx + 1 < params.size()) ss << ",";
                ss << "\n";
            }
            ss << "      }\n";
            ss << "    }" << (++nIdx < nodes.size() ? "," : "") << "\n";
        }
        ss << "  ],\n";

        // Conexiones
        ss << "  \"connections\": [\n";
        const auto& connections = graph.getConnections();
        for (size_t cIdx = 0; cIdx < connections.size(); ++cIdx) {
            const auto& c = connections[cIdx];
            ss << "    { \"id\": " << c.id
               << ", \"srcNode\": " << c.sourceNodeId
               << ", \"srcPin\": " << c.sourcePinId
               << ", \"destNode\": " << c.destNodeId
               << ", \"destPin\": " << c.destPinId << " }"
               << (cIdx + 1 < connections.size() ? "," : "") << "\n";
        }
        ss << "  ]\n";
        ss << "}\n";

        return ss.str();
    }

    /**
     * @brief Deserializa una cadena JSON restaurando la topología y parámetros en el Grafo.
     * Incluye rutinas de migración automática si la versión es anterior (Regla 21).
     */
    static bool deserialize(std::string_view json,
                           Graph& outGraph,
                           PresetMetadata& outMeta,
                           std::array<float, 8>& outMacros,
                           std::string& outError) {
        outError.clear();
        outGraph.clear();

        // 1. Extraer metadatos básicos
        outMeta.schemaVersion = extractUInt(json, "schemaVersion", 1);
        outMeta.name = extractString(json, "name", "Loaded Preset");
        outMeta.author = extractString(json, "author", "Unknown");
        outMeta.category = extractString(json, "category", "General");
        outMeta.description = extractString(json, "description", "");
        outMeta.dryLevel = extractFloat(json, "dryLevel", 1.0f);
        outMeta.wetLevel = extractFloat(json, "wetLevel", 1.0f);

        // Rutina de migración si schemaVersion < CurrentSchemaVersion (Regla 21)
        if (outMeta.schemaVersion < CurrentSchemaVersion) {
            migrateSchema(outMeta.schemaVersion, CurrentSchemaVersion);
        }

        // 2. Extraer Macros
        const auto macrosSection = extractArraySection(json, "macros");
        if (!macrosSection.empty()) {
            std::vector<float> vals = parseNumberList(macrosSection);
            for (size_t i = 0; i < std::min(vals.size(), outMacros.size()); ++i) {
                outMacros[i] = vals[i];
            }
        }

        // 3. Extraer Nodos
        std::unordered_map<NodeId, NodeId> idMap; // Mapeo de IDs viejos a nuevos
        const auto nodesSection = extractArraySection(json, "nodes");
        if (!nodesSection.empty()) {
            const auto nodeObjects = splitObjects(nodesSection);
            for (const auto& nodeJson : nodeObjects) {
                const NodeId origId = extractUInt(nodeJson, "id", 0);
                const std::string name = extractString(nodeJson, "name", "Node");
                const uint32_t typeInt = extractUInt(nodeJson, "type", 0);
                const float x = extractFloat(nodeJson, "x", 50.0f);
                const float y = extractFloat(nodeJson, "y", 50.0f);

                NodeType type = static_cast<NodeType>(typeInt);
                const auto paramsSec = extractObjectSection(nodeJson, "params");

                // Migración inteligente y resolución de tipos legados (Regla 21)
                if (typeInt == 4) {
                    if (name.find("EQ") != std::string::npos || name.find("Parametric") != std::string::npos ||
                        paramsSec.find("\"4\"") != std::string_view::npos || paramsSec.find("\"7\"") != std::string_view::npos) {
                        type = NodeType::ParametricEQ;
                    }
                } else if (typeInt == 5) {
                    if (paramsSec.find("\"4\"") != std::string_view::npos || paramsSec.find("\"6\"") != std::string_view::npos ||
                        name.find("Advanced") != std::string::npos || name.find("Ping-Pong") != std::string::npos ||
                        name.find("Space") != std::string::npos) {
                        type = NodeType::AdvancedDelay;
                    }
                } else if (typeInt == 100 || (typeInt == 12 && name.find("Processor") != std::string::npos)) {
                    type = NodeType::SpectralProcessor;
                }

                auto proc = NodeFactory::getInstance().create(type);
                if (!proc) {
                    proc = NodeFactory::getInstance().create(static_cast<NodeType>(typeInt));
                }
                if (!proc) {
                    continue; // Ignorar nodo no registrado de forma segura
                }

                // Restaurar parámetros del nodo
                if (!paramsSec.empty()) {
                    const auto paramPairs = parseKeyValueFloats(paramsSec);
                    for (const auto& [paramId, val] : paramPairs) {
                        proc->setParameter(paramId, val);
                    }
                }

                NodeId newId = outGraph.addNode(std::move(proc), name, x, y);
                idMap[origId] = newId;
            }
        }

        // 4. Extraer Conexiones
        const auto connSection = extractArraySection(json, "connections");
        if (!connSection.empty()) {
            const auto connObjects = splitObjects(connSection);
            for (const auto& connJson : connObjects) {
                const NodeId origSrc = extractUInt(connJson, "srcNode", 0);
                const PinId srcPin = extractUInt(connJson, "srcPin", 0);
                const NodeId origDest = extractUInt(connJson, "destNode", 0);
                const PinId destPin = extractUInt(connJson, "destPin", 0);

                if (idMap.contains(origSrc) && idMap.contains(origDest)) {
                    outGraph.connect(idMap[origSrc], srcPin, idMap[origDest], destPin);
                }
            }
        }

        // Validar el grafo restaurado
        std::vector<NodeId> sorted;
        if (!outGraph.validateAndTopologicalSort(sorted, outError)) {
            return false;
        }

        return true;
    }

private:
    static void migrateSchema(uint32_t fromVersion, uint32_t toVersion) noexcept {
        // Regla 21: Rutinas de migración entre versiones de esquema
        (void)fromVersion;
        (void)toVersion;
    }

    static std::string escapeJson(std::string_view str) {
        std::string res;
        res.reserve(str.size());
        for (char c : str) {
            if (c == '"') res += "\\\"";
            else if (c == '\\') res += "\\\\";
            else if (c == '\n') res += "\\n";
            else res += c;
        }
        return res;
    }

    static std::string_view extractKey(std::string_view json, std::string_view key) {
        const std::string search = "\"" + std::string(key) + "\"";
        auto pos = json.find(search);
        if (pos == std::string_view::npos) return {};

        pos = json.find(':', pos + search.size());
        if (pos == std::string_view::npos) return {};

        size_t start = pos + 1;
        while (start < json.size() && (json[start] == ' ' || json[start] == '\t' || json[start] == '\r' || json[start] == '\n')) {
            start++;
        }
        return json.substr(start);
    }

    static std::string extractString(std::string_view json, std::string_view key, const std::string& defaultVal) {
        auto val = extractKey(json, key);
        if (val.empty() || val[0] != '"') return defaultVal;

        auto end = val.find('"', 1);
        if (end == std::string_view::npos) return defaultVal;
        return std::string(val.substr(1, end - 1));
    }

    static uint32_t extractUInt(std::string_view json, std::string_view key, uint32_t defaultVal) {
        auto val = extractKey(json, key);
        if (val.empty()) return defaultVal;

        uint32_t res = defaultVal;
        std::from_chars(val.data(), val.data() + val.size(), res);
        return res;
    }

    static float extractFloat(std::string_view json, std::string_view key, float defaultVal) {
        auto val = extractKey(json, key);
        if (val.empty()) return defaultVal;

        try {
            return std::stof(std::string(val));
        } catch (...) {
            return defaultVal;
        }
    }

    static std::string_view extractArraySection(std::string_view json, std::string_view key) {
        auto val = extractKey(json, key);
        if (val.empty() || val[0] != '[') return {};

        int depth = 0;
        for (size_t i = 0; i < val.size(); ++i) {
            if (val[i] == '[') depth++;
            else if (val[i] == ']') {
                depth--;
                if (depth == 0) {
                    return val.substr(1, i - 1);
                }
            }
        }
        return {};
    }

    static std::string_view extractObjectSection(std::string_view json, std::string_view key) {
        auto val = extractKey(json, key);
        if (val.empty() || val[0] != '{') return {};

        int depth = 0;
        for (size_t i = 0; i < val.size(); ++i) {
            if (val[i] == '{') depth++;
            else if (val[i] == '}') {
                depth--;
                if (depth == 0) {
                    return val.substr(1, i - 1);
                }
            }
        }
        return {};
    }

    static std::vector<std::string_view> splitObjects(std::string_view arrayContent) {
        std::vector<std::string_view> objs;
        int depth = 0;
        size_t start = 0;

        for (size_t i = 0; i < arrayContent.size(); ++i) {
            if (arrayContent[i] == '{') {
                if (depth == 0) start = i;
                depth++;
            } else if (arrayContent[i] == '}') {
                depth--;
                if (depth == 0) {
                    objs.push_back(arrayContent.substr(start, i - start + 1));
                }
            }
        }
        return objs;
    }

    static std::vector<float> parseNumberList(std::string_view text) {
        std::vector<float> nums;
        std::string current;
        for (char c : text) {
            if ((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+') {
                current += c;
            } else if (c == ',' || c == ' ' || c == '\n') {
                if (!current.empty()) {
                    try { nums.push_back(std::stof(current)); } catch (...) {}
                    current.clear();
                }
            }
        }
        if (!current.empty()) {
            try { nums.push_back(std::stof(current)); } catch (...) {}
        }
        return nums;
    }

    static std::vector<std::pair<ParameterId, float>> parseKeyValueFloats(std::string_view text) {
        std::vector<std::pair<ParameterId, float>> pairs;
        size_t pos = 0;

        while (pos < text.size()) {
            auto quote1 = text.find('"', pos);
            if (quote1 == std::string_view::npos) break;
            auto quote2 = text.find('"', quote1 + 1);
            if (quote2 == std::string_view::npos) break;

            std::string keyStr(text.substr(quote1 + 1, quote2 - quote1 - 1));
            ParameterId pid = 0;
            try { pid = static_cast<ParameterId>(std::stoul(keyStr)); } catch (...) {}

            auto colon = text.find(':', quote2 + 1);
            if (colon == std::string_view::npos) break;

            auto comma = text.find_first_of(",}\n", colon + 1);
            size_t valEnd = (comma != std::string_view::npos) ? comma : text.size();

            std::string valStr(text.substr(colon + 1, valEnd - colon - 1));
            float val = 0.0f;
            try { val = std::stof(valStr); } catch (...) {}

            pairs.push_back({ pid, val });
            pos = valEnd + 1;
        }

        return pairs;
    }
};

} // namespace audio_graph
