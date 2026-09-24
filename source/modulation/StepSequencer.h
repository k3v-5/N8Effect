#pragma once

#include <array>
#include <algorithm>
#include <cstdint>
#include "ModulationTypes.h"

namespace audio_graph {

/**
 * @brief Estructura de un paso individual del secuenciador (Regla 29)
 */
struct SequencerStep {
    float value{ 0.0f };        // [-1.0, +1.0]
    float probability{ 1.0f };  // [0.0, 1.0]
    float glide{ 0.0f };        // [0.0, 1.0]
    bool active{ true };
};

/**
 * @brief Secuenciador de hasta 32 pasos con probabilidad y glide (Reglas 7, 29, 37)
 */
class StepSequencer {
public:
    static constexpr size_t MaxSteps = 32;

    StepSequencer() {
        for (size_t i = 0; i < MaxSteps; ++i) {
            steps_[i] = {
                .value = static_cast<float>(i % 8) / 7.0f * 2.0f - 1.0f,
                .probability = 1.0f,
                .glide = 0.0f,
                .active = true
            };
        }
    }

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;
        currentStep_ = 0;
        currentOutput_ = 0.0f;
        targetOutput_ = 0.0f;
    }

    void reset() noexcept {
        currentStep_ = 0;
        currentOutput_ = 0.0f;
        targetOutput_ = 0.0f;
        lastPpqStep_ = -1;
    }

    void setNumSteps(uint32_t count) noexcept {
        numSteps_ = std::clamp<uint32_t>(count, 1, MaxSteps);
    }

    void setStep(uint32_t index, float value, float probability = 1.0f, float glide = 0.0f, bool active = true) noexcept {
        if (index < MaxSteps) {
            steps_[index] = {
                .value = std::clamp(value, -1.0f, 1.0f),
                .probability = std::clamp(probability, 0.0f, 1.0f),
                .glide = std::clamp(glide, 0.0f, 1.0f),
                .active = active
            };
        }
    }

    const SequencerStep& getStep(uint32_t index) const noexcept {
        return steps_[index < MaxSteps ? index : 0];
    }

    // Procesa una muestra con sincronización rítmica (Regla 37)
    float processSample(double ppqPosition, bool isHostPlaying, SyncDivision stepRate = SyncDivision::Sixteenth) noexcept {
        if (isHostPlaying) {
            // Calcular qué step corresponde según el PPQ actual
            const double beatsPerStep = 0.25; // 1/16 por defecto
            int64_t stepIdx = static_cast<int64_t>(std::floor(ppqPosition / beatsPerStep));

            if (stepIdx != lastPpqStep_) {
                lastPpqStep_ = stepIdx;
                advanceStep();
            }
        }

        // Aplicar interpolación Glide (Regla 29)
        const float glideCoeff = std::clamp(1.0f - steps_[currentStep_].glide * 0.98f, 0.001f, 1.0f);
        currentOutput_ += glideCoeff * (targetOutput_ - currentOutput_);

        return currentOutput_;
    }

    float getCurrentValue() const noexcept { return currentOutput_; }
    uint32_t getCurrentStep() const noexcept { return currentStep_; }

private:
    void advanceStep() noexcept {
        currentStep_ = (currentStep_ + 1) % numSteps_;
        const auto& step = steps_[currentStep_];

        if (step.active) {
            // Evaluar probabilidad usando PRNG determinista en tiempo real (Regla 9)
            rngState_ = rngState_ * 1664525u + 1013904223u;
            float roll = static_cast<float>(rngState_ & 0xFFFF) / 65535.0f;

            if (roll <= step.probability) {
                targetOutput_ = step.value;
            }
        }
    }

    double sampleRate_{ 44100.0 };
    uint32_t numSteps_{ 16 };
    uint32_t currentStep_{ 0 };
    int64_t lastPpqStep_{ -1 };

    std::array<SequencerStep, MaxSteps> steps_;
    float currentOutput_{ 0.0f };
    float targetOutput_{ 0.0f };
    uint32_t rngState_{ 987654321u };
};

} // namespace audio_graph
