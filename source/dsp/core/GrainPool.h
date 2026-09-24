#pragma once

#include <cstddef>
#include <array>
#include <cmath>
#include <algorithm>
#include <numbers>

namespace audio_graph {

/**
 * @brief Estructura de grano individual para síntesis y procesamiento granular (Reglas 9, 10, 11).
 */
struct Grain {
    bool active = false;
    float sourceStartPos = 0.0f;
    float currentPlayhead = 0.0f;
    float playbackRate = 1.0f;
    int durationSamples = 0;
    int ageSamples = 0;
    float amplitude = 1.0f;
    float panL = 0.7071f;
    float panR = 0.7071f;

    void reset() {
        active = false;
        sourceStartPos = 0.0f;
        currentPlayhead = 0.0f;
        playbackRate = 1.0f;
        durationSamples = 0;
        ageSamples = 0;
        amplitude = 1.0f;
        panL = 0.7071f;
        panR = 0.7071f;
    }

    [[nodiscard]] inline float getWindowEnvelope() const noexcept {
        if (durationSamples <= 0) return 0.0f;
        const float norm = static_cast<float>(ageSamples) / static_cast<float>(durationSamples);
        // Ventana Hann simétrica
        return 0.5f * (1.0f - std::cos(2.0f * std::numbers::pi_v<float> * norm));
    }
};

/**
 * @brief Pool de capacidad fija de 128 granos prealocados (Reglas 9, 10, 11).
 * Garantiza cero allocations dinámicas y reciclaje determinista en el audio thread.
 */
class GrainPool {
public:
    static constexpr size_t MaxGrains = 128;

    GrainPool() noexcept {
        reset();
    }

    void reset() noexcept {
        freeCount_ = MaxGrains;
        activeCount_ = 0;
        for (size_t i = 0; i < MaxGrains; ++i) {
            grains_[i].reset();
            freeIndices_[i] = i;
        }
    }

    [[nodiscard]] Grain* acquire() noexcept {
        if (freeCount_ == 0) {
            return nullptr; // Límite de seguridad alcanzado (Regla 11)
        }
        --freeCount_;
        const size_t idx = freeIndices_[freeCount_];
        ++activeCount_;
        grains_[idx].reset();
        grains_[idx].active = true;
        return &grains_[idx];
    }

    void release(Grain* grain) noexcept {
        if (!grain || !grain->active) return;
        const ptrdiff_t diff = grain - grains_.data();
        if (diff < 0 || static_cast<size_t>(diff) >= MaxGrains) return;

        const size_t idx = static_cast<size_t>(diff);
        grain->active = false;
        if (freeCount_ < MaxGrains) {
            freeIndices_[freeCount_] = idx;
            ++freeCount_;
        }
        if (activeCount_ > 0) {
            --activeCount_;
        }
    }

    [[nodiscard]] size_t getActiveCount() const noexcept { return activeCount_; }
    [[nodiscard]] size_t getAvailableCount() const noexcept { return freeCount_; }

    Grain& getGrain(size_t index) noexcept { return grains_[index]; }
    const Grain& getGrain(size_t index) const noexcept { return grains_[index]; }

private:
    std::array<Grain, MaxGrains> grains_{};
    std::array<size_t, MaxGrains> freeIndices_{};
    size_t freeCount_{ MaxGrains };
    size_t activeCount_{ 0 };
};

} // namespace audio_graph
