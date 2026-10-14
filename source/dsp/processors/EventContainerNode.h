#pragma once

#include <cmath>
#include <array>
#include <vector>
#include <span>
#include <memory>
#include <algorithm>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../../event/EventManager.h"
#include "../../core/RealtimePools.h"
#include "../core/DenormalGuards.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Contenedor de Entorno Aislado de Eventos (Event Rack) (Reglas 2, 3, 6, 9, 10, 46, 47).
 * Encapsula la captura, ciclo de vida (Created->Playing->Releasing->Killed), transmutación
 * y procesamiento en cascada de fragmentos de audio basados en eventos con límite estricto de capacidad.
 * 100% acelerado mediante núcleo nativo en Rust (EventContainerCore / RustEventManager) con SIMD y zero allocations.
 */
class EventContainerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Trigger = 1,
        Density = 2,
        Pitch = 3,
        Duration = 4,
        SourceFollow = 5,
        Mix = 6
    };

    EventContainerNode()
        : rustCore_(n8_rust_event_container_create(44100.0f)),
          eventManager_(rustCore_ ? n8_rust_event_container_get_event_manager(rustCore_) : nullptr, false)
    {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { 3, "Event In", PinType::EventInput, PinDataType::EventMessage };

        params_[0] = { Trigger, "Trigger", 0.0f, 0.0f, 1.0f, false };
        params_[1] = { Density, "Density", 0.5f, 0.0f, 1.0f, true };
        params_[2] = { Pitch, "Pitch", 1.0f, 0.25f, 4.0f, true };
        params_[3] = { Duration, "Duration (ms)", 150.0f, 10.0f, 1000.0f, true };
        params_[4] = { SourceFollow, "Source Follow", 0.0f, 0.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 0.7f, 0.0f, 1.0f, true };
    }

    ~EventContainerNode() override {
        if (rustCore_) {
            n8_rust_event_container_free(rustCore_);
            rustCore_ = nullptr;
        }
    }

    EventContainerNode(const EventContainerNode&) = delete;
    EventContainerNode& operator=(const EventContainerNode&) = delete;

    EventContainerNode(EventContainerNode&& other) noexcept
        : AudioProcessorNode(std::move(other)),
          spec_(other.spec_),
          rustCore_(other.rustCore_),
          eventManager_(std::move(other.eventManager_)),
          eventOutBuffer_(std::move(other.eventOutBuffer_)),
          innerProcessBuffer_(std::move(other.innerProcessBuffer_)),
          innerProcessor_(std::move(other.innerProcessor_)),
          pins_(other.pins_),
          params_(other.params_)
    {
        other.rustCore_ = nullptr;
    }

    EventContainerNode& operator=(EventContainerNode&& other) noexcept {
        if (this != &other) {
            if (rustCore_) {
                n8_rust_event_container_free(rustCore_);
            }
            AudioProcessorNode::operator=(std::move(other));
            spec_ = other.spec_;
            rustCore_ = other.rustCore_;
            eventManager_ = std::move(other.eventManager_);
            eventOutBuffer_ = std::move(other.eventOutBuffer_);
            innerProcessBuffer_ = std::move(other.innerProcessBuffer_);
            innerProcessor_ = std::move(other.innerProcessor_);
            pins_ = other.pins_;
            params_ = other.params_;
            other.rustCore_ = nullptr;
        }
        return *this;
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;

        if (rustCore_) {
            n8_rust_event_container_prepare(rustCore_, static_cast<float>(spec.sampleRate), spec.maximumBlockSize, 64);
        }

        // Bounded preallocated event manager: 64 eventos máximos para este contenedor (Regla 47)
        eventManager_.prepare(spec.sampleRate, spec.maximumBlockSize, 64);

        eventOutBuffer_.prepare(2, spec.maximumBlockSize);
        innerProcessBuffer_.prepare(2, spec.maximumBlockSize);

        reset();

        if (innerProcessor_) {
            innerProcessor_->prepare(spec);
        }
    }

    void reset() override {
        if (rustCore_) {
            n8_rust_event_container_reset(rustCore_);
        }
        eventManager_.reset();

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

    EventManager& getEventManager() noexcept { return eventManager_; }
    const EventManager& getEventManager() const noexcept { return eventManager_; }

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
            float* forwardL = eventOutBuffer_.getWritePointer(0);
            float* forwardR = eventOutBuffer_.getWritePointer(1);

            n8_rust_event_container_inject_forward(
                rustCore_,
                inL, inR,
                forwardL, forwardR,
                context.numSamples
            );

            const float* inPtrs[2] = { forwardL, forwardR };
            float* outPtrs[2] = { innerProcessBuffer_.getWritePointer(0), innerProcessBuffer_.getWritePointer(1) };
            ProcessContext innerCtx{
                .inputChannels = inPtrs,
                .outputChannels = outPtrs,
                .numInputChannels = 2,
                .numOutputChannels = 2,
                .numSamples = context.numSamples
            };
            innerProcessor_->process(innerCtx);

            const float* wetL = innerProcessBuffer_.getReadPointer(0);
            const float* wetR = innerProcessBuffer_.getReadPointer(1);

            n8_rust_event_container_process_return(
                rustCore_,
                inL, inR,
                wetL, wetR,
                outL, outR,
                context.numSamples
            );
        } else {
            n8_rust_event_container_process(
                rustCore_,
                inL, inR,
                outL, outR,
                context.numSamples
            );
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (rustCore_) {
            n8_rust_event_container_set_parameter(rustCore_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustCore_) {
            return n8_rust_event_container_get_parameter(rustCore_, id);
        }
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::EventContainer; }
    const char* getName() const override { return "Event Rack"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    RustEventContainer* rustCore_{ nullptr };
    EventManager eventManager_;
    PreallocatedBuffer eventOutBuffer_;
    PreallocatedBuffer innerProcessBuffer_;
    std::unique_ptr<AudioProcessorNode> innerProcessor_;

    std::array<PinDescriptor, 3> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<EventContainerNode> registerEventContainerNode(
    NodeType::EventContainer, "Event Rack", "Containers"
);

} // namespace audio_graph
