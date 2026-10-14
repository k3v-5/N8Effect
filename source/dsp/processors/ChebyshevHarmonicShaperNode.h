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
 * @brief Chebyshev Polynomial Harmonic Shaper Node (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Genera sobretonos armónicos puros (H2 a H8) mediante polinomios de Chebyshev de primer tipo.
 * Permite control quirúrgico de cada orden armónico sin distorsión de intermodulación cruzada,
 * con filtro DC-blocker integrado y protección de silencio absoluto.
 */
class ChebyshevHarmonicShaperNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Drive = 1,
        Harmonic2 = 2,
        Harmonic3 = 3,
        Harmonic4 = 4,
        Harmonic5 = 5,
        Harmonic6 = 6,
        Harmonic7 = 7,
        Harmonic8 = 8,
        Mix = 9
    };

    ChebyshevHarmonicShaperNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Drive, "Drive", 1.0f, 0.1f, 10.0f, true };
        params_[1] = { Harmonic2, "2nd Harmonic (Octave)", 0.3f, 0.0f, 1.0f, true };
        params_[2] = { Harmonic3, "3rd Harmonic (Fifth)", 0.2f, 0.0f, 1.0f, true };
        params_[3] = { Harmonic4, "4th Harmonic (2 Octaves)", 0.1f, 0.0f, 1.0f, true };
        params_[4] = { Harmonic5, "5th Harmonic", 0.0f, 0.0f, 1.0f, true };
        params_[5] = { Harmonic6, "6th Harmonic", 0.0f, 0.0f, 1.0f, true };
        params_[6] = { Harmonic7, "7th Harmonic", 0.0f, 0.0f, 1.0f, true };
        params_[7] = { Harmonic8, "8th Harmonic", 0.0f, 0.0f, 1.0f, true };
        params_[8] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_chebyshev_harmonic_shaper_create(44100.0f);
    }

    ~ChebyshevHarmonicShaperNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_chebyshev_harmonic_shaper_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::ChebyshevHarmonicShaper; }
    const char* getName() const override { return "Chebyshev Harmonic Shaper"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_chebyshev_harmonic_shaper_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_chebyshev_harmonic_shaper_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_chebyshev_harmonic_shaper_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_chebyshev_harmonic_shaper_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Drive: return params_[0].defaultValue;
            case Harmonic2: return params_[1].defaultValue;
            case Harmonic3: return params_[2].defaultValue;
            case Harmonic4: return params_[3].defaultValue;
            case Harmonic5: return params_[4].defaultValue;
            case Harmonic6: return params_[5].defaultValue;
            case Harmonic7: return params_[6].defaultValue;
            case Harmonic8: return params_[7].defaultValue;
            case Mix: return params_[8].defaultValue;
            default: return 0.0f;
        }
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels != nullptr) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0 && context.outputChannels != nullptr) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels != nullptr) ? context.outputChannels[1] : nullptr;

        if (outL == nullptr || rustInstance_ == nullptr || inL == nullptr) return;

        n8_rust_chebyshev_harmonic_shaper_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustChebyshevHarmonicShaper* rustInstance_{ nullptr };
    ProcessSpec spec_{};
    std::array<PinDescriptor, 2> pins_{};
    std::array<ParameterInfo, 9> params_{};
};

inline AutoRegisterNode<ChebyshevHarmonicShaperNode> registerChebyshevHarmonicShaper(
    NodeType::ChebyshevHarmonicShaper, "chebyshevharmonicshaper", "Distortion");

} // namespace audio_graph
