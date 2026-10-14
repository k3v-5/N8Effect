#pragma once

#include <array>
#include <cmath>
#include <algorithm>
#include "ModulationTypes.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Generador de Envolvente Multi-Segmento (MSEG - Multi-Segment Envelope Generator)
 * con curvas Bézier/tensión configurables y sincronización rítmica (Reglas 7, 8, 25, 37, 46, 47)
 * Delega la evaluación de curvas y trayectoria al núcleo Rust.
 */
class MSEGModulator {
public:
    enum class PlayMode : uint8_t {
        Loop = 0,
        OneShot = 1,
        PingPong = 2
    };

    struct MSEGPoint {
        float time{ 0.0f };  // Tiempo normalizado (0.0 a 1.0)
        float level{ 0.0f }; // Nivel de amplitud (-1.0 a +1.0 o 0.0 a 1.0)
        float curve{ 0.0f }; // Tensión de la curva (-1.0 cóncava, 0.0 lineal, +1.0 convexa)
    };

    static constexpr size_t MaxPoints = 16;

    explicit MSEGModulator(RustMSEGModulator* existing = nullptr, bool owns = true)
        : rustCore_(existing ? existing : n8_rust_mseg_modulator_create()), ownsCore_(owns)
    {
    }

    ~MSEGModulator() {
        if (rustCore_ && ownsCore_) {
            n8_rust_mseg_modulator_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    MSEGModulator(const MSEGModulator&) = delete;
    MSEGModulator& operator=(const MSEGModulator&) = delete;

    MSEGModulator(MSEGModulator&& other) noexcept
        : rustCore_(other.rustCore_), ownsCore_(other.ownsCore_)
    {
        other.rustCore_ = nullptr;
        other.ownsCore_ = false;
    }

    MSEGModulator& operator=(MSEGModulator&& other) noexcept {
        if (this != &other) {
            if (rustCore_ && ownsCore_) {
                n8_rust_mseg_modulator_destroy(rustCore_);
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
            n8_rust_mseg_modulator_prepare(rustCore_, sampleRate);
        }
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_mseg_modulator_reset(rustCore_);
        }
    }

    void setDefaultEnvelope() noexcept {
        if (rustCore_) {
            n8_rust_mseg_modulator_set_default_envelope(rustCore_);
        }
    }

    void setPoint(size_t index, float time, float level, float curve) noexcept {
        if (rustCore_) {
            n8_rust_mseg_modulator_set_point(rustCore_, index, time, level, curve);
        }
    }

    void setNumPoints(size_t count) noexcept {
        if (rustCore_) {
            n8_rust_mseg_modulator_set_num_points(rustCore_, count);
        }
    }

    void setMode(PlayMode mode) noexcept {
        if (rustCore_) {
            n8_rust_mseg_modulator_set_mode(rustCore_, static_cast<uint8_t>(mode));
        }
    }

    void setSync(SyncDivision div, float freeHz = 1.0f) noexcept {
        if (rustCore_) {
            n8_rust_mseg_modulator_set_sync(rustCore_, static_cast<uint8_t>(div), freeHz);
        }
    }

    void trigger() noexcept {
        if (rustCore_) {
            n8_rust_mseg_modulator_trigger(rustCore_);
        }
    }

    void processSample(double bpm, double ppq, bool isPlaying) noexcept {
        if (rustCore_) {
            n8_rust_mseg_modulator_process_sample(rustCore_, bpm, ppq, isPlaying);
        }
    }

    float getCurrentValue() const noexcept {
        if (rustCore_) {
            return n8_rust_mseg_modulator_get_current_value(rustCore_);
        }
        return 0.0f;
    }

private:
    RustMSEGModulator* rustCore_{ nullptr };
    bool ownsCore_{ true };
};

} // namespace audio_graph
