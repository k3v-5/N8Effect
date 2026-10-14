#pragma once

#include <cmath>
#include <array>
#include <vector>
#include <span>
#include <memory>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DenormalGuards.h"
#include "../../core/RealtimePools.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Contenedor de Lazos de Realimentación Controlados con Protección Anti-Runaway (Reglas 6, 11, 12, 34, 46, 47).
 * Encapsula un lazo de feedback con retardo configurable, filtrado de amortiguamiento (damping),
 * bloqueo de continua (DC blocker), detector de energía RMS para limitación dinámica de sobre-ganancia
 * y saturación sigmoidal Padé delegados al núcleo Rust de alto rendimiento con aceleración SIMD AVX2.
 */
class FeedbackContainerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Feedback = 1,
        DelayTime = 2,
        Damping = 3,
        Threshold = 4,
        Mix = 5
    };

    FeedbackContainerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Feedback, "Feedback", 0.5f, 0.0f, 1.5f, true };
        params_[1] = { DelayTime, "Delay (ms)", 50.0f, 1.0f, 500.0f, true };
        params_[2] = { Damping, "Damping", 7000.0f, 1000.0f, 20000.0f, true };
        params_[3] = { Threshold, "Anti-Runaway Thresh", 1.0f, 0.5f, 1.5f, true };
        params_[4] = { Mix, "Mix", 0.5f, 0.0f, 1.0f, true };

        rustCore_ = n8_rust_feedback_container_create(44100.0f);
    }

    ~FeedbackContainerNode() override {
        if (rustCore_) {
            n8_rust_feedback_container_free(rustCore_);
            rustCore_ = nullptr;
        }
    }

    FeedbackContainerNode(const FeedbackContainerNode&) = delete;
    FeedbackContainerNode& operator=(const FeedbackContainerNode&) = delete;

    FeedbackContainerNode(FeedbackContainerNode&& other) noexcept
        : AudioProcessorNode(std::move(other)),
          spec_(other.spec_),
          pins_(other.pins_),
          params_(other.params_),
          forwardBuffer_(std::move(other.forwardBuffer_)),
          innerBuffer_(std::move(other.innerBuffer_)),
          innerProcessor_(std::move(other.innerProcessor_)),
          rustCore_(other.rustCore_)
    {
        other.rustCore_ = nullptr;
    }

    FeedbackContainerNode& operator=(FeedbackContainerNode&& other) noexcept {
        if (this != &other) {
            if (rustCore_) {
                n8_rust_feedback_container_free(rustCore_);
            }
            AudioProcessorNode::operator=(std::move(other));
            spec_ = other.spec_;
            pins_ = other.pins_;
            params_ = other.params_;
            forwardBuffer_ = std::move(other.forwardBuffer_);
            innerBuffer_ = std::move(other.innerBuffer_);
            innerProcessor_ = std::move(other.innerProcessor_);
            rustCore_ = other.rustCore_;
            other.rustCore_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;

        const uint32_t bufferChannels = std::max(2u, static_cast<uint32_t>(spec.numOutputChannels));
        forwardBuffer_.prepare(bufferChannels, spec.maximumBlockSize);
        innerBuffer_.prepare(bufferChannels, spec.maximumBlockSize);

        if (rustCore_) {
            n8_rust_feedback_container_prepare(rustCore_, static_cast<float>(spec.sampleRate));
        }

        if (innerProcessor_) {
            innerProcessor_->prepare(spec);
        }
    }

    void reset() override {
        if (rustCore_) {
            n8_rust_feedback_container_reset(rustCore_);
        }
        if (innerProcessor_) {
            innerProcessor_->reset();
        }
    }

    void setInnerProcessor(std::unique_ptr<AudioProcessorNode> processor) {
        innerProcessor_ = std::move(processor);
        if (innerProcessor_ && spec_.sampleRate > 0) {
            innerProcessor_->prepare(spec_);
        }
    }

    AudioProcessorNode* getInnerProcessor() noexcept { return innerProcessor_.get(); }
    const AudioProcessorNode* getInnerProcessor() const noexcept { return innerProcessor_.get(); }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Reglas 34 y 47
        if (!rustCore_) return;

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr)
            ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr)
            ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr)
            ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr)
            ? context.outputChannels[1] : nullptr;

        if (innerProcessor_) {
            // Si hay procesador interno en el lazo, alimentar su entrada mediante inject_forward
            float* forwardL = forwardBuffer_.getWritePointer(0);
            float* forwardR = forwardBuffer_.getWritePointer(1);

            n8_rust_feedback_container_inject_forward(
                rustCore_, inL, inR, forwardL, forwardR, context.numSamples
            );

            const float* inPtrs[2] = { forwardL, forwardR };
            float* outPtrs[2] = { innerBuffer_.getWritePointer(0), innerBuffer_.getWritePointer(1) };
            ProcessContext innerCtx{
                .inputChannels = inPtrs,
                .outputChannels = outPtrs,
                .numInputChannels = 2,
                .numOutputChannels = 2,
                .numSamples = context.numSamples
            };
            innerProcessor_->process(innerCtx);

            // Cerrar el lazo con la señal procesada internamente (Damping + DC Blocker + AGC + Padé Tanh)
            n8_rust_feedback_container_process_return(
                rustCore_, inL, inR,
                innerBuffer_.getReadPointer(0), innerBuffer_.getReadPointer(1),
                outL, outR, context.numSamples
            );
        } else {
            // Sin procesador interno: procesamiento directo de lazo cerrado optimizado en Rust
            n8_rust_feedback_container_process(
                rustCore_, inL, inR, outL, outR, context.numSamples
            );
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustCore_) {
            n8_rust_feedback_container_set_parameter(rustCore_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustCore_) {
            return n8_rust_feedback_container_get_parameter(rustCore_, id);
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::Feedback; }
    const char* getName() const override { return "Feedback Loop"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    PreallocatedBuffer forwardBuffer_;
    PreallocatedBuffer innerBuffer_;
    std::unique_ptr<AudioProcessorNode> innerProcessor_;
    RustFeedbackContainer* rustCore_{ nullptr };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 5> params_;
};

inline AutoRegisterNode<FeedbackContainerNode> registerFeedbackContainerNode(
    NodeType::Feedback, "Feedback Loop", "Containers"
);

} // namespace audio_graph
