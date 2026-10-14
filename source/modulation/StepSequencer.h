#pragma once

#include <array>
#include <algorithm>
#include <cstdint>
#include "ModulationTypes.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

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
 * Delega toda la lógica rítmica y probabilística al núcleo Rust de alto rendimiento.
 */
class StepSequencer {
public:
    static constexpr size_t MaxSteps = 32;

    explicit StepSequencer(RustStepSequencer* existing = nullptr, bool owns = true)
        : rustCore_(existing ? existing : n8_rust_step_sequencer_create()), ownsCore_(owns)
    {
    }

    ~StepSequencer() {
        if (rustCore_ && ownsCore_) {
            n8_rust_step_sequencer_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    StepSequencer(const StepSequencer&) = delete;
    StepSequencer& operator=(const StepSequencer&) = delete;

    StepSequencer(StepSequencer&& other) noexcept
        : rustCore_(other.rustCore_), ownsCore_(other.ownsCore_)
    {
        other.rustCore_ = nullptr;
        other.ownsCore_ = false;
    }

    StepSequencer& operator=(StepSequencer&& other) noexcept {
        if (this != &other) {
            if (rustCore_ && ownsCore_) {
                n8_rust_step_sequencer_destroy(rustCore_);
            }
            rustCore_ = other.rustCore_;
            ownsCore_ = other.ownsCore_;
            other.rustCore_ = nullptr;
            other.ownsCore_ = false;
        }
        return *this;
    }

    void prepare(double sampleRate) noexcept {
        if (rustCore_) {
            n8_rust_step_sequencer_prepare(rustCore_, sampleRate);
        }
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_step_sequencer_reset(rustCore_);
        }
    }

    void setNumSteps(uint32_t count) noexcept {
        if (rustCore_) {
            n8_rust_step_sequencer_set_num_steps(rustCore_, count);
        }
    }

    void setStep(uint32_t index, float value, float probability = 1.0f, float glide = 0.0f, bool active = true) noexcept {
        if (rustCore_) {
            n8_rust_step_sequencer_set_step(rustCore_, index, value, probability, glide, active);
        }
    }

    const SequencerStep& getStep(uint32_t index) const noexcept {
        if (rustCore_) {
            RustSequencerStep rStep;
            n8_rust_step_sequencer_get_step(rustCore_, index, &rStep);
            cachedStep_.value = rStep.value;
            cachedStep_.probability = rStep.probability;
            cachedStep_.glide = rStep.glide;
            cachedStep_.active = rStep.active;
        }
        return cachedStep_;
    }

    float processSample(double ppqPosition, bool isHostPlaying, SyncDivision /*stepRate*/ = SyncDivision::Sixteenth) noexcept {
        if (rustCore_) {
            return n8_rust_step_sequencer_process_sample(rustCore_, ppqPosition, isHostPlaying);
        }
        return 0.0f;
    }

    float getCurrentValue() const noexcept {
        if (rustCore_) {
            return n8_rust_step_sequencer_get_current_value(rustCore_);
        }
        return 0.0f;
    }

    uint32_t getCurrentStep() const noexcept {
        if (rustCore_) {
            return n8_rust_step_sequencer_get_current_step(rustCore_);
        }
        return 0;
    }

private:
    RustStepSequencer* rustCore_{ nullptr };
    bool ownsCore_{ true };
    mutable SequencerStep cachedStep_{};
};

} // namespace audio_graph
