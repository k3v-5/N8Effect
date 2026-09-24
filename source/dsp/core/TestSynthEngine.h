#pragma once

#include <array>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <atomic>
#include "FastMath.h"
#include "DenormalGuards.h"
#include "../../core/Types.h"

namespace audio_graph {

/**
 * @brief Tipos de timbre para el sintetizador de pruebas (Reglas 13, 34 y 47).
 */
enum class SynthSoundType : uint8_t {
    SynthSaw = 0,
    WarmSine,
    SquareLead,
    Pluck,
    Count
};

/**
 * @brief Sintetizador polifónico en tiempo real para audicionar y probar el motor de efectos (Reglas 9, 13, 14, 34, 35, 47).
 * - Cero asignaciones dinámicas en el audio thread (16 voces prealocadas).
 * - Libre de locks y seguro para ejecución en tiempo real.
 * - Osciladores anti-aliased vía PolyBLEP y envolventes anti-click ADSR.
 * - Desacoplado de frameworks GUI para pruebas unitarias directas (Reglas 13 y 46).
 */
class TestSynthEngine {
public:
    static constexpr size_t MaxVoices = 16;
    static constexpr double TwoPi = 6.28318530717958647692;

    enum class EnvStage : uint8_t {
        Off = 0,
        Attack,
        Decay,
        Sustain,
        Release
    };

    struct SynthVoice {
        bool active{ false };
        int midiNote{ -1 };
        float velocity{ 0.0f };
        double phase{ 0.0 };
        double phaseIncrement{ 0.0 };
        EnvStage stage{ EnvStage::Off };
        float envLevel{ 0.0f };
        uint32_t ageSamples{ 0 };
        float panLeft{ 0.7071f };
        float panRight{ 0.7071f };
    };

    TestSynthEngine() noexcept {
        reset();
    }

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 44100.0;
        reset();
    }

    void reset() noexcept {
        for (auto& v : voices_) {
            v.active = false;
            v.midiNote = -1;
            v.velocity = 0.0f;
            v.phase = 0.0;
            v.phaseIncrement = 0.0;
            v.stage = EnvStage::Off;
            v.envLevel = 0.0f;
            v.ageSamples = 0;
            v.panLeft = 0.7071f;
            v.panRight = 0.7071f;
        }
        pitchBendSemitones_ = 0.0f;
        pitchBendRatio_ = 1.0;
        currentGain_ = targetGain_;
    }

    void setSoundType(SynthSoundType type) noexcept {
        soundType_.store(type, std::memory_order_relaxed);
    }

    SynthSoundType getSoundType() const noexcept {
        return soundType_.load(std::memory_order_relaxed);
    }

    void setGain(float gain) noexcept {
        targetGain_ = std::clamp(gain, 0.0f, 2.0f);
    }

    float getGain() const noexcept {
        return targetGain_;
    }

    void setEnabled(bool enabled) noexcept {
        enabled_.store(enabled, std::memory_order_relaxed);
        if (!enabled) {
            allNotesOff();
        }
    }

    bool isEnabled() const noexcept {
        return enabled_.load(std::memory_order_relaxed);
    }

    size_t getActiveVoiceCount() const noexcept {
        size_t count = 0;
        for (const auto& v : voices_) {
            if (v.active) ++count;
        }
        return count;
    }

    void setPitchBend(float bendSemitones) noexcept {
        pitchBendSemitones_ = std::clamp(bendSemitones, -12.0f, 12.0f);
        pitchBendRatio_ = std::pow(2.0, pitchBendSemitones_ / 12.0);
        updateVoiceIncrements();
    }

