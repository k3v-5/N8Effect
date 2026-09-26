#pragma once

#include <cmath>
#include <array>
#include <algorithm>
#include "BiquadFilter.h"
#include "DenormalGuards.h"
#include "FastMath.h"

namespace audio_graph {

/**
 * @brief Resonador físico de cuerpo acústico con 4 modos resonantes (Reglas 5, 32, 34, 46, 47).
 * Emula la respuesta acústica de caja de resonancia en instrumentos de madera (guitarras, laúdes, cellos).
 * Modos físicos implementados:
 * 1. Cavidad de aire Helmholtz (A0 ~ 100 Hz): Profundidad y aire.
 * 2. Tapa armónica de madera (T1 ~ 220 Hz): Flexión principal de la tapa frontal.
 * 3. Fondo de resonancia (B1 ~ 450 Hz): Proyección de medios.
 * 4. Puente y acoplamiento (P1 ~ 1200 Hz): Definición de ataque y brillo de la madera.
 */
class AcousticBodyResonator {
public:
    AcousticBodyResonator() = default;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;
        reset();
        updateModes();
    }

    void reset() noexcept {
        for (auto& ch : modes_) {
            for (auto& filter : ch) {
                filter.reset();
            }
        }
    }

    /**
     * @brief Configura las propiedades físicas del cuerpo acústico.
     * @param bodySize Escala de tamaño del cuerpo (0.5 = pequeño/ukelele, 1.0 = estándar/guitarra, 2.0 = grande/cello)
     * @param bodyDecay Factor de resonancia/Q de la madera (0.1 a 3.0)
     * @param bodyMix Nivel de mezcla de la resonancia de cuerpo (0.0 = bypass, 1.0 = 100% cuerpo resonante)
     */
    void setParameters(float bodySize, float bodyDecay, float bodyMix) noexcept {
        const float newSize = std::clamp(bodySize, 0.5f, 2.0f);
        const float newDecay = std::clamp(bodyDecay, 0.1f, 3.0f);
        const float newMix = std::clamp(bodyMix, 0.0f, 1.0f);

        if (std::abs(newSize - bodySize_) > 1e-4f || std::abs(newDecay - bodyDecay_) > 1e-4f) {
            bodySize_ = newSize;
            bodyDecay_ = newDecay;
            updateModes();
        }
        bodyMix_ = newMix;
    }

    void processSample(float inL, float inR, float& outL, float& outR) noexcept {
        if (bodyMix_ <= 0.0f) {
            outL = inL;
            outR = inR;
            return;
        }

        // Excitación distribuida en los 4 modos acústicos
        // Pesos modales organológicos calibrados:
        // Helmholtz (0.35), Top Plate (0.30), Back Plate (0.25), Bridge (0.18)
        float modeSumL = 0.35f * modes_[0][0].processSample(inL)
                       + 0.30f * modes_[0][1].processSample(inL)
                       + 0.25f * modes_[0][2].processSample(inL)
                       + 0.18f * modes_[0][3].processSample(inL);

        float modeSumR = 0.35f * modes_[1][0].processSample(inR)
                       + 0.30f * modes_[1][1].processSample(inR)
                       + 0.25f * modes_[1][2].processSample(inR)
                       + 0.18f * modes_[1][3].processSample(inR);

        outL = (1.0f - bodyMix_) * inL + bodyMix_ * modeSumL;
        outR = (1.0f - bodyMix_) * inR + bodyMix_ * modeSumR;
    }

    void processBlock(const float* inL, const float* inR, float* outL, float* outR, size_t numSamples) noexcept {
        if (!outL || numSamples == 0) return;

        for (size_t s = 0; s < numSamples; ++s) {
            const float sL = inL ? inL[s] : 0.0f;
            const float sR = inR ? inR[s] : sL;
            float rL = 0.0f, rR = 0.0f;
            processSample(sL, sR, rL, rR);
            outL[s] = rL;
            if (outR) outR[s] = rR;
        }
    }

    float getBodySize() const noexcept { return bodySize_; }
    float getBodyDecay() const noexcept { return bodyDecay_; }
    float getBodyMix() const noexcept { return bodyMix_; }

private:
    void updateModes() noexcept {
        if (sampleRate_ <= 0.0) return;

        // Frecuencias base en Hz a tamaño 1.0 (Guitarra clásica/acústica)
        constexpr std::array<float, 4> baseFreqs{ 104.0f, 220.0f, 460.0f, 1250.0f };
        // Factores Q base correspondientes a la absorción de la madera
        constexpr std::array<float, 4> baseQ{ 4.5f, 6.0f, 5.5f, 7.0f };

        const float nyquist = static_cast<float>(sampleRate_ * 0.49);

        for (size_t m = 0; m < 4; ++m) {
            // Escalar frecuencia inversamente proporcional al tamaño del cuerpo (F = F0 / Size)
            const float freq = std::clamp(baseFreqs[m] / bodySize_, 20.0f, nyquist);
            const float q = std::clamp(baseQ[m] * bodyDecay_, 0.5f, 30.0f);

            // Biquad Bandpass resonante de segundo orden
            modes_[0][m].setCoefficients(BiquadFilter::Type::Bandpass, sampleRate_, freq, q);
            // Ligero decalaje estéreo para ensanchamiento binaural orgánico (+1.5% en R)
            const float freqR = std::clamp(freq * 1.015f, 20.0f, nyquist);
            modes_[1][m].setCoefficients(BiquadFilter::Type::Bandpass, sampleRate_, freqR, q);
        }
    }

    double sampleRate_{ 44100.0 };
    float bodySize_{ 1.0f };
    float bodyDecay_{ 1.0f };
    float bodyMix_{ 0.4f };

    // 2 canales estéreo, 4 modos resonantes por canal
    std::array<std::array<BiquadFilter, 4>, 2> modes_;
};

} // namespace audio_graph
