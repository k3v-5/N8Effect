#pragma once

#include <chrono>
#include <atomic>
#include <array>
#include <cstdint>
#include <algorithm>

namespace audio_graph {

/**
 * @brief Etapas identificables para perfilado de DSP (Reglas 9, 39, 47)
 */
enum class ProfilerStage : uint8_t {
    Analysis = 0,
    Events,
    Graph,
    NumStages
};

/**
 * @brief Estructura de telemetría de rendimiento y CPU (Reglas 26, 39, 47)
 */
struct PerformanceMetrics {
    float currentCpuPercent{ 0.0f };   // Carga actual en % (DSP Time / Buffer Budget * 100)
    float peakCpuPercent{ 0.0f };      // Pico de carga en % con decaimiento suave
    float totalDspUs{ 0.0f };          // Tiempo total de procesamiento en microsegundos
    float bufferBudgetUs{ 0.0f };      // Presupuesto total del bloque en microsegundos
    float analysisUs{ 0.0f };          // Tiempo invertido en Analysis Engine (µs)
    float eventEngineUs{ 0.0f };       // Tiempo invertido en Event Engine (µs)
    float graphExecutionUs{ 0.0f };    // Tiempo invertido en Graph Executor (µs)
    uint32_t activeEvents{ 0 };        // Conteo de eventos activos en el EventPool
    uint32_t activeGrains{ 0 };        // Conteo de granos activos en el GrainPool
    uint32_t totalNodes{ 0 };          // Cantidad de nodos presentes en el Grafo
    bool overloadDetected{ false };    // Indicador de sobrecarga / xrun si totalDspUs > bufferBudgetUs
};

/**
 * @brief Perfilador de alta precisión en tiempo real sin allocations ni locks (Reglas 9, 26, 39, 47)
 */
class CpuProfiler {
public:
    CpuProfiler() = default;

    void prepare(double sampleRate, uint32_t maxBlockSize) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 44100.0;
        maxBlockSize_ = (maxBlockSize > 0) ? maxBlockSize : 512;
        budgetUs_ = static_cast<float>((static_cast<double>(maxBlockSize_) / sampleRate_) * 1000000.0);
        reset();
    }

    void reset() noexcept {
        smoothedCpuPercent_ = 0.0f;
        peakCpuPercent_ = 0.0f;
        smoothedTotalDspUs_ = 0.0f;
        overloadFlag_.store(false, std::memory_order_relaxed);

        for (size_t i = 0; i < static_cast<size_t>(ProfilerStage::NumStages); ++i) {
            stageDurationsUs_[i] = 0.0f;
        }

        publishMetrics(0.0f, 0.0f, 0.0f, 0, 0, 0);
    }

    // Inicia el cronómetro del bloque completo (Audio Thread)
    void startBlock() noexcept {
        blockStartTime_ = std::chrono::steady_clock::now();
    }

    // Inicia el cronómetro de una etapa específica (Audio Thread)
    void startStage(ProfilerStage stage) noexcept {
        const size_t idx = static_cast<size_t>(stage);
        if (idx < static_cast<size_t>(ProfilerStage::NumStages)) {
            stageStartTimes_[idx] = std::chrono::steady_clock::now();
        }
    }

    // Finaliza el cronómetro de una etapa específica (Audio Thread)
    void endStage(ProfilerStage stage) noexcept {
        const auto now = std::chrono::steady_clock::now();
        const size_t idx = static_cast<size_t>(stage);
        if (idx < static_cast<size_t>(ProfilerStage::NumStages)) {
            const auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(now - stageStartTimes_[idx]).count();
            stageDurationsUs_[idx] = static_cast<float>(elapsedNs) * 0.001f;
        }
    }

    // Finaliza el bloque y publica métricas de forma atómica y lock-free (Audio Thread)
    void endBlock(uint32_t numSamples, uint32_t activeEvents, uint32_t activeGrains, uint32_t totalNodes) noexcept {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(now - blockStartTime_).count();
        const float totalUs = static_cast<float>(elapsedNs) * 0.001f;

        // Presupuesto real disponible para este bloque en microsegundos
        const float actualSamples = (numSamples > 0) ? static_cast<float>(numSamples) : static_cast<float>(maxBlockSize_);
        const float budgetUs = (actualSamples / static_cast<float>(sampleRate_)) * 1000000.0f;

        // Cálculo de % de CPU instantáneo
        const float rawCpuPercent = (budgetUs > 0.0f) ? ((totalUs / budgetUs) * 100.0f) : 0.0f;

        // Suavizado exponencial para display estable (filtro 1 polo a ~10 Hz)
        smoothedCpuPercent_ = (smoothedCpuPercent_ * 0.85f) + (rawCpuPercent * 0.15f);
        smoothedTotalDspUs_ = (smoothedTotalDspUs_ * 0.85f) + (totalUs * 0.15f);

        // Seguimiento de pico con decaimiento suave (decay rate 0.996 por bloque)
        if (rawCpuPercent > peakCpuPercent_) {
            peakCpuPercent_ = rawCpuPercent;
        } else {
            peakCpuPercent_ = std::max(smoothedCpuPercent_, peakCpuPercent_ * 0.996f);
        }

        // Detección de sobrecarga / xrun
        if (totalUs > budgetUs) {
            overloadFlag_.store(true, std::memory_order_relaxed);
        }

        publishMetrics(smoothedCpuPercent_, peakCpuPercent_, smoothedTotalDspUs_, activeEvents, activeGrains, totalNodes);
    }

