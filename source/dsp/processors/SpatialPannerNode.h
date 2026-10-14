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
 * @brief Nodo de Posicionamiento Espacial 3D por Efecto (Reglas 5, 8, 13, 16, 34, 46, 47).
 * Permite que cualquier procesador o cadena en el grafo decida exactamente sus coordenadas
 * tridimensionales (Azimut, Elevación y Distancia) en el campo binaural/estéreo.
 * Delegado al motor DSP SIMD en Rust (n8_rust_core).
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

        rustPanner_ = n8_rust_spatial_panner_create(44100.0f);
    }

    ~SpatialPannerNode() override {
        if (rustPanner_) {
            n8_rust_spatial_panner_destroy(rustPanner_);
            rustPanner_ = nullptr;
        }
    }

    SpatialPannerNode(const SpatialPannerNode&) = delete;
    SpatialPannerNode& operator=(const SpatialPannerNode&) = delete;

    SpatialPannerNode(SpatialPannerNode&& other) noexcept
        : pins_(other.pins_), params_(other.params_), rustPanner_(other.rustPanner_) {
        other.rustPanner_ = nullptr;
    }

    SpatialPannerNode& operator=(SpatialPannerNode&& other) noexcept {
        if (this != &other) {
            if (rustPanner_) {
                n8_rust_spatial_panner_destroy(rustPanner_);
            }
            pins_ = other.pins_;
            params_ = other.params_;
            rustPanner_ = other.rustPanner_;
            other.rustPanner_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        if (rustPanner_) {
            n8_rust_spatial_panner_prepare(rustPanner_, static_cast<float>(spec.sampleRate));
        }
    }

    void reset() override {
        if (rustPanner_) {
            n8_rust_spatial_panner_reset(rustPanner_);
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Reglas 34 y 47
        if (!rustPanner_ || context.numSamples == 0) return;

        const float* inL = (context.numInputChannels > 0) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1) ? context.inputChannels[1] : inL;
        float* outL = (context.numOutputChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : nullptr;

        n8_rust_spatial_panner_process(rustPanner_, inL, inR, outL, outR, context.numSamples);
    }

    void setParameter(ParameterId id, float value) override {
        if (rustPanner_) {
            n8_rust_spatial_panner_set_parameter(rustPanner_, static_cast<uint32_t>(id), value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustPanner_) {
            return n8_rust_spatial_panner_get_parameter(rustPanner_, static_cast<uint32_t>(id));
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::SpatialPanner; }
    const char* getName() const override { return "3D Spatial Panner"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 4> params_;
    RustSpatialPanner* rustPanner_{ nullptr };
};

inline AutoRegisterNode<SpatialPannerNode> registerSpatialPannerNode(
    NodeType::SpatialPanner, "3D Spatial Panner", "Spatial"
);

} // namespace audio_graph
