#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include "../core/Types.h"
#include "../core/RealtimePools.h"
#include "../graph/GraphExecutor.h"
#include "../event/EventManager.h"
#include "../modulation/ModulationEngine.h"
#include "../analysis/AnalysisEngine.h"
#include "../core/CpuProfiler.h"

namespace audio_graph {

/**
 * @brief Motor Dual World: Dry World soberano vs Event/Wet World (Reglas 1, 4, 7, 9, 17, 34, 35)
 */
class DualWorldEngine {
public:
    DualWorldEngine() = default;

    void prepare(const ProcessSpec& spec) {
        spec_ = spec;
        dryBuffer_.prepare(spec.numOutputChannels, spec.maximumBlockSize);
        wetBuffer_.prepare(spec.numOutputChannels, spec.maximumBlockSize);
        eventBuffer_.prepare(spec.numOutputChannels, spec.maximumBlockSize);

        executor_.prepare(spec);
        eventManager_.prepare(spec.sampleRate, spec.maximumBlockSize);
        modulationEngine_.prepare(spec);
        analysisEngine_.prepare(spec);
        profiler_.prepare(spec.sampleRate, spec.maximumBlockSize);

        currentDryLevel_ = targetDryLevel_;
        currentWetLevel_ = targetWetLevel_;
    }

    void reset() {
        executor_.reset();
        eventManager_.reset();
        modulationEngine_.reset();
        analysisEngine_.reset();
        profiler_.reset();
    }

    void setDryLevel(float level) noexcept {
        targetDryLevel_ = std::clamp(level, 0.0f, 2.0f);
    }

    float getDryLevel() const noexcept {
        return targetDryLevel_;
    }

    void setWetLevel(float level) noexcept {
        targetWetLevel_ = std::clamp(level, 0.0f, 2.0f);
    }

    float getWetLevel() const noexcept {
        return targetWetLevel_;
    }

    GraphExecutor& getExecutor() noexcept {
        return executor_;
    }

    const GraphExecutor& getExecutor() const noexcept {
        return executor_;
    }

    EventManager& getEventManager() noexcept {
        return eventManager_;
    }

    ModulationEngine& getModulationEngine() noexcept {
        return modulationEngine_;
    }

    AnalysisEngine& getAnalysisEngine() noexcept {
        return analysisEngine_;
    }

    const AnalysisEngine& getAnalysisEngine() const noexcept {
        return analysisEngine_;
    }

    CpuProfiler& getCpuProfiler() noexcept {
        return profiler_;
    }

    const CpuProfiler& getCpuProfiler() const noexcept {
        return profiler_;
    }

    PerformanceMetrics getPerformanceMetrics() const noexcept {
        return profiler_.getLatestMetrics();
    }

    /**
     * @brief Procesa el bloque de audio dividiendo el flujo en Dry World, Analysis y Event World (Reglas 1, 2, 7, 17)
     */
    void process(const ExecutionPlan& plan, const ProcessContext& context, float* const* finalOutput, Graph* graph = nullptr) {
        profiler_.startBlock();
        DenormalDisabler denormalGuard; // Regla 9 y 34
        const uint32_t numSamples = context.numSamples;

        // 0. ANÁLISIS EN TIEMPO REAL (Reglas 1 y 7)
        profiler_.startStage(ProfilerStage::Analysis);
        analysisEngine_.process(context.inputChannels, context.numInputChannels, numSamples);
        const auto& snap = analysisEngine_.getSnapshot();
        modulationEngine_.updateAnalysisSources(snap.onsetStrength, snap.pitchNormalized, snap.spectralCentroid, snap.spectralFlux);
        profiler_.endStage(ProfilerStage::Analysis);

        // ACTUALIZAR MODULACIÓN UNIVERSAL (Regla 7)
        if (graph != nullptr) {
            modulationEngine_.process(context, *graph);
        }

        const uint32_t numChannels = std::min(context.numOutputChannels, spec_.numOutputChannels);

        // 1. DRY WORLD: Copia directa 100% pura e independiente de la entrada (Regla 1 y 17)
        dryBuffer_.copyFrom(context.inputChannels, context.numInputChannels, numSamples);

        // 2. EVENT WORLD:
        profiler_.startStage(ProfilerStage::Events);
        // A. Capturar audio entrante en el buffer de historia de eventos
        eventManager_.captureInputAudio(context.inputChannels, context.numInputChannels, numSamples);

        // B. Calcular nivel RMS de la fuente para Source Following (Regla 3)
        float sourceEnergy = 0.0f;
        if (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) {
            for (uint32_t s = 0; s < numSamples; ++s) {
                sourceEnergy += std::abs(context.inputChannels[0][s]);
            }
            sourceEnergy /= static_cast<float>(numSamples);
        }

        // C. Renderizar eventos activos en el eventBuffer
        eventBuffer_.clear(numSamples);
        if (numChannels >= 2) {
            eventManager_.render(eventBuffer_.getWritePointer(0), eventBuffer_.getWritePointer(1), numSamples, sourceEnergy);
        }
        profiler_.endStage(ProfilerStage::Events);

        // D. Si hay eventos activos, se envían al Grafo de Efectos; si no, pasa el audio directo al grafo
        ProcessContext eventContext = context;
        if (eventManager_.getActiveEventCount() > 0) {
            eventContext.inputChannels = eventBuffer_.getArrayOfReadPointers();
            eventContext.numInputChannels = eventBuffer_.getNumChannels();
        }

        // E. Ejecución del grafo de efectos en un buffer aislado sin tocar el Dry
        profiler_.startStage(ProfilerStage::Graph);
        executor_.process(plan, eventContext, wetBuffer_);
        profiler_.endStage(ProfilerStage::Graph);

        // 3. MASTER STAGE: Mezcla limpia de Dry + Wet con suavizado de volumen (Regla 35)
        const float alpha = 0.005f; // Suavizado anti-click
        for (uint32_t s = 0; s < numSamples; ++s) {
            currentDryLevel_ += alpha * (targetDryLevel_ - currentDryLevel_);
            currentWetLevel_ += alpha * (targetWetLevel_ - currentWetLevel_);

            for (uint32_t ch = 0; ch < numChannels; ++ch) {
                const float drySample = dryBuffer_.getReadPointer(ch)[s] * currentDryLevel_;
                const float wetSample = wetBuffer_.getReadPointer(ch)[s] * currentWetLevel_;
                
                // Mezcla hacia la salida final del plugin
                finalOutput[ch][s] = drySample + wetSample;
            }
        }

        const uint32_t activeEvents = static_cast<uint32_t>(eventManager_.getActiveEventCount());
        const uint32_t nodeCount = (graph != nullptr) ? static_cast<uint32_t>(graph->getNodes().size()) : 0;
        profiler_.endBlock(numSamples, activeEvents, 0, nodeCount);
    }

private:
    ProcessSpec spec_;
    PreallocatedBuffer dryBuffer_;
    PreallocatedBuffer wetBuffer_;
    PreallocatedBuffer eventBuffer_;
    GraphExecutor executor_;
    EventManager eventManager_;
    ModulationEngine modulationEngine_;
    AnalysisEngine analysisEngine_;
    CpuProfiler profiler_;

    float targetDryLevel_{ 1.0f }; // Por defecto 100% puro (Regla 1)
    float currentDryLevel_{ 1.0f };
    float targetWetLevel_{ 1.0f };
    float currentWetLevel_{ 1.0f };
};

} // namespace audio_graph
