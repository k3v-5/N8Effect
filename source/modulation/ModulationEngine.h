#pragma once

#include <array>
#include "ModulationTypes.h"
#include "LFO.h"
#include "EnvelopeGenerator.h"
#include "StepSequencer.h"
#include "AudioFollower.h"
#include "MacroManager.h"
#include "ModulationMatrix.h"
#include "MSEGModulator.h"
#include "EuclideanModulator.h"
#include "ChaosModulator.h"
#include "../graph/Graph.h"

namespace audio_graph {

/**
 * @brief Orquestador del Motor de Modulación Universal (Reglas 7, 8, 25, 37, 46)
 */
class ModulationEngine {
public:
    ModulationEngine() {
        sourceValues_.fill(0.0f);
    }

    void prepare(const ProcessSpec& spec) noexcept {
        for (auto& lfo : lfos_) lfo.prepare(spec.sampleRate);
        for (auto& env : envelopes_) env.prepare(spec.sampleRate);
        stepSeq_.prepare(spec.sampleRate);
        audioFollower_.prepare(spec.sampleRate);
        for (auto& mseg : msegs_) mseg.prepare(spec.sampleRate);
        for (auto& euc : euclideans_) euc.prepare(spec.sampleRate);
        chaos_.prepare(spec.sampleRate);
    }

    void reset() noexcept {
        for (auto& lfo : lfos_) lfo.reset();
        for (auto& env : envelopes_) env.reset();
        stepSeq_.reset();
        audioFollower_.reset();
        macroManager_.reset();
        for (auto& mseg : msegs_) mseg.reset();
        for (auto& euc : euclideans_) euc.reset();
        chaos_.reset();
        sourceValues_.fill(0.0f);
    }

    LFO& getLFO(size_t index) noexcept { return lfos_[index < 4 ? index : 0]; }
    EnvelopeGenerator& getEnvelope(size_t index) noexcept { return envelopes_[index < 2 ? index : 0]; }
    StepSequencer& getStepSequencer() noexcept { return stepSeq_; }
    AudioFollower& getAudioFollower() noexcept { return audioFollower_; }
    MacroManager& getMacroManager() noexcept { return macroManager_; }
    ModulationMatrix& getMatrix() noexcept { return matrix_; }
    MSEGModulator& getMSEG(size_t index) noexcept { return msegs_[index < 2 ? index : 0]; }
    EuclideanModulator& getEuclidean(size_t index) noexcept { return euclideans_[index < 2 ? index : 0]; }
    ChaosModulator& getChaos() noexcept { return chaos_; }

    float getSourceValue(ModSourceType type) const noexcept {
        size_t idx = static_cast<size_t>(type);
        return (idx < sourceValues_.size()) ? sourceValues_[idx] : 0.0f;
    }

    void updateAnalysisSources(float transient, float pitchNormalized, float centroid, float flux) noexcept {
        sourceValues_[static_cast<size_t>(ModSourceType::AnalysisTransient)] = transient;
        sourceValues_[static_cast<size_t>(ModSourceType::AnalysisPitch)] = pitchNormalized;
        sourceValues_[static_cast<size_t>(ModSourceType::AnalysisCentroid)] = centroid;
        sourceValues_[static_cast<size_t>(ModSourceType::AnalysisFlux)] = flux;
    }

