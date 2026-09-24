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

namespace audio_graph {

/**
 * @brief Contenedor de Entorno Aislado de Eventos (Event Rack) (Reglas 2, 3, 6, 9, 10, 46, 47).
 * Encapsula la captura, ciclo de vida (Created->Playing->Releasing->Killed), transmutación
 * y procesamiento en cascada de fragmentos de audio basados en eventos con límite estricto de capacidad.
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

    EventContainerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };
        pins_[2] = { 3, "Event In", PinType::EventInput, PinDataType::EventMessage };

        params_[0] = { Trigger, "Trigger", 0.0f, 0.0f, 1.0f, false };
        params_[1] = { Density, "Density", 0.0f, 0.0f, 1.0f, true };
        params_[2] = { Pitch, "Pitch", 1.0f, 0.25f, 4.0f, true };
        params_[3] = { Duration, "Duration (ms)", 150.0f, 10.0f, 1000.0f, true };
        params_[4] = { SourceFollow, "Source Follow", 0.0f, 0.0f, 1.0f, true };
        params_[5] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;

        // Bounded preallocated event manager: 64 eventos máximos para este contenedor (Regla 47)
        eventManager_.prepare(spec.sampleRate, spec.maximumBlockSize, 64);

        eventOutBuffer_.prepare(2, spec.maximumBlockSize);
        innerProcessBuffer_.prepare(2, spec.maximumBlockSize);

        reset();

        if (innerProcessor_) {
            innerProcessor_->prepare(spec);
        }

        currentMix_ = targetMix_;
        lastTrigger_ = targetTrigger_;
        autoTriggerCounter_ = 0;
    }

    void reset() override {
        eventManager_.reset();
        autoTriggerCounter_ = 0;
        currentMix_ = targetMix_;
        lastTrigger_ = targetTrigger_;

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
        const float alpha = 0.005f;        // Anti-click smoothing (Regla 35)

        // 1. Capturar el audio de entrada en el buffer circular de eventos (Reglas 2 y 9)
        eventManager_.captureInputAudio(context.inputChannels, context.numInputChannels, context.numSamples);

        // 2. Calcular nivel RMS de la fuente para Source Following (Regla 3)
        float sumSq = 0.0f;
        for (uint32_t s = 0; s < context.numSamples; ++s) {
            float inSample = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr)
                ? context.inputChannels[0][s] : 0.0f;
            sumSq += inSample * inSample;
        }
        const float sourceLevel = std::sqrt(sumSq / static_cast<float>(context.numSamples > 0 ? context.numSamples : 1));

        // 3. Manejo de disparador manual (edge detector)
        if (targetTrigger_ >= 0.5f && lastTrigger_ < 0.5f) {
            const uint64_t durSamples = static_cast<uint64_t>(targetDurationMs_ * 0.001 * spec_.sampleRate);
            eventManager_.spawnEvent(EventType::Grain, 0.0, durSamples, targetPitch_, 1.0f, 0.0f,
                                    targetSourceFollow_, SourceDisappearanceMode::Fade);
        }
        lastTrigger_ = targetTrigger_;

        // 4. Generador estocástico/rítmico automático de eventos si Density > 0.05
        if (targetDensity_ > 0.05f) {
            autoTriggerCounter_ += context.numSamples;
            const uint32_t triggerInterval = static_cast<uint32_t>(spec_.sampleRate * (1.0f - targetDensity_ * 0.85f) * 0.25f);
            if (autoTriggerCounter_ >= std::max(256u, triggerInterval)) {
                autoTriggerCounter_ = 0;
                const uint64_t durSamples = static_cast<uint64_t>(targetDurationMs_ * 0.001 * spec_.sampleRate);
                eventManager_.spawnEvent(EventType::Grain, 0.0, durSamples, targetPitch_, 0.85f, 0.0f,
                                        targetSourceFollow_, SourceDisappearanceMode::Fade);
            }
        }

        // 5. Renderizado en tiempo real de los eventos activos hacia los buffers estéreo
        float* evL = eventOutBuffer_.getWritePointer(0);
        float* evR = eventOutBuffer_.getWritePointer(1);
        std::fill_n(evL, context.numSamples, 0.0f);
        std::fill_n(evR, context.numSamples, 0.0f);

        eventManager_.render(evL, evR, context.numSamples, sourceLevel);

        // 6. Si hay un procesador interno dedicado a los eventos, procesar los fragmentos
        const float* finalEvL = evL;
        const float* finalEvR = evR;

        if (innerProcessor_) {
            const float* inPtrs[2] = { evL, evR };
            float* outPtrs[2] = { innerProcessBuffer_.getWritePointer(0), innerProcessBuffer_.getWritePointer(1) };
            ProcessContext innerCtx{
                .inputChannels = inPtrs,
                .outputChannels = outPtrs,
                .numInputChannels = 2,
                .numOutputChannels = 2,
                .numSamples = context.numSamples
            };
            innerProcessor_->process(innerCtx);
            finalEvL = innerProcessBuffer_.getReadPointer(0);
            finalEvR = innerProcessBuffer_.getReadPointer(1);
        }

        // 7. Sumar hacia el bus húmedo/salida con mezcla suave (Reglas 17 y 35)
        for (uint32_t s = 0; s < context.numSamples; ++s) {
            currentMix_ += alpha * (targetMix_ - currentMix_);

            const float dryL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr)
                ? context.inputChannels[0][s] : 0.0f;
            const float dryR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr)
                ? context.inputChannels[1][s] : dryL;

            if (context.numOutputChannels > 0 && context.outputChannels[0] != nullptr) {
                context.outputChannels[0][s] = dryL * (1.0f - currentMix_) + finalEvL[s] * currentMix_;
            }
            if (context.numOutputChannels > 1 && context.outputChannels[1] != nullptr) {
                context.outputChannels[1][s] = dryR * (1.0f - currentMix_) + finalEvR[s] * currentMix_;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Trigger:      targetTrigger_ = value; break;
            case Density:      targetDensity_ = std::clamp(value, 0.0f, 1.0f); break;
            case Pitch:        targetPitch_ = std::clamp(value, 0.25f, 4.0f); break;
            case Duration:     targetDurationMs_ = std::clamp(value, 10.0f, 1000.0f); break;
            case SourceFollow: targetSourceFollow_ = std::clamp(value, 0.0f, 1.0f); break;
            case Mix:          targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Trigger:      return targetTrigger_;
            case Density:      return targetDensity_;
            case Pitch:        return targetPitch_;
            case Duration:     return targetDurationMs_;
            case SourceFollow: return targetSourceFollow_;
            case Mix:          return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::EventContainer; }
    const char* getName() const override { return "Event Rack"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    EventManager eventManager_;
    PreallocatedBuffer eventOutBuffer_;
    PreallocatedBuffer innerProcessBuffer_;
    std::unique_ptr<AudioProcessorNode> innerProcessor_;

    float targetTrigger_{ 0.0f };
    float lastTrigger_{ 0.0f };
    float targetDensity_{ 0.0f };
    float targetPitch_{ 1.0f };
    float targetDurationMs_{ 150.0f };
    float targetSourceFollow_{ 0.0f };
    float targetMix_{ 1.0f };
    float currentMix_{ 1.0f };

    uint32_t autoTriggerCounter_{ 0 };

    std::array<PinDescriptor, 3> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<EventContainerNode> registerEventContainerNode(
    NodeType::EventContainer, "Event Rack", "Containers"
);

} // namespace audio_graph
