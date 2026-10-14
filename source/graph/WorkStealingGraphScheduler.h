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
#include "../dsp/rust_core/include/n8_rust_core.h"

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
 * @brief Cola circular Lock-Free Chase-Lev de robo de trabajo (Work-Stealing Deque) delegada al Rust Core (Reglas 4, 9, 10, 11, 47).
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
        rustQueue_ = n8_rust_work_stealing_queue_create(static_cast<uint32_t>(Capacity));
    }

    ~WorkStealingQueue() noexcept {
        if (rustQueue_) {
            n8_rust_work_stealing_queue_destroy(rustQueue_);
            rustQueue_ = nullptr;
        }
    }

    WorkStealingQueue(const WorkStealingQueue&) = delete;
    WorkStealingQueue& operator=(const WorkStealingQueue&) = delete;

    WorkStealingQueue(WorkStealingQueue&& other) noexcept : rustQueue_(other.rustQueue_) {
        other.rustQueue_ = nullptr;
    }

    WorkStealingQueue& operator=(WorkStealingQueue&& other) noexcept {
        if (this != &other) {
            if (rustQueue_) n8_rust_work_stealing_queue_destroy(rustQueue_);
            rustQueue_ = other.rustQueue_;
            other.rustQueue_ = nullptr;
        }
        return *this;
    }

    bool push(T item) noexcept {
        if (!rustQueue_) return false;
        return n8_rust_work_stealing_queue_push(rustQueue_, static_cast<uint32_t>(item));
    }

    bool pop(T& item) noexcept {
        if (!rustQueue_) return false;
        uint32_t val = 0;
        if (n8_rust_work_stealing_queue_pop(rustQueue_, &val)) {
            item = static_cast<T>(val);
            return true;
        }
        return false;
    }

    bool steal(T& item) noexcept {
        if (!rustQueue_) return false;
        uint32_t val = 0;
        if (n8_rust_work_stealing_queue_steal(rustQueue_, &val)) {
            item = static_cast<T>(val);
            return true;
        }
        return false;
    }

    bool empty() const noexcept {
        if (!rustQueue_) return true;
        return n8_rust_work_stealing_queue_empty(rustQueue_);
    }

    void clear() noexcept {
        if (rustQueue_) {
            n8_rust_work_stealing_queue_clear(rustQueue_);
        }
    }

    RustWorkStealingQueue* getRustQueue() noexcept { return rustQueue_; }
    const RustWorkStealingQueue* getRustQueue() const noexcept { return rustQueue_; }

private:
    RustWorkStealingQueue* rustQueue_{ nullptr };
};

/**
 * @brief Despachador de Grafo Concurrente Multihilo Lock-Free (Work-Stealing Graph Scheduler) (Reglas 4, 9, 10, 26, 47).
 * Delega la coordinación de colas de robo de trabajo, resolución de DAG y dependencias al Rust Core.
 * Ejecuta ramas paralelas independientes del DAG de audio en núcleos de CPU separados con sincronización lock-free
 * y cero alocaciones en el hilo de audio.
 */
class WorkStealingGraphScheduler {
public:
    static constexpr size_t MaxSteps = 64;
    static constexpr size_t MaxWorkers = 4;

    WorkStealingGraphScheduler() noexcept {
        rustCore_ = n8_rust_work_stealing_scheduler_create();
        running_.store(false, std::memory_order_relaxed);
        activeEpoch_.store(0, std::memory_order_relaxed);
    }

    WorkStealingGraphScheduler(const WorkStealingGraphScheduler&) = delete;
    WorkStealingGraphScheduler& operator=(const WorkStealingGraphScheduler&) = delete;
    WorkStealingGraphScheduler(WorkStealingGraphScheduler&&) = delete;
    WorkStealingGraphScheduler& operator=(WorkStealingGraphScheduler&&) = delete;