    // Consulta de métricas para el GUI Thread (Lock-Free)
    PerformanceMetrics getLatestMetrics() const noexcept {
        PerformanceMetrics m;
        m.currentCpuPercent = atomicCpuPercent_.load(std::memory_order_relaxed);
        m.peakCpuPercent = atomicPeakCpuPercent_.load(std::memory_order_relaxed);
        m.totalDspUs = atomicTotalDspUs_.load(std::memory_order_relaxed);
        m.bufferBudgetUs = budgetUs_;
        m.analysisUs = atomicAnalysisUs_.load(std::memory_order_relaxed);
        m.eventEngineUs = atomicEventsUs_.load(std::memory_order_relaxed);
        m.graphExecutionUs = atomicGraphUs_.load(std::memory_order_relaxed);
        m.activeEvents = atomicActiveEvents_.load(std::memory_order_relaxed);
        m.activeGrains = atomicActiveGrains_.load(std::memory_order_relaxed);
        m.totalNodes = atomicTotalNodes_.load(std::memory_order_relaxed);
        m.overloadDetected = overloadFlag_.load(std::memory_order_relaxed);
        return m;
    }

    void resetOverload() noexcept {
        overloadFlag_.store(false, std::memory_order_relaxed);
    }

private:
    void publishMetrics(float cpu, float peak, float totalUs, uint32_t events, uint32_t grains, uint32_t nodes) noexcept {
        atomicCpuPercent_.store(cpu, std::memory_order_relaxed);
        atomicPeakCpuPercent_.store(peak, std::memory_order_relaxed);
        atomicTotalDspUs_.store(totalUs, std::memory_order_relaxed);
        atomicActiveEvents_.store(events, std::memory_order_relaxed);
        atomicActiveGrains_.store(grains, std::memory_order_relaxed);
        atomicTotalNodes_.store(nodes, std::memory_order_relaxed);

        atomicAnalysisUs_.store(stageDurationsUs_[static_cast<size_t>(ProfilerStage::Analysis)], std::memory_order_relaxed);
        atomicEventsUs_.store(stageDurationsUs_[static_cast<size_t>(ProfilerStage::Events)], std::memory_order_relaxed);
        atomicGraphUs_.store(stageDurationsUs_[static_cast<size_t>(ProfilerStage::Graph)], std::memory_order_relaxed);
    }

    double sampleRate_{ 44100.0 };
    uint32_t maxBlockSize_{ 512 };
    float budgetUs_{ 11610.0f };

    float smoothedCpuPercent_{ 0.0f };
    float peakCpuPercent_{ 0.0f };
    float smoothedTotalDspUs_{ 0.0f };

    std::chrono::steady_clock::time_point blockStartTime_{};
    std::array<std::chrono::steady_clock::time_point, static_cast<size_t>(ProfilerStage::NumStages)> stageStartTimes_{};
    std::array<float, static_cast<size_t>(ProfilerStage::NumStages)> stageDurationsUs_{};

    // Publicación atómica para el GUI Thread (Regla 26)
    std::atomic<float> atomicCpuPercent_{ 0.0f };
    std::atomic<float> atomicPeakCpuPercent_{ 0.0f };
    std::atomic<float> atomicTotalDspUs_{ 0.0f };
    std::atomic<float> atomicAnalysisUs_{ 0.0f };
    std::atomic<float> atomicEventsUs_{ 0.0f };
    std::atomic<float> atomicGraphUs_{ 0.0f };
    std::atomic<uint32_t> atomicActiveEvents_{ 0 };
    std::atomic<uint32_t> atomicActiveGrains_{ 0 };
    std::atomic<uint32_t> atomicTotalNodes_{ 0 };
    std::atomic<bool> overloadFlag_{ false };
};

} // namespace audio_graph
