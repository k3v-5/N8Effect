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
 * @brief Moldeador Dinámico de Transientes (Transient Shaper / Designer)
 * Control independiente de Ataque (Punch) y Sostenido (Sustain) en modo Banda Ancha o Multibanda LR4 de 3 bandas (Reglas 5, 8, 14, 32, 34, 46, 47)
 * Acelerado con núcleo en Rust de alto rendimiento.
 */
class TransientShaperNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Attack = 1,          // -1.0 (-100%) a +1.0 (+100%) [Global / Master]
        Sustain = 2,         // -1.0 (-100%) a +1.0 (+100%) [Global / Master]
        Speed = 3,           // Velocidad de respuesta de transientes (0.5x a 2.0x)
        SoftClip = 4,        // 0: Off, 1: On (Saturador no lineal anti-picos)
        OutputGain = 5,      // Ganancia de salida en dB (-12 dB a +12 dB)
        Mix = 6,             // Dry / Wet
        Mode = 7,            // 0: Wideband (Banda Completa), 1: Multiband (3 Bandas LR4)
        LowAttack = 8,       // -1.0 a +1.0 (Punch sub/bombo)
        MidAttack = 9,       // -1.0 a +1.0 (Cuerpo caja/guitarras/voz)
        HighAttack = 10,     // -1.0 a +1.0 (Chasquido charles/crack)
        LowSustain = 11,     // -1.0 a +1.0 (Cola graves)
        MidSustain = 12,     // -1.0 a +1.0 (Resonancia medios)
        HighSustain = 13,    // -1.0 a +1.0 (Brillo/aire sustain)
        CrossoverLowMid = 14,// Frecuencia de corte Low-Mid (40 a 1000 Hz)
        CrossoverMidHigh = 15// Frecuencia de corte Mid-High (1000 a 12000 Hz)
    };

    TransientShaperNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0]  = { Attack, "Attack", 0.0f, -1.0f, 1.0f, true };
        params_[1]  = { Sustain, "Sustain", 0.0f, -1.0f, 1.0f, true };
        params_[2]  = { Speed, "Speed", 1.0f, 0.5f, 2.0f, true };
        params_[3]  = { SoftClip, "Soft Clip", 1.0f, 0.0f, 1.0f, false };
        params_[4]  = { OutputGain, "Gain dB", 0.0f, -12.0f, 12.0f, true };
        params_[5]  = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
        params_[6]  = { Mode, "Mode", 0.0f, 0.0f, 1.0f, false };
        params_[7]  = { LowAttack, "Low Attack", 0.0f, -1.0f, 1.0f, true };
        params_[8]  = { MidAttack, "Mid Attack", 0.0f, -1.0f, 1.0f, true };
        params_[9]  = { HighAttack, "High Attack", 0.0f, -1.0f, 1.0f, true };
        params_[10] = { LowSustain, "Low Sustain", 0.0f, -1.0f, 1.0f, true };
        params_[11] = { MidSustain, "Mid Sustain", 0.0f, -1.0f, 1.0f, true };
        params_[12] = { HighSustain, "High Sustain", 0.0f, -1.0f, 1.0f, true };
        params_[13] = { CrossoverLowMid, "Xover Low-Mid", 250.0f, 40.0f, 1000.0f, true };
        params_[14] = { CrossoverMidHigh, "Xover Mid-High", 3000.0f, 1000.0f, 12000.0f, true };

        rustInstance_ = n8_rust_transient_shaper_create(44100.0);
    }

    ~TransientShaperNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_transient_shaper_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_transient_shaper_set_sample_rate(rustInstance_, spec.sampleRate);
            n8_rust_transient_shaper_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_transient_shaper_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_transient_shaper_set_parameter(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustInstance_ != nullptr) {
            return n8_rust_transient_shaper_get_parameter(rustInstance_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::TransientShaper; }
    const char* getName() const override { return "Transient Shaper"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47

        const uint32_t numSamples = context.numSamples;
        if (numSamples == 0 || rustInstance_ == nullptr) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        n8_rust_transient_shaper_process(rustInstance_, inL, inR, outL, outR, numSamples);
    }

private:
    ProcessSpec spec_;
    RustTransientShaper* rustInstance_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 15> params_;
};

inline AutoRegisterNode<TransientShaperNode> registerTransientShaper(NodeType::TransientShaper, "transient_shaper", "Dynamics");

} // namespace audio_graph
