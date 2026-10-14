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
 * @brief De-Esser Dinámico Profesional de Precisión Vocal (Reglas 5, 8, 9, 14, 34, 46, 47)
 * Detección y atenuación quirúrgica de sibilancias ("S", "T", "Ch" entre 3 y 10 kHz).
 * Soporta modo Split-Band (atenuación exclusiva de la banda sibilante sin tocar el cuerpo de la voz),
 * modo Wideband (reducción global), ajuste de Q/frecuencia y modo Listen para calibración de monitoreo.
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class DeEsserNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Frequency = 1,  // 2000.0 a 12000.0 Hz (Frecuencia central de sibilancia)
        Bandwidth = 2,  // 0.5 a 5.0 Q (Ancho de banda de detección)
        Threshold = 3,  // -50.0 a 0.0 dB (Umbral de activación)
        Reduction = 4,  // 0.0 a 30.0 dB (Atenuación máxima de sibilantes)
        Mode = 5,       // 0.0 = Split Band, 1.0 = Wideband
        Listen = 6      // 0.0 = Normal, 1.0 = Monitoreo de banda aislada
    };

    DeEsserNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Frequency, "Frequency", 6500.0f, 2000.0f, 12000.0f, true };
        params_[1] = { Bandwidth, "Bandwidth Q", 1.5f, 0.5f, 5.0f, true };
        params_[2] = { Threshold, "Threshold", -20.0f, -50.0f, 0.0f, true };
        params_[3] = { Reduction, "Reduction", 12.0f, 0.0f, 30.0f, true };
        params_[4] = { Mode, "Mode", 0.0f, 0.0f, 1.0f, true };
        params_[5] = { Listen, "Listen Band", 0.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_de_esser_create(44100.0f);
    }

    ~DeEsserNode() override {
        if (rustInstance_) {
            n8_rust_de_esser_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    DeEsserNode(const DeEsserNode&) = delete;
    DeEsserNode& operator=(const DeEsserNode&) = delete;

    DeEsserNode(DeEsserNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), spec_(other.spec_), rustInstance_(other.rustInstance_) {
        other.rustInstance_ = nullptr;
    }

    DeEsserNode& operator=(DeEsserNode&& other) noexcept {
        if (this != &other) {
            if (rustInstance_) {
                n8_rust_de_esser_destroy(rustInstance_);
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
            n8_rust_de_esser_prepare(rustInstance_, sr);
        }
    }

    void reset() override {
        if (rustInstance_) {
            n8_rust_de_esser_reset(rustInstance_);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;
        if (!rustInstance_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0]) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1]) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0]) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1]) ? context.outputChannels[1] : nullptr;

        if (!inL || !outL) return;

        n8_rust_de_esser_process(
            rustInstance_,
            inL,
            inR,
            outL,
            outR,
            static_cast<uint32_t>(context.numSamples)
        );
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_) {
            n8_rust_de_esser_set_parameter(rustInstance_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_) {
            return n8_rust_de_esser_get_parameter(rustInstance_, id);
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::DeEsser; }
    const char* getName() const override { return "De-Esser"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_;
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
    RustDeEsser* rustInstance_{ nullptr };
};

inline AutoRegisterNode<DeEsserNode> registerDeEsser(NodeType::DeEsser, "de_esser", "Dynamics");

} // namespace audio_graph
