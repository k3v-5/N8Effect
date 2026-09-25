#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <sstream>
#include <iomanip>
#include "../core/Types.h"
#include "../graph/Graph.h"

namespace audio_graph {

/**
 * @brief Curvas de respuesta para el mapeo MIDI CC
 */
enum class MidiCurve : uint8_t {
    Linear = 0,
    Exponential,  // x^2: mayor resolución en valores bajos
    Logarithmic,  // sqrt(x): mayor resolución en valores altos
    SShaped       // Smoothstep (3x^2 - 2x^3): zona muerta suave en extremos
};

/**
 * @brief Mapeo individual entre un mensaje MIDI CC y un parámetro objetivo
 */
struct MidiMapping {
    uint8_t ccNumber{ 0 };                 // 0 a 127
    uint8_t channel{ 0 };                  // 0 = Omni (cualquier canal), 1..16 = canal específico
    ParameterId paramId{ 0 };              // ID numérico del parámetro
    NodeId targetNodeId{ InvalidNodeId };  // InvalidNodeId para macros o parámetros globales
    float minVal{ 0.0f };                  // Valor mínimo asignado
    float maxVal{ 1.0f };                  // Valor máximo asignado
    bool inverted{ false };                // Invierte el sentido del recorrido
    MidiCurve curve{ MidiCurve::Linear };  // Curva de interpolación
    std::string customLabel{};             // Etiqueta legible para la GUI
};

/**
 * @brief Administrador universal de MIDI Learn y Perfiles de Controladores (.n8midi)
 * Permite el control en tiempo real con latencia cero y sin alocaciones en el hilo de audio (Reglas 8, 9, 21, 46).
 */
class MidiMappingManager {
public:
    static constexpr size_t MaxMappings = 128;

    struct LearnTarget {
        ParameterId paramId{ 0 };
        NodeId targetNodeId{ InvalidNodeId };
        float minVal{ 0.0f };
        float maxVal{ 1.0f };
        MidiCurve curve{ MidiCurve::Linear };
        bool inverted{ false };
    };

    MidiMappingManager() {
        mappings_.reserve(MaxMappings);
    }

    void reset() noexcept {
        cancelLearning();
        mappings_.clear();
    }

    // Modo MIDI Learn
    void startLearning(ParameterId paramId,
                       NodeId targetNodeId = InvalidNodeId,
                       float minVal = 0.0f,
                       float maxVal = 1.0f,
                       MidiCurve curve = MidiCurve::Linear,
                       bool inverted = false) noexcept
    {
        pendingTarget_ = LearnTarget{
            .paramId = paramId,
            .targetNodeId = targetNodeId,
            .minVal = minVal,
            .maxVal = maxVal,
            .curve = curve,
            .inverted = inverted
        };
        isLearning_.store(true, std::memory_order_release);
    }

    void cancelLearning() noexcept {
        isLearning_.store(false, std::memory_order_release);
    }

    bool isLearning() const noexcept {
        return isLearning_.load(std::memory_order_acquire);
    }

    const LearnTarget& getLearnTarget() const noexcept {
        return pendingTarget_;
    }

    // Vinculación manual de mapeo
    bool addMapping(const MidiMapping& mapping) {
        // Eliminar mapeo existente para el mismo parámetro si ya existe
        removeMapping(mapping.paramId, mapping.targetNodeId);

        if (mappings_.size() < MaxMappings) {
            mappings_.push_back(mapping);
            return true;
        }
        return false;
    }

    bool removeMapping(ParameterId paramId, NodeId targetNodeId = InvalidNodeId) {
        auto it = std::remove_if(mappings_.begin(), mappings_.end(), [=](const MidiMapping& m) {
            return m.paramId == paramId && m.targetNodeId == targetNodeId;
        });
        if (it != mappings_.end()) {
            mappings_.erase(it, mappings_.end());
            return true;
        }
        return false;
    }

    void removeMappingByCC(uint8_t ccNumber, uint8_t channel = 0) {
        auto it = std::remove_if(mappings_.begin(), mappings_.end(), [=](const MidiMapping& m) {
            return m.ccNumber == ccNumber && (channel == 0 || m.channel == 0 || m.channel == channel);
        });
        if (it != mappings_.end()) {
            mappings_.erase(it, mappings_.end());
        }
    }

    const std::vector<MidiMapping>& getMappings() const noexcept {
        return mappings_;
    }

