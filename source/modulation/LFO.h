#pragma once

#include <cmath>
#include <numbers>
#include <algorithm>
#include "ModulationTypes.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Oscilador de Baja Frecuencia (LFO) multi-forma sincronizable (Reglas 7, 28, 35, 37)
 * Delega toda la generación y sincronización al núcleo Rust de alto rendimiento.
 */
class LFO {
public:
    explicit LFO(RustLFO* existing = nullptr, bool owns = true)
        : rustCore_(existing ? existing : n8_rust_lfo_create()), ownsCore_(owns)
    {
    }

    ~LFO() {
        if (rustCore_ && ownsCore_) {
            n8_rust_lfo_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    LFO(const LFO&) = delete;
    LFO& operator=(const LFO&) = delete;

    LFO(LFO&& other) noexcept
        : rustCore_(other.rustCore_), ownsCore_(other.ownsCore_)
    {
        other.rustCore_ = nullptr;
        other.ownsCore_ = false;
    }

    LFO& operator=(LFO&& other) noexcept {
        if (this != &other) {
            if (rustCore_ && ownsCore_) {
                n8_rust_lfo_destroy(rustCore_);
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
            n8_rust_lfo_prepare(rustCore_, sampleRate);
        }
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_lfo_reset(rustCore_);
        }
    }

    void setShape(LFOShape shape) noexcept {
        if (rustCore_) {
            n8_rust_lfo_set_shape(rustCore_, static_cast<uint8_t>(shape));
        }
    }

    void setSyncDivision(SyncDivision div) noexcept {
        if (rustCore_) {
            n8_rust_lfo_set_sync_division(rustCore_, static_cast<uint8_t>(div));
        }
    }

    void setFrequencyHz(float freq) noexcept {
        if (rustCore_) {
            n8_rust_lfo_set_frequency_hz(rustCore_, freq);
        }
    }

    void setPulseWidth(float pw) noexcept {
        if (rustCore_) {
            n8_rust_lfo_set_pulse_width(rustCore_, pw);
        }
    }

    float processSample(double bpm, double ppqPosition, bool isHostPlaying) noexcept {
        if (rustCore_) {
            return n8_rust_lfo_process_sample(rustCore_, bpm, ppqPosition, isHostPlaying);
        }
        return 0.0f;
    }

    float getCurrentValue() const noexcept {
        if (rustCore_) {
            return n8_rust_lfo_get_current_value(rustCore_);
        }
        return 0.0f;
    }

private:
    RustLFO* rustCore_{ nullptr };
    bool ownsCore_{ true };
};

} // namespace audio_graph
