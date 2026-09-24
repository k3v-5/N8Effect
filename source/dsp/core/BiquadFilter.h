#pragma once

#include <cmath>
#include <numbers>
#include <algorithm>

namespace audio_graph {

/**
 * @brief Filtro IIR Biquad Direct Form II Transposed reutilizable (Reglas 32, 34, 38, 46)
 */
class BiquadFilter {
public:
    enum class Type {
        Lowpass,
        Highpass,
        Bandpass,
        Notch,
        Peak,
        LowShelf,
        HighShelf,
        Allpass
    };

    BiquadFilter() = default;

    void reset() noexcept {
        z1_ = 0.0f;
        z2_ = 0.0f;
    }

    void setCoefficients(Type type, double sampleRate, float frequencyHz, float q, float gainDb = 0.0f) noexcept {
        const double sr = (sampleRate > 0.0) ? sampleRate : 44100.0;
        const double nyquist = sr * 0.499;
        const double fc = std::clamp(static_cast<double>(frequencyHz), 10.0, nyquist);
        const double Q = std::max(0.01, static_cast<double>(q));
        const double A = std::pow(10.0, static_cast<double>(gainDb) / 40.0);
        const double omega = 2.0 * std::numbers::pi_v<double> * fc / sr;
        const double sn = std::sin(omega);
        const double cs = std::cos(omega);
        const double alpha = sn / (2.0 * Q);

        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a0 = 1.0, a1 = 0.0, a2 = 0.0;

        switch (type) {
            case Type::Lowpass:
                b0 = (1.0 - cs) * 0.5;
                b1 = 1.0 - cs;
                b2 = (1.0 - cs) * 0.5;
                a0 = 1.0 + alpha;
                a1 = -2.0 * cs;
                a2 = 1.0 - alpha;
                break;

            case Type::Highpass:
                b0 = (1.0 + cs) * 0.5;
                b1 = -(1.0 + cs);
                b2 = (1.0 + cs) * 0.5;
                a0 = 1.0 + alpha;
                a1 = -2.0 * cs;
                a2 = 1.0 - alpha;
                break;

            case Type::Bandpass:
                b0 = alpha;
                b1 = 0.0;
                b2 = -alpha;
                a0 = 1.0 + alpha;
                a1 = -2.0 * cs;
                a2 = 1.0 - alpha;
                break;

            case Type::Notch:
                b0 = 1.0;
                b1 = -2.0 * cs;
                b2 = 1.0;
                a0 = 1.0 + alpha;
                a1 = -2.0 * cs;
                a2 = 1.0 - alpha;
                break;

            case Type::Peak: {
                b0 = 1.0 + alpha * A;
                b1 = -2.0 * cs;
                b2 = 1.0 - alpha * A;
                a0 = 1.0 + alpha / A;
                a1 = -2.0 * cs;
                a2 = 1.0 - alpha / A;
                break;
            }

            case Type::LowShelf: {
                const double beta = 2.0 * std::sqrt(A) * alpha;
                b0 = A * ((A + 1.0) - (A - 1.0) * cs + beta);
                b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cs);
                b2 = A * ((A + 1.0) - (A - 1.0) * cs - beta);
                a0 = (A + 1.0) + (A - 1.0) * cs + beta;
                a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cs);
                a2 = (A + 1.0) + (A - 1.0) * cs - beta;
                break;
            }

            case Type::HighShelf: {
                const double beta = 2.0 * std::sqrt(A) * alpha;
                b0 = A * ((A + 1.0) + (A - 1.0) * cs + beta);
                b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cs);
                b2 = A * ((A + 1.0) + (A - 1.0) * cs - beta);
                a0 = (A + 1.0) - (A - 1.0) * cs + beta;
                a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cs);
                a2 = (A + 1.0) - (A - 1.0) * cs - beta;
                break;
            }

            case Type::Allpass:
                b0 = 1.0 - alpha;
                b1 = -2.0 * cs;
                b2 = 1.0 + alpha;
                a0 = 1.0 + alpha;
                a1 = -2.0 * cs;
                a2 = 1.0 - alpha;
                break;
        }

        const double invA0 = 1.0 / a0;
        b0_ = static_cast<float>(b0 * invA0);
        b1_ = static_cast<float>(b1 * invA0);
        b2_ = static_cast<float>(b2 * invA0);
        a1_ = static_cast<float>(a1 * invA0);
        a2_ = static_cast<float>(a2 * invA0);
    }

    // Procesa una muestra con protección estricta contra NaN/Inf (Regla 38)
    float processSample(float in) noexcept {
        const float out = b0_ * in + z1_;
        z1_ = b1_ * in - a1_ * out + z2_;
        z2_ = b2_ * in - a2_ * out;

        if (std::isnan(z1_) || std::isinf(z1_)) z1_ = 0.0f;
        if (std::isnan(z2_) || std::isinf(z2_)) z2_ = 0.0f;

        return out;
    }

private:
    float b0_{ 1.0f }, b1_{ 0.0f }, b2_{ 0.0f };
    float a1_{ 0.0f }, a2_{ 0.0f };
    float z1_{ 0.0f }, z2_{ 0.0f };
};

} // namespace audio_graph
