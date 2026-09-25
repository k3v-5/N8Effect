#pragma once

#include <random>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include "../graph/Graph.h"
#include "../graph/NodeFactory.h"
#include "GraphUndoManager.h"
#include "GraphSerializer.h"

namespace audio_graph {

/**
 * @brief Motor de Aleatorización Inteligente con Salvaguardas Acústicas (Smart Randomizer)
 * (Reglas 4, 11, 12, 21, 28, 30, 34, 46, 47)
 */
class SmartRandomizer {
public:
    enum class RandomMode : uint8_t {
        SubtleTweak = 0,     // Variación tímbrica sutil (±15% en parámetros continuos)
        ModerateMutation = 1,// Variación pronunciada (±35% con rotación de modos)
        SurprisePatch = 2    // Generación procedimental de un patch creativo completo
    };

    /**
     * @brief Aplica aleatorización inteligente al grafo y guarda el snapshot en UndoManager
     */
    static bool applyRandom(Graph& graph,
                            PresetMetadata& meta,
                            std::array<float, 8>& macros,
                            GraphUndoManager& undoManager,
                            RandomMode mode = RandomMode::ModerateMutation) {
        // 1. Guardar estado actual en el historial de Deshacer (Undo)
        undoManager.pushState(graph, meta, macros);

        // 2. Ejecutar la mutación según el modo seleccionado
        switch (mode) {
            case RandomMode::SubtleTweak:
                return tweakParameters(graph, macros, 0.15f);

            case RandomMode::ModerateMutation:
                return tweakParameters(graph, macros, 0.35f, true);

            case RandomMode::SurprisePatch:
                return generateSurprisePatch(graph, meta, macros);

            default:
                return false;
        }
    }

private:
    static uint32_t getRng() noexcept {
        static uint32_t seed = 0x87654321u;
        seed ^= (seed << 13);
        seed ^= (seed >> 17);
        seed ^= (seed << 5);
        return seed;
    }

    static float randomUniform(float minVal = 0.0f, float maxVal = 1.0f) noexcept {
        float norm = static_cast<float>(getRng() & 0x7FFFFFFF) / 2147483648.0f;
        return minVal + norm * (maxVal - minVal);
    }

    /**
     * @brief Modifica parámetros de los nodos existentes con salvaguardas acústicas estrictas
     */
    static bool tweakParameters(Graph& graph, std::array<float, 8>& macros, float intensity, bool allowModeFlips = false) {
        for (const auto& [nodeId, instance] : graph.getNodes()) {
            if (!instance || !instance->processor) continue;

            // Ignorar nodos de sistema Input (1) y Output (2)
            if (nodeId == 1 || nodeId == 2) continue;

            auto* proc = instance->processor.get();
            for (const auto& param : proc->getParameters()) {
                const std::string name = param.name;
                float currentVal = proc->getParameter(param.id);
                const float range = param.maxValue - param.minValue;

                // Si es un parámetro booleano o discreto (como selector de modo o sync)
                if (!param.isSmoothed) {
                    if (allowModeFlips && randomUniform() < 0.25f) {
                        float newVal = std::round(randomUniform(param.minValue, param.maxValue));
                        proc->setParameter(param.id, newVal);
                    }
                    continue;
                }

                // Perturbación acotada por intensidad
                float delta = (randomUniform(-1.0f, 1.0f) * intensity) * range;
                float candidateVal = currentVal + delta;

                // Salvaguardas acústicas críticas (Reglas 11, 12, 34 y 47)
                if (name.find("Feedback") != std::string::npos || name.find("feedback") != std::string::npos) {
                    // Prevenir retroalimentación destructiva desbocada
                    float maxSafeFdbk = (proc->getType() == NodeType::Feedback) ? 1.08f : 0.82f;
                    candidateVal = std::clamp(candidateVal, param.minValue, maxSafeFdbk);
                } else if (name.find("Resonance") != std::string::npos || name.find(" Q") != std::string::npos) {
                    // Prevenir picos de auto-oscilación chirriantes
                    candidateVal = std::clamp(candidateVal, param.minValue, std::min(param.maxValue, 14.0f));
                } else if (name.find("Drive") != std::string::npos || name.find("Gain") != std::string::npos) {
                    // Mantener distorsiones en rangos no ensordecedores
                    candidateVal = std::clamp(candidateVal, param.minValue, param.minValue + range * 0.85f);
                } else {
                    candidateVal = std::clamp(candidateVal, param.minValue, param.maxValue);
                }

                proc->setParameter(param.id, candidateVal);
            }
        }

        // Variar macros globales
        for (auto& m : macros) {
            m = std::clamp(m + randomUniform(-intensity, intensity), 0.0f, 1.0f);
        }

        return true;
    }

