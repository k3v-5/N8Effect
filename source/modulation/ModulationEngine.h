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
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Orquestador del Motor de Modulación Universal (Reglas 7, 8, 25, 37, 46)
 * Delega toda la computación y gestión de fuentes al núcleo Rust de alto rendimiento.
 */
class ModulationEngine {
public:
    ModulationEngine() {
        rustCore_ = n8_rust_modulation_engine_create();
        rebindSubModulators();
        sourceValues_.fill(0.0f);
    }

    ~ModulationEngine() {
        if (rustCore_) {
            n8_rust_modulation_engine_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    ModulationEngine(const ModulationEngine&) = delete;
    ModulationEngine& operator=(const ModulationEngine&) = delete;

    ModulationEngine(ModulationEngine&& other) noexcept
        : rustCore_(other.rustCore_)
        , sourceValues_(other.sourceValues_)
    {
        other.rustCore_ = nullptr;
        rebindSubModulators();
    }

    ModulationEngine& operator=(ModulationEngine&& other) noexcept {
        if (this != &other) {
            if (rustCore_) {
                n8_rust_modulation_engine_destroy(rustCore_);
            }
            rustCore_ = other.rustCore_;
            sourceValues_ = other.sourceValues_;
            other.rustCore_ = nullptr;
            rebindSubModulators();
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) noexcept {
        if (rustCore_) {
            n8_rust_modulation_engine_prepare(rustCore_, spec.sampleRate);
        }
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_modulation_engine_reset(rustCore_);
        }
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
        if (rustCore_) {
            return n8_rust_modulation_engine_get_source_value(rustCore_, static_cast<uint16_t>(type));
        }
        return 0.0f;
    }

    float calculateOffset(NodeId targetNode, ParameterId targetParam) const noexcept {
        if (rustCore_) {
            return n8_rust_modulation_engine_calculate_offset(rustCore_, targetNode, targetParam);
        }
        return 0.0f;
    }

    void updateAnalysisSources(float transient, float pitchNormalized, float centroid, float flux) noexcept {
        if (rustCore_) {
            n8_rust_modulation_engine_update_analysis_sources(rustCore_, transient, pitchNormalized, centroid, flux);
        }
    }

    // Actualiza todas las fuentes de modulación y despacha los valores modulados a los nodos del grafo
    void process(const ProcessContext& context, Graph& graph) noexcept {
        if (rustCore_) {
            n8_rust_modulation_engine_process_sources(
                rustCore_,
                context.bpm,
                context.ppqPosition,
                context.isPlaying,
                context.inputChannels,
                context.numInputChannels,
                context.numSamples
            );
        }

        // Aplicar modulación a todos los nodos registrados en el grafo
        for (const auto& [nodeId, instance] : graph.getNodes()) {
            if (!instance || !instance->processor) continue;

            auto* proc = instance->processor.get();
            for (const auto& paramInfo : proc->getParameters()) {
                float modOffset = 0.0f;
                if (rustCore_) {
                    modOffset = n8_rust_modulation_engine_calculate_offset(rustCore_, nodeId, paramInfo.id);
                }
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
    void rebindSubModulators() noexcept {
        if (rustCore_) {
            for (size_t i = 0; i < 4; ++i) {
                lfos_[i] = LFO(n8_rust_modulation_engine_get_lfo(rustCore_, i), false);
            }
            for (size_t i = 0; i < 2; ++i) {
                envelopes_[i] = EnvelopeGenerator(n8_rust_modulation_engine_get_envelope(rustCore_, i), false);
            }
            stepSeq_ = StepSequencer(n8_rust_modulation_engine_get_step_sequencer(rustCore_), false);
            audioFollower_ = AudioFollower(n8_rust_modulation_engine_get_audio_follower(rustCore_), false);
            macroManager_ = MacroManager(n8_rust_modulation_engine_get_macro_manager(rustCore_), false);
            matrix_ = ModulationMatrix(n8_rust_modulation_engine_get_matrix(rustCore_), false);
            for (size_t i = 0; i < 2; ++i) {
                msegs_[i] = MSEGModulator(n8_rust_modulation_engine_get_mseg(rustCore_, i), false);
            }
            for (size_t i = 0; i < 2; ++i) {
                euclideans_[i] = EuclideanModulator(n8_rust_modulation_engine_get_euclidean(rustCore_, i), false);
            }
            chaos_ = ChaosModulator(n8_rust_modulation_engine_get_chaos(rustCore_), false);
        }
    }

    RustModulationEngine* rustCore_{ nullptr };

    std::array<LFO, 4> lfos_{ LFO(nullptr, false), LFO(nullptr, false), LFO(nullptr, false), LFO(nullptr, false) };
    std::array<EnvelopeGenerator, 2> envelopes_{ EnvelopeGenerator(nullptr, false), EnvelopeGenerator(nullptr, false) };
    StepSequencer stepSeq_{ nullptr, false };
    AudioFollower audioFollower_{ nullptr, false };
    MacroManager macroManager_{ nullptr, false };
    ModulationMatrix matrix_{ nullptr, false };
    std::array<MSEGModulator, 2> msegs_{ MSEGModulator(nullptr, false), MSEGModulator(nullptr, false) };
    std::array<EuclideanModulator, 2> euclideans_{ EuclideanModulator(nullptr, false), EuclideanModulator(nullptr, false) };
    ChaosModulator chaos_{ nullptr, false };

    std::array<float, static_cast<size_t>(ModSourceType::Count)> sourceValues_{};
};

} // namespace audio_graph
