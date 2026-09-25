#pragma once

#include <vector>
#include <array>
#include <unordered_map>
#include <memory>
#include <atomic>
#include "Graph.h"
#include "../core/RealtimePools.h"
#include "../dsp/core/DenormalGuards.h"

namespace audio_graph {

/**
 * @brief Paso de ejecución compilado para el Runtime Graph (Regla 28)
 */
/**
 * @brief Paso de ejecución compilado para el Runtime Graph (Regla 28)
 */
struct ExecutionStep {
    AudioProcessorNode* processor{ nullptr };
    NodeId nodeId{ InvalidNodeId };
    std::vector<NodeId> predecessorNodes;
    std::vector<NodeId> successorNodes;
    std::vector<size_t> predecessorStepIndices;
    int sidechainStepIndex{ -1 };
    int audioRateModStepIndex{ -1 };
    bool isRoot{ true };
    bool isLeaf{ true };
    bool isBypassed{ false };
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
        hasExplicitConnections_ = !graph.getConnections().empty();

        // Mapeo de nodeId a índice de paso
        std::unordered_map<NodeId, size_t> nodeToStepIdx;

        for (size_t i = 0; i < topologicalOrder.size(); ++i) {
            NodeId id = topologicalOrder[i];
            auto* proc = graph.getNodeProcessor(id);
            if (proc == nullptr) continue;

            nodeToStepIdx[id] = steps_.size();

            ExecutionStep step;
            step.processor = proc;
            step.nodeId = id;
            step.isBypassed = graph.isNodeBypassed(id);
            steps_.push_back(std::move(step));
        }

        // Resolver dependencias de predecesores, sucesores, sidechain y audio-rate mod (Reglas 4, 6 y 7)
        for (const auto& conn : graph.getConnections()) {
            auto srcIt = nodeToStepIdx.find(conn.sourceNodeId);
            auto destIt = nodeToStepIdx.find(conn.destNodeId);

            if (srcIt != nodeToStepIdx.end() && destIt != nodeToStepIdx.end()) {
                const size_t srcIdx = srcIt->second;
                const size_t destIdx = destIt->second;

                steps_[srcIdx].successorNodes.push_back(conn.destNodeId);
                steps_[srcIdx].isLeaf = false;

                if (conn.destPinId == SidechainPinId) {
                    // Es una conexión de Sidechain modular: no se mezcla con el audio principal
                    steps_[destIdx].sidechainStepIndex = static_cast<int>(srcIdx);
                } else if (conn.destPinId == AudioRateModPinId) {
                    // Es una conexión de Modulación Audio-Rate: no se mezcla con el audio principal
                    steps_[destIdx].audioRateModStepIndex = static_cast<int>(srcIdx);
                } else {
                    steps_[destIdx].predecessorNodes.push_back(conn.sourceNodeId);
                    steps_[destIdx].predecessorStepIndices.push_back(srcIdx);
                    steps_[destIdx].isRoot = false;
                }
            }
        }

        // Regla 4: Si el grafo tiene conexiones explícitas, los nodos aislados (sin cables conectados)
        // permanecen inactivos en el audio stream (ni reciben señal maestra ni se mezclan al master)
        if (hasExplicitConnections_) {
            for (auto& step : steps_) {
                if (step.predecessorNodes.empty() && step.successorNodes.empty() &&
                    step.sidechainStepIndex < 0 && step.audioRateModStepIndex < 0) {
                    step.isRoot = false;
                    step.isLeaf = false;
                }
            }
        }
    }

    const std::vector<ExecutionStep>& getSteps() const noexcept {
        return steps_;
    }

    bool hasExplicitConnections() const noexcept { return hasExplicitConnections_; }
    bool isEmpty() const noexcept { return steps_.empty(); }

private:
    std::vector<ExecutionStep> steps_;
    bool hasExplicitConnections_{ false };
};

/**
 * @brief Ejecutor en tiempo real del Grafo (Regla 9: Cero mallocs en process, Regla 47: Anti-Memory Abuse y Caché L1)
 */
class GraphExecutor {
public:
    GraphExecutor() = default;

