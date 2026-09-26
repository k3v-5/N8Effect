#pragma once

#include <vector>
#include <array>
#include <cmath>
#include <atomic>
#include <algorithm>
#include <numbers>
#include "../../core/Types.h"
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/FastMath.h"
#include "../core/DenormalGuards.h"

namespace audio_graph {

/**
 * @brief Reproductor de Muestras de Audio Polifónico / Monofónico (Nodo DSP #53).
 * Soporta carga en caliente de archivos WAV/AIF, resampleo transparente, transposición por semitonos,
 * modos One-Shot y Bucle continuo, control de envolvente de ataque/liberación y forma de onda en miniatura.
 * Cumple con Reglas 5, 8, 9, 14, 34, 46, 47 (Zero-allocations en audio thread, Denormals Are Zero, Anti-click).
 */
class SamplePlayerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        PitchSemitones = 1, // -24.0 a +24.0 semitonos
        RootNote = 2,       // 0.0 a 127.0 (Por defecto 60 = C4)
        PlaybackSpeed = 3,  // 0.1 a 4.0 (1.0 = velocidad original)
        LoopMode = 4,       // 0.0 = One-Shot, 1.0 = Bucle continuo
        AttackMs = 5,       // 1.0 a 1000.0 ms
        ReleaseMs = 6,      // 5.0 a 2000.0 ms
        Gain = 7,           // 0.0 a 2.0 (1.0 = 0 dB)
        Mix = 8             // 0.0 a 1.0 (Mezcla de entrada de audio pasante y muestra)
    };

    struct SampleBufferSlot {
        std::vector<float> sampleL;
        std::vector<float> sampleR;
        size_t numSamples{ 0 };
        double sampleRate{ 48000.0 };
    };

    SamplePlayerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { SidechainPinId, "Trigger In", PinType::AudioInput, PinDataType::AudioMono };

        params_[0] = { PitchSemitones, "Pitch", 0.0f, -24.0f, 24.0f, true };
        params_[1] = { RootNote, "Root Note", 60.0f, 0.0f, 127.0f, false };
        params_[2] = { PlaybackSpeed, "Speed", 1.0f, 0.1f, 4.0f, true };
        params_[3] = { LoopMode, "Loop", 1.0f, 0.0f, 1.0f, false };
        params_[4] = { AttackMs, "Attack", 5.0f, 1.0f, 1000.0f, true };
        params_[5] = { ReleaseMs, "Release", 50.0f, 5.0f, 2000.0f, true };
        params_[6] = { Gain, "Gain", 1.0f, 0.0f, 2.0f, true };
        params_[7] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        thumbnail_.fill(0.0f);
    }

    NodeType getType() const noexcept override { return NodeType::SamplePlayer; }
    const char* getName() const noexcept override { return "Sample Player"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        playheadPos_ = 0.0;
        isPlaying_ = true;
        envLevel_ = 0.0f;
        prevTrigger_ = 0.0f;

        // Si aún no se ha cargado una muestra personalizada, generar una muestra predeterminada musical
        const size_t activeIdx = activeSlotIndex_.load(std::memory_order_relaxed);
        if (slots_[activeIdx].numSamples == 0) {
            generateDefaultSample(spec.sampleRate);
        }
    }

    void reset() noexcept override {
        playheadPos_ = 0.0;
        isPlaying_ = true;
        envLevel_ = 0.0f;
        prevTrigger_ = 0.0f;
    }

    void setParameter(ParameterId id, float value) noexcept override {
        switch (id) {
            case PitchSemitones: targetPitch_ = value; break;
            case RootNote: targetRootNote_ = value; break;
            case PlaybackSpeed: targetSpeed_ = value; break;
            case LoopMode: targetLoop_ = value; break;
            case AttackMs: targetAttack_ = value; break;
            case ReleaseMs: targetRelease_ = value; break;
            case Gain: targetGain_ = value; break;
            case Mix: targetMix_ = value; break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const noexcept override {
        switch (id) {
            case PitchSemitones: return targetPitch_;
            case RootNote: return targetRootNote_;
            case PlaybackSpeed: return targetSpeed_;
            case LoopMode: return targetLoop_;
            case AttackMs: return targetAttack_;
            case ReleaseMs: return targetRelease_;
            case Gain: return targetGain_;
            case Mix: return targetMix_;
            default: return 0.0f;
        }
    }

    std::span<const PinDescriptor> getPins() const noexcept override { return pins_; }
    std::span<const ParameterInfo> getParameters() const noexcept override { return params_; }

    /**
     * @brief Carga en caliente una muestra de audio estéreo mediante doble búfer lock-free (Reglas 9 y 30).
     * Se invoca desde el hilo GUI / background fuera del audio thread.
     */
    void loadAudioSample(const float* lData, const float* rData, size_t numSamples, double sourceSampleRate) {
        if (!lData || numSamples == 0) return;

        const size_t activeIdx = activeSlotIndex_.load(std::memory_order_relaxed);
        const size_t inactiveIdx = 1 - activeIdx;
        auto& slot = slots_[inactiveIdx];

        slot.sampleL.assign(lData, lData + numSamples);
        if (rData != nullptr) {
            slot.sampleR.assign(rData, rData + numSamples);
        } else {
            slot.sampleR = slot.sampleL;
        }
        slot.numSamples = numSamples;
        slot.sampleRate = sourceSampleRate > 0.0 ? sourceSampleRate : 44100.0;

        // Actualizar miniatura de forma de onda (64 puntos)
        updateThumbnail(slot.sampleL.data(), slot.sampleR.data(), numSamples);

        // Intercambio atómico con barrera de memoria release
        activeSlotIndex_.store(inactiveIdx, std::memory_order_release);
        playheadPos_ = 0.0;
        isPlaying_ = true;
    }

    const std::array<float, 64>& getThumbnail() const noexcept { return thumbnail_; }

    void triggerPlayback() noexcept {
        playheadPos_ = 0.0;
        isPlaying_ = true;
        envLevel_ = 0.0f;
    }

    void process(ProcessContext& ctx) noexcept override {
        ScopedDenormalGuard denormalGuard; // Regla 47: Anti-denormales en hardware

        if (ctx.numSamples == 0 || ctx.outputChannels == nullptr) return;

        const float* inL = (ctx.numInputChannels > 0 && ctx.inputChannels[0]) ? ctx.inputChannels[0] : nullptr;
        const float* inR = (ctx.numInputChannels > 1 && ctx.inputChannels[1]) ? ctx.inputChannels[1] : inL;
        float* outL = (ctx.numOutputChannels > 0 && ctx.outputChannels[0]) ? ctx.outputChannels[0] : nullptr;
        float* outR = (ctx.numOutputChannels > 1 && ctx.outputChannels[1]) ? ctx.outputChannels[1] : outL;

        if (!outL) return;

        // Detección de trigger por pin de modulación / sidechain
        const float* triggerSig = (ctx.numSidechainChannels > 0 && ctx.sidechainChannels != nullptr) ? ctx.sidechainChannels[0] : nullptr;

        const size_t activeIdx = activeSlotIndex_.load(std::memory_order_acquire);
        const auto& slot = slots_[activeIdx];

        if (slot.numSamples == 0 || (!isPlaying_ && targetLoop_ < 0.5f)) {
            // Passthrough de audio de entrada si no hay sample cargado o ya terminó
            for (uint32_t s = 0; s < ctx.numSamples; ++s) {
                outL[s] = inL ? inL[s] : 0.0f;
                if (outR) outR[s] = inR ? inR[s] : 0.0f;
            }
            return;
        }

        const double hostSR = spec_.sampleRate > 0.0 ? spec_.sampleRate : 48000.0;
        const float pitchFactor = std::pow(2.0f, targetPitch_ / 12.0f);
        const double rateRatio = (slot.sampleRate / hostSR) * static_cast<double>(targetSpeed_) * static_cast<double>(pitchFactor);

        const float mix = std::clamp(targetMix_, 0.0f, 1.0f);
        const float gain = std::clamp(targetGain_, 0.0f, 2.0f);
        const bool isLoop = (targetLoop_ > 0.5f);
        const size_t totalSamples = slot.numSamples;

        // Coeficientes de envolvente
        const float attackSamples = std::max(1.0f, targetAttack_ * 0.001f * static_cast<float>(hostSR));
        const float releaseSamples = std::max(1.0f, targetRelease_ * 0.001f * static_cast<float>(hostSR));
        const float attackStep = 1.0f / attackSamples;

        for (uint32_t s = 0; s < ctx.numSamples; ++s) {
            // Comprobar flanco ascendente de trigger
            if (triggerSig != nullptr) {
                const float trigVal = triggerSig[s];
                if (trigVal > 0.5f && prevTrigger_ <= 0.5f) {
                    playheadPos_ = 0.0;
                    isPlaying_ = true;
                    envLevel_ = 0.0f;
                }
                prevTrigger_ = trigVal;
            }

            if (!isPlaying_) {
                outL[s] = inL ? inL[s] : 0.0f;
                if (outR) outR[s] = inR ? inR[s] : 0.0f;
                continue;
            }

            // Interpolación lineal de alta precisión
            const size_t idx0 = static_cast<size_t>(playheadPos_);
            const size_t idx1 = (idx0 + 1 < totalSamples) ? (idx0 + 1) : (isLoop ? 0 : idx0);
            const float frac = static_cast<float>(playheadPos_ - static_cast<double>(idx0));

            const float s0L = slot.sampleL[idx0];
            const float s1L = slot.sampleL[idx1];
            const float sampleL = (s0L + frac * (s1L - s0L));

            const float s0R = slot.sampleR[idx0];
            const float s1R = slot.sampleR[idx1];
            const float sampleR = (s0R + frac * (s1R - s0R));

            // Envolvente de ataque
            if (envLevel_ < 1.0f) {
                envLevel_ = std::min(1.0f, envLevel_ + attackStep);
            }

            // Envolvente de desvanecimiento cerca del final en modo One-Shot
            float finalEnv = envLevel_;
            if (!isLoop) {
                const double remaining = static_cast<double>(totalSamples) - playheadPos_;
                if (remaining < static_cast<double>(releaseSamples)) {
                    finalEnv *= static_cast<float>(remaining / static_cast<double>(releaseSamples));
                }
            }

            const float wetL = sampleL * finalEnv * gain;
            const float wetR = sampleR * finalEnv * gain;
            const float dryL = inL ? inL[s] : 0.0f;
            const float dryR = inR ? inR[s] : 0.0f;

            outL[s] = dryL * (1.0f - mix) + wetL * mix;
            if (outR) {
                outR[s] = dryR * (1.0f - mix) + wetR * mix;
            }

            // Avanzar cabezal de lectura
            playheadPos_ += rateRatio;

            if (playheadPos_ >= static_cast<double>(totalSamples)) {
                if (isLoop) {
                    playheadPos_ = std::fmod(playheadPos_, static_cast<double>(totalSamples));
                } else {
                    isPlaying_ = false;
                    playheadPos_ = 0.0;
                }
            }
        }
    }

private:
    void generateDefaultSample(double sampleRate) {
        // Muestra inicial de tono cálido con armónicos tipo campana/piano eléctrico (1.5 segundos)
        const size_t totalSamples = static_cast<size_t>(sampleRate * 1.5);
        std::vector<float> defL(totalSamples, 0.0f);
        std::vector<float> defR(totalSamples, 0.0f);

        const float fundamental = 261.63f; // C4
        const float omega = 2.0f * std::numbers::pi_v<float> * fundamental / static_cast<float>(sampleRate);

        for (size_t n = 0; n < totalSamples; ++n) {
            const float t = static_cast<float>(n);
            const float decay1 = std::exp(-3.5f * t / static_cast<float>(sampleRate));
            const float decay2 = std::exp(-6.0f * t / static_cast<float>(sampleRate));
            const float decay3 = std::exp(-9.0f * t / static_cast<float>(sampleRate));

            const float s1 = std::sin(omega * t) * decay1;
            const float s2 = std::sin(omega * 2.0f * t) * 0.4f * decay2;
            const float s3 = std::sin(omega * 3.0f * t) * 0.15f * decay3;

            defL[n] = (s1 + s2 + s3) * 0.8f;
            defR[n] = (s1 * 0.95f + s2 * 1.05f + s3) * 0.8f;
        }

        auto& slot = slots_[0];
        slot.sampleL = std::move(defL);
        slot.sampleR = std::move(defR);
        slot.numSamples = totalSamples;
        slot.sampleRate = sampleRate;
        updateThumbnail(slot.sampleL.data(), slot.sampleR.data(), totalSamples);
    }

    void updateThumbnail(const float* lData, const float* rData, size_t numSamples) {
        if (!lData || numSamples == 0) {
            thumbnail_.fill(0.0f);
            return;
        }

        float globalMax = 0.0f;
        for (size_t pt = 0; pt < 64; ++pt) {
            const size_t start = pt * numSamples / 64;
            size_t end = (pt + 1) * numSamples / 64;
            if (end <= start) end = start + 1;
            if (end > numSamples) end = numSamples;

            float peak = 0.0f;
            for (size_t i = start; i < end; ++i) {
                float valL = std::abs(lData[i]);
                float valR = rData ? std::abs(rData[i]) : valL;
                float val = std::max(valL, valR);
                if (val > peak) peak = val;
            }
            thumbnail_[pt] = peak;
            if (peak > globalMax) globalMax = peak;
        }

        if (globalMax > 1e-5f) {
            for (float& v : thumbnail_) {
                v /= globalMax;
            }
        }
    }

    std::array<PinDescriptor, 3> pins_;
    std::array<ParameterInfo, 8> params_;

    float targetPitch_{ 0.0f };
    float targetRootNote_{ 60.0f };
    float targetSpeed_{ 1.0f };
    float targetLoop_{ 1.0f };
    float targetAttack_{ 5.0f };
    float targetRelease_{ 50.0f };
    float targetGain_{ 1.0f };
    float targetMix_{ 1.0f };

    ProcessSpec spec_{ 48000.0, 256, 2, 2 };

    // Doble búfer seguro para intercambio atómico de audio
    std::array<SampleBufferSlot, 2> slots_{};
    std::atomic<size_t> activeSlotIndex_{ 0 };

    double playheadPos_{ 0.0 };
    bool isPlaying_{ true };
    float envLevel_{ 0.0f };
    float prevTrigger_{ 0.0f };

    std::array<float, 64> thumbnail_{};
};

inline AutoRegisterNode<SamplePlayerNode> registerSamplePlayer(NodeType::SamplePlayer, "Sample Player", "Synth & Sampler");

} // namespace audio_graph
