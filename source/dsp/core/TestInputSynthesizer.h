#pragma once

#include <cmath>
#include <array>
#include <atomic>
#include <algorithm>
#include <numbers>
#include "../../core/Types.h"
#include "../../core/RealtimePools.h"
#include "DenormalGuards.h"
#include "FastMath.h"

namespace audio_graph {

/**
 * @brief Modo tímbrico del generador de señal de prueba.
 */
enum class TestTimbreMode : uint8_t {
    ElectricPiano = 0,
    Sine = 1,
    Triangle = 2,
    WarmSaw = 3,
    LoadedSample = 4
};

/**
 * @brief Voz individual polifónica del generador de señal de audio de prueba (Reglas 9, 10, 47).
 * Prealocada, bounded, anti-denormales y libre de alocaciones en tiempo de ejecución.
 */
struct SynthVoice {
    bool active{ false };
    int midiNote{ -1 };
    float velocity{ 0.0f };
    float phase{ 0.0f };
    float phaseIncrement{ 0.0f };
    double samplePlayhead{ 0.0 };

    // Envolvente rápida ADSR musical anti-click
    enum class EnvStage { Off, Attack, Decay, Sustain, Release };
    EnvStage envStage{ EnvStage::Off };
    float currentLevel{ 0.0f };
    float attackRate{ 0.005f };
    float decayRate{ 0.0005f };
    float sustainLevel{ 0.70f };
    float releaseRate{ 0.002f };

    void start(int note, float vel, double sampleRate) noexcept {
        midiNote = note;
        velocity = vel;
        phase = 0.0f;
        samplePlayhead = 0.0;

        // Frecuencia estándar MIDI (A4 = 440 Hz)
        const float freq = 440.0f * std::pow(2.0f, static_cast<float>(note - 69) / 12.0f);
        phaseIncrement = static_cast<float>(freq / (sampleRate > 0.0 ? sampleRate : 44100.0));

        // Parámetros de envolvente según sample rate
        const float sr = static_cast<float>(sampleRate > 0.0 ? sampleRate : 44100.0f);
        attackRate = 1.0f / (0.003f * sr);  // 3 ms de ataque rápido anti-click
        decayRate = 1.0f / (0.120f * sr);   // 120 ms de decaimiento
        sustainLevel = 0.65f;
        releaseRate = 1.0f / (0.015f * sr); // 15 ms de liberación al soltar la tecla

        envStage = EnvStage::Attack;
        active = true;
    }

    void stop() noexcept {
        if (active) {
            envStage = EnvStage::Release;
        }
    }

    void kill() noexcept {
        active = false;
        envStage = EnvStage::Off;
        currentLevel = 0.0f;
        midiNote = -1;
        samplePlayhead = 0.0;
    }

