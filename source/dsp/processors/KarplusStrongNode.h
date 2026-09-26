#pragma once

#include <cmath>
#include <array>
#include <vector>
#include <span>
#include <algorithm>
#include <numbers>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../core/FastMath.h"
#include "../core/AcousticBodyResonator.h"

namespace audio_graph {

/**
 * @brief Resonador de Modelado Físico de Cuerda Pulsada (Karplus-Strong)
 * Línea de retardo sintonizada fraccional con filtrado de amortiguamiento en lazo (Reglas 5, 8, 14, 34, 46, 47)
 */
class KarplusStrongNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Pitch = 1,        // Frecuencia fundamental en Hz (25 Hz a 2000 Hz)
        Damping = 2,      // Amortiguamiento de alta frecuencia (0.01 a 0.99)
        Decay = 3,        // Duración de decaimiento en segundos (0.1s a 5.0s)
        PickPosition = 4, // Posición de ataque en la cuerda (0.05 a 0.95)
        AudioTrigger = 5, // 0: Resonador Continuo, 1: Disparo por Golpes de Audio
        Mix = 6,          // Dry / Wet
        BodySize = 7,     // Escala de tamaño del cuerpo acústico (0.5 a 2.0)
        BodyDecay = 8,    // Resonancia / Q de la madera (0.1 a 3.0)
        BodyMix = 9       // Nivel de caja acústica (0.0 a 1.0)
    };

    KarplusStrongNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Pitch, "Pitch Hz", 220.0f, 25.0f, 2000.0f, true };
        params_[1] = { Damping, "Damping", 0.5f, 0.01f, 0.99f, true };
        params_[2] = { Decay, "Decay Time", 1.5f, 0.1f, 5.0f, true };
        params_[3] = { PickPosition, "Pick Pos", 0.25f, 0.05f, 0.95f, true };
        params_[4] = { AudioTrigger, "Trigger Mode", 1.0f, 0.0f, 1.0f, false };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
        params_[6] = { BodySize, "Body Size", 1.0f, 0.5f, 2.0f, true };
        params_[7] = { BodyDecay, "Body Decay", 1.0f, 0.1f, 3.0f, true };
        params_[8] = { BodyMix, "Body Mix", 0.4f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        // Bounded delay buffer: longitud máxima a 20 Hz y 192 kHz (~9600 muestras) (Regla 47)
        maxDelaySamples_ = std::max<size_t>(static_cast<size_t>((spec.sampleRate > 0 ? spec.sampleRate : 44100.0) / 20.0) + 16, 1024);
        for (auto& buf : delayBuffers_) {
            buf.assign(maxDelaySamples_, 0.0f);
        }
        writeIdx_ = 0;
        rngState_ = 0x98765432u;
        prevFeedbackSample_.fill(0.0f);
        envFollower_ = 0.0f;
        burstCounter_ = 0;

        bodyResonator_.prepare(spec.sampleRate);
        bodyResonator_.setParameters(targetBodySize_, targetBodyDecay_, targetBodyMix_);

        reset();
    }

    void reset() override {
        for (auto& buf : delayBuffers_) std::fill(buf.begin(), buf.end(), 0.0f);
        writeIdx_ = 0;
        prevFeedbackSample_.fill(0.0f);
        envFollower_ = 0.0f;
        burstCounter_ = 0;
        bodyResonator_.reset();
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Pitch:        targetPitch_ = std::clamp(value, 25.0f, 2000.0f); break;
            case Damping:      targetDamping_ = std::clamp(value, 0.01f, 0.99f); break;
            case Decay:        targetDecay_ = std::clamp(value, 0.1f, 5.0f); break;
            case PickPosition: targetPick_ = std::clamp(value, 0.05f, 0.95f); break;
            case AudioTrigger: targetTriggerMode_ = (value >= 0.5f); break;
            case Mix:          targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            case BodySize:
                targetBodySize_ = std::clamp(value, 0.5f, 2.0f);
                bodyResonator_.setParameters(targetBodySize_, targetBodyDecay_, targetBodyMix_);
                break;
            case BodyDecay:
                targetBodyDecay_ = std::clamp(value, 0.1f, 3.0f);
                bodyResonator_.setParameters(targetBodySize_, targetBodyDecay_, targetBodyMix_);
                break;
            case BodyMix:
                targetBodyMix_ = std::clamp(value, 0.0f, 1.0f);
                bodyResonator_.setParameters(targetBodySize_, targetBodyDecay_, targetBodyMix_);
                break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Pitch:        return targetPitch_;
            case Damping:      return targetDamping_;
            case Decay:        return targetDecay_;
            case PickPosition: return targetPick_;
            case AudioTrigger: return targetTriggerMode_ ? 1.0f : 0.0f;
            case Mix:          return targetMix_;
            case BodySize:     return targetBodySize_;
            case BodyDecay:    return targetBodyDecay_;
            case BodyMix:      return targetBodyMix_;
            default:           return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::KarplusStrong; }
    const char* getName() const override { return "Karplus-Strong"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47

        if (context.numSamples == 0 || maxDelaySamples_ == 0) return;

        const size_t numSamples = context.numSamples;
        const size_t numChannels = std::min<size_t>(context.numOutputChannels, 2);

        const float* inL = (context.numInputChannels > 0) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : inL;
        float* outL = (numChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (numChannels > 1) ? context.outputChannels[1] : nullptr;

        const double sampleRate = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        const double fundamental = std::clamp(static_cast<double>(targetPitch_), 25.0, sampleRate * 0.45);

        // Longitud fraccional exacta de retardo compensando retardo de fase del filtro pasa-bajos (-0.5 muestras)
        const double delayLength = (sampleRate / fundamental) - 0.5;

        // Coeficiente de pérdida en el lazo según el tiempo de decaimiento
        const double loopGain = std::pow(0.001, 1.0 / (static_cast<double>(targetDecay_) * fundamental));
        const float feedbackGain = static_cast<float>(std::clamp(loopGain, 0.85, 0.9995));

        const float damping = targetDamping_;
        const float pickPos = targetPick_;
        const bool triggerMode = targetTriggerMode_;
        const float mix = targetMix_;

        for (size_t s = 0; s < numSamples; ++s) {
            const float rawInL = inL ? inL[s] : 0.0f;
            const float rawInR = inR ? inR[s] : rawInL;
            const float inMono = 0.5f * (rawInL + rawInR);

            float excitation = 0.0f;

            if (triggerMode) {
                // Detección de transiente para excitación de cuerda pulsada
                const float inMag = std::abs(inMono);
                if (inMag > envFollower_ + 0.12f && burstCounter_ == 0) {
                    burstCounter_ = static_cast<int>(delayLength * 0.75); // Ráfaga de ruido durante ~1 ciclo
                }
                envFollower_ = 0.98f * envFollower_ + 0.02f * inMag;

                if (burstCounter_ > 0) {
                    --burstCounter_;
                    excitation = nextNoise() * 0.8f;
                }
            } else {
                // Modo resonador continuo: el audio entrante excita el lazo directamente
                excitation = inMono * 0.6f;
            }

            // Simulación de posición de púa (Pick Position Comb Filter)
            const double pickDelay = delayLength * static_cast<double>(pickPos);
            float shapedExcitationL = excitation - 0.9f * readHermite(0, pickDelay);
            float shapedExcitationR = excitation - 0.9f * readHermite(1, pickDelay);

            // Lectura del final de la cuerda (bucle Karplus-Strong)
            const float delayOutL = readHermite(0, delayLength);
            const float delayOutR = readHermite(1, delayLength);

            // Filtro pasa-bajos de amortiguamiento en lazo (Lowpass Damping Filter)
            const float filteredL = (1.0f - damping) * delayOutL + damping * prevFeedbackSample_[0];
            const float filteredR = (1.0f - damping) * delayOutR + damping * prevFeedbackSample_[1];
            prevFeedbackSample_[0] = delayOutL;
            prevFeedbackSample_[1] = delayOutR;

            // Retroalimentación con contención no lineal suave
            const float nextLoopL = FastMath::fastTanh(shapedExcitationL + filteredL * feedbackGain);
            const float nextLoopR = FastMath::fastTanh(shapedExcitationR + filteredR * feedbackGain);

            delayBuffers_[0][writeIdx_] = nextLoopL;
            delayBuffers_[1][writeIdx_] = nextLoopR;

            writeIdx_ = (writeIdx_ + 1) % maxDelaySamples_;

            const float rawWetL = delayOutL * 1.4f;
            const float rawWetR = delayOutR * 1.4f;
            float wetL = 0.0f, wetR = 0.0f;
            bodyResonator_.processSample(rawWetL, rawWetR, wetL, wetR);

            if (outL) outL[s] = (1.0f - mix) * rawInL + mix * wetL;
            if (outR) outR[s] = (1.0f - mix) * rawInR + mix * wetR;
        }
    }

private:
    float nextNoise() noexcept {
        rngState_ ^= (rngState_ << 13);
        rngState_ ^= (rngState_ >> 17);
        rngState_ ^= (rngState_ << 5);
        return (static_cast<float>(rngState_ & 0x7FFFFFFF) / 1073741824.0f) - 1.0f;
    }

    float readHermite(size_t ch, double delaySamples) const noexcept {
        const double readPos = static_cast<double>(writeIdx_) - delaySamples;
        double wrappedPos = std::fmod(readPos, static_cast<double>(maxDelaySamples_));
        if (wrappedPos < 0.0) wrappedPos += static_cast<double>(maxDelaySamples_);

        const size_t i1 = static_cast<size_t>(wrappedPos);
        const size_t i0 = (i1 + maxDelaySamples_ - 1) % maxDelaySamples_;
        const size_t i2 = (i1 + 1) % maxDelaySamples_;
        const size_t i3 = (i1 + 2) % maxDelaySamples_;

        const float frac = static_cast<float>(wrappedPos - static_cast<double>(i1));

        const float y0 = delayBuffers_[ch][i0];
        const float y1 = delayBuffers_[ch][i1];
        const float y2 = delayBuffers_[ch][i2];
        const float y3 = delayBuffers_[ch][i3];

        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    ProcessSpec spec_;
    size_t maxDelaySamples_{ 4096 };
    std::array<std::vector<float>, 2> delayBuffers_;
    size_t writeIdx_{ 0 };

    std::array<float, 2> prevFeedbackSample_{ 0.0f, 0.0f };
    uint32_t rngState_{ 0x98765432u };
    float envFollower_{ 0.0f };
    int burstCounter_{ 0 };

    float targetPitch_{ 220.0f };
    float targetDamping_{ 0.5f };
    float targetDecay_{ 1.5f };
    float targetPick_{ 0.25f };
    bool targetTriggerMode_{ true };
    float targetMix_{ 1.0f };
    float targetBodySize_{ 1.0f };
    float targetBodyDecay_{ 1.0f };
    float targetBodyMix_{ 0.4f };

    AcousticBodyResonator bodyResonator_;

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 9> params_;
};

inline AutoRegisterNode<KarplusStrongNode> registerKarplus(NodeType::KarplusStrong, "karplus_strong", "Resonance");

} // namespace audio_graph
