#pragma once

#include <vector>
#include <array>
#include <thread>
#include <atomic>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include "Graph.h"
#include "AudioProcessorNode.h"
#include "../core/RealtimePools.h"
#include "../dsp/core/FastMath.h"

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#endif

#include "ExecutionStep.h"

namespace audio_graph {

// Forward declaration de ExecutionPlan
class ExecutionPlan;

/**
 * @brief Cola circular Lock-Free Chase-Lev de robo de trabajo (Work-Stealing Deque) (Reglas 9, 10, 11, 47).
 * - El hilo propietario empuja y extrae del fondo en modo LIFO (máxima localidad de caché L1/L2).
 * - Los hilos ladrones (hilos libres) roban de la cabeza en modo FIFO.
 * - Estructura alineada a 64 bytes para erradicar False Sharing entre núcleos de CPU.
 */
template <typename T, size_t Capacity = 256>
class alignas(64) WorkStealingQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of two");
public:
    static constexpr size_t Mask = Capacity - 1;

    WorkStealingQueue() noexcept {
        top_.store(0, std::memory_order_relaxed);
        bottom_.store(0, std::memory_order_relaxed);
    }

    // Push por el hilo propietario (LIFO)
    bool push(T item) noexcept {
        const int64_t b = bottom_.load(std::memory_order_relaxed);
        const int64_t t = top_.load(std::memory_order_acquire);
        if (b - t >= static_cast<int64_t>(Capacity - 1)) {
            return false; // Cola llena
        }
        buffer_[b & Mask] = item;
        bottom_.store(b + 1, std::memory_order_release);
        return true;
    }

    // Pop por el hilo propietario (LIFO)
    bool pop(T& item) noexcept {
        const int64_t b = bottom_.load(std::memory_order_relaxed) - 1;
        bottom_.store(b, std::memory_order_seq_cst);
        int64_t t = top_.load(std::memory_order_seq_cst);

        if (t <= b) {
            item = buffer_[b & Mask];
            if (t == b) {
                // Último elemento: carrera con posibles ladrones
                bool success = top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed);
                bottom_.store(t + 1, std::memory_order_relaxed);
                return success;
            }
            return true;
        } else {
            bottom_.store(t, std::memory_order_relaxed);
            return false;
        }
    }

    // Steal por hilos ladrones (FIFO)
    bool steal(T& item) noexcept {
        int64_t t = top_.load(std::memory_order_acquire);
        const int64_t b = bottom_.load(std::memory_order_acquire);
        if (t >= b) return false;

        item = buffer_[t & Mask];
        return top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed);
    }

    bool empty() const noexcept {
        return top_.load(std::memory_order_relaxed) >= bottom_.load(std::memory_order_relaxed);
    }

    void clear() noexcept {
        top_.store(0, std::memory_order_relaxed);
        bottom_.store(0, std::memory_order_relaxed);
    }

private:
    alignas(64) std::atomic<int64_t> top_{ 0 };
    alignas(64) std::atomic<int64_t> bottom_{ 0 };
    alignas(64) std::array<T, Capacity> buffer_{};
};

/**
 * @brief Despachador de Grafo Concurrente Multihilo Lock-Free (Work-Stealing Graph Scheduler) (Reglas 4, 9, 10, 26, 47).
 * Ejecuta ramas paralelas independientes del DAG de audio en núcleos de CPU separados con sincronización lock-free
 * y cero alocaciones en el hilo de audio.
 */
class WorkStealingGraphScheduler {
public:
    static constexpr size_t MaxSteps = 64;
    static constexpr size_t MaxWorkers = 4;

    WorkStealingGraphScheduler() noexcept {
        running_.store(false, std::memory_order_relaxed);
        activeEpoch_.store(0, std::memory_order_relaxed);
        stepsRemaining_.store(0, std::memory_order_relaxed);
        for (size_t i = 0; i < MaxSteps; ++i) {
            pendingDeps_[i].store(0, std::memory_order_relaxed);
        }
    }

    WorkStealingGraphScheduler(const WorkStealingGraphScheduler&) = delete;
    WorkStealingGraphScheduler& operator=(const WorkStealingGraphScheduler&) = delete;
    WorkStealingGraphScheduler(WorkStealingGraphScheduler&&) = delete;
    WorkStealingGraphScheduler& operator=(WorkStealingGraphScheduler&&) = delete;

    ~WorkStealingGraphScheduler() {
        stopWorkers();
    }

