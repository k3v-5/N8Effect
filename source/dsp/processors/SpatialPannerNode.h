#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/SpatialPannerCore.h"
#include "../core/DenormalGuards.h"

namespace audio_graph {

/**
 * @brief Nodo de Posicionamiento Espacial 3D por Efecto (Reglas 5, 8, 13, 16, 34, 46, 47).
 * Permite que cualquier procesador o cadena en el grafo decida exactamente sus coordenadas
 * tridimensionales (Azimut, Elevación y Distancia) en el campo binaural/estéreo.
 */
class SpatialPannerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Azimuth = 1,
        Elevation = 2,
        Distance = 3,
        Mix = 4
    };

    SpatialPannerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Azimuth, "Azimuth", 0.0f, -180.0f, 180.0f, true };
        params_[1] = { Elevation, "Elevation", 0.0f, -90.0f, 90.0f, true };
        params_[2] = { Distance, "Distance (m)", 1.0f, 0.1f, 10.0f, true };
        params_[3] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        pannerCore_.prepare(spec.sampleRate);
        pannerCore_.setCoordinates(targetAzimuth_, targetElevation_, targetDistance_);
        currentMix_ = targetMix_;
    }

    void reset() override {
        pannerCore_.reset();
        currentMix_ = targetMix_;
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Reglas 34 y 47
        const float alpha = 0.005f;

        pannerCore_.setCoordinates(targetAzimuth_, targetElevation_, targetDistance_);

        for (uint32_t s = 0; s < context.numSamples; ++s) {
            currentMix_ += alpha * (targetMix_ - currentMix_);

            const float inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr)
                ? context.inputChannels[0][s] : 0.0f;
            const float inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr)
                ? context.inputChannels[1][s] : inL;

            float spatialL = 0.0f;
            float spatialR = 0.0f;
            pannerCore_.processSample(inL, inR, spatialL, spatialR);

            if (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) {
                context.outputChannels[0][s] = inL * (1.0f - currentMix_) + spatialL * currentMix_;
            }
            if (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) {
                context.outputChannels[1][s] = inR * (1.0f - currentMix_) + spatialR * currentMix_;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Azimuth:   targetAzimuth_ = std::clamp(value, -180.0f, 180.0f); break;
            case Elevation: targetElevation_ = std::clamp(value, -90.0f, 90.0f); break;
            case Distance:  targetDistance_ = std::clamp(value, 0.1f, 10.0f); break;
            case Mix:       targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Azimuth:   return targetAzimuth_;
            case Elevation: return targetElevation_;
            case Distance:  return targetDistance_;
            case Mix:       return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::SpatialPanner; }
    const char* getName() const override { return "3D Spatial Panner"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    SpatialPannerCore& getCore() noexcept { return pannerCore_; }

private:
    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    SpatialPannerCore pannerCore_;

    float targetAzimuth_{ 0.0f };
    float targetElevation_{ 0.0f };
    float targetDistance_{ 1.0f };
    float targetMix_{ 1.0f };
    float currentMix_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 4> params_;
};

inline AutoRegisterNode<SpatialPannerNode> registerSpatialPannerNode(
    NodeType::SpatialPanner, "3D Spatial Panner", "Spatial"
);

} // namespace audio_graph
