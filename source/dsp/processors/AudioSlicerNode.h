#pragma once

#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <random>
#include "../../core/Types.h"
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/BiquadFilter.h"
#include "../core/FastMath.h"

namespace audio_graph {

/**
 * @brief Buffer Audio Slicer & Frozen Granular Playground (Nodo DSP #49).
 * Captura audio continuo en un buffer circular de 3 segundos, permitiendo slicing rítmico,
 * saltos de rebanada, scrubbing, stutters, reproducción inversa y síntesis granular congelada.
 * Cumple con Reglas 5, 8, 9, 14, 34, 35, 46, 47 (Zero-allocations, Anti-click, Denormals Are Zero).
 */
class AudioSlicerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Freeze = 1,              // 0.0: Captura continua / Grabación, 1.0: Congelado en memoria
        SliceCount = 2,          // 4, 8, 16, 32 rebanadas
        ActiveSlice = 3,         // -1: Automático secuencial, 0..N: Rebanada fija en bucle
        PlaybackSpeed = 4,       // -2.0 a +2.0 (Valores negativos reproducen en reversa)
        PositionScrub = 5,       // 0.0 a 1.0 (Posición de reproducción dentro del buffer)
        GrainSizeMs = 6,         // 10.0 a 500.0 ms
        GrainDensity = 7,        // 1 a 8 voces de granos concurrentes
        Jitter = 8,              // 0.0 a 1.0 (Dispersión aleatoria de posición de granos)
        PitchShiftSemitones = 9, // -24.0 a +24.0 semitonos
        FilterCutoff = 10,       // 20.0 a 20000.0 Hz
        Mix = 11                 // 0.0 a 1.0 (Dry / Wet)
    };

    struct GrainVoice {
        bool active{ false };
        double bufferPos{ 0.0 };
        double playbackRate{ 1.0 };
        size_t currentSample{ 0 };
        size_t totalSamples{ 2400 };
    };

    static constexpr size_t MaxGrains = 8;

    AudioSlicerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { SidechainPinId, "Trigger In", PinType::AudioInput, PinDataType::AudioMono };
        pins_[3] = { AudioRateModPinId, "Position Mod In", PinType::AudioRateModulationInput, PinDataType::AudioRateSignal };

        params_[0] = { Freeze, "Freeze", 0.0f, 0.0f, 1.0f, false };
        params_[1] = { SliceCount, "Slices", 8.0f, 2.0f, 32.0f, false };
        params_[2] = { ActiveSlice, "Active Slice", -1.0f, -1.0f, 31.0f, false };
        params_[3] = { PlaybackSpeed, "Speed", 1.0f, -2.0f, 2.0f, true };
        params_[4] = { PositionScrub, "Position", 0.0f, 0.0f, 1.0f, true };
        params_[5] = { GrainSizeMs, "Grain Size", 80.0f, 10.0f, 500.0f, true };
        params_[6] = { GrainDensity, "Density", 4.0f, 1.0f, 8.0f, false };
        params_[7] = { Jitter, "Jitter", 0.15f, 0.0f, 1.0f, true };
        params_[8] = { PitchShiftSemitones, "Pitch", 0.0f, -24.0f, 24.0f, true };
        params_[9] = { FilterCutoff, "Cutoff", 18000.0f, 20.0f, 20000.0f, true };
        params_[10] = { Mix, "Mix", 0.85f, 0.0f, 1.0f, true };
    }

    NodeType getType() const noexcept override { return NodeType::AudioSlicer; }
    const char* getName() const noexcept override { return "Audio Slicer"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        bufferCapacity_ = static_cast<size_t>(spec.sampleRate * 3.0); // 3 segundos
        if (bufferCapacity_ < 1024) bufferCapacity_ = 1024;

        ringBuffer_[0].assign(bufferCapacity_, 0.0f);
        ringBuffer_[1].assign(bufferCapacity_, 0.0f);
        writeIndex_ = 0;
        playheadPos_ = 0.0;
        grainTimer_ = 0;

        for (auto& g : grains_) {
            g.active = false;
        }

        filter_[0].setLowpass(spec.sampleRate, 18000.0f, 0.707f);
        filter_[1].setLowpass(spec.sampleRate, 18000.0f, 0.707f);
    }

    void reset() noexcept override {
        std::fill(ringBuffer_[0].begin(), ringBuffer_[0].end(), 0.0f);
        std::fill(ringBuffer_[1].begin(), ringBuffer_[1].end(), 0.0f);
        writeIndex_ = 0;
        playheadPos_ = 0.0;
        grainTimer_ = 0;
        for (auto& g : grains_) {
            g.active = false;
        }
        filter_[0].reset();
        filter_[1].reset();
    }

    void setParameter(ParameterId id, float value) noexcept override {
        switch (id) {
            case Freeze: targetFreeze_ = value; break;
            case SliceCount: targetSliceCount_ = value; break;
            case ActiveSlice: targetActiveSlice_ = value; break;
            case PlaybackSpeed: targetSpeed_ = value; break;
            case PositionScrub: targetPosition_ = value; break;
            case GrainSizeMs: targetGrainSize_ = value; break;
            case GrainDensity: targetDensity_ = value; break;
            case Jitter: targetJitter_ = value; break;
            case PitchShiftSemitones: targetPitch_ = value; break;
            case FilterCutoff: targetCutoff_ = value; break;
            case Mix: targetMix_ = value; break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const noexcept override {
        switch (id) {
            case Freeze: return targetFreeze_;
            case SliceCount: return targetSliceCount_;
            case ActiveSlice: return targetActiveSlice_;
            case PlaybackSpeed: return targetSpeed_;
            case PositionScrub: return targetPosition_;
            case GrainSizeMs: return targetGrainSize_;
            case GrainDensity: return targetDensity_;
            case Jitter: return targetJitter_;
            case PitchShiftSemitones: return targetPitch_;
            case FilterCutoff: return targetCutoff_;
            case Mix: return targetMix_;
            default: return 0.0f;
        }
    }

    std::span<const PinDescriptor> getPins() const noexcept override { return pins_; }
    std::span<const ParameterInfo> getParameters() const noexcept override { return params_; }

    void process(ProcessContext& ctx) noexcept override {
        ScopedDenormalGuard denormalGuard; // Regla 47: Anti-denormals en hardware

        if (ctx.numSamples == 0 || bufferCapacity_ == 0) return;

        const float* inL = (ctx.numInputChannels > 0 && ctx.inputChannels[0]) ? ctx.inputChannels[0] : nullptr;
        const float* inR = (ctx.numInputChannels > 1 && ctx.inputChannels[1]) ? ctx.inputChannels[1] : inL;
        float* outL = (ctx.numOutputChannels > 0 && ctx.outputChannels[0]) ? ctx.outputChannels[0] : nullptr;
        float* outR = (ctx.numOutputChannels > 1 && ctx.outputChannels[1]) ? ctx.outputChannels[1] : outL;

        if (!outL) return;

        const bool isFrozen = (targetFreeze_ > 0.5f);
        const float mix = std::clamp(targetMix_, 0.0f, 1.0f);
        const int numSlices = std::clamp(static_cast<int>(targetSliceCount_), 2, 32);
        const float sliceLen = static_cast<float>(bufferCapacity_) / static_cast<float>(numSlices);

        // Actualizar filtro
        filter_[0].setLowpass(spec_.sampleRate, std::clamp(targetCutoff_, 20.0f, 20000.0f), 0.707f);
        filter_[1].setLowpass(spec_.sampleRate, std::clamp(targetCutoff_, 20.0f, 20000.0f), 0.707f);

        // Velocidad con pitch
        const float pitchRatio = std::pow(2.0f, targetPitch_ / 12.0f);
        const float effectiveSpeed = targetSpeed_ * pitchRatio;

        const size_t grainLengthSamples = static_cast<size_t>(std::clamp(targetGrainSize_ * 0.001f * static_cast<float>(spec_.sampleRate), 128.0f, static_cast<float>(bufferCapacity_ * 0.5)));
        const size_t maxActiveGrains = std::clamp(static_cast<size_t>(targetDensity_), size_t{ 1 }, MaxGrains);

        for (uint32_t s = 0; s < ctx.numSamples; ++s) {
            const float dryL = inL ? inL[s] : 0.0f;
            const float dryR = inR ? inR[s] : dryL;

            // 1. Escritura en ring buffer si NO está congelado
            if (!isFrozen) {
                ringBuffer_[0][writeIndex_] = dryL;
                ringBuffer_[1][writeIndex_] = dryR;
                writeIndex_ = (writeIndex_ + 1) % bufferCapacity_;
            }

            float wetL = 0.0f;
            float wetR = 0.0f;

            // 2. Modo Granular Frozen Playground vs Modo Slice Player
            if (isFrozen) {
                // Generador de granos sobre buffer congelado
                grainTimer_++;
                if (grainTimer_ >= (grainLengthSamples / (maxActiveGrains * 2 + 1))) {
                    grainTimer_ = 0;
                    // Buscar grano inactivo
                    for (size_t g = 0; g < maxActiveGrains; ++g) {
                        if (!grains_[g].active) {
                            grains_[g].active = true;
                            grains_[g].currentSample = 0;
                            grains_[g].totalSamples = grainLengthSamples;
                            grains_[g].playbackRate = effectiveSpeed;

                            // Posición base según scrubbing + jitter aleatorio
                            double basePos = targetPosition_ * static_cast<double>(bufferCapacity_);
                            double jitterOffset = (static_cast<double>(pseudoRandom(s + static_cast<uint32_t>(g))) - 0.5) * targetJitter_ * static_cast<double>(bufferCapacity_ * 0.25);
                            double startPos = std::fmod(basePos + jitterOffset + static_cast<double>(bufferCapacity_), static_cast<double>(bufferCapacity_));
                            grains_[g].bufferPos = startPos;
                            break;
                        }
                    }
                }

                // Acumular granos activos con ventana de Hann
                float grainSumL = 0.0f;
                float grainSumR = 0.0f;
                int activeCount = 0;

                for (size_t g = 0; g < maxActiveGrains; ++g) {
                    if (!grains_[g].active) continue;

                    auto& gr = grains_[g];
                    const float winPhase = static_cast<float>(gr.currentSample) / static_cast<float>(gr.totalSamples);
                    // Ventana Hann: 0.5 * (1 - cos(2*pi*phase))
                    const float win = 0.5f * (1.0f - std::cos(2.0f * std::numbers::pi_v<float> * winPhase));

                    // Interpolación lineal
                    const size_t idx0 = static_cast<size_t>(gr.bufferPos) % bufferCapacity_;
                    const size_t idx1 = (idx0 + 1) % bufferCapacity_;
                    const float frac = static_cast<float>(gr.bufferPos - std::floor(gr.bufferPos));

                    const float sL = ringBuffer_[0][idx0] + frac * (ringBuffer_[0][idx1] - ringBuffer_[0][idx0]);
                    const float sR = ringBuffer_[1][idx0] + frac * (ringBuffer_[1][idx1] - ringBuffer_[1][idx0]);

                    grainSumL += sL * win;
                    grainSumR += sR * win;
                    activeCount++;

                    // Avanzar grano
                    gr.bufferPos = std::fmod(gr.bufferPos + gr.playbackRate + static_cast<double>(bufferCapacity_), static_cast<double>(bufferCapacity_));
                    gr.currentSample++;
                    if (gr.currentSample >= gr.totalSamples) {
                        gr.active = false;
                    }
                }

                if (activeCount > 0) {
                    const float normGain = 1.0f / std::sqrt(static_cast<float>(activeCount));
                    wetL = grainSumL * normGain;
                    wetR = grainSumR * normGain;
                }
            } else {
                // Modo Rhythmic Slicer Player
                int sliceIdx = static_cast<int>(targetActiveSlice_);
                if (sliceIdx < 0) {
                    // Automático: la posición avanza con speed
                    playheadPos_ = std::fmod(playheadPos_ + effectiveSpeed + static_cast<double>(bufferCapacity_), static_cast<double>(bufferCapacity_));
                } else {
                    // Rebanada fija en bucle con scrubbing
                    const double sliceStart = static_cast<double>(sliceIdx) * sliceLen;
                    playheadPos_ = sliceStart + std::fmod(playheadPos_ - sliceStart + effectiveSpeed + sliceLen, sliceLen);
                }

                const size_t p0 = static_cast<size_t>(playheadPos_) % bufferCapacity_;
                const size_t p1 = (p0 + 1) % bufferCapacity_;
                const float frac = static_cast<float>(playheadPos_ - std::floor(playheadPos_));

                wetL = ringBuffer_[0][p0] + frac * (ringBuffer_[0][p1] - ringBuffer_[0][p0]);
                wetR = ringBuffer_[1][p0] + frac * (ringBuffer_[1][p1] - ringBuffer_[1][p0]);
            }

            // Filtrado del wet
            wetL = filter_[0].processSample(wetL);
            wetR = filter_[1].processSample(wetR);

            // Mezcla final Dry / Wet
            outL[s] = dryL * (1.0f - mix) + wetL * mix;
            if (outR) {
                outR[s] = dryR * (1.0f - mix) + wetR * mix;
            }
        }
    }

    void getWaveformThumbnail(std::vector<float>& outPoints, size_t numPoints) const {
        outPoints.assign(numPoints, 0.0f);
        if (bufferCapacity_ == 0 || numPoints == 0) return;

        const size_t step = bufferCapacity_ / numPoints;
        for (size_t p = 0; p < numPoints; ++p) {
            float peak = 0.0f;
            const size_t start = p * step;
            const size_t end = std::min(start + step, bufferCapacity_);
            for (size_t i = start; i < end; ++i) {
                float val = std::abs(ringBuffer_[0][i]);
                if (val > peak) peak = val;
            }
            outPoints[p] = std::clamp(peak, 0.0f, 1.0f);
        }
    }

private:
    static float pseudoRandom(uint32_t seed) noexcept {
        seed = (seed ^ 61) ^ (seed >> 16);
        seed += (seed << 3);
        seed ^= (seed >> 4);
        seed *= 0x27d4eb2d;
        seed ^= (seed >> 15);
        return static_cast<float>(seed & 0xffff) / 65535.0f;
    }

    ProcessSpec spec_{ 48000.0, 256, 2, 2 };
    size_t bufferCapacity_{ 144000 };
    std::array<std::vector<float>, 2> ringBuffer_;
    size_t writeIndex_{ 0 };
    double playheadPos_{ 0.0 };
    size_t grainTimer_{ 0 };

    std::array<GrainVoice, MaxGrains> grains_;
    std::array<BiquadFilter, 2> filter_;

    float targetFreeze_{ 0.0f };
    float targetSliceCount_{ 8.0f };
    float targetActiveSlice_{ -1.0f };
    float targetSpeed_{ 1.0f };
    float targetPosition_{ 0.0f };
    float targetGrainSize_{ 80.0f };
    float targetDensity_{ 4.0f };
    float targetJitter_{ 0.15f };
    float targetPitch_{ 0.0f };
    float targetCutoff_{ 18000.0f };
    float targetMix_{ 0.85f };

    std::array<PinDescriptor, 4> pins_;
    std::array<ParameterInfo, 11> params_;
};

inline AutoRegisterNode<AudioSlicerNode> registerAudioSlicer(NodeType::AudioSlicer, "audioslicer", "Glitch & Granular");

} // namespace audio_graph
