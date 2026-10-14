#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Seguidor de Tono / Pitch Tracker en Tiempo Real (Reglas 1, 7, 9, 34, 46, 47).
 * Utiliza el algoritmo YIN / Función de Diferencia Acumulativa Media Normalizada (CMNDF)
 * con aceleración SIMD AVX2, terminación temprana y refinamiento parabólico sub-sample
 * delegados al núcleo de Rust de alto rendimiento.
 */
class PitchTracker {
public:
    static constexpr size_t WindowSize = 2048;

    explicit PitchTracker(RustPitchTracker* existing = nullptr, bool owns = true)
        : rustCore_(existing ? existing : n8_rust_pitch_tracker_create(44100.0f)), ownsCore_(owns)
    {
    }

    ~PitchTracker() {
        if (rustCore_ && ownsCore_) {
            n8_rust_pitch_tracker_free(rustCore_);
            rustCore_ = nullptr;
        }
    }

    PitchTracker(const PitchTracker&) = delete;
    PitchTracker& operator=(const PitchTracker&) = delete;

    PitchTracker(PitchTracker&& other) noexcept
        : rustCore_(other.rustCore_), ownsCore_(other.ownsCore_)
    {
        other.rustCore_ = nullptr;
        other.ownsCore_ = false;
    }

    PitchTracker& operator=(PitchTracker&& other) noexcept {
        if (this != &other) {
            if (rustCore_ && ownsCore_) {
                n8_rust_pitch_tracker_free(rustCore_);
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
            n8_rust_pitch_tracker_prepare(rustCore_, static_cast<float>(sampleRate));
        }
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_pitch_tracker_reset(rustCore_);
        }
    }

    void process(const float* input, uint32_t numSamples) noexcept {
        if (rustCore_) {
            n8_rust_pitch_tracker_process(rustCore_, input, numSamples);
        }
    }

    float getFundamentalHz() const noexcept {
        if (rustCore_) {
            return n8_rust_pitch_tracker_get_fundamental_hz(rustCore_);
        }
        return 440.0f;
    }

    float getClarity() const noexcept {
        if (rustCore_) {
            return n8_rust_pitch_tracker_get_clarity(rustCore_);
        }
        return 0.0f;
    }

    float getMidiNote() const noexcept {
        if (rustCore_) {
            return n8_rust_pitch_tracker_get_midi_note(rustCore_);
        }
        return 69.0f;
    }

    float getPitchNormalized() const noexcept {
        if (rustCore_) {
            return n8_rust_pitch_tracker_get_pitch_normalized(rustCore_);
        }
        return 0.5f;
    }

private:
    RustPitchTracker* rustCore_{ nullptr };
    bool ownsCore_{ true };
};

} // namespace audio_graph