    void prepare(const ProcessSpec& spec, uint32_t numWorkers = 2) {
        spec_ = spec;
        stopWorkers();

        // Determinar cantidad de hilos trabajadores auxiliares (excluyendo el audio thread)
        const uint32_t hw = std::thread::hardware_concurrency();
        uint32_t targetWorkers = numWorkers > 0 ? numWorkers : (hw > 2 ? hw - 2 : 1);
        numWorkers_ = std::clamp<uint32_t>(targetWorkers, 1, static_cast<uint32_t>(MaxWorkers));

        running_.store(true, std::memory_order_release);
        activeEpoch_.store(0, std::memory_order_relaxed);
        stepsRemaining_.store(0, std::memory_order_relaxed);

        for (size_t w = 0; w < numWorkers_ + 1; ++w) {
            queues_[w].clear();
        }

        // Crear hilos trabajadores auxiliares (Worker 1..N; el Worker 0 es el audio thread)
        workerThreads_.reserve(numWorkers_);
        for (uint32_t w = 1; w <= numWorkers_; ++w) {
            workerThreads_.emplace_back(&WorkStealingGraphScheduler::workerLoop, this, w);
        }
    }

    void stopWorkers() {
        if (running_.load(std::memory_order_relaxed)) {
            running_.store(false, std::memory_order_release);
            activeEpoch_.fetch_add(1, std::memory_order_release);

            for (auto& t : workerThreads_) {
                if (t.joinable()) {
                    t.join();
                }
            }
            workerThreads_.clear();
        }
    }

    /**
     * @brief Ejecuta el plan de ejecución DAG de forma concurrente con Work-Stealing en tiempo real.
     */
    void executePlan(const std::vector<ExecutionStep>& steps,
                     const ProcessContext& mainContext,
                     std::array<PreallocatedBuffer*, MaxSteps>& stepOutputBuffers,
                     AudioBufferPool& bufferPool) {
        const size_t totalSteps = std::min(steps.size(), MaxSteps);
        if (totalSteps == 0) return;

        // Contexto global compartido por los hilos para el bloque actual
        currentSteps_ = &steps;
        currentContext_ = &mainContext;
        currentBuffers_ = &stepOutputBuffers;
        currentPool_ = &bufferPool;
        totalSteps_ = totalSteps;

        // 1. Inicializar contadores atómicos de dependencias
        for (size_t i = 0; i < totalSteps; ++i) {
            const size_t inDeps = steps[i].predecessorStepIndices.size();
            pendingDeps_[i].store(static_cast<int32_t>(inDeps), std::memory_order_relaxed);
        }

        stepsRemaining_.store(static_cast<int32_t>(totalSteps), std::memory_order_release);
        const uint64_t epoch = activeEpoch_.fetch_add(1, std::memory_order_release) + 1;
        (void)epoch;

        // 2. Empujar todos los nodos raíz (sin dependencias pendientes) a la cola local del audio thread (Worker 0)
        for (size_t i = 0; i < totalSteps; ++i) {
            if (pendingDeps_[i].load(std::memory_order_relaxed) == 0) {
                queues_[0].push(static_cast<uint32_t>(i));
            }
        }

        // 3. El hilo de audio participa activamente como Worker 0
        runWorkerTask(0);

        // 4. Espera activa lock-free determinista hasta que todas las tareas concluyan
        while (stepsRemaining_.load(std::memory_order_acquire) > 0) {
            runWorkerTask(0);
#if defined(_MSC_VER) || defined(__x86_64__) || defined(__i386__)
            _mm_pause();
#endif
        }
    }

    uint32_t getNumWorkers() const noexcept { return numWorkers_; }

private:
    void workerLoop(uint32_t workerId) {
        uint64_t lastSeenEpoch = 0;

        while (running_.load(std::memory_order_relaxed)) {
            const uint64_t currentEpoch = activeEpoch_.load(std::memory_order_acquire);

            if (currentEpoch != lastSeenEpoch) {
                lastSeenEpoch = currentEpoch;
            }

            if (stepsRemaining_.load(std::memory_order_acquire) > 0) {
                runWorkerTask(workerId);
            } else {
#if defined(_MSC_VER) || defined(__x86_64__) || defined(__i386__)
                _mm_pause();
#else
                std::this_thread::yield();
#endif
            }
        }
    }

    void runWorkerTask(uint32_t workerId) {
        uint32_t stepIdx = 0;
        bool hasTask = queues_[workerId].pop(stepIdx);

        // Si la cola local está vacía, intentar robar tareas de otros hilos (Work-Stealing)
        if (!hasTask) {
            const size_t totalQueues = numWorkers_ + 1;
            for (size_t i = 1; i < totalQueues; ++i) {
                const size_t victim = (workerId + i) % totalQueues;
                if (queues_[victim].steal(stepIdx)) {
                    hasTask = true;
                    break;
                }
            }
        }

        if (!hasTask) return;

        // Ejecutar el paso de procesamiento
        executeSingleStep(stepIdx);

        // Decrementar el contador global de pasos restantes
        stepsRemaining_.fetch_sub(1, std::memory_order_acq_rel);

        // Notificar a los sucesores
        for (size_t s = 0; s < totalSteps_; ++s) {
            const auto& candidateSuccessor = (*currentSteps_)[s];
            const auto& preds = candidateSuccessor.predecessorStepIndices;
            if (std::find(preds.begin(), preds.end(), stepIdx) != preds.end()) {
                if (pendingDeps_[s].fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    // Todas las dependencias resueltas: despachar inmediatamente a la cola local
                    queues_[workerId].push(static_cast<uint32_t>(s));
                }
            }
        }
    }

