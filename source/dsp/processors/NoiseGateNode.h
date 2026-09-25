#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/BiquadFilter.h"
#include "../core/EnvelopeDetector.h"
#include "../core/DenormalGuards.h"

namespace audio_graph {

/**
 * @brief Puerta de Ruido y Expansor Hacia Abajo Profesional (Reglas 5, 8, 9, 14, 34, 46, 47)
 * Proporciona supresión de ruido y aislamiento de transitorios con histéresis anti-chatter,
 * temporizador de retención (Hold), envolvente suave Attack/Release, control de rango (Range)
 * y filtro pasa-altos en la cadena de detección sidechain.
 */
class NoiseGateNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Threshold = 1,      // -80.0 a 0.0 dB (Umbral de apertura)
        Hysteresis = 2,     // 0.0 a 12.0 dB (Diferencia de cierre para evitar oscilaciones)
        Attack = 3,         // 0.05 a 50.0 ms (Tiempo de apertura de la compuerta)
        Hold = 4,           // 0.0 a 500.0 ms (Tiempo de retención en apertura)
        Release = 5,        // 5.0 a 1000.0 ms (Tiempo de cierre gradual)
        Range = 6,          // -80.0 a 0.0 dB (Piso de atenuación máxima)
        SidechainHPF = 7    // 20.0 a 1000.0 Hz (Filtro pasa-altos en la detección)
    };

    NoiseGateNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { SidechainPinId, "Sidechain", PinType::AudioInput, PinDataType::AudioStereo };

        params_[0] = { Threshold, "Threshold", -40.0f, -80.0f, 0.0f, true };
        params_[1] = { Hysteresis, "Hysteresis", 4.0f, 0.0f, 12.0f, true };
        params_[2] = { Attack, "Attack", 1.0f, 0.05f, 50.0f, true };
        params_[3] = { Hold, "Hold", 25.0f, 0.0f, 500.0f, true };
        params_[4] = { Release, "Release", 80.0f, 5.0f, 1000.0f, true };
        params_[5] = { Range, "Range", -80.0f, -80.0f, 0.0f, true };
        params_[6] = { SidechainHPF, "SC Filter", 80.0f, 20.0f, 1000.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        currentGain_ = 0.0f;
        holdCounter_ = 0;
        gateOpen_ = false;

        sidechainFilter_[0].reset();
        sidechainFilter_[1].reset();

        updateFilters();
        updateCoefficients();
    }

    void reset() override {
        currentGain_ = 0.0f;
        holdCounter_ = 0;
        gateOpen_ = false;
        sidechainFilter_[0].reset();
        sidechainFilter_[1].reset();
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const float openThresholdLin = EnvelopeDetector::dbToLinear(targetThreshold_);
        const float closeThresholdLin = EnvelopeDetector::dbToLinear(targetThreshold_ - targetHysteresis_);
        const float rangeFloorLin = EnvelopeDetector::dbToLinear(targetRange_);

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0]) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1]) ? context.inputChannels[1] : inL;

        const float* scLPtr = (context.numSidechainChannels > 0 && context.sidechainChannels != nullptr && context.sidechainChannels[0] != nullptr)
            ? context.sidechainChannels[0] : inL;
        const float* scRPtr = (context.numSidechainChannels > 1 && context.sidechainChannels != nullptr && context.sidechainChannels[1] != nullptr)
            ? context.sidechainChannels[1] : scLPtr;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0]) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1]) ? context.outputChannels[1] : nullptr;

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            const float rawInL = inL ? inL[s] : 0.0f;
            const float rawInR = inR ? inR[s] : 0.0f;

            const float rawScL = scLPtr ? scLPtr[s] : rawInL;
            const float rawScR = scRPtr ? scRPtr[s] : rawInR;

            // 1. Detección Sidechain filtrada por HPF (Reglas 6, 13 y 34)
            const float filteredL = sidechainFilter_[0].processSample(rawScL);
            const float filteredR = sidechainFilter_[1].processSample(rawScR);
            const float level = std::max(std::abs(filteredL), std::abs(filteredR));

            // 2. Máquina de estados de la compuerta con Histéresis y Hold
            if (level >= openThresholdLin) {
                gateOpen_ = true;
                holdCounter_ = holdSamples_;
            } else if (level < closeThresholdLin) {
                if (holdCounter_ > 0) {
                    --holdCounter_;
                } else {
                    gateOpen_ = false;
                }
            }

            // 3. Suavizado balístico del coeficiente de ganancia
            const float targetGain = gateOpen_ ? 1.0f : rangeFloorLin;
            if (targetGain > currentGain_) {
                currentGain_ += attackCoeff_ * (targetGain - currentGain_);
            } else {
                currentGain_ += releaseCoeff_ * (targetGain - currentGain_);
            }

            // 4. Aplicación de ganancia a la señal limpia
            if (outL) outL[s] = rawInL * currentGain_;
            if (outR) outR[s] = rawInR * currentGain_;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Threshold: targetThreshold_ = std::clamp(value, -80.0f, 0.0f); break;
            case Hysteresis: targetHysteresis_ = std::clamp(value, 0.0f, 12.0f); break;
            case Attack: {
                targetAttack_ = std::clamp(value, 0.05f, 50.0f);
                updateCoefficients();
                break;
            }
            case Hold: {
                targetHold_ = std::clamp(value, 0.0f, 500.0f);
                updateCoefficients();
                break;
            }
            case Release: {
                targetRelease_ = std::clamp(value, 5.0f, 1000.0f);
                updateCoefficients();
                break;
            }
            case Range: targetRange_ = std::clamp(value, -80.0f, 0.0f); break;
            case SidechainHPF: {
                targetSidechainHPF_ = std::clamp(value, 20.0f, 1000.0f);
                updateFilters();
                break;
            }
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Threshold: return targetThreshold_;
            case Hysteresis: return targetHysteresis_;
            case Attack: return targetAttack_;
            case Hold: return targetHold_;
            case Release: return targetRelease_;
            case Range: return targetRange_;
            case SidechainHPF: return targetSidechainHPF_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::NoiseGate; }
    const char* getName() const override { return "Noise Gate"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateFilters() noexcept {
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        for (auto& f : sidechainFilter_) {
            f.setCoefficients(BiquadFilter::Type::Highpass, sr, targetSidechainHPF_, 0.707f);
        }
    }

    void updateCoefficients() noexcept {
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        const float attSamples = (targetAttack_ * 0.001f) * static_cast<float>(sr);
        const float relSamples = (targetRelease_ * 0.001f) * static_cast<float>(sr);

        attackCoeff_ = 1.0f - std::exp(-1.0f / std::max(1.0f, attSamples));
        releaseCoeff_ = 1.0f - std::exp(-1.0f / std::max(1.0f, relSamples));
        holdSamples_ = static_cast<uint32_t>((targetHold_ * 0.001f) * static_cast<float>(sr));
    }

    ProcessSpec spec_;
    std::array<BiquadFilter, 2> sidechainFilter_;

    float currentGain_{ 0.0f };
    float attackCoeff_{ 0.05f };
    float releaseCoeff_{ 0.005f };
    uint32_t holdSamples_{ 0 };
    uint32_t holdCounter_{ 0 };
    bool gateOpen_{ false };

    float targetThreshold_{ -40.0f };
    float targetHysteresis_{ 4.0f };
    float targetAttack_{ 1.0f };
    float targetHold_{ 25.0f };
    float targetRelease_{ 80.0f };
    float targetRange_{ -80.0f };
    float targetSidechainHPF_{ 80.0f };

    std::array<PinDescriptor, 3> pins_;
    std::array<ParameterInfo, 7> params_;
};

inline AutoRegisterNode<NoiseGateNode> registerNoiseGate(NodeType::NoiseGate, "noise_gate", "Dynamics");

} // namespace audio_graph