    /**
     * @brief Genera procedimentalmente un patch completo coherente, musical y libre de errores
     */
    static bool generateSurprisePatch(Graph& graph, PresetMetadata& meta, std::array<float, 8>& macros) {
        // Familias de procesadores complementarios para construir cadenas ricas
        static const std::vector<NodeType> toneFamily = {
            NodeType::Tape, NodeType::FormantFilter, NodeType::Filter,
            NodeType::ParametricEQ, NodeType::Distortion, NodeType::NoiseTexture
        };

        static const std::vector<NodeType> modulationFamily = {
            NodeType::Chorus, NodeType::Phaser, NodeType::Flanger,
            NodeType::RingModulator, NodeType::FrequencyShifter, NodeType::Resonator
        };

        static const std::vector<NodeType> spaceTimeFamily = {
            NodeType::AdvancedDelay, NodeType::Delay, NodeType::Reverb,
            NodeType::TapeStop, NodeType::Glitch, NodeType::Granular,
            NodeType::SpatialPanner
        };

        // Crear una instancia de grafo temporal limpia
        Graph newGraph;
        newGraph.clear();

        // Nodos del sistema canónicos
        newGraph.addNode(NodeFactory::getInstance().create(NodeType::Input), "", 100.0f, 250.0f);
        newGraph.addNode(NodeFactory::getInstance().create(NodeType::Output), "", 920.0f, 250.0f);

        // Elegir aleatoriamente 2 o 3 efectos de familias distintas
        auto& factory = NodeFactory::getInstance();
        NodeType type1 = toneFamily[getRng() % toneFamily.size()];
        NodeType type2 = modulationFamily[getRng() % modulationFamily.size()];
        NodeType type3 = spaceTimeFamily[getRng() % spaceTimeFamily.size()];

        NodeId id1 = newGraph.addNode(factory.create(type1), "", 320.0f, 220.0f);
        NodeId id2 = newGraph.addNode(factory.create(type2), "", 520.0f, 220.0f);
        NodeId id3 = newGraph.addNode(factory.create(type3), "", 720.0f, 220.0f);

        if (id1 == InvalidNodeId || id2 == InvalidNodeId || id3 == InvalidNodeId) {
            return false;
        }

        // Conectar la cadena canónica en serie garantizando grafo DAG acíclico
        newGraph.connect(1, 1, id1, 1);   // Input -> Efecto 1
        newGraph.connect(id1, 2, id2, 1); // Efecto 1 -> Efecto 2
        newGraph.connect(id2, 2, id3, 1); // Efecto 2 -> Efecto 3
        newGraph.connect(id3, 2, 2, 1);   // Efecto 3 -> Output

        // Ajustar parámetros con rangos estéticos balanceados
        tweakParameters(newGraph, macros, 0.4f, true);

        // Validar orden topológico (Reglas 4, 28, 29)
        std::vector<NodeId> sortedIds;
        std::string errMsg;
        if (!newGraph.validateAndTopologicalSort(sortedIds, errMsg)) {
            return false;
        }

        // Nombres aleatorios creativos de patch
        static const char* adjectives[] = { "Ethereal", "Cosmic", "Submerged", "Cyber", "Haunted", "Analog", "Liquid", "Fractal", "Radiant", "Velvet" };
        static const char* nouns[] = { "Mirage", "Nebula", "Vortex", "Tesseract", "Odyssey", "Chamber", "Drift", "Labyrinth", "Echo", "Prism" };

        const char* adj = adjectives[getRng() % 10];
        const char* n = nouns[getRng() % 10];

        meta.name = std::string(adj) + " " + std::string(n);
        meta.category = "Procedural / Random";
        meta.description = "Smart procedurally synthesized patch combining " +
                           std::string(newGraph.getNode(id1)->processor->getName()) + ", " +
                           std::string(newGraph.getNode(id2)->processor->getName()) + " and " +
                           std::string(newGraph.getNode(id3)->processor->getName()) + ".";

        // Asignar el nuevo grafo a la salida de forma segura
        graph = std::move(newGraph);
        return true;
    }
};

} // namespace audio_graph
