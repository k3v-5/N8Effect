#pragma once

#include <cmath>
#include <algorithm>

namespace audio_graph {

/**
 * @brief Detector balístico de envolvente Peak / RMS desacoplado (Reglas 32 y 46)
 */
class EnvelopeDetector {
public:
    enum class Mode {
        Peak,
        RMS
    };

    EnvelopeDetector() = default;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;
        envelope_ = 0.0f;
        updateCoefficients();
    }

    void reset() noexcept {
        envelope_ = 0.0f;
    }

    void setAttackMs(float ms) noexcept { attackMs_ = std::max(0.05f, ms); updateCoefficients(); }
    void setReleaseMs(float ms) noexcept { releaseMs_ = std::max(0.5f, ms); updateCoefficients(); }
    void setMode(Mode m) noexcept { mode_ = m; }

    float processSample(float in) noexcept {
        float inputLevel = std::abs(in);
        if (mode_ == Mode::RMS) {
            inputLevel = in * in;
        }

        if (inputLevel > envelope_) {
            envelope_ += attackCoeff_ * (inputLevel - envelope_);
        } else {
            envelope_ += releaseCoeff_ * (inputLevel - envelope_);
        }

        if (std::isnan(envelope_) || std::isinf(envelope_)) envelope_ = 0.0f;

        if (mode_ == Mode::RMS) {
            return std::sqrt(std::max(0.0f, envelope_));
        }
        return envelope_;
    }

    static float linearToDb(float lin) noexcept {
        return (lin > 1e-5f) ? 20.0f * std::log10(lin) : -100.0f;
    }

    static float dbToLinear(float db) noexcept {
        return std::pow(10.0f, db * 0.05f);
    }

private:
    void updateCoefficients() noexcept {
        const float attSamples = (attackMs_ * 0.001f) * static_cast<float>(sampleRate_);
        const float relSamples = (releaseMs_ * 0.001f) * static_cast<float>(sampleRate_);

        attackCoeff_ = 1.0f - std::exp(-1.0f / std::max(1.0f, attSamples));
        releaseCoeff_ = 1.0f - std::exp(-1.0f / std::max(1.0f, relSamples));
    }

    double sampleRate_{ 44100.0 };
    float attackMs_{ 10.0f };
    float releaseMs_{ 100.0f };
    float attackCoeff_{ 0.01f };
    float releaseCoeff_{ 0.001f };
    float envelope_{ 0.0f };
    Mode mode_{ Mode::Peak };
};

} // namespace audio_graph
