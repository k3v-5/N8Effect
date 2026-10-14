#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Puerta de Ruido y Expansor Hacia Abajo Profesional (Reglas 5, 8, 9, 14, 34, 46, 47)
 * Proporciona supresión de ruido y aislamiento de transitorios con histéresis anti-chatter,
 * temporizador de retención (Hold), envolvente suave Attack/Release, control de rango (Range)
 * y filtro pasa-altos en la cadena de detección sidechain.
 * Acelerado con núcleo en Rust de alto rendimiento.
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

        rustInstance_ = n8_rust_noise_gate_create(44100.0f);
    }

    ~NoiseGateNode() override {
        if (rustInstance_) {
            n8_rust_noise_gate_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NoiseGateNode(const NoiseGateNode&) = delete;
    NoiseGateNode& operator=(const NoiseGateNode&) = delete;

    NoiseGateNode(NoiseGateNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    NoiseGateNode& operator=(NoiseGateNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_noise_gate_destroy(rustInstance_);
            }
            pins_ = other.pins_;
            params_ = other.params_;
            spec_ = other.spec_;
            rustInstance_ = other.rustInstance_;
            other.rustInstance_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        const float sr = spec.sampleRate > 0.0 ? static_cast<float>(spec.sampleRate) : 44100.0f;
        if (rustInstance_) {
            n8_rust_noise_gate_prepare(rustInstance_, sr);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_noise_gate_reset(rustInstance_);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;
        if (!rustInstance_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0]) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1]) ? context.inputChannels[1] : inL;

        const float* scL = (context.numSidechainChannels > 0 && context.sidechainChannels != nullptr && context.sidechainChannels[0] != nullptr)
            ? context.sidechainChannels[0] : inL;
        const float* scR = (context.numSidechainChannels > 1 && context.sidechainChannels != nullptr && context.sidechainChannels[1] != nullptr)
            ? context.sidechainChannels[1] : scL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0]) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1]) ? context.outputChannels[1] : nullptr;

        if (!inL || !outL) return;

        n8_rust_noise_gate_process(
            rustInstance_,
            inL,
            inR,
            scL,
            scR,
            outL,
            outR,
            static_cast<uint32_t>(context.numSamples)
        );
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_) {
            n8_rust_noise_gate_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_noise_gate_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::NoiseGate; }
    const char* getName() const override { return "Noise Gate"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 3> pins_;
    std::array<ParameterInfo, 7> params_;
    RustNoiseGate* rustInstance_{ nullptr };
};

inline AutoRegisterNode<NoiseGateNode> registerNoiseGate(NodeType::NoiseGate, "noise_gate", "Dynamics");

} // namespace audio_graph