    float getNextSample(TestTimbreMode timbre, const float* sampleData = nullptr, size_t sampleLen = 0, double sampleSr = 44100.0, double hostSr = 44100.0) noexcept {
        if (!active) return 0.0f;

        // Avanzar máquina de estados de envolvente
        switch (envStage) {
            case EnvStage::Attack:
                currentLevel += attackRate;
                if (currentLevel >= 1.0f) {
                    currentLevel = 1.0f;
                    envStage = EnvStage::Decay;
                }
                break;
            case EnvStage::Decay:
                currentLevel -= decayRate;
                if (currentLevel <= sustainLevel) {
                    currentLevel = sustainLevel;
                    envStage = EnvStage::Sustain;
                }
                break;
            case EnvStage::Sustain:
                break;
            case EnvStage::Release:
                currentLevel -= releaseRate;
                if (currentLevel <= 0.0001f) {
                    currentLevel = 0.0f;
                    kill();
                    return 0.0f;
                }
                break;
            case EnvStage::Off:
                kill();
                return 0.0f;
        }

        // Generar forma de onda
        float osc = 0.0f;
        const float p = phase;

        switch (timbre) {
            case TestTimbreMode::Sine:
                osc = FastMath::fastSin(p * 2.0f * std::numbers::pi_v<float>);
                break;

            case TestTimbreMode::Triangle:
                osc = 2.0f * std::abs(2.0f * (p - std::floor(p + 0.5f))) - 1.0f;
                break;

            case TestTimbreMode::WarmSaw: {
                // Diente de sierra suave con aproximación Padé
                const float rawSaw = 2.0f * (p - std::floor(p + 0.5f));
                osc = FastMath::fastTanh(rawSaw * 1.5f);
                break;
            }

            case TestTimbreMode::LoadedSample: {
                if (sampleData != nullptr && sampleLen > 0) {
                    const size_t idx0 = static_cast<size_t>(samplePlayhead);
                    const size_t idx1 = (idx0 + 1 < sampleLen) ? (idx0 + 1) : idx0;
                    const float frac = static_cast<float>(samplePlayhead - static_cast<double>(idx0));
                    const float s0 = sampleData[idx0];
                    const float s1 = sampleData[idx1];
                    osc = s0 + frac * (s1 - s0);

                    const float pitchFactor = std::pow(2.0f, static_cast<float>(midiNote - 60) / 12.0f);
                    const double rate = pitchFactor * (sampleSr / (hostSr > 0.0 ? hostSr : 44100.0));
                    samplePlayhead += rate;
                    if (samplePlayhead >= static_cast<double>(sampleLen)) {
                        kill();
                    }
                }
                break;
            }

            case TestTimbreMode::ElectricPiano:
            default: {
                // Tono tipo Rhodes/E-Piano: Fundamental + armónico segundo sutil con calidez sigmoidal
                const float f1 = FastMath::fastSin(p * 2.0f * std::numbers::pi_v<float>);
                const float f2 = FastMath::fastSin(std::fmod(p * 2.0f, 1.0f) * 2.0f * std::numbers::pi_v<float>) * 0.35f;
                const float f3 = FastMath::fastSin(std::fmod(p * 3.0f, 1.0f) * 2.0f * std::numbers::pi_v<float>) * 0.15f;
                osc = FastMath::fastTanh((f1 + f2 + f3) * 1.2f);
                break;
            }
        }

        // Avanzar fase
        phase += phaseIncrement;
        if (phase >= 1.0f) {
            phase -= std::floor(phase);
        }

        return osc * currentLevel * velocity * 0.35f;
    }
};

/**
 * @brief Sintetizador Polifónico de Entrada para Pruebas en Tiempo Real (Reglas 9, 13, 34, 47).
 * Diseñado específicamente para que plugins de efecto puedan probar toda la cadena DSP
 * sin requerir una pista de audio previa en el DAW o en modo Standalone.
 */
class TestInputSynthesizer {
public:
    static constexpr size_t MaxVoices = 8;

    struct LoadedSampleData {
        std::vector<float> data;
        size_t length{ 0 };
        double sampleRate{ 44100.0 };
    };

    TestInputSynthesizer() = default;

    void prepare(double sampleRate, int samplesPerBlock) {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
        samplesPerBlock_ = samplesPerBlock > 0 ? samplesPerBlock : 512;
        internalBuffer_.prepare(2, static_cast<uint32_t>(samplesPerBlock_));
        reset();
    }

    void reset() noexcept {
        for (auto& v : voices_) {
            v.kill();
        }
        activeVoicesCount_.store(0, std::memory_order_relaxed);
    }

    void setTimbreMode(TestTimbreMode mode) noexcept {
        timbreMode_.store(mode, std::memory_order_relaxed);
    }

    TestTimbreMode getTimbreMode() const noexcept {
        return timbreMode_.load(std::memory_order_relaxed);
    }

    void setLoadedSample(const float* data, size_t length, double sampleRate) {
        if (!data || length == 0) return;
        const size_t active = activeSampleIdx_.load(std::memory_order_relaxed);
        const size_t inactive = 1 - active;
        loadedSamples_[inactive].data.assign(data, data + length);
        loadedSamples_[inactive].length = length;
        loadedSamples_[inactive].sampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        activeSampleIdx_.store(inactive, std::memory_order_release);
        timbreMode_.store(TestTimbreMode::LoadedSample, std::memory_order_relaxed);
    }