    /**
     * @brief Procesa un mensaje CC entrante en tiempo real (Audio Thread Safe)
     */
    void processControlChange(uint8_t channel, uint8_t ccNumber, uint8_t ccValue, Graph* graph = nullptr) noexcept {
        // 1. Si está en modo MIDI Learn, capturar el primer CC que llegue
        if (isLearning_.load(std::memory_order_acquire)) {
            MidiMapping newMap;
            newMap.ccNumber = ccNumber;
            newMap.channel = 0; // Omni por defecto
            newMap.paramId = pendingTarget_.paramId;
            newMap.targetNodeId = pendingTarget_.targetNodeId;
            newMap.minVal = pendingTarget_.minVal;
            newMap.maxVal = pendingTarget_.maxVal;
            newMap.curve = pendingTarget_.curve;
            newMap.inverted = pendingTarget_.inverted;

            // Almacenar en la lista acotada
            if (mappings_.size() < MaxMappings) {
                // Eliminar previo
                for (size_t i = 0; i < mappings_.size(); ++i) {
                    if (mappings_[i].paramId == newMap.paramId && mappings_[i].targetNodeId == newMap.targetNodeId) {
                        mappings_[i] = newMap;
                        isLearning_.store(false, std::memory_order_release);
                        return;
                    }
                }
                mappings_.push_back(newMap);
            }
            isLearning_.store(false, std::memory_order_release);
            return;
        }

        // 2. Si no está en learn, despachar a los parámetros vinculados
        for (const auto& map : mappings_) {
            if (map.ccNumber == ccNumber) {
                if (map.channel == 0 || map.channel == channel) {
                    const float finalValue = calculateMappedValue(map, ccValue);

                    // Despachar hacia el nodo destino
                    if (graph != nullptr && map.targetNodeId != InvalidNodeId) {
                        auto* proc = graph->getNodeProcessor(map.targetNodeId);
                        if (proc != nullptr) {
                            proc->setParameter(map.paramId, finalValue);
                        }
                    }
                }
            }
        }
    }

    /**
     * @brief Transforma un valor CC (0..127) según la curva, inversión y rango configurado
     */
    static float calculateMappedValue(const MidiMapping& map, uint8_t ccValue) noexcept {
        float norm = std::clamp(static_cast<float>(ccValue) / 127.0f, 0.0f, 1.0f);
        if (map.inverted) {
            norm = 1.0f - norm;
        }

        switch (map.curve) {
            case MidiCurve::Linear:
                break;
            case MidiCurve::Exponential:
                norm = norm * norm;
                break;
            case MidiCurve::Logarithmic:
                norm = std::sqrt(norm);
                break;
            case MidiCurve::SShaped:
                norm = norm * norm * (3.0f - 2.0f * norm); // Smoothstep
                break;
        }

        return map.minVal + norm * (map.maxVal - map.minVal);
    }

    // =========================================================================
    // Plantillas de Controladores Hardware Preconfiguradas
    // =========================================================================
    void loadFactoryProfile(std::string_view profileName) {
        mappings_.clear();

        if (profileName == "Akai MPK Mini MK3") {
            // Perillas K1..K8 en CC 70..77 mapeadas a Macros 1..8
            for (uint8_t i = 0; i < 8; ++i) {
                addMapping(MidiMapping{
                    .ccNumber = static_cast<uint8_t>(70 + i),
                    .channel = 0,
                    .paramId = static_cast<ParameterId>(i + 1),
                    .targetNodeId = InvalidNodeId,
                    .minVal = 0.0f,
                    .maxVal = 1.0f,
                    .inverted = false,
                    .curve = MidiCurve::Linear,
                    .customLabel = "Macro " + std::to_string(i + 1)
                });
            }
        } else if (profileName == "Arturia KeyLab Essential") {
            // Faders 1..8 en CC 73..80 mapeados a Macros 1..8
            for (uint8_t i = 0; i < 8; ++i) {
                addMapping(MidiMapping{
                    .ccNumber = static_cast<uint8_t>(73 + i),
                    .channel = 0,
                    .paramId = static_cast<ParameterId>(i + 1),
                    .targetNodeId = InvalidNodeId,
                    .minVal = 0.0f,
                    .maxVal = 1.0f,
                    .inverted = false,
                    .curve = MidiCurve::Linear,
                    .customLabel = "Macro " + std::to_string(i + 1)
                });
            }
        } else if (profileName == "Novation Launchkey") {
            // Faders 1..8 en CC 21..28 mapeados a Macros 1..8
            for (uint8_t i = 0; i < 8; ++i) {
                addMapping(MidiMapping{
                    .ccNumber = static_cast<uint8_t>(21 + i),
                    .channel = 0,
                    .paramId = static_cast<ParameterId>(i + 1),
                    .targetNodeId = InvalidNodeId,
                    .minVal = 0.0f,
                    .maxVal = 1.0f,
                    .inverted = false,
                    .curve = MidiCurve::Linear,
                    .customLabel = "Macro " + std::to_string(i + 1)
                });
            }
        } else {
            // Plantilla Universal de 8 Faders / Perillas (CC 14..21)
            for (uint8_t i = 0; i < 8; ++i) {
                addMapping(MidiMapping{
                    .ccNumber = static_cast<uint8_t>(14 + i),
                    .channel = 0,
                    .paramId = static_cast<ParameterId>(i + 1),
                    .targetNodeId = InvalidNodeId,
                    .minVal = 0.0f,
                    .maxVal = 1.0f,
                    .inverted = false,
                    .curve = MidiCurve::Linear,
                    .customLabel = "Macro " + std::to_string(i + 1)
                });
            }
        }
    }

