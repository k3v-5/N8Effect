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
        float energyL = 0.0f;
        float energyR = 0.0f;
        if (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) {
            for (uint32_t s = 0; s < numSamples; ++s) {
                const float sL = std::abs(context.inputChannels[0][s]);
                const float sR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr)
                    ? std::abs(context.inputChannels[1][s]) : sL;
                energyL += sL;
                energyR += sR;
            }
            sourceEnergy = (energyL + energyR) / (static_cast<float>(numSamples) * 2.0f);
        }

        // C. Renderizar eventos activos en el eventBuffer sumados al audio entrante (Reglas 1, 2, 17)
        eventBuffer_.copyFrom(context.inputChannels, context.numInputChannels, numSamples);
        if (numChannels >= 2) {
            eventManager_.render(eventBuffer_.getWritePointer(0), eventBuffer_.getWritePointer(1), numSamples, sourceEnergy);
        }
        profiler_.endStage(ProfilerStage::Events);

        // D. Telemetría reactiva en tiempo real para el Radar 3D (Reglas 9, 23, 26)
        if (snap.onsetStrength > 0.15f || sourceEnergy > 0.003f) {
            const float tot = energyL + energyR;
            const float pan = (tot > 1e-5f) ? std::clamp((energyR - energyL) / tot, -1.0f, 1.0f) : 0.0f;
            const float pitchRatio = (snap.pitchNormalized > 0.01f)
                ? std::clamp(std::pow(2.0f, (snap.pitchNormalized - 0.5f) * 2.5f), 0.25f, 4.0f)
                : 1.0f;
            const float dist = std::clamp(10.0f * (1.0f - snap.spectralCentroid * 0.7f) / (sourceEnergy + 0.2f), 0.5f, 10.0f);

            EventTelemetryItem tItem;
            tItem.pan = pan;
            tItem.pitchRatio = pitchRatio;
            tItem.energy = std::clamp(snap.onsetStrength * 1.2f + sourceEnergy * 3.0f, 0.2f, 1.0f);
            tItem.distance = dist;
            tItem.azimuth = pan * 90.0f;
            tItem.type = (snap.onsetStrength > 0.35f) ? EventType::Transient : EventType::Fragment;
            tItem.generation = 0;
            tItem.isAlive = true;
            eventManager_.getTelemetryBuffer().push(tItem);
        }

        // E. El Grafo de Efectos recibe la suma de audio entrante + eventos en eventBuffer
        ProcessContext eventContext = context;
        eventContext.inputChannels = eventBuffer_.getArrayOfReadPointers();
        eventContext.numInputChannels = eventBuffer_.getNumChannels();

        // F. Ejecución del grafo de efectos en un buffer aislado sin tocar el Dry
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
