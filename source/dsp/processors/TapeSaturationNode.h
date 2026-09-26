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
#include "../core/BiquadFilter.h"
#include "../core/MagneticHysteresis.h"

namespace audio_graph {

/**
 * @brief Simulador de Cinta Magnética Analógica con Saturación, Head Bump y Wow/Flutter (Reglas 5, 8, 14, 34, 46, 47)
 */
class TapeSaturationNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Drive = 1,
        TapeSpeed = 2, // 0: 7.5 ips, 1: 15 ips, 2: 30 ips
        WowFlutter = 3,
        Warmth = 4,
        Mix = 5,
        Hysteresis = 6 // Intensidad de memoria magnética Jiles-Atherton (0.0 a 1.0)
    };

    TapeSaturationNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Drive, "Drive", 2.0f, 1.0f, 10.0f, true };
        params_[1] = { TapeSpeed, "Speed", 1.0f, 0.0f, 2.0f, false };
        params_[2] = { WowFlutter, "Flutter", 0.25f, 0.0f, 1.0f, true };
        params_[3] = { Warmth, "Warmth", 0.5f, 0.0f, 1.0f, true };
        params_[4] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
        params_[5] = { Hysteresis, "Hysteresis", 0.5f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        // Bounded delay buffer para wow/flutter: máximo 10 ms (Regla 47)
        maxDelaySamples_ = static_cast<size_t>(spec.sampleRate * 0.01);
        flutterBuffers_[0].assign(maxDelaySamples_, 0.0f);
        flutterBuffers_[1].assign(maxDelaySamples_, 0.0f);
        writeIndex_ = 0;
        flutterPhase1_ = 0.0f;
        flutterPhase2_ = 0.0f;

        updateFilters();
        updateHysteresis();
        reset();
    }

    void reset() override {
        for (auto& buf : flutterBuffers_) std::fill(buf.begin(), buf.end(), 0.0f);
        headBumpFilter_[0].reset();
        headBumpFilter_[1].reset();
        rolloffFilter_[0].reset();
        rolloffFilter_[1].reset();
        hysteresisModel_.reset();
        writeIndex_ = 0;
        flutterPhase1_ = 0.0f;
        flutterPhase2_ = 0.0f;
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        if (flutterBuffers_[0].empty()) return;

        updateFilters();

        const float flutterAmount = targetWowFlutter_;
        const float mix = targetMix_;

        // Wow: 0.8 Hz, Flutter: 4.5 Hz
        const float wowInc = (2.0f * std::numbers::pi_v<float> * 0.8f) / static_cast<float>(spec_.sampleRate);
        const float flutterInc = (2.0f * std::numbers::pi_v<float> * 4.5f) / static_cast<float>(spec_.sampleRate);

        const uint32_t channels = std::min(context.numOutputChannels, static_cast<uint32_t>(2));

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            // Modulación de deriva de velocidad
            const float wow = std::sin(flutterPhase1_);
            const float flutter = std::sin(flutterPhase2_);
            const float totalMod = (wow * 0.7f + flutter * 0.3f) * flutterAmount * 0.0015f; // Segundos de deriva
            const float delaySamples = std::max(1.0f, (0.003f + totalMod) * static_cast<float>(spec_.sampleRate));

            for (uint32_t ch = 0; ch < channels; ++ch) {
                const float in = (ch < context.numInputChannels && context.inputChannels[ch] != nullptr)
                    ? context.inputChannels[ch][s]
                    : 0.0f;

                // 1. Escribir en buffer de flutter
                flutterBuffers_[ch][writeIndex_] = in;
                float signal = readInterpolated(ch, delaySamples);

                // 2. Head bump de graves magnéticos (60-90 Hz)
                signal = headBumpFilter_[ch].processSample(signal);

                // 3. Saturación analógica de cinta con histéresis magnética no lineal (Reglas 5, 8, 34)
                signal = hysteresisModel_.processChannel(ch, signal);

                // 4. Pérdida de altas frecuencias en entrehierro (Tape gap loss rolloff)
                signal = rolloffFilter_[ch].processSample(signal);

                context.outputChannels[ch][s] = (1.0f - mix) * in + mix * signal;
            }

            writeIndex_ = (writeIndex_ + 1) % maxDelaySamples_;
            flutterPhase1_ += wowInc;
            flutterPhase2_ += flutterInc;
            if (flutterPhase1_ >= 2.0f * std::numbers::pi_v<float>) flutterPhase1_ -= 2.0f * std::numbers::pi_v<float>;
            if (flutterPhase2_ >= 2.0f * std::numbers::pi_v<float>) flutterPhase2_ -= 2.0f * std::numbers::pi_v<float>;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Drive:
                targetDrive_ = std::clamp(value, 1.0f, 10.0f);
                updateHysteresis();
                break;
            case TapeSpeed:
                targetSpeed_ = std::clamp(static_cast<int>(std::round(value)), 0, 2);
                updateHysteresis();
                break;
            case WowFlutter:
                targetWowFlutter_ = std::clamp(value, 0.0f, 1.0f);
                break;
            case Warmth:
                targetWarmth_ = std::clamp(value, 0.0f, 1.0f);
                break;
            case Mix:
                targetMix_ = std::clamp(value, 0.0f, 1.0f);
                break;
            case Hysteresis:
                targetHysteresis_ = std::clamp(value, 0.0f, 1.0f);
                updateHysteresis();
                break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Drive: return targetDrive_;
            case TapeSpeed: return static_cast<float>(targetSpeed_);
            case WowFlutter: return targetWowFlutter_;
            case Warmth: return targetWarmth_;
            case Mix: return targetMix_;
            case Hysteresis: return targetHysteresis_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Tape; }
    const char* getName() const override { return "Tape"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateFilters() noexcept {
        float bumpFreq = 65.0f;
        float rolloffFreq = 16000.0f;

        if (targetSpeed_ == 0) { // 7.5 ips: más cálido, más rolloff
            bumpFreq = 55.0f;
            rolloffFreq = 11000.0f - targetWarmth_ * 3000.0f;
        } else if (targetSpeed_ == 1) { // 15 ips: estándar estudio
            bumpFreq = 70.0f;
            rolloffFreq = 15000.0f - targetWarmth_ * 3000.0f;
        } else { // 30 ips: alta fidelidad
            bumpFreq = 85.0f;
            rolloffFreq = 19000.0f - targetWarmth_ * 2000.0f;
        }

        const float bumpGainDb = 1.5f + targetWarmth_ * 2.5f;

        for (size_t ch = 0; ch < 2; ++ch) {
            headBumpFilter_[ch].setCoefficients(BiquadFilter::Type::Peak, spec_.sampleRate, bumpFreq, 1.2f, bumpGainDb);
            rolloffFilter_[ch].setCoefficients(BiquadFilter::Type::Lowpass, spec_.sampleRate, rolloffFreq, 0.707f);
        }
    }

    void updateHysteresis() noexcept {
        const float speedFactor = (targetSpeed_ == 0) ? 0.5f : ((targetSpeed_ == 1) ? 1.0f : 1.5f);
        hysteresisModel_.setParameters(targetDrive_, targetHysteresis_, speedFactor);
    }

    float readInterpolated(size_t ch, float delaySamples) const noexcept {
        const float readPos = static_cast<float>(writeIndex_) - delaySamples;
        float wrappedPos = std::fmod(readPos, static_cast<float>(maxDelaySamples_));
        if (wrappedPos < 0.0f) wrappedPos += static_cast<float>(maxDelaySamples_);

        const size_t idx0 = static_cast<size_t>(wrappedPos);
        const size_t idx1 = (idx0 + 1) % maxDelaySamples_;
        const float frac = wrappedPos - static_cast<float>(idx0);

        return (1.0f - frac) * flutterBuffers_[ch][idx0] + frac * flutterBuffers_[ch][idx1];
    }

    ProcessSpec spec_;
    size_t maxDelaySamples_{ 960 };
    std::array<std::vector<float>, 2> flutterBuffers_;
    size_t writeIndex_{ 0 };
    float flutterPhase1_{ 0.0f };
    float flutterPhase2_{ 0.0f };

    std::array<BiquadFilter, 2> headBumpFilter_;
    std::array<BiquadFilter, 2> rolloffFilter_;
    MagneticHysteresis hysteresisModel_;

    float targetDrive_{ 2.0f };
    int targetSpeed_{ 1 };
    float targetWowFlutter_{ 0.25f };
    float targetWarmth_{ 0.5f };
    float targetMix_{ 1.0f };
    float targetHysteresis_{ 0.5f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<TapeSaturationNode> registerTape(NodeType::Tape, "tape", "Distortion");

} // namespace audio_graph
