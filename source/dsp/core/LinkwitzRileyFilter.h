#pragma once

#include <cmath>
#include <algorithm>
#include <span>
#include "BiquadFilter.h"

namespace audio_graph {

/**
 * @brief Crossover Linkwitz-Riley LR4 (24 dB/octava) estéreo de 3 bandas (Reglas 32, 33, 34, 46).
 *
 * Utiliza pares de filtros Butterworth de 2º orden en cascada (Q = 0.70710678)
 * con compensación de fase allpass en la banda grave, garantizando una suma
 * de magnitud perfectamente plana (0 dB) a lo largo de todo el espectro.
 */
class LinkwitzRileyCrossover3Way {
public:
    LinkwitzRileyCrossover3Way() = default;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;
        reset();
        updateCoefficients();
    }

    void reset() noexcept {
        for (int ch = 0; ch < 2; ++ch) {
            lpLow1_[ch].reset();
            lpLow2_[ch].reset();
            hpRest1_[ch].reset();
            hpRest2_[ch].reset();
            apLow_[ch].reset();
            lpMid1_[ch].reset();
            lpMid2_[ch].reset();
            hpHigh1_[ch].reset();
            hpHigh2_[ch].reset();
        }
    }

    void setCrossoverFrequencies(float fLowMidHz, float fMidHighHz) noexcept {
        const float nyquist = static_cast<float>(sampleRate_ * 0.49);
        fLowMid_ = std::clamp(fLowMidHz, 20.0f, nyquist * 0.5f);
        fMidHigh_ = std::clamp(fMidHighHz, fLowMid_ * 1.2f, nyquist);
        updateCoefficients();
    }

    struct OutputSample {
        float low;
        float mid;
        float high;
    };

    /**
     * @brief Procesa una muestra individual para un canal (ch: 0 para L, 1 para R).
     */
    inline OutputSample processSample(int ch, float in) noexcept {
        const int c = (ch != 0) ? 1 : 0;

        // 1. Crossover Low-Mid en fLowMid
        const float low1 = lpLow1_[c].processSample(in);
        const float lowRaw = lpLow2_[c].processSample(low1);

        const float rest1 = hpRest1_[c].processSample(in);
        const float rest = hpRest2_[c].processSample(rest1);

        // Compensación de fase para la banda Low respecto a fMidHigh
        const float lowAligned = apLow_[c].processSample(lowRaw);

        // 2. Crossover Mid-High en fMidHigh
        const float mid1 = lpMid1_[c].processSample(rest);
        const float mid = lpMid2_[c].processSample(mid1);

        const float high1 = hpHigh1_[c].processSample(rest);
        const float high = hpHigh2_[c].processSample(high1);

        return { lowAligned, mid, high };
    }

    /**
     * @brief Procesa un bloque estéreo dividiéndolo en 3 bandas estéreo.
     */
    void processBlock(const float* const* inChannels,
                      float* const* lowChannels,
                      float* const* midChannels,
                      float* const* highChannels,
                      size_t numSamples) noexcept {
        for (size_t s = 0; s < numSamples; ++s) {
            for (int ch = 0; ch < 2; ++ch) {
                const float in = (inChannels && inChannels[ch]) ? inChannels[ch][s] : 0.0f;
                const auto out = processSample(ch, in);

                if (lowChannels && lowChannels[ch])   lowChannels[ch][s] = out.low;
                if (midChannels && midChannels[ch])   midChannels[ch][s] = out.mid;
                if (highChannels && highChannels[ch]) highChannels[ch][s] = out.high;
            }
        }
    }

private:
    void updateCoefficients() noexcept {
        constexpr float Q_Butterworth = 0.70710678f;

        for (int ch = 0; ch < 2; ++ch) {
            // Lowpass LR4 en fLowMid
            lpLow1_[ch].setCoefficients(BiquadFilter::Type::Lowpass, sampleRate_, fLowMid_, Q_Butterworth);
            lpLow2_[ch].setCoefficients(BiquadFilter::Type::Lowpass, sampleRate_, fLowMid_, Q_Butterworth);

            // Highpass LR4 en fLowMid
            hpRest1_[ch].setCoefficients(BiquadFilter::Type::Highpass, sampleRate_, fLowMid_, Q_Butterworth);
            hpRest2_[ch].setCoefficients(BiquadFilter::Type::Highpass, sampleRate_, fLowMid_, Q_Butterworth);

            // Allpass en fMidHigh para alinear Low
            apLow_[ch].setCoefficients(BiquadFilter::Type::Allpass, sampleRate_, fMidHigh_, Q_Butterworth);

            // Lowpass LR4 en fMidHigh (Banda Mid)
            lpMid1_[ch].setCoefficients(BiquadFilter::Type::Lowpass, sampleRate_, fMidHigh_, Q_Butterworth);
            lpMid2_[ch].setCoefficients(BiquadFilter::Type::Lowpass, sampleRate_, fMidHigh_, Q_Butterworth);

            // Highpass LR4 en fMidHigh (Banda High)
            hpHigh1_[ch].setCoefficients(BiquadFilter::Type::Highpass, sampleRate_, fMidHigh_, Q_Butterworth);
            hpHigh2_[ch].setCoefficients(BiquadFilter::Type::Highpass, sampleRate_, fMidHigh_, Q_Butterworth);
        }
    }

    double sampleRate_{ 44100.0 };
    float fLowMid_{ 250.0f };
    float fMidHigh_{ 2500.0f };

    // Filtros por canal [0 = L, 1 = R]
    BiquadFilter lpLow1_[2]{};
    BiquadFilter lpLow2_[2]{};
    BiquadFilter hpRest1_[2]{};
    BiquadFilter hpRest2_[2]{};
    BiquadFilter apLow_[2]{};
    BiquadFilter lpMid1_[2]{};
    BiquadFilter lpMid2_[2]{};
    BiquadFilter hpHigh1_[2]{};
    BiquadFilter hpHigh2_[2]{};
};

} // namespace audio_graph
