#pragma once

#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <span>
#include <atomic>
#include "../../core/Types.h"
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Motor de Convolución Particionada Zero-Latency (IR Reverb & Cab Sim) - Nodo DSP #48
 * Convolución híbrida multietapa: partición directa temporal en cabeza (0 latencia)
 * y particiones uniformes en frecuencia por bloques FFT (Reglas 5, 8, 9, 14, 34, 46, 47).
 * Delegado al motor de alto rendimiento SIMD AVX2 en Rust (n8_rust_convolution).
 */
class ConvolutionNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        IRType = 1,       // 0: Small Room, 1: Large Hall, 2: Dark Plate, 3: Vintage Cab 4x12, 4: Spring Tank, 5: Custom
        PreDelayMs = 2,   // 0.0 a 100.0 ms
        LowCutHz = 3,     // 20.0 a 1000.0 Hz (Filtro paso-altos para limpiar graves de la IR)
        HighCutHz = 4,    // 1000.0 a 20000.0 Hz (Filtro paso-bajos para amortiguar agudos)
        StereoWidth = 5,  // 0.0 a 2.0 (Apertura Mid/Side de la reverberación / cabina)
        Mix = 6           // 0.0 a 1.0 (Mezcla Dry / Wet)
    };

    static constexpr size_t PartitionSize = 128;
    static constexpr size_t FftSize = PartitionSize * 2; // 256
    static constexpr size_t MaxPartitions = 64;          // 64 * 128 = 8192 muestras (~170ms a 48kHz o escalable)

    ConvolutionNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { IRType, "IR Model", 0.0f, 0.0f, 5.0f, false };
        params_[1] = { PreDelayMs, "Pre-Delay", 0.0f, 0.0f, 100.0f, true };
        params_[2] = { LowCutHz, "Low Cut", 40.0f, 20.0f, 1000.0f, true };
        params_[3] = { HighCutHz, "High Cut", 16000.0f, 1000.0f, 20000.0f, true };
        params_[4] = { StereoWidth, "Width", 1.0f, 0.0f, 2.0f, true };
        params_[5] = { Mix, "Mix", 0.5f, 0.0f, 1.0f, true };

        irThumbnail_.fill(0.0f);
        rustNode_ = n8_rust_convolution_create();
    }

    ~ConvolutionNode() override {
        if (rustNode_) {
            n8_rust_convolution_destroy(rustNode_);
            rustNode_ = nullptr;
        }
    }

    ConvolutionNode(const ConvolutionNode&) = delete;
    ConvolutionNode& operator=(const ConvolutionNode&) = delete;

    ConvolutionNode(ConvolutionNode&& other) noexcept
        : spec_(other.spec_),
          pins_(other.pins_),
          params_(other.params_),
          irThumbnail_(other.irThumbnail_),
          targetIRType_(other.targetIRType_),
          targetPreDelayMs_(other.targetPreDelayMs_),
          targetLowCut_(other.targetLowCut_),
          targetHighCut_(other.targetHighCut_),
          targetWidth_(other.targetWidth_),
          targetMix_(other.targetMix_),
          rustNode_(other.rustNode_) {
        other.rustNode_ = nullptr;
    }

    ConvolutionNode& operator=(ConvolutionNode&& other) noexcept {
        if (this != &other) {
            if (rustNode_) {
                n8_rust_convolution_destroy(rustNode_);
            }
            spec_ = other.spec_;
            pins_ = other.pins_;
            params_ = other.params_;
            irThumbnail_ = other.irThumbnail_;
            targetIRType_ = other.targetIRType_;
            targetPreDelayMs_ = other.targetPreDelayMs_;
            targetLowCut_ = other.targetLowCut_;
            targetHighCut_ = other.targetHighCut_;
            targetWidth_ = other.targetWidth_;
            targetMix_ = other.targetMix_;
            rustNode_ = other.rustNode_;
            other.rustNode_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        if (rustNode_) {
            n8_rust_convolution_prepare(rustNode_, spec.sampleRate);
        }
    }

    void reset() noexcept override {
        if (rustNode_) {
            n8_rust_convolution_reset(rustNode_);
        }
    }

    void process(ProcessContext& ctx) noexcept override {
        if (!rustNode_ || ctx.numSamples == 0 || ctx.outputChannels == nullptr) return;

        const float* inL = (ctx.numInputChannels > 0 && ctx.inputChannels[0]) ? ctx.inputChannels[0] : nullptr;
        const float* inR = (ctx.numInputChannels > 1 && ctx.inputChannels[1]) ? ctx.inputChannels[1] : inL;
        float* outL = (ctx.numOutputChannels > 0 && ctx.outputChannels[0]) ? ctx.outputChannels[0] : nullptr;
        float* outR = (ctx.numOutputChannels > 1 && ctx.outputChannels[1]) ? ctx.outputChannels[1] : outL;

        if (!outL) return;

        n8_rust_convolution_process(rustNode_, inL, inR, outL, outR, ctx.numSamples);
    }

    std::span<const PinDescriptor> getPins() const noexcept override { return pins_; }
    std::span<const ParameterInfo> getParameters() const noexcept override { return params_; }

    NodeType getType() const noexcept override { return NodeType::Convolution; }
    const char* getName() const noexcept override { return "Convolution"; }

    float getParameter(ParameterId id) const override {
        if (rustNode_) {
            return n8_rust_convolution_get_parameter(rustNode_, static_cast<uint32_t>(id));
        }
        switch (id) {
            case IRType: return targetIRType_;
            case PreDelayMs: return targetPreDelayMs_;
            case LowCutHz: return targetLowCut_;
            case HighCutHz: return targetHighCut_;
            case StereoWidth: return targetWidth_;
            case Mix: return targetMix_;
            default: return 0.0f;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case IRType: targetIRType_ = std::clamp(value, 0.0f, 5.0f); break;
            case PreDelayMs: targetPreDelayMs_ = std::clamp(value, 0.0f, 100.0f); break;
            case LowCutHz: targetLowCut_ = std::clamp(value, 20.0f, 1000.0f); break;
            case HighCutHz: targetHighCut_ = std::clamp(value, 1000.0f, 20000.0f); break;
            case StereoWidth: targetWidth_ = std::clamp(value, 0.0f, 2.0f); break;
            case Mix: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
        if (rustNode_) {
            n8_rust_convolution_set_parameter(rustNode_, static_cast<uint32_t>(id), value);
        }
    }

    /**
     * @brief Acceso a la miniatura envolvente normalizada de 64 puntos de la IR (para visualizadores LCD)
     */
    const std::array<float, 64>& getIRThumbnail() const noexcept {
        if (rustNode_) {
            n8_rust_convolution_get_thumbnail(rustNode_, const_cast<float*>(irThumbnail_.data()));
        }
        return irThumbnail_;
    }

    /**
     * @brief Permite cargar una respuesta al impulso personalizada desde memoria con doble buffering lock-free
     */
    void setCustomImpulseResponse(const float* lData, const float* rData, size_t numSamples) {
        if (!lData || numSamples == 0) return;
        targetIRType_ = 5.0f;
        if (rustNode_) {
            n8_rust_convolution_set_custom_ir(rustNode_, lData, rData, numSamples);
            n8_rust_convolution_get_thumbnail(rustNode_, irThumbnail_.data());
        }
    }

private:
    ProcessSpec spec_{ 48000.0, 256, 2, 2 };

    mutable std::array<float, 64> irThumbnail_{};

    float targetIRType_{ 0.0f };
    float targetPreDelayMs_{ 0.0f };
    float targetLowCut_{ 40.0f };
    float targetHighCut_{ 16000.0f };
    float targetWidth_{ 1.0f };
    float targetMix_{ 0.5f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;

    RustConvolution* rustNode_{ nullptr };
};

inline AutoRegisterNode<ConvolutionNode> registerConvolution(NodeType::Convolution, "convolution", "Reverb");

} // namespace audio_graph
