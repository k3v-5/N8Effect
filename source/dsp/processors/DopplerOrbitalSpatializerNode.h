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
 * @brief Doppler 3D Orbital Spatializer Node (Rules 5, 8, 9, 14, 19, 34, 46, 47)
 * Simulador fÃ­sico de trayectoria acÃºstica orbital 3D alrededor de la cabeza del oyente.
 * Modela la velocidad de propagaciÃ³n del sonido (343 m/s), el corrimiento de tono Doppler real,
 * absorciÃ³n dependiente de la distancia en el aire (filtro paso-bajo dinÃ¡mico) y micro-retardos interaurales continuos.
 */
class DopplerOrbitalSpatializerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        OrbitRateHz = 1,
        OrbitRadiusM = 2,
        ElevationDeg = 3,
        DopplerDepth = 4,
        AirDamping = 5,
        EarWidthM = 6,
        Mix = 7
    };

    DopplerOrbitalSpatializerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { OrbitRateHz, "Orbit Rate (Hz)", 0.35f, 0.02f, 10.0f, true };
        params_[1] = { OrbitRadiusM, "Orbit Radius (m)", 3.5f, 0.5f, 12.0f, true };
        params_[2] = { ElevationDeg, "Elevation Angle (deg)", 0.0f, -45.0f, 45.0f, true };
        params_[3] = { DopplerDepth, "Doppler Pitch Depth", 0.8f, 0.0f, 1.0f, true };
        params_[4] = { AirDamping, "Air Absorption Damping", 0.5f, 0.0f, 1.0f, true };
        params_[5] = { EarWidthM, "Ear Spacing (m)", 0.22f, 0.15f, 0.35f, true };
        params_[6] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };

        rustInstance_ = n8_rust_doppler_orbital_spatializer_create(44100.0f);
    }

    ~DopplerOrbitalSpatializerNode() override {
        if (rustInstance_ != nullptr) {
            n8_rust_doppler_orbital_spatializer_destroy(rustInstance_);
            rustInstance_ = nullptr;
        }
    }

    NodeType getType() const override { return NodeType::DopplerOrbitalSpatializer; }
    const char* getName() const override { return "Doppler Orbital Spatializer"; }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustInstance_ != nullptr) {
            n8_rust_doppler_orbital_spatializer_prepare(rustInstance_, static_cast<float>(spec.sampleRate));
            n8_rust_doppler_orbital_spatializer_reset(rustInstance_);
        }
    }

    void reset() override {
        if (rustInstance_ != nullptr) {
            n8_rust_doppler_orbital_spatializer_reset(rustInstance_);
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustInstance_ != nullptr) {
            n8_rust_doppler_orbital_spatializer_set_param(rustInstance_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case OrbitRateHz: return params_[0].defaultValue;
            case OrbitRadiusM: return params_[1].defaultValue;
            case ElevationDeg: return params_[2].defaultValue;
            case DopplerDepth: return params_[3].defaultValue;
            case AirDamping: return params_[4].defaultValue;
            case EarWidthM: return params_[5].defaultValue;
            case Mix: return params_[6].defaultValue;
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

        n8_rust_doppler_orbital_spatializer_process(rustInstance_, inL, inR, outL, outR, context.numSamples);
    }

private:
    RustDopplerOrbitalSpatializer* rustInstance_{ nullptr };
    ProcessSpec spec_{};
    std::array<PinDescriptor, 2> pins_{};
    std::array<ParameterInfo, 7> params_{};
};

inline AutoRegisterNode<DopplerOrbitalSpatializerNode> registerDopplerOrbitalSpatializer(
    NodeType::DopplerOrbitalSpatializer, "dopplerorbitalspatializer", "Spatial");

} // namespace audio_graph