    // =========================================================================
    // Serialización / Deserialización JSON de Perfiles (.n8midi)
    // =========================================================================
    std::string exportToJson(std::string_view profileName = "Custom Hardware Profile") const {
        std::ostringstream ss;
        ss << "{\n";
        ss << "  \"schemaVersion\": 1,\n";
        ss << "  \"profileName\": \"" << profileName << "\",\n";
        ss << "  \"mappings\": [\n";

        for (size_t i = 0; i < mappings_.size(); ++i) {
            const auto& m = mappings_[i];
            ss << "    {\n";
            ss << "      \"cc\": " << static_cast<int>(m.ccNumber) << ",\n";
            ss << "      \"channel\": " << static_cast<int>(m.channel) << ",\n";
            ss << "      \"paramId\": " << m.paramId << ",\n";
            ss << "      \"targetNodeId\": " << m.targetNodeId << ",\n";
            ss << "      \"minVal\": " << m.minVal << ",\n";
            ss << "      \"maxVal\": " << m.maxVal << ",\n";
            ss << "      \"inverted\": " << (m.inverted ? "true" : "false") << ",\n";
            ss << "      \"curve\": " << static_cast<int>(m.curve) << ",\n";
            ss << "      \"label\": \"" << m.customLabel << "\"\n";
            ss << "    }" << (i + 1 < mappings_.size() ? ",\n" : "\n");
        }

        ss << "  ]\n";
        ss << "}\n";
        return ss.str();
    }

    bool importFromJson(std::string_view jsonStr) {
        if (jsonStr.empty()) return false;

        // Parser JSON ligero y determinista
        std::vector<MidiMapping> loaded;
        size_t pos = 0;

        auto findNext = [&](std::string_view key, size_t from) -> size_t {
            return jsonStr.find(key, from);
        };

        auto extractInt = [&](std::string_view key, size_t from, int defVal) -> int {
            size_t idx = findNext(key, from);
            if (idx == std::string_view::npos) return defVal;
            idx = jsonStr.find(':', idx);
            if (idx == std::string_view::npos) return defVal;
            ++idx;
            while (idx < jsonStr.size() && (jsonStr[idx] == ' ' || jsonStr[idx] == '\t')) ++idx;
            try {
                return std::stoi(std::string(jsonStr.substr(idx, 16)));
            } catch (...) {
                return defVal;
            }
        };

        auto extractFloat = [&](std::string_view key, size_t from, float defVal) -> float {
            size_t idx = findNext(key, from);
            if (idx == std::string_view::npos) return defVal;
            idx = jsonStr.find(':', idx);
            if (idx == std::string_view::npos) return defVal;
            ++idx;
            while (idx < jsonStr.size() && (jsonStr[idx] == ' ' || jsonStr[idx] == '\t')) ++idx;
            try {
                return std::stof(std::string(jsonStr.substr(idx, 16)));
            } catch (...) {
                return defVal;
            }
        };

        auto extractBool = [&](std::string_view key, size_t from, bool defVal) -> bool {
            size_t idx = findNext(key, from);
            if (idx == std::string_view::npos) return defVal;
            idx = jsonStr.find(':', idx);
            if (idx == std::string_view::npos) return defVal;
            ++idx;
            while (idx < jsonStr.size() && (jsonStr[idx] == ' ' || jsonStr[idx] == '\t')) ++idx;
            return jsonStr.substr(idx, 4) == "true";
        };

        size_t mapIdx = findNext("\"mappings\"", 0);
        if (mapIdx == std::string_view::npos) return false;

        while ((pos = jsonStr.find('{', pos + 1)) != std::string_view::npos) {
            // Comprobar si estamos dentro de un objeto de mapeo
            if (pos <= mapIdx) continue;

            size_t endObj = jsonStr.find('}', pos);
            if (endObj == std::string_view::npos) break;

            MidiMapping m;
            m.ccNumber = static_cast<uint8_t>(extractInt("\"cc\"", pos, 0));
            m.channel = static_cast<uint8_t>(extractInt("\"channel\"", pos, 0));
            m.paramId = static_cast<ParameterId>(extractInt("\"paramId\"", pos, 0));
            m.targetNodeId = static_cast<NodeId>(extractInt("\"targetNodeId\"", pos, 0));
            m.minVal = extractFloat("\"minVal\"", pos, 0.0f);
            m.maxVal = extractFloat("\"maxVal\"", pos, 1.0f);
            m.inverted = extractBool("\"inverted\"", pos, false);
            m.curve = static_cast<MidiCurve>(extractInt("\"curve\"", pos, 0));

            loaded.push_back(m);
            pos = endObj;
        }

        if (!loaded.empty()) {
            mappings_ = std::move(loaded);
            return true;
        }

        return false;
    }

private:
    std::vector<MidiMapping> mappings_;
    std::atomic<bool> isLearning_{ false };
    LearnTarget pendingTarget_{};
};

} // namespace audio_graph
