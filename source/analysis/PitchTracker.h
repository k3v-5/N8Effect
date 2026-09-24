#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace audio_graph {

/**
 * @brief Seguidor de Tono / Pitch Tracker en Tiempo Real (Reglas 1, 7, 9, 34, 46, 47).
 * Utiliza el algoritmo YIN / Función de Diferencia Acumulativa Normalizada (CMDF)
 * con interpolación parabólica sub-sample para estimación continua y precisa de la frecuencia fundamental.
 */
class PitchTracker {
public:
    static constexpr size_t WindowSize = 2048; // Bounded ring buffer prealocado (Regla 47)

    PitchTracker() = default;

    void prepare(double sampleRate) {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;

        buffer_.assign(WindowSize * 2, 0.0f);
        yinBuffer_.assign(WindowSize, 0.0f);
        writeIndex_ = 0;
        bufferedSamples_ = 0;

        minLag_ = static_cast<size_t>(sampleRate_ / 2000.0); // 2000 Hz
        maxLag_ = std::min(WindowSize - 1, static_cast<size_t>(sampleRate_ / 40.0)); // 40 Hz
        if (minLag_ < 2) minLag_ = 2;

        reset();
    }

    void reset() noexcept {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        std::fill(yinBuffer_.begin(), yinBuffer_.end(), 0.0f);
        writeIndex_ = 0;
        bufferedSamples_ = 0;
        currentPitchHz_ = 440.0f;
        targetPitchHz_ = 440.0f;
        clarity_ = 0.0f;
    }

    void process(const float* input, uint32_t numSamples) noexcept {
        if (input == nullptr || numSamples == 0) return;

        // Escribir en el buffer circular
        for (uint32_t s = 0; s < numSamples; ++s) {
            buffer_[writeIndex_] = input[s];
            buffer_[writeIndex_ + WindowSize] = input[s]; // Buffer duplicado para lectura contigua sin saltos
            writeIndex_ = (writeIndex_ + 1) % WindowSize;
        }

        bufferedSamples_ += numSamples;
        // Analizar periódicamente (cada ~256 muestras para eficiencia de CPU, Regla 47)
        if (bufferedSamples_ >= 256) {
            bufferedSamples_ = 0;
            estimatePitch();
        }

        // Suavizado anti-click del tono estimado (Regla 35)
        const float alpha = 0.1f;
        currentPitchHz_ += alpha * (targetPitchHz_ - currentPitchHz_);
    }

    float getFundamentalHz() const noexcept { return currentPitchHz_; }
    float getClarity() const noexcept { return clarity_; }

    float getMidiNote() const noexcept {
        if (currentPitchHz_ <= 10.0f) return 0.0f;
        return 69.0f + 12.0f * std::log2(currentPitchHz_ / 440.0f);
    }

    // Retorna el tono normalizado de 0.0 (A0 = 27.5 Hz) a 1.0 (C8 = 4186 Hz) para modulación
    float getPitchNormalized() const noexcept {
        const float midi = getMidiNote();
        return std::clamp((midi - 21.0f) / (108.0f - 21.0f), 0.0f, 1.0f);
    }

private:
    void estimatePitch() noexcept {
        const float* window = buffer_.data() + writeIndex_;
        const size_t W = WindowSize / 2;

        // Paso 1: Función de Diferencia YIN: d(tau) = sum((x[j] - x[j+tau])^2)
        yinBuffer_[0] = 1.0f;
        for (size_t tau = minLag_; tau <= maxLag_; ++tau) {
            float diffSum = 0.0f;
            for (size_t j = 0; j < W; ++j) {
                const float delta = window[j] - window[j + tau];
                diffSum += delta * delta;
            }
            yinBuffer_[tau] = diffSum;
        }

        // Paso 2: Diferencia acumulativa media normalizada
        float runningSum = 0.0f;
        for (size_t tau = minLag_; tau <= maxLag_; ++tau) {
            runningSum += yinBuffer_[tau];
            if (runningSum > 1e-6f) {
                yinBuffer_[tau] *= static_cast<float>(tau) / runningSum;
            } else {
                yinBuffer_[tau] = 1.0f;
            }
        }

        // Paso 3: Detección del primer mínimo local por debajo del umbral YIN (0.15)
        const float threshold = 0.15f;
        size_t bestTau = 0;

        for (size_t tau = minLag_; tau <= maxLag_; ++tau) {
            if (yinBuffer_[tau] < threshold) {
                while (tau + 1 <= maxLag_ && yinBuffer_[tau + 1] < yinBuffer_[tau]) {
                    ++tau;
                }
                bestTau = tau;
                break;
            }
        }

        // Si no se encontró por debajo del umbral, buscar el mínimo global absoluto
        if (bestTau == 0) {
            float minVal = 100.0f;
            for (size_t tau = minLag_; tau <= maxLag_; ++tau) {
                if (yinBuffer_[tau] < minVal) {
                    minVal = yinBuffer_[tau];
                    bestTau = tau;
                }
            }
        }

        if (bestTau >= minLag_ && bestTau <= maxLag_) {
            // Paso 4: Interpolación parabólica sub-sample para máxima precisión (Regla 34)
            float refinedTau = static_cast<float>(bestTau);
            if (bestTau > minLag_ && bestTau < maxLag_) {
                const float y0 = yinBuffer_[bestTau - 1];
                const float y1 = yinBuffer_[bestTau];
                const float y2 = yinBuffer_[bestTau + 1];
                const float denom = 2.0f * (y0 - 2.0f * y1 + y2);
                if (std::abs(denom) > 1e-6f) {
                    refinedTau += (y0 - y2) / denom;
                }
            }

            if (refinedTau > 0.0f) {
                const float estimatedHz = static_cast<float>(sampleRate_) / refinedTau;
                const float currentVal = yinBuffer_[bestTau];
                clarity_ = std::clamp(1.0f - currentVal, 0.0f, 1.0f);

                if (clarity_ > 0.25f && estimatedHz >= 40.0f && estimatedHz <= 2200.0f) {
                    targetPitchHz_ = estimatedHz;
                }
            }
        }
    }

    double sampleRate_{ 44100.0 };
    std::vector<float> buffer_;
    std::vector<float> yinBuffer_;
    size_t writeIndex_{ 0 };
    uint32_t bufferedSamples_{ 0 };

    size_t minLag_{ 22 };
    size_t maxLag_{ 1102 };

    float currentPitchHz_{ 440.0f };
    float targetPitchHz_{ 440.0f };
    float clarity_{ 0.0f };
};

} // namespace audio_graph
