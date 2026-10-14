#pragma once

#include <vector>
#include <algorithm>
#include <cstdint>
#include "../core/Types.h"
#include "../dsp/rust_core/include/n8_rust_core.h"
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
 * Toda la coordinación, mezcla mono y algoritmos de análisis se ejecutan en el núcleo Rust.
 */
class AnalysisEngine {
public:
    AnalysisEngine() {
        rustCore_ = n8_rust_analysis_engine_create(44100.0, 512);
        rebindSubDetectors();
    }

    ~AnalysisEngine() {
        if (rustCore_) {
            n8_rust_analysis_engine_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    AnalysisEngine(const AnalysisEngine&) = delete;
    AnalysisEngine& operator=(const AnalysisEngine&) = delete;

    AnalysisEngine(AnalysisEngine&& other) noexcept
        : spec_(other.spec_)
        , snapshot_(other.snapshot_)
        , rustCore_(other.rustCore_)
    {
        other.rustCore_ = nullptr;
        rebindSubDetectors();
    }

    AnalysisEngine& operator=(AnalysisEngine&& other) noexcept {
        if (this != &other) {
            if (rustCore_) {
                n8_rust_analysis_engine_destroy(rustCore_);
            }
            spec_ = other.spec_;
            snapshot_ = other.snapshot_;
            rustCore_ = other.rustCore_;
            other.rustCore_ = nullptr;
            rebindSubDetectors();
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) noexcept {
        spec_ = spec;
        if (rustCore_) {
            n8_rust_analysis_engine_prepare(rustCore_, spec.sampleRate, spec.maximumBlockSize);
        }
        reset();
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_analysis_engine_reset(rustCore_);
        }
        snapshot_ = AnalysisSnapshot{};
    }

    void process(const float* const* inputChannels, uint32_t numChannels, uint32_t numSamples) noexcept {
        if (rustCore_) {
            n8_rust_analysis_engine_process(rustCore_, inputChannels, numChannels, numSamples);
            RustAnalysisSnapshot rustSnap;
            n8_rust_analysis_engine_get_snapshot(rustCore_, &rustSnap);
            snapshot_.onsetStrength = rustSnap.onset_strength;
            snapshot_.isTransient = rustSnap.is_transient;
            snapshot_.pitchHz = rustSnap.pitch_hz;
            snapshot_.pitchClarity = rustSnap.pitch_clarity;
            snapshot_.clarity = rustSnap.clarity;
            snapshot_.pitchMidi = rustSnap.pitch_midi;
            snapshot_.pitchNormalized = rustSnap.pitch_normalized;
            snapshot_.spectralCentroid = rustSnap.spectral_centroid;
            snapshot_.spectralFlux = rustSnap.spectral_flux;
            snapshot_.spectralFlatness = rustSnap.spectral_flatness;
            snapshot_.energy = rustSnap.energy;
        }
    }

    const AnalysisSnapshot& getSnapshot() const noexcept {
        return snapshot_;
    }

    TransientDetector& getTransientDetector() noexcept { return transientDetector_; }
    PitchTracker& getPitchTracker() noexcept { return pitchTracker_; }
    SpectralFeatureExtractor& getSpectralExtractor() noexcept { return spectralExtractor_; }

private:
    void rebindSubDetectors() noexcept {
        if (rustCore_) {
            transientDetector_ = TransientDetector(n8_rust_analysis_engine_get_transient_detector(rustCore_), false);
            pitchTracker_ = PitchTracker(n8_rust_analysis_engine_get_pitch_tracker(rustCore_), false);
            spectralExtractor_ = SpectralFeatureExtractor(n8_rust_analysis_engine_get_spectral_extractor(rustCore_), false);
        }
    }

    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    AnalysisSnapshot snapshot_;
    RustAnalysisEngine* rustCore_{ nullptr };

    TransientDetector transientDetector_{ nullptr, false };
    PitchTracker pitchTracker_{ nullptr, false };
    SpectralFeatureExtractor spectralExtractor_{ nullptr, false };
};

} // namespace audio_graph