    bool hasLoadedSample() const noexcept {
        const size_t active = activeSampleIdx_.load(std::memory_order_relaxed);
        return loadedSamples_[active].length > 0;
    }

    void noteOn(int midiNote, float velocity) noexcept {
        if (midiNote < 0 || midiNote > 127) return;

        // Si la nota ya está sonando en alguna voz, re-dispararla
        for (auto& v : voices_) {
            if (v.active && v.midiNote == midiNote) {
                v.start(midiNote, velocity, sampleRate_);
                updateActiveCount();
                return;
            }
        }

        // Asignar primera voz libre
        for (auto& v : voices_) {
            if (!v.active) {
                v.start(midiNote, velocity, sampleRate_);
                updateActiveCount();
                return;
            }
        }

        // Voice stealing: robar la voz con el nivel más bajo
        size_t quietestIdx = 0;
        float minLevel = 1.0f;
        for (size_t i = 0; i < MaxVoices; ++i) {
            if (voices_[i].currentLevel < minLevel) {
                minLevel = voices_[i].currentLevel;
                quietestIdx = i;
            }
        }
        voices_[quietestIdx].start(midiNote, velocity, sampleRate_);
        updateActiveCount();
    }

    void noteOff(int midiNote) noexcept {
        for (auto& v : voices_) {
            if (v.active && v.midiNote == midiNote) {
                v.stop();
            }
        }
        updateActiveCount();
    }

    void allNotesOff() noexcept {
        for (auto& v : voices_) {
            v.stop();
        }
        updateActiveCount();
    }

    bool hasActiveVoices() const noexcept {
        return activeVoicesCount_.load(std::memory_order_relaxed) > 0;
    }

    /**
     * @brief Renderiza el audio de prueba polifónico y lo suma en los canales estéreo de entrada (Regla 9).
     */
    void renderAndInject(float* const* channels, uint32_t numChannels, uint32_t numSamples) noexcept {
        if (!hasActiveVoices() || channels == nullptr || numChannels == 0 || numSamples == 0) {
            return;
        }

        ScopedDenormalGuard denormalGuard; // Reglas 34 y 47
        const TestTimbreMode timbre = timbreMode_.load(std::memory_order_relaxed);

        const size_t activeIdx = activeSampleIdx_.load(std::memory_order_acquire);
        const auto& sampleSlot = loadedSamples_[activeIdx];
        const float* samplePtr = (sampleSlot.length > 0) ? sampleSlot.data.data() : nullptr;
        const size_t sampleLen = sampleSlot.length;
        const double sampleSr = sampleSlot.sampleRate;

        for (uint32_t s = 0; s < numSamples; ++s) {
            float sumSample = 0.0f;

            for (auto& v : voices_) {
                if (v.active) {
                    sumSample += v.getNextSample(timbre, samplePtr, sampleLen, sampleSr, sampleRate_);
                }
            }

            // Inyectar en los canales estéreo de entrada
            for (uint32_t ch = 0; ch < numChannels; ++ch) {
                if (channels[ch] != nullptr) {
                    channels[ch][s] += sumSample;
                }
            }
        }

        updateActiveCount();
    }

private:
    void updateActiveCount() noexcept {
        int count = 0;
        for (const auto& v : voices_) {
            if (v.active) count++;
        }
        activeVoicesCount_.store(count, std::memory_order_relaxed);
    }

    double sampleRate_{ 44100.0 };
    int samplesPerBlock_{ 512 };
    std::array<SynthVoice, MaxVoices> voices_{};
    std::atomic<int> activeVoicesCount_{ 0 };
    std::atomic<TestTimbreMode> timbreMode_{ TestTimbreMode::ElectricPiano };
    std::array<LoadedSampleData, 2> loadedSamples_{};
    std::atomic<size_t> activeSampleIdx_{ 0 };
    PreallocatedBuffer internalBuffer_;
};

} // namespace audio_graph