    void noteOn(int noteNumber, float velocity) noexcept {
        if (!enabled_.load(std::memory_order_relaxed) || velocity <= 0.0f) {
            return;
        }

        // 1. Si la nota ya está sonando en alguna voz, re-dispararla
        for (auto& v : voices_) {
            if (v.active && v.midiNote == noteNumber) {
                initVoice(v, noteNumber, velocity);
                return;
            }
        }

        // 2. Buscar voz inactiva
        for (auto& v : voices_) {
            if (!v.active) {
                initVoice(v, noteNumber, velocity);
                return;
            }
        }

        // 3. Robo de voz (Voice Stealing): buscar voz en Release con menor nivel
        SynthVoice* bestVoice = nullptr;
        float minReleaseLevel = 1000.0f;
        for (auto& v : voices_) {
            if (v.stage == EnvStage::Release && v.envLevel < minReleaseLevel) {
                minReleaseLevel = v.envLevel;
                bestVoice = &v;
            }
        }

        // 4. Si ninguna está en Release, robar la más longeva (ageSamples más alto)
        if (bestVoice == nullptr) {
            uint32_t maxAge = 0;
            for (auto& v : voices_) {
                if (v.ageSamples >= maxAge) {
                    maxAge = v.ageSamples;
                    bestVoice = &v;
                }
            }
        }

        if (bestVoice != nullptr) {
            initVoice(*bestVoice, noteNumber, velocity);
        }
    }

    void noteOff(int noteNumber) noexcept {
        for (auto& v : voices_) {
            if (v.active && v.midiNote == noteNumber && v.stage != EnvStage::Release) {
                v.stage = EnvStage::Release;
            }
        }
    }

    void allNotesOff() noexcept {
        for (auto& v : voices_) {
            if (v.active && v.stage != EnvStage::Release) {
                v.stage = EnvStage::Release;
            }
        }
    }

    /**
     * @brief Renderiza y suma el audio del sintetizador directamente en los canales de entrada del plugin (Reglas 9, 34 y 35).
     */
    void renderAudioAdding(float* const* channels, uint32_t numChannels, uint32_t numSamples) noexcept {
        if (!enabled_.load(std::memory_order_relaxed) || numChannels == 0 || channels == nullptr || numSamples == 0) {
            return;
        }

        // Comprobar si hay alguna voz activa
        bool hasActiveVoices = false;
        for (const auto& v : voices_) {
            if (v.active) {
                hasActiveVoices = true;
                break;
            }
        }
        if (!hasActiveVoices) {
            return;
        }

        DenormalDisabler denormalGuard; // Protección anti-denormal (Regla 47)
        const SynthSoundType currentType = soundType_.load(std::memory_order_relaxed);

        const float attackRate = static_cast<float>(1.0 / (sampleRate_ * 0.005));   // 5ms
        const float decayRate  = static_cast<float>(1.0 / (sampleRate_ * 0.120));   // 120ms
        const float releaseRate = static_cast<float>(1.0 / (sampleRate_ * 0.080));  // 80ms
        const float sustainLevel = (currentType == SynthSoundType::Pluck) ? 0.0f : 0.65f;

        const float gainSmoothing = 0.005f;

        for (uint32_t s = 0; s < numSamples; ++s) {
            currentGain_ += gainSmoothing * (targetGain_ - currentGain_);

            float sumL = 0.0f;
            float sumR = 0.0f;

            for (auto& v : voices_) {
                if (!v.active) continue;

                v.ageSamples++;

                // 1. Máquina de estados de envolvente ADSR anti-click
                switch (v.stage) {
                    case EnvStage::Attack:
                        v.envLevel += attackRate;
                        if (v.envLevel >= 1.0f) {
                            v.envLevel = 1.0f;
                            v.stage = EnvStage::Decay;
                        }
                        break;

                    case EnvStage::Decay:
                        v.envLevel -= decayRate;
                        if (v.envLevel <= sustainLevel) {
                            v.envLevel = sustainLevel;
                            v.stage = (sustainLevel > 0.001f) ? EnvStage::Sustain : EnvStage::Release;
                        }
                        break;

                    case EnvStage::Sustain:
                        v.envLevel = sustainLevel;
                        break;

                    case EnvStage::Release:
                        v.envLevel -= releaseRate;
                        if (v.envLevel <= 1e-4f) {
                            v.envLevel = 0.0f;
                            v.stage = EnvStage::Off;
                            v.active = false;
                            continue;
                        }
                        break;

                    case EnvStage::Off:
                    default:
                        v.active = false;
                        continue;
                }

                // 2. Generación de forma de onda
                float sample = 0.0f;
                const double phase = v.phase;
                const double phaseInc = v.phaseIncrement * pitchBendRatio_;

                switch (currentType) {
                    case SynthSoundType::WarmSine:
                        sample = static_cast<float>(std::sin(phase * TwoPi));
                        break;

                    case SynthSoundType::SynthSaw: {
                        float naive = static_cast<float>(2.0 * phase - 1.0);
                        sample = naive - polyBlep(phase, phaseInc);
                        break;
                    }

                    case SynthSoundType::SquareLead: {
                        float naive = (phase < 0.5) ? 1.0f : -1.0f;
                        sample = naive + polyBlep(phase, phaseInc) - polyBlep(std::fmod(phase + 0.5, 1.0), phaseInc);
                        break;
                    }

                    case SynthSoundType::Pluck: {
                        float fundamental = static_cast<float>(std::sin(phase * TwoPi));
                        float harmonic = static_cast<float>(std::sin(phase * TwoPi * 2.0)) * 0.35f;
                        sample = fundamental + harmonic;
                        break;
                    }

                    default:
                        sample = static_cast<float>(std::sin(phase * TwoPi));
                        break;
                }

                // Avanzar fase
                v.phase += phaseInc;
                if (v.phase >= 1.0) {
                    v.phase -= 1.0;
                }

                const float voiceSample = sample * v.envLevel * v.velocity;
                sumL += voiceSample * v.panLeft;
                sumR += voiceSample * v.panRight;
            }

            const float totalGain = currentGain_ * 0.4f; // Nivel cómodo para entrada del rack
            sumL *= totalGain;
            sumR *= totalGain;

            if (channels[0] != nullptr) {
                channels[0][s] += sumL;
            }
            if (numChannels > 1 && channels[1] != nullptr) {
                channels[1][s] += sumR;
            }
        }
    }

private:
    static inline float polyBlep(double t, double dt) noexcept {
        if (dt <= 0.0) return 0.0f;
        if (t < dt) {
            t /= dt;
            return static_cast<float>(t + t - t * t - 1.0);
        }
        if (t > 1.0 - dt) {
            t = (t - 1.0) / dt;
            return static_cast<float>(t * t + t + t + 1.0);
        }
        return 0.0f;
    }

