#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>
#include "FastMath.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Estimador Adaptativo de Piso de Ruido y Disparador Schmitt de Histéresis (Reglas 1, 3, 9, 27, 47).
 * Delegado al Rust Core n8_rust_core para procesamiento de alta precisión numérica, anti-denormal y cero alocaciones.
 */
class AdaptiveNoiseFloorEstimator {
public:
    static constexpr float MinNoiseFloor = 1.58489e-5f; // -96 dBFS
    static constexpr float MaxNoiseFloor = 0.01f;       // -40 dBFS
    static constexpr float DefaultFloor  = 1.0e-4f;      // -80 dBFS

    AdaptiveNoiseFloorEstimator() noexcept {
        rustCore_ = n8_rust_adaptive_noise_floor_estimator_create();
    }

    ~AdaptiveNoiseFloorEstimator() {
        if (rustCore_) {
            n8_rust_adaptive_noise_floor_estimator_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    AdaptiveNoiseFloorEstimator(const AdaptiveNoiseFloorEstimator&) = delete;
    AdaptiveNoiseFloorEstimator& operator=(const AdaptiveNoiseFloorEstimator&) = delete;

    AdaptiveNoiseFloorEstimator(AdaptiveNoiseFloorEstimator&& other) noexcept
        : rustCore_(other.rustCore_) {
        other.rustCore_ = nullptr;
    }

    AdaptiveNoiseFloorEstimator& operator=(AdaptiveNoiseFloorEstimator&& other) noexcept {
        if (this != &other) {
            if (rustCore_) n8_rust_adaptive_noise_floor_estimator_destroy(rustCore_);
            rustCore_ = other.rustCore_;
            other.rustCore_ = nullptr;
        }
        return *this;
    }

    void prepare(double sampleRate) noexcept {
        if (rustCore_) {
            n8_rust_adaptive_noise_floor_estimator_prepare(rustCore_, sampleRate);
        }
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_adaptive_noise_floor_estimator_reset(rustCore_);
        }
    }

    void updateBlock(float blockRms, uint32_t numSamples = 128) noexcept {
        if (rustCore_) {
            n8_rust_adaptive_noise_floor_estimator_update_block(rustCore_, blockRms, numSamples);
        }
    }

    bool isSourceActive() const noexcept {
        if (rustCore_) return n8_rust_adaptive_noise_floor_estimator_is_source_active(rustCore_);
        return false;
    }

    bool isSourceSilent() const noexcept {
        if (rustCore_) return n8_rust_adaptive_noise_floor_estimator_is_source_silent(rustCore_);
        return true;
    }

    float getNoiseFloor() const noexcept {
        if (rustCore_) return n8_rust_adaptive_noise_floor_estimator_get_noise_floor(rustCore_);
        return DefaultFloor;
    }

    float getSmoothedLevel() const noexcept {
        if (rustCore_) return n8_rust_adaptive_noise_floor_estimator_get_smoothed_level(rustCore_);
        return 0.0f;
    }

    void setDebounceBlocks(uint32_t blocks) noexcept {
        if (rustCore_) {
            n8_rust_adaptive_noise_floor_estimator_set_debounce_blocks(rustCore_, blocks);
        }
    }

    RustAdaptiveNoiseFloorEstimator* getRustEstimator() noexcept { return rustCore_; }
    const RustAdaptiveNoiseFloorEstimator* getRustEstimator() const noexcept { return rustCore_; }

private:
    RustAdaptiveNoiseFloorEstimator* rustCore_{ nullptr };
};

} // namespace audio_graph
