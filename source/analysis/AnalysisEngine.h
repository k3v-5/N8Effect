#pragma once

#include <vector>
#include <algorithm>
#include <cstdint>
#include "../core/Types.h"
#include "TransientDetector.h"
#include "PitchTracker.h"
#include "SpectralFeatureExtractor.h"

namespace audio_graph {

/**
 * @brief Estructura de métricas instantáneas extraídas por el motor de análisis
 */
struct AnalysisSnapshot {
    float onsetStrength{ 0.0f };
    bool isTransient{ false };
    float pitchHz{ 440.0f };
    float pitchClarity{ 0.0f };
    float clarity{ 0.0f }; // Alias para compatibilidad universal
    float pitchMidi{ 69.0f };
    float pitchNormalized{ 0.5f };
    float spectralCentroid{ 0.2f };
    float spectralFlux{ 0.0f };
    float spectralFlatness{ 0.0f };
    float energy{ 0.0f };
};

/**
 * @brief Motor Soberano de Análisis en Tiempo Real (Reglas 1, 7, 9, 34, 46, 47).
 * Extrae características acústicas y perceptuales del audio entrante para auto-disparar eventos
 * y modular parámetros universales en tiempo real sin allocations de memoria.
 * Incluye Silence Gating (< -70 dBFS / 0.0003 RMS) para bypass completo de YIN y FFT en reposo.
 */
class AnalysisEngine {
public:
    AnalysisEngine() = default;

    void prepare(const ProcessSpec& spec) {
        spec_ = spec;
        monoBuffer_.assign(spec.maximumBlockSize, 0.0f);

        transientDetector_.prepare(spec.sampleRate);
        pitchTracker_.prepare(spec.sampleRate);
        spectralExtractor_.prepare(spec.sampleRate, 1024, 256);

        reset();
    }

    void reset() noexcept {
        std::fill(monoBuffer_.begin(), monoBuffer_.end(), 0.0f);
        transientDetector_.reset();
        pitchTracker_.reset();
        spectralExtractor_.reset();
        snapshot_ = AnalysisSnapshot{};
        wasSilent_ = true;
    }

    // Procesa un bloque de audio de entrada multicanal con suma mono y análisis desacoplado (Reglas 1 y 9)
    void process(const float* const* inputChannels, uint32_t numChannels, uint32_t numSamples) noexcept {
        if (inputChannels == nullptr || numChannels == 0 || numSamples == 0) {
            snapshot_.isTransient = false;
            snapshot_.onsetStrength = 0.0f;
            snapshot_.pitchClarity = 0.0f;
            snapshot_.clarity = 0.0f;
            snapshot_.energy = 0.0f;
            snapshot_.spectralFlux = 0.0f;
            snapshot_.spectralFlatness = 0.0f;
            wasSilent_ = true;
            return;
        }

        const uint32_t samplesToProcess = std::min(numSamples, static_cast<uint32_t>(monoBuffer_.size()));

        // 1. Suma mono para análisis acústico neutral y cálculo de energía RMS del bloque
        float sumSq = 0.0f;
        if (numChannels == 1 || inputChannels[1] == nullptr) {
            const float* in0 = inputChannels[0];
            for (uint32_t s = 0; s < samplesToProcess; ++s) {
                const float val = in0[s];
                monoBuffer_[s] = val;
                sumSq += val * val;
            }
        } else {
            const float* in0 = inputChannels[0];
            const float* in1 = inputChannels[1];
            for (uint32_t s = 0; s < samplesToProcess; ++s) {
                const float val = 0.5f * (in0[s] + in1[s]);
                monoBuffer_[s] = val;
                sumSq += val * val;
            }
        }

        const float rms = (samplesToProcess > 0)
            ? std::sqrt(sumSq / static_cast<float>(samplesToProcess))
            : 0.0f;
        const bool isSilent = (rms < 0.0003f); // Umbral de silencio < -70.45 dBFS (Regla 47)

        if (isSilent) {
            // Silence Gating: decaimiento suave en transientes y bypass completo de YIN y FFT
            transientDetector_.process(nullptr, 0);

            snapshot_.isTransient = false;
            snapshot_.onsetStrength = 0.0f;
            snapshot_.pitchClarity = 0.0f;
            snapshot_.clarity = 0.0f;
            snapshot_.energy = 0.0f;
            snapshot_.spectralFlux = 0.0f;
            snapshot_.spectralFlatness = 0.0f;
            // Preservar snapshot_.pitchHz, snapshot_.pitchMidi, snapshot_.pitchNormalized y snapshot_.spectralCentroid
            // para evitar saltos o discontinuidades audibles en parámetros modulados (Regla 35)
            wasSilent_ = true;
            return;
        }

        // Si el audio pasa de silencio a activo, reiniciar suavemente el detector para evitar falsos transitorios de borde
        if (wasSilent_) {
            transientDetector_.reset();
            wasSilent_ = false;
        }

        // 2. Ejecutar analizadores en audio activo
        transientDetector_.process(monoBuffer_.data(), samplesToProcess);
        pitchTracker_.process(monoBuffer_.data(), samplesToProcess);
        spectralExtractor_.process(monoBuffer_.data(), samplesToProcess);

        // 3. Actualizar snapshot atómico/thread-safe para modulación y GUI
        snapshot_.onsetStrength = transientDetector_.getOnsetStrength();
        snapshot_.isTransient = transientDetector_.isTransientDetected();

        snapshot_.pitchHz = pitchTracker_.getFundamentalHz();
        snapshot_.pitchClarity = pitchTracker_.getClarity();
        snapshot_.clarity = pitchTracker_.getClarity();
        snapshot_.pitchMidi = pitchTracker_.getMidiNote();
        snapshot_.pitchNormalized = pitchTracker_.getPitchNormalized();

        snapshot_.spectralCentroid = spectralExtractor_.getSpectralCentroid();
        snapshot_.spectralFlux = spectralExtractor_.getSpectralFlux();
        snapshot_.spectralFlatness = spectralExtractor_.getSpectralFlatness();
        snapshot_.energy = spectralExtractor_.getEnergy();
    }

    const AnalysisSnapshot& getSnapshot() const noexcept { return snapshot_; }

    TransientDetector& getTransientDetector() noexcept { return transientDetector_; }
    PitchTracker& getPitchTracker() noexcept { return pitchTracker_; }
    SpectralFeatureExtractor& getSpectralExtractor() noexcept { return spectralExtractor_; }

private:
    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    std::vector<float> monoBuffer_;

    TransientDetector transientDetector_;
    PitchTracker pitchTracker_;
    SpectralFeatureExtractor spectralExtractor_;

    AnalysisSnapshot snapshot_;
    bool wasSilent_{ true };
};

} // namespace audio_graph
