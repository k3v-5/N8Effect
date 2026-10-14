#pragma once

#include <cmath>
#include <algorithm>
#include "ModulationTypes.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Generador Polirrítmico Euclidiano de Bjorklund (Reglas 7, 8, 37, 46, 47)
 * Delega la distribución euclidiana O(1) de pulsos y la envolvente al núcleo Rust.
 */
class EuclideanModulator {
public:
    explicit EuclideanModulator(RustEuclideanModulator* existing = nullptr, bool owns = true)
        : rustCore_(existing ? existing : n8_rust_euclidean_modulator_create()), ownsCore_(owns)
    {
    }

    ~EuclideanModulator() {
        if (rustCore_ && ownsCore_) {
            n8_rust_euclidean_modulator_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    EuclideanModulator(const EuclideanModulator&) = delete;
    EuclideanModulator& operator=(const EuclideanModulator&) = delete;

    EuclideanModulator(EuclideanModulator&& other) noexcept
        : rustCore_(other.rustCore_), ownsCore_(other.ownsCore_)
    {
        other.rustCore_ = nullptr;
        other.ownsCore_ = false;
    }

    EuclideanModulator& operator=(EuclideanModulator&& other) noexcept {
        if (this != &other) {
            if (rustCore_ && ownsCore_) {
                n8_rust_euclidean_modulator_destroy(rustCore_);
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
            n8_rust_euclidean_modulator_prepare(rustCore_, sampleRate);
        }
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_euclidean_modulator_reset(rustCore_);
        }
    }

    void setSteps(int steps) noexcept {
        if (rustCore_) {
            n8_rust_euclidean_modulator_set_steps(rustCore_, steps);
        }
    }

    void setPulses(int pulses) noexcept {
        if (rustCore_) {
            n8_rust_euclidean_modulator_set_pulses(rustCore_, pulses);
        }
    }

    void setRotation(int rotation) noexcept {
        if (rustCore_) {
            n8_rust_euclidean_modulator_set_rotation(rustCore_, rotation);
        }
    }

    void setDecay(float seconds) noexcept {
        if (rustCore_) {
            n8_rust_euclidean_modulator_set_decay(rustCore_, seconds);
        }
    }

    void setSync(SyncDivision div, float freeBpm = 120.0f) noexcept {
        if (rustCore_) {
            n8_rust_euclidean_modulator_set_sync(rustCore_, static_cast<uint8_t>(div), freeBpm);
        }
    }

    bool hasPulseAtStep(int step) const noexcept {
        if (rustCore_) {
            return n8_rust_euclidean_modulator_has_pulse_at_step(rustCore_, step);
        }
        return false;
    }

    void processSample(double bpm, double ppq, bool isPlaying) noexcept {
        if (rustCore_) {
            n8_rust_euclidean_modulator_process_sample(rustCore_, bpm, ppq, isPlaying);
        }
    }

    float getCurrentValue() const noexcept {
        if (rustCore_) {
            return n8_rust_euclidean_modulator_get_current_value(rustCore_);
        }
        return 0.0f;
    }

private:
    RustEuclideanModulator* rustCore_{ nullptr };
    bool ownsCore_{ true };
};

} // namespace audio_graph
