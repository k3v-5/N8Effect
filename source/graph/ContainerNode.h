#pragma once

#include <array>
#include <span>
#include <string>
#include <memory>
#include <algorithm>
#include <cstring>
#include "AudioProcessorNode.h"
#include "Graph.h"
#include "GraphExecutor.h"
#include "NodeFactory.h"
#include "../dsp/core/DenormalGuards.h"

namespace audio_graph {

/**
 * @brief Contenedor de Subgrafos Jerárquicos Anidados (Reglas 5, 6, 9, 28, 46, 47).
 * Encapsula un subgrafo completo (Graph, ExecutionPlan, GraphExecutor) dentro de un único
 * AudioProcessorNode, permitiendo modularidad multinivel sin duplicación de código DSP.
 */
class ContainerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Mix = 1,
        OutputGain = 2
    };

    ContainerNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
        params_[1] = { OutputGain, "Gain", 1.0f, 0.0f, 2.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        // Bounded preallocated buffer para la salida del subgrafo (Reglas 9 y 47)
        innerOutputBuffer_.prepare(spec.numOutputChannels, spec.maximumBlockSize);
        // Prealocación compacta del pool para el ejecutor interno (8 buffers en caché L1/L2)
        innerExecutor_.prepare(spec, 8);

        // Preparar todos los nodos contenidos en el subgrafo
        for (const auto& [id, node] : innerGraph_.getNodes()) {
            if (node && node->processor) {
                node->processor->prepare(spec);
            }
        }

        currentMix_ = targetMix_;
        currentGain_ = targetGain_;
    }

    void reset() override {
        innerExecutor_.reset();
        for (const auto& [id, node] : innerGraph_.getNodes()) {
            if (node && node->processor) {
                node->processor->reset();
            }
        }
        currentMix_ = targetMix_;
        currentGain_ = targetGain_;
    }

    // Acceso al subgrafo interno para construcción jerárquica
    Graph& getInnerGraph() noexcept { return innerGraph_; }
    const Graph& getInnerGraph() const noexcept { return innerGraph_; }

    // Compilación y validación topológica del subgrafo interno (Regla 29)
    bool compileInnerGraph(std::string& outErrorMessage) {
        std::vector<NodeId> sorted;
        if (!innerGraph_.validateAndTopologicalSort(sorted, outErrorMessage)) {
            return false;
        }
        innerPlan_.compileFrom(innerGraph_, sorted);
        return true;
    }

    const ExecutionPlan& getInnerExecutionPlan() const noexcept { return innerPlan_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Reglas 34 y 47
        const float alpha = 0.005f;        // Anti-click smoothing (Regla 35)

        // Si el subgrafo está vacío, opera como passthrough transparente
        if (innerPlan_.isEmpty()) {
            for (uint32_t s = 0; s < context.numSamples; ++s) {
                currentGain_ += alpha * (targetGain_ - currentGain_);
                for (uint32_t ch = 0; ch < context.numOutputChannels; ++ch) {
                    float inSample = (ch < context.numInputChannels && context.inputChannels[ch] != nullptr)
                        ? context.inputChannels[ch][s] : 0.0f;
                    context.outputChannels[ch][s] = inSample * currentGain_;
                }
            }
            return;
        }

        // Ejecutar el subgrafo interno en tiempo real con cero allocations (Regla 9)
        innerExecutor_.process(innerPlan_, context, innerOutputBuffer_);

        // Mezcla Dry/Wet y ganancia de salida suavizada
        for (uint32_t s = 0; s < context.numSamples; ++s) {
            currentMix_ += alpha * (targetMix_ - currentMix_);
            currentGain_ += alpha * (targetGain_ - currentGain_);

            for (uint32_t ch = 0; ch < context.numOutputChannels; ++ch) {
                float dry = (ch < context.numInputChannels && context.inputChannels[ch] != nullptr)
                    ? context.inputChannels[ch][s] : 0.0f;
                float wet = (ch < innerOutputBuffer_.getNumChannels())
                    ? innerOutputBuffer_.getReadPointer(ch)[s] : 0.0f;

                float mixed = dry + currentMix_ * (wet - dry);
                context.outputChannels[ch][s] = mixed * currentGain_;
            }
        }
    }

    void setParameter(ParameterId id, float value) override {
        if (id == Mix) targetMix_ = std::clamp(value, 0.0f, 1.0f);
        else if (id == OutputGain) targetGain_ = std::clamp(value, 0.0f, 2.0f);
    }

    float getParameter(ParameterId id) const override {
        if (id == Mix) return targetMix_;
        if (id == OutputGain) return targetGain_;
        return 0.0f;
    }

    NodeType getType() const override { return NodeType::Container; }
    const char* getName() const override { return "Container"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    Graph innerGraph_;
    ExecutionPlan innerPlan_;
    GraphExecutor innerExecutor_;
    PreallocatedBuffer innerOutputBuffer_;

    float targetMix_{ 1.0f };
    float currentMix_{ 1.0f };
    float targetGain_{ 1.0f };
    float currentGain_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 2> params_;
};

inline AutoRegisterNode<ContainerNode> registerContainerNode(
    NodeType::Container, "Container", "Containers"
);

} // namespace audio_graph
