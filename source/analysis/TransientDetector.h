#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Detector de Transitorios y Onsets en Tiempo Real (Reglas 1, 7, 9, 34, 46, 47).
 * Utiliza filtro de pre-énfasis de alta frecuencia, dos seguidores de envolvente desacoplados
 * (rápido y lento) y ventana refractaria delegados al núcleo Rust de alto rendimiento.
 */
class TransientDetector {
public:
    explicit TransientDetector(RustTransientDetector* existing = nullptr, bool owns = true)
        : rustCore_(existing ? existing : n8_rust_transient_detector_create(44100.0f)), ownsCore_(owns)
    {
    }

    ~TransientDetector() {
        if (rustCore_ && ownsCore_) {
            n8_rust_transient_detector_free(rustCore_);
            rustCore_ = nullptr;
        }
    }

    TransientDetector(const TransientDetector&) = delete;
    TransientDetector& operator=(const TransientDetector&) = delete;

    TransientDetector(TransientDetector&& other) noexcept
        : rustCore_(other.rustCore_), ownsCore_(other.ownsCore_)
    {
        other.rustCore_ = nullptr;
        other.ownsCore_ = false;
    }

    TransientDetector& operator=(TransientDetector&& other) noexcept {
        if (this != &other) {
            if (rustCore_ && ownsCore_) {
                n8_rust_transient_detector_free(rustCore_);
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
            n8_rust_transient_detector_prepare(rustCore_, static_cast<float>(sampleRate));
        }
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_transient_detector_reset(rustCore_);
        }
    }

    void setSensitivity(float sensitivity) noexcept {
        if (rustCore_) {
            n8_rust_transient_detector_set_sensitivity(rustCore_, sensitivity);
        }
    }

    float getSensitivity() const noexcept {
        if (rustCore_) {
            return n8_rust_transient_detector_get_sensitivity(rustCore_);
        }
        return 0.7f;
    }

    void process(const float* input, uint32_t numSamples) noexcept {
        if (rustCore_) {
            n8_rust_transient_detector_process(rustCore_, input, numSamples);
        }
    }

    bool isTransientDetected() const noexcept {
        if (rustCore_) {
            return n8_rust_transient_detector_is_transient_detected(rustCore_);
        }
        return false;
    }

    float getOnsetStrength() const noexcept {
        if (rustCore_) {
            return n8_rust_transient_detector_get_onset_strength(rustCore_);
        }
        return 0.0f;
    }

private:
    RustTransientDetector* rustCore_{ nullptr };
    bool ownsCore_{ true };
};

} // namespace audio_graph