    /**
     * @brief Prepara el ejecutor con dimensionamiento acotado y racional (Regla 47).
     */
    void prepare(const ProcessSpec& spec, uint32_t maxConcurrentBuffers = 64) {
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

    // Ejecuta el plan compilado en tiempo real con cero allocations y soporte completo de rutas DAG (Reglas 4, 9, 34 y 47)
    void process(const ExecutionPlan& plan, const ProcessContext& mainContext, PreallocatedBuffer& finalOutput) {
        ScopedDenormalGuard denormalGuard; // Erradicación de denormales por hardware (Reglas 34 y 47)
        bufferPool_.releaseAll();

        const auto& steps = plan.getSteps();
        if (steps.empty()) {
            finalOutput.clear(mainContext.numSamples);
            return;
        }

        const uint32_t numSamples = mainContext.numSamples;
        const uint32_t numChannels = finalOutput.getNumChannels();

        // 1. CASO COMPATIBILIDAD RETROACTIVA: Si no hay conexiones explícitas, ejecutar en cadena lineal ping-pong
        if (!plan.hasExplicitConnections()) {
            PreallocatedBuffer* bufA = bufferPool_.acquire();
            if (bufA == nullptr) {
                finalOutput.clear(numSamples);
                return;
            }
            bufA->copyFrom(mainContext.inputChannels, mainContext.numInputChannels, numSamples);

            PreallocatedBuffer* bufB = bufferPool_.acquire();
            if (bufB == nullptr) {
                finalOutput.copyFrom(bufA->getArrayOfReadPointers(), bufA->getNumChannels(), numSamples);
                bufferPool_.releaseAll();
                return;
            }

            PreallocatedBuffer* currentInput = bufA;
            PreallocatedBuffer* currentOutput = bufB;

            for (const auto& step : steps) {
                if (step.isBypassed) {
                    currentOutput->copyFrom(currentInput->getArrayOfReadPointers(), currentInput->getNumChannels(), numSamples);
                } else {
                    ProcessContext stepContext = mainContext;
                    stepContext.inputChannels = currentInput->getArrayOfReadPointers();
                    stepContext.outputChannels = currentOutput->getArrayOfWritePointers();
                    stepContext.numInputChannels = currentInput->getNumChannels();
                    stepContext.numOutputChannels = currentOutput->getNumChannels();

                    step.processor->process(stepContext);
                }
                std::swap(currentInput, currentOutput);
            }

            finalOutput.copyFrom(currentInput->getArrayOfReadPointers(), currentInput->getNumChannels(), numSamples);
            bufferPool_.releaseAll();
            return;
        }

        // 2. CASO DAG MODULAR: Conexiones explícitas, bifurcaciones paralelas y mezclas (Reglas 4, 6, 9)
        constexpr size_t MaxSteps = 64;
        std::array<PreallocatedBuffer*, MaxSteps> stepOutputBuffers{};
        stepOutputBuffers.fill(nullptr);

        const size_t totalSteps = std::min(steps.size(), MaxSteps);

        for (size_t i = 0; i < totalSteps; ++i) {
            const auto& step = steps[i];
            PreallocatedBuffer* outBuf = bufferPool_.acquire();
            if (outBuf == nullptr) {
                break; // Protección de sobrecarga de pool
            }
            stepOutputBuffers[i] = outBuf;

            const float* inL = nullptr;
            const float* inR = nullptr;

            // Determinar fuente de entrada para este nodo
            if (step.isRoot) {
                // Nodo raíz: recibe directamente la señal principal del bloque
                inL = (mainContext.numInputChannels > 0 && mainContext.inputChannels[0] != nullptr)
                    ? mainContext.inputChannels[0] : nullptr;
                inR = (mainContext.numInputChannels > 1 && mainContext.inputChannels[1] != nullptr)
                    ? mainContext.inputChannels[1] : inL;
            } else if (step.predecessorStepIndices.size() == 1) {
                // 1 predecesor: enlace directo sin copia
                const size_t predIdx = step.predecessorStepIndices[0];
                PreallocatedBuffer* predBuf = (predIdx < MaxSteps) ? stepOutputBuffers[predIdx] : nullptr;
                if (predBuf != nullptr) {
                    inL = predBuf->getReadPointer(0);
                    inR = (predBuf->getNumChannels() > 1) ? predBuf->getReadPointer(1) : inL;
                }
            } else {
                // Múltiples predecesores (FAN-IN / MERGE): mezclar señales en un buffer acumulador
                PreallocatedBuffer* mixBuf = bufferPool_.acquire();
                if (mixBuf != nullptr) {
                    mixBuf->clear(numSamples);
                    float* mixL = mixBuf->getWritePointer(0);
                    float* mixR = (mixBuf->getNumChannels() > 1) ? mixBuf->getWritePointer(1) : mixL;

                    for (size_t predIdx : step.predecessorStepIndices) {
                        if (predIdx < MaxSteps && stepOutputBuffers[predIdx] != nullptr) {
                            const auto* pBuf = stepOutputBuffers[predIdx];
                            const float* pL = pBuf->getReadPointer(0);
                            const float* pR = (pBuf->getNumChannels() > 1) ? pBuf->getReadPointer(1) : pL;
                            for (uint32_t s = 0; s < numSamples; ++s) {
                                mixL[s] += pL[s];
                                mixR[s] += pR[s];
                            }
                        }
                    }
                    inL = mixBuf->getReadPointer(0);
                    inR = (mixBuf->getNumChannels() > 1) ? mixBuf->getReadPointer(1) : inL;
                }
            }

            // Resolver canales de Sidechain (Regla 6 y 13)
            const float* scL = nullptr;
            const float* scR = nullptr;
            if (step.sidechainStepIndex >= 0 && static_cast<size_t>(step.sidechainStepIndex) < MaxSteps && stepOutputBuffers[step.sidechainStepIndex] != nullptr) {
                const auto* scBuf = stepOutputBuffers[step.sidechainStepIndex];
                scL = scBuf->getReadPointer(0);
                scR = (scBuf->getNumChannels() > 1) ? scBuf->getReadPointer(1) : scL;
            } else {
                scL = inL;
                scR = inR;
            }
            const float* scPointers[2] = { scL, scR };

            // Resolver canales de Audio-Rate Modulation (Módulo 3)
            const float* armL = nullptr;
            const float* armR = nullptr;
            if (step.audioRateModStepIndex >= 0 && static_cast<size_t>(step.audioRateModStepIndex) < MaxSteps && stepOutputBuffers[step.audioRateModStepIndex] != nullptr) {
                const auto* armBuf = stepOutputBuffers[step.audioRateModStepIndex];
                armL = armBuf->getReadPointer(0);
                armR = (armBuf->getNumChannels() > 1) ? armBuf->getReadPointer(1) : armL;
            }
            const float* armPointers[2] = { armL, armR };

            // Preparar contexto del paso
            const float* inPointers[2] = { inL, inR };
            float* outPointers[2] = { outBuf->getWritePointer(0), (outBuf->getNumChannels() > 1) ? outBuf->getWritePointer(1) : nullptr };

            ProcessContext stepContext = mainContext;
            stepContext.inputChannels = inPointers;
            stepContext.outputChannels = outPointers;
            stepContext.sidechainChannels = scPointers;
            stepContext.audioRateModChannels = (armL != nullptr) ? armPointers : nullptr;
            stepContext.numInputChannels = (inL != nullptr) ? (inR != nullptr ? 2 : 1) : 0;
            stepContext.numOutputChannels = numChannels;
            stepContext.numSidechainChannels = (scL != nullptr) ? (scR != nullptr ? 2 : 1) : 0;
            stepContext.numAudioRateModChannels = (armL != nullptr) ? (armR != nullptr ? 2 : 1) : 0;

            if (step.isBypassed) {
                // Modo Bypass (Regla 5 y 9): traspasar la señal entrante sin procesamiento
                if (inL != nullptr && outPointers[0] != nullptr) {
                    std::memcpy(outPointers[0], inL, numSamples * sizeof(float));
                } else if (outPointers[0] != nullptr) {
                    std::memset(outPointers[0], 0, numSamples * sizeof(float));
                }
                if (outPointers[1] != nullptr) {
                    if (inR != nullptr) std::memcpy(outPointers[1], inR, numSamples * sizeof(float));
                    else if (inL != nullptr) std::memcpy(outPointers[1], inL, numSamples * sizeof(float));
                    else std::memset(outPointers[1], 0, numSamples * sizeof(float));
                }
            } else {
                step.processor->process(stepContext);
            }

            // Telemetría de nodo probado (Node Probing para el Visualizador, Regla 23 y 26)
            if (probeVisualizer_ != nullptr && step.nodeId == probeNodeId_.load(std::memory_order_relaxed)) {
                probeVisualizer_->writeBlock(outBuf->getReadPointer(0),
                                             (outBuf->getNumChannels() > 1) ? outBuf->getReadPointer(1) : outBuf->getReadPointer(0),
                                             numSamples);
            }
        }

        // 3. MEZCLA HACIA LA SALIDA FINAL: Combinar todos los nodos terminales (hojas del grafo)
        finalOutput.clear(numSamples);
        bool anyLeafWritten = false;

        for (size_t i = 0; i < totalSteps; ++i) {
            if (steps[i].isLeaf && stepOutputBuffers[i] != nullptr) {
                const auto* leafBuf = stepOutputBuffers[i];
                const float* leafL = leafBuf->getReadPointer(0);
                const float* leafR = (leafBuf->getNumChannels() > 1) ? leafBuf->getReadPointer(1) : leafL;

                float* outL = finalOutput.getWritePointer(0);
                float* outR = (numChannels > 1) ? finalOutput.getWritePointer(1) : outL;

                if (!anyLeafWritten) {
                    for (uint32_t s = 0; s < numSamples; ++s) {
                        outL[s] = leafL[s];
                        outR[s] = leafR[s];
                    }
                    anyLeafWritten = true;
                } else {
                    for (uint32_t s = 0; s < numSamples; ++s) {
                        outL[s] += leafL[s];
                        outR[s] += leafR[s];
                    }
                }
            }
        }

        // Reciclar todos los buffers del pool para el próximo bloque
        bufferPool_.releaseAll();
    }

    void setProbeNodeId(NodeId id) noexcept { probeNodeId_.store(id, std::memory_order_relaxed); }
    NodeId getProbeNodeId() const noexcept { return probeNodeId_.load(std::memory_order_relaxed); }
    void setProbeVisualizer(AudioVisualizerBuffer* viz) noexcept { probeVisualizer_ = viz; }

private:
    ProcessSpec spec_;
    AudioBufferPool bufferPool_;
    std::atomic<NodeId> probeNodeId_{ InvalidNodeId };
    AudioVisualizerBuffer* probeVisualizer_{ nullptr };
};

} // namespace audio_graph