    void executeSingleStep(uint32_t stepIdx) {
        if (stepIdx >= totalSteps_ || currentSteps_ == nullptr) return;

        const auto& step = (*currentSteps_)[stepIdx];
        PreallocatedBuffer* outBuf = (*currentBuffers_)[stepIdx];
        if (outBuf == nullptr) return;

        const ProcessContext& mainContext = *currentContext_;
        const uint32_t numSamples = mainContext.numSamples;
        const uint32_t numChannels = outBuf->getNumChannels();

        const float* inL = nullptr;
        const float* inR = nullptr;

        // Determinar fuente de entrada para este nodo
        if (step.isRoot) {
            inL = (mainContext.numInputChannels > 0 && mainContext.inputChannels[0] != nullptr)
                ? mainContext.inputChannels[0] : nullptr;
            inR = (mainContext.numInputChannels > 1 && mainContext.inputChannels[1] != nullptr)
                ? mainContext.inputChannels[1] : inL;
        } else if (step.predecessorStepIndices.size() == 1) {
            const size_t predIdx = step.predecessorStepIndices[0];
            PreallocatedBuffer* predBuf = (predIdx < MaxSteps) ? (*currentBuffers_)[predIdx] : nullptr;
            if (predBuf != nullptr) {
                inL = predBuf->getReadPointer(0);
                inR = (predBuf->getNumChannels() > 1) ? predBuf->getReadPointer(1) : inL;
            }
        } else {
            // Múltiples predecesores: mezcla acumulativa en buffer temporal
            PreallocatedBuffer* mixBuf = currentPool_->acquire();
            if (mixBuf != nullptr) {
                mixBuf->clear(numSamples);
                float* mixL = mixBuf->getWritePointer(0);
                float* mixR = (mixBuf->getNumChannels() > 1) ? mixBuf->getWritePointer(1) : mixL;

                for (size_t predIdx : step.predecessorStepIndices) {
                    if (predIdx < MaxSteps && (*currentBuffers_)[predIdx] != nullptr) {
                        const auto* pBuf = (*currentBuffers_)[predIdx];
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

        // Sidechain y modulación
        const float* scL = (step.sidechainStepIndex >= 0 && static_cast<size_t>(step.sidechainStepIndex) < MaxSteps && (*currentBuffers_)[step.sidechainStepIndex] != nullptr)
            ? (*currentBuffers_)[step.sidechainStepIndex]->getReadPointer(0) : inL;
        const float* scR = (step.sidechainStepIndex >= 0 && static_cast<size_t>(step.sidechainStepIndex) < MaxSteps && (*currentBuffers_)[step.sidechainStepIndex] != nullptr && (*currentBuffers_)[step.sidechainStepIndex]->getNumChannels() > 1)
            ? (*currentBuffers_)[step.sidechainStepIndex]->getReadPointer(1) : scL;
        const float* scPointers[2] = { scL, scR };

        const float* inPointers[2] = { inL, inR };
        float* outPointers[2] = { outBuf->getWritePointer(0), (numChannels > 1) ? outBuf->getWritePointer(1) : nullptr };

        ProcessContext stepContext = mainContext;
        stepContext.inputChannels = inPointers;
        stepContext.outputChannels = outPointers;
        stepContext.sidechainChannels = scPointers;
        stepContext.numInputChannels = (inL != nullptr) ? (inR != nullptr ? 2 : 1) : 0;
        stepContext.numOutputChannels = numChannels;

        if (step.isBypassed) {
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
    }

    ProcessSpec spec_;
    uint32_t numWorkers_{ 1 };
    std::atomic<bool> running_{ false };
    std::atomic<uint64_t> activeEpoch_{ 0 };
    std::atomic<int32_t> stepsRemaining_{ 0 };

    alignas(64) std::array<std::atomic<int32_t>, MaxSteps> pendingDeps_{};
    alignas(64) std::array<WorkStealingQueue<uint32_t, 256>, MaxWorkers + 1> queues_{};
    std::vector<std::thread> workerThreads_;

    const std::vector<ExecutionStep>* currentSteps_{ nullptr };
    const ProcessContext* currentContext_{ nullptr };
    std::array<PreallocatedBuffer*, MaxSteps>* currentBuffers_{ nullptr };
    AudioBufferPool* currentPool_{ nullptr };
    size_t totalSteps_{ 0 };
};

} // namespace audio_graph
