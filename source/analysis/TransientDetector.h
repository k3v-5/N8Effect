#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>

namespace audio_graph {

/**
 * @brief Detector de Transitorios y Onsets en Tiempo Real (Reglas 1, 7, 9, 34, 46, 47).
 * Utiliza filtro de pre-énfasis de alta frecuencia, dos seguidores de envolvente desacoplados
 * (rápido y lento) y ventana refractaria para evitar re-disparos erráticos.
 */
class TransientDetector {
public:
    TransientDetector() = default;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;

        // Coeficientes de ataque rápido (~2 ms) y relajación lenta (~40 ms)
        fastAlpha_ = 1.0f - std::exp(-1.0f / (0.002f * static_cast<float>(sampleRate_)));
        slowAlpha_ = 1.0f - std::exp(-1.0f / (0.040f * static_cast<float>(sampleRate_)));

        refractorySamplesLimit_ = static_cast<uint32_t>(sampleRate_ * 0.015); // 15 ms refractario
        reset();
    }

    void reset() noexcept {
        prevSample_ = 0.0f;
        fastEnv_ = 0.0f;
        slowEnv_ = 0.0f;
        refractoryCounter_ = 0;
        onsetStrength_ = 0.0f;
        isTransient_ = false;
    }

    void setSensitivity(float sensitivity) noexcept {
        // Sensibilidad: 0.0 (poco sensible, ratio alto) a 1.0 (muy sensible, ratio bajo)
        sensitivity_ = std::clamp(sensitivity, 0.0f, 1.0f);
        thresholdRatio_ = 1.5f + (1.0f - sensitivity_) * 3.5f; // Rango de ratio: 1.5 a 5.0
    }

    float getSensitivity() const noexcept { return sensitivity_; }

    // Procesa un bloque de muestras de entrada y detecta transitorios
    void process(const float* input, uint32_t numSamples) noexcept {
        if (input == nullptr || numSamples == 0) {
            isTransient_ = false;
            onsetStrength_ *= 0.95f;
            return;
        }

        isTransient_ = false;
        const float decayFactor = 1.0f - (1.0f / (0.050f * static_cast<float>(sampleRate_)));

        for (uint32_t s = 0; s < numSamples; ++s) {
            const float in = input[s];

            // 1. Filtro de pre-énfasis de alta frecuencia (diferenciador de primer orden)
            const float hp = in - 0.95f * prevSample_;
            prevSample_ = in;

            const float energy = hp * hp;

            // 2. Seguidores de envolvente rápida y lenta
            fastEnv_ += fastAlpha_ * (energy - fastEnv_);
            slowEnv_ += slowAlpha_ * (energy - slowEnv_);

            // 3. Ratio de energía transitoria
            const float ratio = fastEnv_ / (slowEnv_ + 1e-6f);

            if (refractoryCounter_ > 0) {
                --refractoryCounter_;
            } else if (ratio > thresholdRatio_ && energy > 1e-4f) {
                // Transitorio detectado
                isTransient_ = true;
                refractoryCounter_ = refractorySamplesLimit_;
                onsetStrength_ = std::clamp((ratio - 1.0f) / thresholdRatio_, 0.2f, 1.0f);
            }

            // Suavizado continuo de la señal de modulación
            onsetStrength_ = std::max(0.0f, onsetStrength_ * decayFactor);
        }
    }

    bool isTransientDetected() const noexcept { return isTransient_; }
    float getOnsetStrength() const noexcept { return onsetStrength_; }

private:
    double sampleRate_{ 44100.0 };
    float fastAlpha_{ 0.05f };
    float slowAlpha_{ 0.005f };
    float prevSample_{ 0.0f };
    float fastEnv_{ 0.0f };
    float slowEnv_{ 0.0f };

    float sensitivity_{ 0.7f };
    float thresholdRatio_{ 2.5f };

    uint32_t refractorySamplesLimit_{ 660 };
    uint32_t refractoryCounter_{ 0 };

    float onsetStrength_{ 0.0f };
    bool isTransient_{ false };
};

} // namespace audio_graph
