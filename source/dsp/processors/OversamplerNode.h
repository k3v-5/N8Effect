#pragma once

#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include "../../core/Types.h"
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/PolyphaseOversampler.h"
#include "../core/BiquadFilter.h"
#include "../core/FastMath.h"

namespace audio_graph {

/**
 * @brief Nodo de Acondicionamiento y Saturación con Sobremuestreo Polifásico (OversamplerNode)
 * Permite procesar con sobremuestreo 2x, 4x u 8x y saturación hiper-limpia libre de aliasing
 * con compensación de fase (PDC) (Reglas 5, 8, 9, 14, 34, 46, 47).
 */
class OversamplerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Factor = 1,       // 0: 1x (Bypass), 1: 2x, 2: 4x, 3: 8x
        DriveDb = 2,      // 0.0 a 24.0 dB de empuje armónico
        Saturation = 3,   // 0: Off, 1: Soft Tape, 2: Hard Clip, 3: Tube Wavefold
        LowCutHz = 4,     // 20.0 a 500.0 Hz
        HighCutHz = 5,    // 2000.0 a 20000.0 Hz
        Mix = 6           // 0.0 a 1.0 (Dry / Wet)
    };

    OversamplerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Factor, "Factor", 1.0f, 0.0f, 3.0f, false };
        params_[1] = { DriveDb, "Drive dB", 0.0f, 0.0f, 24.0f, true };
        params_[2] = { Saturation, "Mode", 1.0f, 0.0f, 3.0f, false };
        params_[3] = { LowCutHz, "Low Cut", 30.0f, 20.0f, 500.0f, true };
        params_[4] = { HighCutHz, "High Cut", 18000.0f, 2000.0f, 20000.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        oversampler_.prepare(spec.maximumBlockSize, PolyphaseOversampler::Factor2x);

        for (size_t ch = 0; ch < 2; ++ch) {
            lowCut_[ch].setHighpass(spec.sampleRate, 30.0f, 0.707f);
            highCut_[ch].setLowpass(spec.sampleRate, 18000.0f, 0.707f);
        }

        // Buffer temporal para procesamiento a tasa sobremuestreada
        oversampledBufferL_.assign(spec.maximumBlockSize * 8, 0.0f);
        oversampledBufferR_.assign(spec.maximumBlockSize * 8, 0.0f);
    }

    void reset() noexcept override {
        oversampler_.reset();
        for (size_t ch = 0; ch < 2; ++ch) {
            lowCut_[ch].reset();
            highCut_[ch].reset();
        }
    }

    void process(ProcessContext& ctx) noexcept override {
        if (ctx.numSamples == 0 || ctx.outputChannels == nullptr) return;

        // Configurar factor de sobremuestreo
        const int fInt = static_cast<int>(std::round(targetFactor_));
        PolyphaseOversampler::Factor f = PolyphaseOversampler::Factor1x;
        if (fInt == 1) f = PolyphaseOversampler::Factor2x;
        else if (fInt == 2) f = PolyphaseOversampler::Factor4x;
        else if (fInt == 3) f = PolyphaseOversampler::Factor8x;
        oversampler_.setFactor(f);

        // Actualizar filtros
        if (std::abs(lastLowCut_ - targetLowCut_) > 1.0f) {
            lastLowCut_ = targetLowCut_;
            for (size_t ch = 0; ch < 2; ++ch) lowCut_[ch].setHighpass(spec_.sampleRate, targetLowCut_, 0.707f);
        }
        if (std::abs(lastHighCut_ - targetHighCut_) > 5.0f) {
            lastHighCut_ = targetHighCut_;
            for (size_t ch = 0; ch < 2; ++ch) highCut_[ch].setLowpass(spec_.sampleRate, targetHighCut_, 0.707f);
        }

        const float inDrive = std::pow(10.0f, targetDriveDb_ / 20.0f);
        const int satMode = static_cast<int>(std::round(targetSatMode_));
        const float mix = targetMix_;

        const float* inL = (ctx.numInputChannels > 0 && ctx.inputChannels[0]) ? ctx.inputChannels[0] : nullptr;
        const float* inR = (ctx.numInputChannels > 1 && ctx.inputChannels[1]) ? ctx.inputChannels[1] : inL;

        // 1. Sobremuestrear audio entrante (1x -> 2x/4x/8x)
        auto [overPointers, numOverSamples] = oversampler_.processUpsample(inL, inR, ctx.numSamples);

        // Copiar a buffers de trabajo modificables
        if (overPointers[0] && overPointers[1]) {
            std::copy_n(overPointers[0], numOverSamples, oversampledBufferL_.data());
            std::copy_n(overPointers[1], numOverSamples, oversampledBufferR_.data());
        }

        // 2. Procesar saturación no lineal a la tasa sobremuestreada (cero aliasing)
        if (satMode > 0 || inDrive > 1.01f) {
            for (size_t s = 0; s < numOverSamples; ++s) {
                float l = oversampledBufferL_[s] * inDrive;
                float r = oversampledBufferR_[s] * inDrive;

                if (satMode == 1) { // Soft Tape
                    l = FastMath::fastTanh(l);
                    r = FastMath::fastTanh(r);
                } else if (satMode == 2) { // Hard Clip
                    l = std::clamp(l, -1.0f, 1.0f);
                    r = std::clamp(r, -1.0f, 1.0f);
                } else if (satMode == 3) { // Tube Wavefold
                    float foldedL = std::sin(l * 1.5707963f);
                    float foldedR = std::sin(r * 1.5707963f);
                    l = FastMath::fastTanh(foldedL);
                    r = FastMath::fastTanh(foldedR);
                }

                // Normalizar ganancia inversa
                float outGain = (inDrive > 1.0f) ? (1.0f / std::sqrt(inDrive)) : 1.0f;
                oversampledBufferL_[s] = FastMath::flushDenormal(l * outGain);
                oversampledBufferR_[s] = FastMath::flushDenormal(r * outGain);
            }
        }

        // 3. Diezmar y filtrar con filtro halfband de alta atenuación (>90dB)
        oversampler_.processDownsample(oversampledBufferL_.data(), oversampledBufferR_.data(),
                                      numOverSamples,
                                      ctx.outputChannels[0],
                                      (ctx.numOutputChannels > 1) ? ctx.outputChannels[1] : nullptr,
                                      ctx.numSamples);

        // 4. Filtrado post y mezcla Dry/Wet
        for (size_t s = 0; s < ctx.numSamples; ++s) {
            float wetL = ctx.outputChannels[0][s];
            float wetR = (ctx.numOutputChannels > 1 && ctx.outputChannels[1]) ? ctx.outputChannels[1][s] : wetL;

            wetL = lowCut_[0].process(wetL);
            wetL = highCut_[0].process(wetL);
            wetR = lowCut_[1].process(wetR);
            wetR = highCut_[1].process(wetR);

            float rawInL = inL ? inL[s] : 0.0f;
            float rawInR = inR ? inR[s] : rawInL;

            ctx.outputChannels[0][s] = rawInL * (1.0f - mix) + wetL * mix;
            if (ctx.numOutputChannels > 1 && ctx.outputChannels[1]) {
                ctx.outputChannels[1][s] = rawInR * (1.0f - mix) + wetR * mix;
            }
        }
    }

    std::span<const PinDescriptor> getPins() const noexcept override { return pins_; }
    std::span<const ParameterInfo> getParameters() const noexcept override { return params_; }

    NodeType getType() const noexcept override { return NodeType::Oversampler; }
    const char* getName() const noexcept override { return "Oversampler"; }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Factor: return targetFactor_;
            case DriveDb: return targetDriveDb_;
            case Saturation: return targetSatMode_;
            case LowCutHz: return targetLowCut_;
            case HighCutHz: return targetHighCut_;
            case Mix: return targetMix_;
            default: return 0.0f;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Factor: targetFactor_ = std::clamp(value, 0.0f, 3.0f); break;
            case DriveDb: targetDriveDb_ = std::clamp(value, 0.0f, 24.0f); break;
            case Saturation: targetSatMode_ = std::clamp(value, 0.0f, 3.0f); break;
            case LowCutHz: targetLowCut_ = std::clamp(value, 20.0f, 500.0f); break;
            case HighCutHz: targetHighCut_ = std::clamp(value, 2000.0f, 20000.0f); break;
            case Mix: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    size_t getPdcLatencySamples() const noexcept {
        return oversampler_.getLatencyInSamples();
    }

private:
    ProcessSpec spec_{ 48000.0, 256, 2, 2 };
    PolyphaseOversampler oversampler_;

    std::array<BiquadFilter, 2> lowCut_;
    std::array<BiquadFilter, 2> highCut_;
    float lastLowCut_{ 30.0f };
    float lastHighCut_{ 18000.0f };

    std::vector<float> oversampledBufferL_;
    std::vector<float> oversampledBufferR_;

    float targetFactor_{ 1.0f }; // 2x por defecto
    float targetDriveDb_{ 0.0f };
    float targetSatMode_{ 1.0f };
    float targetLowCut_{ 30.0f };
    float targetHighCut_{ 18000.0f };
    float targetMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<OversamplerNode> registerOversampler(NodeType::Oversampler, "oversampler", "Dynamics");

} // namespace audio_graph