    ~WorkStealingGraphScheduler() {
        stopWorkers();
        if (rustCore_) {
            n8_rust_work_stealing_scheduler_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    void prepare(const ProcessSpec& spec, uint32_t numWorkers = 2) {
        spec_ = spec;
        stopWorkers();

        // Determinar cantidad de hilos trabajadores auxiliares (excluyendo el audio thread)
        const uint32_t hw = std::thread::hardware_concurrency();
        uint32_t targetWorkers = numWorkers > 0 ? numWorkers : (hw > 2 ? hw - 2 : 1);
        numWorkers_ = std::clamp<uint32_t>(targetWorkers, 1, static_cast<uint32_t>(MaxWorkers));

        if (rustCore_) {
            n8_rust_work_stealing_scheduler_prepare(rustCore_, numWorkers_);
        }

        running_.store(true, std::memory_order_release);
        activeEpoch_.store(0, std::memory_order_relaxed);

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
            activeEpoch_.notify_all();

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
        if (totalSteps == 0 || rustCore_ == nullptr) return;

        // Contexto global compartido por los hilos para el bloque actual
        currentSteps_ = &steps;
        currentContext_ = &mainContext;
        currentBuffers_ = &stepOutputBuffers;
        currentPool_ = &bufferPool;
        totalSteps_ = totalSteps;

        // 1. Inicializar cada paso y sus dependencias/sucesores en Rust
        for (size_t i = 0; i < totalSteps; ++i) {
            size_t inDeps = steps[i].predecessorStepIndices.size();
            if (steps[i].sidechainStepIndex >= 0) inDeps++;
            if (steps[i].audioRateModStepIndex >= 0) inDeps++;

            uint32_t succs[MaxSteps];
            size_t numSuccs = 0;
            for (size_t s = 0; s < totalSteps; ++s) {
                const auto& candidateSuccessor = steps[s];
                const auto& preds = candidateSuccessor.predecessorStepIndices;
                const bool isDep = (std::find(preds.begin(), preds.end(), i) != preds.end())
                                || (candidateSuccessor.sidechainStepIndex == static_cast<int>(i))
                                || (candidateSuccessor.audioRateModStepIndex == static_cast<int>(i));
                if (isDep) {
                    succs[numSuccs++] = static_cast<uint32_t>(s);
                }
            }

            n8_rust_work_stealing_scheduler_init_step(rustCore_, i, static_cast<int32_t>(inDeps), succs, numSuccs);
        }

        // 2. Iniciar ejecución DAG en Rust (empuja los nodos raíz a Worker 0)
        n8_rust_work_stealing_scheduler_start_execution(rustCore_, totalSteps);

        // 3. Notificar a los trabajadores dormidos en espera pasiva (Milestone 1, Reglas 9 y 47)
        activeEpoch_.fetch_add(1, std::memory_order_release);
        activeEpoch_.notify_all();

        // 4. El hilo de audio participa activamente como Worker 0
        runWorkerTask(0);

        // 5. Espera activa lock-free determinista hasta que todas las tareas concluyan
        while (n8_rust_work_stealing_scheduler_get_steps_remaining(rustCore_) > 0) {
            runWorkerTask(0);
#if defined(_MSC_VER) || defined(__x86_64__) || defined(__i386__)
            _mm_pause();
#endif
        }
    }

    uint32_t getNumWorkers() const noexcept { return numWorkers_; }

    RustWorkStealingScheduler* getRustScheduler() noexcept { return rustCore_; }
    const RustWorkStealingScheduler* getRustScheduler() const noexcept { return rustCore_; }

private:
    void workerLoop(uint32_t workerId) {
        uint64_t lastSeenEpoch = activeEpoch_.load(std::memory_order_acquire);

        while (running_.load(std::memory_order_relaxed)) {
            const uint64_t currentEpoch = activeEpoch_.load(std::memory_order_acquire);

            if (currentEpoch != lastSeenEpoch) {
                lastSeenEpoch = currentEpoch;
            }

            if (n8_rust_work_stealing_scheduler_get_steps_remaining(rustCore_) > 0) {
                if (!runWorkerTask(workerId)) {
#if defined(_MSC_VER) || defined(__x86_64__) || defined(__i386__)
                    _mm_pause();
#else
                    std::this_thread::yield();
#endif
                }
            } else {
                // Sincronización pasiva de coste cero en kernel: suspende el hilo sin consumir CPU (Milestone 1, Regla 47)
                activeEpoch_.wait(lastSeenEpoch, std::memory_order_relaxed);
            }
        }
    }

    bool runWorkerTask(uint32_t workerId) {
        if (!rustCore_) return false;

        uint32_t stepIdx = 0;
        if (!n8_rust_work_stealing_scheduler_pop_or_steal(rustCore_, workerId, &stepIdx)) {
            return false;
        }

        // Ejecutar el paso de procesamiento DSP
        executeSingleStep(stepIdx);

        // Notificar a los sucesores y decrementar steps_remaining en Rust
        n8_rust_work_stealing_scheduler_finish_step(rustCore_, workerId, stepIdx);
        return true;
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

        // Resolver canales de Sidechain (Regla 6 y 13)
        const float* scL = nullptr;
        const float* scR = nullptr;
        if (step.sidechainStepIndex >= 0 && static_cast<size_t>(step.sidechainStepIndex) < MaxSteps && (*currentBuffers_)[step.sidechainStepIndex] != nullptr) {
            const auto* scBuf = (*currentBuffers_)[step.sidechainStepIndex];
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
        if (step.audioRateModStepIndex >= 0 && static_cast<size_t>(step.audioRateModStepIndex) < MaxSteps && (*currentBuffers_)[step.audioRateModStepIndex] != nullptr) {
            const auto* armBuf = (*currentBuffers_)[step.audioRateModStepIndex];
            armL = armBuf->getReadPointer(0);
            armR = (armBuf->getNumChannels() > 1) ? armBuf->getReadPointer(1) : armL;
        }
        const float* armPointers[2] = { armL, armR };

        const float* inPointers[2] = { inL, inR };
        float* outPointers[2] = { outBuf->getWritePointer(0), (numChannels > 1) ? outBuf->getWritePointer(1) : nullptr };

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

    RustWorkStealingScheduler* rustCore_{ nullptr };
    std::vector<std::thread> workerThreads_;

    const std::vector<ExecutionStep>* currentSteps_{ nullptr };
    const ProcessContext* currentContext_{ nullptr };
    std::array<PreallocatedBuffer*, MaxSteps>* currentBuffers_{ nullptr };
    AudioBufferPool* currentPool_{ nullptr };
    size_t totalSteps_{ 0 };
};

} // namespace audio_graph
