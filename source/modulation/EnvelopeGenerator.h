#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Generador de Envolvente ADSR de alta precisión para modulación (Reglas 7 y 46)
 * Delega toda la computación al núcleo Rust de alto rendimiento.
 */
class EnvelopeGenerator {
public:
    enum class State : uint8_t {
        Idle = 0,
        Attack = 1,
        Decay = 2,
        Sustain = 3,
        Release = 4
    };

    explicit EnvelopeGenerator(RustEnvelopeGenerator* existing = nullptr, bool owns = true)
        : rustCore_(existing ? existing : n8_rust_envelope_create()), ownsCore_(owns)
    {
    }

    ~EnvelopeGenerator() {
        if (rustCore_ && ownsCore_) {
            n8_rust_envelope_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    EnvelopeGenerator(const EnvelopeGenerator&) = delete;
    EnvelopeGenerator& operator=(const EnvelopeGenerator&) = delete;

    EnvelopeGenerator(EnvelopeGenerator&& other) noexcept
        : rustCore_(other.rustCore_), ownsCore_(other.ownsCore_)
    {
        other.rustCore_ = nullptr;
        other.ownsCore_ = false;
    }

    EnvelopeGenerator& operator=(EnvelopeGenerator&& other) noexcept {
        if (this != &other) {
            if (rustCore_ && ownsCore_) {
                n8_rust_envelope_destroy(rustCore_);
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
            n8_rust_envelope_prepare(rustCore_, sampleRate);
        }
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_envelope_reset(rustCore_);
        }
    }

    void setAttackMs(float ms) noexcept {
        if (rustCore_) {
            n8_rust_envelope_set_attack_ms(rustCore_, ms);
        }
    }

    void setDecayMs(float ms) noexcept {
        if (rustCore_) {
            n8_rust_envelope_set_decay_ms(rustCore_, ms);
        }
    }

    void setSustainLevel(float lvl) noexcept {
        if (rustCore_) {
            n8_rust_envelope_set_sustain_level(rustCore_, lvl);
        }
    }

    void setReleaseMs(float ms) noexcept {
        if (rustCore_) {
            n8_rust_envelope_set_release_ms(rustCore_, ms);
        }
    }

    void triggerGate(bool gateOn) noexcept {
        if (rustCore_) {
            n8_rust_envelope_trigger_gate(rustCore_, gateOn);
        }
    }

    float processSample() noexcept {
        if (rustCore_) {
            return n8_rust_envelope_process_sample(rustCore_);
        }
        return 0.0f;
    }

    float getCurrentValue() const noexcept {
        if (rustCore_) {
            return n8_rust_envelope_get_current_value(rustCore_);
        }
        return 0.0f;
    }

    State getState() const noexcept {
        if (rustCore_) {
            return static_cast<State>(n8_rust_envelope_get_state(rustCore_));
        }
        return State::Idle;
    }

    bool isActive() const noexcept {
        if (rustCore_) {
            return n8_rust_envelope_is_active(rustCore_);
        }
        return false;
    }

private:
    RustEnvelopeGenerator* rustCore_{ nullptr };
    bool ownsCore_{ true };
};

} // namespace audio_graph
