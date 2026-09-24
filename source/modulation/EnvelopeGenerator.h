#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace audio_graph {

/**
 * @brief Generador de Envolvente ADSR de alta precisión para modulación (Reglas 7 y 46)
 */
class EnvelopeGenerator {
public:
    enum class State : uint8_t {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    EnvelopeGenerator() = default;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;
        updateCoefficients();
        reset();
    }

    void reset() noexcept {
        state_ = State::Idle;
        currentLevel_ = 0.0f;
    }

    void setAttackMs(float ms) noexcept { attackMs_ = std::max(0.5f, ms); updateCoefficients(); }
    void setDecayMs(float ms) noexcept { decayMs_ = std::max(0.5f, ms); updateCoefficients(); }
    void setSustainLevel(float lvl) noexcept { sustainLevel_ = std::clamp(lvl, 0.0f, 1.0f); }
    void setReleaseMs(float ms) noexcept { releaseMs_ = std::max(0.5f, ms); updateCoefficients(); }

    void triggerGate(bool gateOn) noexcept {
        if (gateOn) {
            state_ = State::Attack;
        } else if (state_ != State::Idle) {
            state_ = State::Release;
        }
    }

    // Calcula y avanza la siguiente muestra de la envolvente
    float processSample() noexcept {
        switch (state_) {
            case State::Idle:
                currentLevel_ = 0.0f;
                break;

            case State::Attack:
                currentLevel_ += attackCoeff_;
                if (currentLevel_ >= 1.0f) {
                    currentLevel_ = 1.0f;
                    state_ = State::Decay;
                }
                break;

            case State::Decay:
                currentLevel_ -= decayCoeff_ * (currentLevel_ - sustainLevel_);
                if (currentLevel_ <= sustainLevel_ + 0.001f) {
                    currentLevel_ = sustainLevel_;
                    state_ = State::Sustain;
                }
                break;

            case State::Sustain:
                currentLevel_ = sustainLevel_;
                break;

            case State::Release:
                currentLevel_ -= releaseCoeff_ * currentLevel_;
                if (currentLevel_ <= 0.0005f) {
                    currentLevel_ = 0.0f;
                    state_ = State::Idle;
                }
                break;
        }

        return currentLevel_;
    }

    float getCurrentValue() const noexcept { return currentLevel_; }
    State getState() const noexcept { return state_; }
    bool isActive() const noexcept { return state_ != State::Idle; }

private:
    void updateCoefficients() noexcept {
        const float attackSamples = (attackMs_ * 0.001f) * static_cast<float>(sampleRate_);
        const float decaySamples = (decayMs_ * 0.001f) * static_cast<float>(sampleRate_);
        const float releaseSamples = (releaseMs_ * 0.001f) * static_cast<float>(sampleRate_);

        attackCoeff_ = 1.0f / std::max(1.0f, attackSamples);
        decayCoeff_ = 1.0f / std::max(1.0f, decaySamples);
        releaseCoeff_ = 1.0f / std::max(1.0f, releaseSamples);
    }

    double sampleRate_{ 44100.0 };
    float attackMs_{ 10.0f };
    float decayMs_{ 100.0f };
    float sustainLevel_{ 0.7f };
    float releaseMs_{ 200.0f };

    float attackCoeff_{ 0.001f };
    float decayCoeff_{ 0.001f };
    float releaseCoeff_{ 0.001f };

    State state_{ State::Idle };
    float currentLevel_{ 0.0f };
};

} // namespace audio_graph