    void initVoice(SynthVoice& v, int noteNumber, float velocity) noexcept {
        v.active = true;
        v.midiNote = noteNumber;
        v.velocity = std::clamp(velocity, 0.01f, 1.0f);
        v.phase = 0.0;
        
        // Conversión estándar MIDI note -> frecuencia en Hz
        const double freq = 440.0 * std::pow(2.0, (static_cast<double>(noteNumber) - 69.0) / 12.0);
        v.phaseIncrement = std::clamp(freq / sampleRate_, 0.00001, 0.49);

        v.stage = EnvStage::Attack;
        v.ageSamples = 0;

        // Distribución estéreo suave (Pan spread de -0.25 a +0.25)
        const float pan = std::clamp((static_cast<float>(noteNumber) - 60.0f) * 0.015f, -0.25f, 0.25f);
        const float panAngle = (pan + 1.0f) * 0.78539816339f; // Pi / 4
        v.panLeft = std::cos(panAngle);
        v.panRight = std::sin(panAngle);
    }

    void updateVoiceIncrements() noexcept {
        for (auto& v : voices_) {
            if (v.active && v.midiNote >= 0) {
                const double freq = 440.0 * std::pow(2.0, (static_cast<double>(v.midiNote) - 69.0) / 12.0);
                v.phaseIncrement = std::clamp(freq / sampleRate_, 0.00001, 0.49);
            }
        }
    }

    double sampleRate_{ 44100.0 };
    std::atomic<bool> enabled_{ true };
    std::atomic<SynthSoundType> soundType_{ SynthSoundType::SynthSaw };
    float targetGain_{ 0.65f };
    float currentGain_{ 0.65f };
    float pitchBendSemitones_{ 0.0f };
    double pitchBendRatio_{ 1.0 };

    std::array<SynthVoice, MaxVoices> voices_{};
};

} // namespace audio_graph
