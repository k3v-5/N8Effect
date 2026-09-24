#pragma once

#include <cmath>
#include <algorithm>

namespace audio_graph {

/**
 * @brief Seguidor de envolvente de audio (Audio Follower) para modulación dinámica (Reglas 7 y 46)
 */
class AudioFollower {
public:
    AudioFollower() = default;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;
        envelope_ = 0.0f;
        updateCoefficients();
    }

    void reset() noexcept {
        envelope_ = 0.0f;
    }

    void setAttackMs(float ms) noexcept { attackMs_ = std::max(0.1f, ms); updateCoefficients(); }
    void setReleaseMs(float ms) noexcept { releaseMs_ = std::max(1.0f, ms); updateCoefficients(); }

    // Procesa un bloque de audio estéreo y actualiza la envolvente seguida
    float processBlock(const float* const* input, uint32_t numChannels, uint32_t numSamples) noexcept {
        if (input == nullptr || numSamples == 0 || numChannels == 0) {
            return envelope_;
        }

        float blockPeak = 0.0f;
        for (uint32_t s = 0; s < numSamples; ++s) {
            float sampleMag = 0.0f;
            for (uint32_t ch = 0; ch < numChannels; ++ch) {
                if (input[ch] != nullptr) {
                    sampleMag = std::max(sampleMag, std::abs(input[ch][s]));
                }
            }
            blockPeak = std::max(blockPeak, sampleMag);
        }

        // Filtro de seguimiento balístico
        if (blockPeak > envelope_) {
            envelope_ += attackCoeff_ * (blockPeak - envelope_);
        } else {
            envelope_ += releaseCoeff_ * (blockPeak - envelope_);
        }

        return std::clamp(envelope_, 0.0f, 1.0f);
    }

    float getCurrentValue() const noexcept { return envelope_; }

private:
    void updateCoefficients() noexcept {
        const float attackSamples = (attackMs_ * 0.001f) * static_cast<float>(sampleRate_);
        const float releaseSamples = (releaseMs_ * 0.001f) * static_cast<float>(sampleRate_);

        attackCoeff_ = 1.0f - std::exp(-1.0f / std::max(1.0f, attackSamples));
        releaseCoeff_ = 1.0f - std::exp(-1.0f / std::max(1.0f, releaseSamples));
    }

    double sampleRate_{ 44100.0 };
    float attackMs_{ 10.0f };
    float releaseMs_{ 100.0f };
    float attackCoeff_{ 0.01f };
    float releaseCoeff_{ 0.001f };
    float envelope_{ 0.0f };
};

} // namespace audio_graph
