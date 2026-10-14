#pragma once

#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <span>
#include "../../core/Types.h"
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Reproductor de Muestras de Audio Polifónico / Monofónico (Nodo DSP #53).
 * Soporta carga en caliente de archivos WAV/AIF, resampleo transparente, transposición por semitonos,
 * modos One-Shot y Bucle continuo, control de envolvente de ataque/liberación y forma de onda en miniatura.
 * Cumple con Reglas 5, 8, 9, 14, 34, 46, 47 (Zero-allocations en audio thread, Denormals Are Zero, Anti-click).
 * Delegado al motor de alto rendimiento en Rust (n8_rust_sample_player).
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
        rustNode_ = n8_rust_sample_player_create();
    }

    ~SamplePlayerNode() override {
        if (rustNode_) {
            n8_rust_sample_player_destroy(rustNode_);
            rustNode_ = nullptr;
        }
    }

    SamplePlayerNode(const SamplePlayerNode&) = delete;
    SamplePlayerNode& operator=(const SamplePlayerNode&) = delete;

    SamplePlayerNode(SamplePlayerNode&& other) noexcept
        : pins_(other.pins_),
          params_(other.params_),
          spec_(other.spec_),
          thumbnail_(other.thumbnail_),
          targetPitch_(other.targetPitch_),
          targetRootNote_(other.targetRootNote_),
          targetSpeed_(other.targetSpeed_),
          targetLoop_(other.targetLoop_),
          targetAttack_(other.targetAttack_),
          targetRelease_(other.targetRelease_),
          targetGain_(other.targetGain_),
          targetMix_(other.targetMix_),
          rustNode_(other.rustNode_) {
        other.rustNode_ = nullptr;
    }

    SamplePlayerNode& operator=(SamplePlayerNode&& other) noexcept {
        if (this != &other) {
            if (rustNode_) {
                n8_rust_sample_player_destroy(rustNode_);
            }
            pins_ = other.pins_;
            params_ = other.params_;
            spec_ = other.spec_;
            thumbnail_ = other.thumbnail_;
            targetPitch_ = other.targetPitch_;
            targetRootNote_ = other.targetRootNote_;
            targetSpeed_ = other.targetSpeed_;
            targetLoop_ = other.targetLoop_;
            targetAttack_ = other.targetAttack_;
            targetRelease_ = other.targetRelease_;
            targetGain_ = other.targetGain_;
            targetMix_ = other.targetMix_;
            rustNode_ = other.rustNode_;
            other.rustNode_ = nullptr;
        }
        return *this;
    }

    NodeType getType() const noexcept override { return NodeType::SamplePlayer; }
    const char* getName() const noexcept override { return "Sample Player"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustNode_) {
            n8_rust_sample_player_prepare(rustNode_, spec.sampleRate);
        }
    }

    void reset() noexcept override {
        if (rustNode_) {
            n8_rust_sample_player_reset(rustNode_);
        }
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
        if (rustNode_) {
            n8_rust_sample_player_set_parameter(rustNode_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const noexcept override {
        if (rustNode_) {
            return n8_rust_sample_player_get_parameter(rustNode_, static_cast<uint32_t>(id));
        }
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
        if (rustNode_) {
            n8_rust_sample_player_load_sample(rustNode_, lData, rData, numSamples, sourceSampleRate);
            n8_rust_sample_player_get_thumbnail(rustNode_, thumbnail_.data());
        }
    }

    const std::array<float, 64>& getThumbnail() const noexcept {
        if (rustNode_) {
            n8_rust_sample_player_get_thumbnail(rustNode_, const_cast<float*>(thumbnail_.data()));
        }
        return thumbnail_;
    }

    void triggerPlayback() noexcept {
        if (rustNode_) {
            n8_rust_sample_player_trigger(rustNode_);
        }
    }

    void process(ProcessContext& ctx) noexcept override {
        if (!rustNode_ || ctx.numSamples == 0 || ctx.outputChannels == nullptr) return;

        const float* inL = (ctx.numInputChannels > 0 && ctx.inputChannels[0]) ? ctx.inputChannels[0] : nullptr;
        const float* inR = (ctx.numInputChannels > 1 && ctx.inputChannels[1]) ? ctx.inputChannels[1] : inL;
        float* outL = (ctx.numOutputChannels > 0 && ctx.outputChannels[0]) ? ctx.outputChannels[0] : nullptr;
        float* outR = (ctx.numOutputChannels > 1 && ctx.outputChannels[1]) ? ctx.outputChannels[1] : outL;

        if (!outL) return;

        // Detección de trigger por pin de modulación / sidechain
        const float* triggerSig = (ctx.numSidechainChannels > 0 && ctx.sidechainChannels != nullptr) ? ctx.sidechainChannels[0] : nullptr;

        n8_rust_sample_player_process(rustNode_, inL, inR, triggerSig, outL, outR, ctx.numSamples);
    }

private:
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
    mutable std::array<float, 64> thumbnail_{};

    RustSamplePlayer* rustNode_{ nullptr };
};

inline AutoRegisterNode<SamplePlayerNode> registerSamplePlayer(NodeType::SamplePlayer, "Sample Player", "Synth & Sampler");

} // namespace audio_graph
