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

    struct Coefficients {
        float b0{ 1.0f }, b1{ 0.0f }, b2{ 0.0f };
        float a1{ 0.0f }, a2{ 0.0f };
    };

    BiquadFilter() = default;

    void reset() noexcept {
        z1_ = 0.0f;
        z2_ = 0.0f;
        remainingRampSamples_ = 0;
    }

    static Coefficients calculateCoefficients(Type type, double sampleRate, float frequencyHz, float q, float gainDb = 0.0f) noexcept {
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
                b2 = A * ((A + 1.0) - (A - 1.0) * cs - beta);
                a0 = (A + 1.0) - (A - 1.0) * cs + beta;
                a1 = 2.0 * ((A - 1.0) + (A + 1.0) * cs);
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
        return Coefficients{
            static_cast<float>(b0 * invA0),
            static_cast<float>(b1 * invA0),
            static_cast<float>(b2 * invA0),
            static_cast<float>(a1 * invA0),
            static_cast<float>(a2 * invA0)
        };
    }

    // Configuración instantánea de coeficientes
    void setCoefficients(Type type, double sampleRate, float frequencyHz, float q, float gainDb = 0.0f) noexcept {
        Coefficients c = calculateCoefficients(type, sampleRate, frequencyHz, q, gainDb);
        b0_ = c.b0; b1_ = c.b1; b2_ = c.b2;
        a1_ = c.a1; a2_ = c.a2;
        targetB0_ = b0_; targetB1_ = b1_; targetB2_ = b2_;
        targetA1_ = a1_; targetA2_ = a2_;
        deltaB0_ = deltaB1_ = deltaB2_ = deltaA1_ = deltaA2_ = 0.0f;
        remainingRampSamples_ = 0;
    }

    // Configuración suave con interpolación por rampa de muestras para eliminar zipper noise (Reglas 1, 34 y 35)
    void setCoefficientsSmooth(Type type, double sampleRate, float frequencyHz, float q, float gainDb = 0.0f, uint32_t rampSamples = 64) noexcept {
        Coefficients c = calculateCoefficients(type, sampleRate, frequencyHz, q, gainDb);
        if (rampSamples <= 1) {
            setCoefficients(type, sampleRate, frequencyHz, q, gainDb);
            return;
        }
        targetB0_ = c.b0; targetB1_ = c.b1; targetB2_ = c.b2;
        targetA1_ = c.a1; targetA2_ = c.a2;
        const float invRamp = 1.0f / static_cast<float>(rampSamples);
        deltaB0_ = (targetB0_ - b0_) * invRamp;
        deltaB1_ = (targetB1_ - b1_) * invRamp;
        deltaB2_ = (targetB2_ - b2_) * invRamp;
        deltaA1_ = (targetA1_ - a1_) * invRamp;
        deltaA2_ = (targetA2_ - a2_) * invRamp;
        remainingRampSamples_ = rampSamples;
    }

    void setLowpass(double sampleRate, float frequencyHz, float q = 0.707f) noexcept {
        setCoefficients(Type::Lowpass, sampleRate, frequencyHz, q);
    }

    void setHighpass(double sampleRate, float frequencyHz, float q = 0.707f) noexcept {
        setCoefficients(Type::Highpass, sampleRate, frequencyHz, q);
    }

    void setLowpassSmooth(double sampleRate, float frequencyHz, float q = 0.707f, uint32_t rampSamples = 64) noexcept {
        setCoefficientsSmooth(Type::Lowpass, sampleRate, frequencyHz, q, 0.0f, rampSamples);
    }

    void setHighpassSmooth(double sampleRate, float frequencyHz, float q = 0.707f, uint32_t rampSamples = 64) noexcept {
        setCoefficientsSmooth(Type::Highpass, sampleRate, frequencyHz, q, 0.0f, rampSamples);
    }

    // Procesa una muestra con rampa de coeficientes y protección estricta contra NaN/Inf (Reglas 35 y 38)
    float processSample(float in) noexcept {
        if (remainingRampSamples_ > 0) {
            b0_ += deltaB0_;
            b1_ += deltaB1_;
            b2_ += deltaB2_;
            a1_ += deltaA1_;
            a2_ += deltaA2_;
            --remainingRampSamples_;
            if (remainingRampSamples_ == 0) {
                b0_ = targetB0_;
                b1_ = targetB1_;
                b2_ = targetB2_;
                a1_ = targetA1_;
                a2_ = targetA2_;
            }
        }

        const float out = b0_ * in + z1_;
        z1_ = b1_ * in - a1_ * out + z2_;
        z2_ = b2_ * in - a2_ * out;

        if (std::isnan(z1_) || std::isinf(z1_)) z1_ = 0.0f;
        if (std::isnan(z2_) || std::isinf(z2_)) z2_ = 0.0f;

        return out;
    }

    float process(float in) noexcept {
        return processSample(in);
    }

    void processBlock(const float* in, float* out, size_t numSamples) noexcept {
        if (!in || !out) return;
        for (size_t i = 0; i < numSamples; ++i) {
            out[i] = processSample(in[i]);
        }
    }

    bool isRamping() const noexcept { return remainingRampSamples_ > 0; }

private:
    float b0_{ 1.0f }, b1_{ 0.0f }, b2_{ 0.0f };
    float a1_{ 0.0f }, a2_{ 0.0f };
    float targetB0_{ 1.0f }, targetB1_{ 0.0f }, targetB2_{ 0.0f };
    float targetA1_{ 0.0f }, targetA2_{ 0.0f };
    float deltaB0_{ 0.0f }, deltaB1_{ 0.0f }, deltaB2_{ 0.0f };
    float deltaA1_{ 0.0f }, deltaA2_{ 0.0f };
    uint32_t remainingRampSamples_{ 0 };
    float z1_{ 0.0f }, z2_{ 0.0f };
};

} // namespace audio_graph
