#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include "Graph.h"
#include "../core/RealtimePools.h"
#include "../dsp/core/DenormalGuards.h"

namespace audio_graph {

/**
 * @brief Paso de ejecución compilado para el Runtime Graph (Regla 28)
 */
struct ExecutionStep {
    AudioProcessorNode* processor{ nullptr };
    NodeId nodeId{ InvalidNodeId };
    std::vector<NodeId> predecessorNodes;
    std::vector<NodeId> successorNodes;
};

/**
 * @brief Plan de Ejecución compilado y optimizado (Reglas 28, 29, 30).
 */
class ExecutionPlan {
public:
    ExecutionPlan() = default;

    void compileFrom(const Graph& graph, const std::vector<NodeId>& topologicalOrder) {
        steps_.clear();
        steps_.reserve(topologicalOrder.size());

        for (NodeId id : topologicalOrder) {
            auto* proc = graph.getNodeProcessor(id);
            if (proc == nullptr) continue;

            ExecutionStep step;
            step.processor = proc;
            step.nodeId = id;

            // Encontrar predecesores y sucesores de audio
            for (const auto& conn : graph.getConnections()) {
                if (conn.destNodeId == id) {
                    step.predecessorNodes.push_back(conn.sourceNodeId);
                }
                if (conn.sourceNodeId == id) {
                    step.successorNodes.push_back(conn.destNodeId);
                }
            }
            steps_.push_back(std::move(step));
        }
    }

    const std::vector<ExecutionStep>& getSteps() const noexcept {
        return steps_;
    }

    bool isEmpty() const noexcept { return steps_.empty(); }

private:
    std::vector<ExecutionStep> steps_;
};

/**
 * @brief Ejecutor en tiempo real del Grafo (Regla 9: Cero mallocs en process, Regla 47: Anti-Memory Abuse y Caché L1)
 */
class GraphExecutor {
public:
    GraphExecutor() = default;

    /**
     * @brief Prepara el ejecutor con dimensionamiento acotado y racional (Regla 47).
     * En lugar de sobredimensionar buffers innecesariamente, prealoca un pool compacto
     * para mantener los datos calientes en la memoria caché L1/L2.
     */
    void prepare(const ProcessSpec& spec, uint32_t maxConcurrentBuffers = 16) {
        spec_ = spec;
        bufferPool_.prepare(maxConcurrentBuffers, spec.numOutputChannels, spec.maximumBlockSize);
    }

    void reset() {
        bufferPool_.releaseAll();
    }

    size_t getBufferPoolCapacity() const noexcept {
        return bufferPool_.getTotalCapacity();
    }

    size_t getBufferPoolAvailable() const noexcept {
        return bufferPool_.getAvailableCount();
    }

    // Ejecuta el plan compilado en tiempo real con cero allocations y reciclaje de caché (Reglas 9, 34 y 47)
    void process(const ExecutionPlan& plan, const ProcessContext& mainContext, PreallocatedBuffer& finalOutput) {
        ScopedDenormalGuard denormalGuard; // Erradicación de denormales por hardware (Reglas 34 y 47)
        bufferPool_.releaseAll();

        const auto& steps = plan.getSteps();
        if (steps.empty()) {
            finalOutput.clear(mainContext.numSamples);
            return;
        }

        // Buffer primario para la señal de entrada
        PreallocatedBuffer* bufA = bufferPool_.acquire();
        if (bufA == nullptr) {
            finalOutput.clear(mainContext.numSamples);
            return;
        }

        bufA->copyFrom(mainContext.inputChannels, mainContext.numInputChannels, mainContext.numSamples);

        // Buffer secundario para alternancia ping-pong en caché L1
        PreallocatedBuffer* bufB = bufferPool_.acquire();
        if (bufB == nullptr) {
            finalOutput.copyFrom(bufA->getArrayOfReadPointers(), bufA->getNumChannels(), mainContext.numSamples);
            bufferPool_.releaseAll();
            return;
        }

        PreallocatedBuffer* currentInput = bufA;
        PreallocatedBuffer* currentOutput = bufB;

        for (const auto& step : steps) {
            ProcessContext stepContext = mainContext;
            stepContext.inputChannels = currentInput->getArrayOfReadPointers();
            stepContext.outputChannels = currentOutput->getArrayOfWritePointers();
            stepContext.numInputChannels = currentInput->getNumChannels();
            stepContext.numOutputChannels = currentOutput->getNumChannels();

            step.processor->process(stepContext);

            // Reutilización inmediata: la salida actual se convierte en la entrada del siguiente
            std::swap(currentInput, currentOutput);
        }

        // currentInput contiene el último resultado procesado tras el swap
        finalOutput.copyFrom(currentInput->getArrayOfReadPointers(), currentInput->getNumChannels(), mainContext.numSamples);

        // Reciclar todos los buffers del pool para el próximo bloque
        bufferPool_.releaseAll();
    }

private:
    ProcessSpec spec_;
    AudioBufferPool bufferPool_;
};

} // namespace audio_graph