    // Actualiza todas las fuentes de modulación y despacha los valores modulados a los nodos del grafo
    void process(const ProcessContext& context, Graph& graph) noexcept {
        // 1. Procesar fuentes autónomas
        macroManager_.processSmoothing();

        // Audio Follower
        float followerVal = audioFollower_.processBlock(context.inputChannels, context.numInputChannels, context.numSamples);

        // LFOs (procesados para el inicio del bloque con sincronización rítmica)
        for (size_t i = 0; i < 4; ++i) {
            lfos_[i].processSample(context.bpm, context.ppqPosition, context.isPlaying);
        }

        // Envolventes
        for (size_t i = 0; i < 2; ++i) {
            envelopes_[i].processSample();
        }

        // Secuenciador de pasos
        stepSeq_.processSample(context.ppqPosition, context.isPlaying);

        // MSEGs (Multi-Segment Envelopes)
        for (size_t i = 0; i < 2; ++i) {
            msegs_[i].processSample(context.bpm, context.ppqPosition, context.isPlaying);
        }

        // Moduladores Euclidianos
        for (size_t i = 0; i < 2; ++i) {
            euclideans_[i].processSample(context.bpm, context.ppqPosition, context.isPlaying);
        }

        // Modulador Caótico de Lorenz
        chaos_.processSample();

        // 2. Almacenar valores actuales de cada fuente
        sourceValues_[static_cast<size_t>(ModSourceType::LFO1)] = lfos_[0].getCurrentValue();
        sourceValues_[static_cast<size_t>(ModSourceType::LFO2)] = lfos_[1].getCurrentValue();
        sourceValues_[static_cast<size_t>(ModSourceType::LFO3)] = lfos_[2].getCurrentValue();
        sourceValues_[static_cast<size_t>(ModSourceType::LFO4)] = lfos_[3].getCurrentValue();

        sourceValues_[static_cast<size_t>(ModSourceType::Envelope1)] = envelopes_[0].getCurrentValue();
        sourceValues_[static_cast<size_t>(ModSourceType::Envelope2)] = envelopes_[1].getCurrentValue();

        sourceValues_[static_cast<size_t>(ModSourceType::StepSeq)] = stepSeq_.getCurrentValue();
        sourceValues_[static_cast<size_t>(ModSourceType::AudioFollower)] = followerVal;

        sourceValues_[static_cast<size_t>(ModSourceType::MSEG1)] = msegs_[0].getCurrentValue();
        sourceValues_[static_cast<size_t>(ModSourceType::MSEG2)] = msegs_[1].getCurrentValue();

        sourceValues_[static_cast<size_t>(ModSourceType::Euclidean1)] = euclideans_[0].getCurrentValue();
        sourceValues_[static_cast<size_t>(ModSourceType::Euclidean2)] = euclideans_[1].getCurrentValue();

        sourceValues_[static_cast<size_t>(ModSourceType::ChaosX)] = chaos_.getNormalizedX();
        sourceValues_[static_cast<size_t>(ModSourceType::ChaosY)] = chaos_.getNormalizedY();
        sourceValues_[static_cast<size_t>(ModSourceType::ChaosZ)] = chaos_.getNormalizedZ();

        sourceValues_[static_cast<size_t>(ModSourceType::MacroTexture)] = macroManager_.getSmoothedMacro(MacroManager::Texture);
        sourceValues_[static_cast<size_t>(ModSourceType::MacroMotion)] = macroManager_.getSmoothedMacro(MacroManager::Motion);
        sourceValues_[static_cast<size_t>(ModSourceType::MacroSpace)] = macroManager_.getSmoothedMacro(MacroManager::Space);
        sourceValues_[static_cast<size_t>(ModSourceType::MacroColor)] = macroManager_.getSmoothedMacro(MacroManager::Color);
        sourceValues_[static_cast<size_t>(ModSourceType::MacroChaos)] = macroManager_.getSmoothedMacro(MacroManager::Chaos);
        sourceValues_[static_cast<size_t>(ModSourceType::MacroDensity)] = macroManager_.getSmoothedMacro(MacroManager::Density);
        sourceValues_[static_cast<size_t>(ModSourceType::MacroEnergy)] = macroManager_.getSmoothedMacro(MacroManager::Energy);
        sourceValues_[static_cast<size_t>(ModSourceType::MacroMorph)] = macroManager_.getSmoothedMacro(MacroManager::Morph);

        sourceValues_[static_cast<size_t>(ModSourceType::PadX)] = macroManager_.getSmoothedPadX();
        sourceValues_[static_cast<size_t>(ModSourceType::PadY)] = macroManager_.getSmoothedPadY();

        // 3. Aplicar modulación a todos los nodos registrados en el grafo
        for (const auto& [nodeId, instance] : graph.getNodes()) {
            if (!instance || !instance->processor) continue;

            auto* proc = instance->processor.get();
            for (const auto& paramInfo : proc->getParameters()) {
                float modOffset = matrix_.calculateModulationOffset(nodeId, paramInfo.id, sourceValues_);
                if (std::abs(modOffset) > 1e-5f) {
                    float currentVal = proc->getParameter(paramInfo.id);
                    float range = paramInfo.maxValue - paramInfo.minValue;
                    float modulatedVal = std::clamp(currentVal + modOffset * range, paramInfo.minValue, paramInfo.maxValue);
                    proc->setParameter(paramInfo.id, modulatedVal);
                }
            }

            // Aplicar secuenciador personal de automatización rítmica por efecto
            instance->sequencer.processBlock(context, proc);
        }
    }

private:
    std::array<LFO, 4> lfos_;
    std::array<EnvelopeGenerator, 2> envelopes_;
    StepSequencer stepSeq_;
    AudioFollower audioFollower_;
    MacroManager macroManager_;
    ModulationMatrix matrix_;
    std::array<MSEGModulator, 2> msegs_;
    std::array<EuclideanModulator, 2> euclideans_;
    ChaosModulator chaos_;

    std::array<float, static_cast<size_t>(ModSourceType::Count)> sourceValues_;
};

} // namespace audio_graph
