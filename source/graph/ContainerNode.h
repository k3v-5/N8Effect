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
#include "../dsp/rust_core/include/n8_rust_core.h"
#include "../preset/GraphSerializer.h"

namespace audio_graph {

/**
 * @brief Contenedor de Subgrafos Jerárquicos Anidados (Reglas 5, 6, 9, 28, 46, 47).
 * Encapsula un subgrafo completo (Graph, ExecutionPlan, GraphExecutor) dentro de un único
 * AudioProcessorNode, permitiendo modularidad multinivel sin duplicación de código DSP.
 * Acelerado mediante el núcleo nativo en Rust (RustContainerCore) para enrutamiento,
 * suavizado anti-click, mezcla Dry/Wet, passthrough multicanal y validación de subgrafos.
 */
class ContainerNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        Mix = 1,
        OutputGain = 2
    };

    ContainerNode()
        : rustCore_(n8_rust_container_create(44100.0f))
    {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
        params_[1] = { OutputGain, "Gain", 1.0f, 0.0f, 2.0f, true };
    }

    ~ContainerNode() override {
        if (rustCore_) {
            n8_rust_container_free(rustCore_);
            rustCore_ = nullptr;
        }
    }

    ContainerNode(const ContainerNode&) = delete;
    ContainerNode& operator=(const ContainerNode&) = delete;

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

        if (rustCore_) {
            n8_rust_container_prepare(rustCore_, static_cast<float>(spec.sampleRate), spec.maximumBlockSize);
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
        if (rustCore_) {
            n8_rust_container_reset(rustCore_);
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

    // Exportación e importación de subgrafos independientes como macro-módulos (Regla 6, 16, 21, 22)
    std::string exportSubGraphJson(std::string_view moduleName = "SubGraph Module") const {
        PresetMetadata meta;
        meta.name = std::string(moduleName);
        meta.category = "SubGraph Module";
        return GraphSerializer::serialize(innerGraph_, meta);
    }

    bool importSubGraphJson(std::string_view json, std::string& outErrorMessage) {
        PresetMetadata meta;
        std::array<float, 8> dummyMacros{};
        bool ok = GraphSerializer::deserialize(json, innerGraph_, meta, dummyMacros, outErrorMessage);
        if (ok) {
            return compileInnerGraph(outErrorMessage);
        }
        return false;
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Reglas 34 y 47

        // Si el subgrafo está vacío, opera como passthrough transparente
        if (innerPlan_.isEmpty()) {
            if (rustCore_) {
                n8_rust_container_process_channels_passthrough(
                    rustCore_,
                    context.inputChannels, context.numInputChannels,
                    context.outputChannels, context.numOutputChannels,
                    context.numSamples
                );
            } else {
                const float alpha = 0.005f; // Anti-click smoothing (Regla 35)
                for (uint32_t s = 0; s < context.numSamples; ++s) {
                    currentGain_ += alpha * (targetGain_ - currentGain_);
                    for (uint32_t ch = 0; ch < context.numOutputChannels; ++ch) {
                        float inSample = (ch < context.numInputChannels && context.inputChannels[ch] != nullptr)
                            ? context.inputChannels[ch][s] : 0.0f;
                        context.outputChannels[ch][s] = inSample * currentGain_;
                    }
                }
            }
            return;
        }

        // Ejecutar el subgrafo interno en tiempo real con cero allocations (Regla 9)
        innerExecutor_.process(innerPlan_, context, innerOutputBuffer_);

        // Mezcla Dry/Wet y ganancia de salida suavizada delegado al núcleo Rust
        if (rustCore_) {
            const float* const* wetChannels = innerOutputBuffer_.getArrayOfReadPointers();
            n8_rust_container_process_channels_mix(
                rustCore_,
                context.inputChannels, context.numInputChannels,
                wetChannels, innerOutputBuffer_.getNumChannels(),
                context.outputChannels, context.numOutputChannels,
                context.numSamples
            );
        } else {
            const float alpha = 0.005f; // Anti-click smoothing (Regla 35)
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
    }

    void setParameter(ParameterId id, float value) override {
        if (id == Mix) targetMix_ = std::clamp(value, 0.0f, 1.0f);
        else if (id == OutputGain) targetGain_ = std::clamp(value, 0.0f, 2.0f);

        if (rustCore_) {
            n8_rust_container_set_parameter(rustCore_, id, value);
        }
    }

    float getParameter(ParameterId id) const override {
        if (rustCore_) {
            return n8_rust_container_get_parameter(rustCore_, id);
        }
        if (id == Mix) return targetMix_;
        if (id == OutputGain) return targetGain_;
        return 0.0f;
    }

    RustContainerCore* getRustCore() noexcept { return rustCore_; }
    const RustContainerCore* getRustCore() const noexcept { return rustCore_; }

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
    RustContainerCore* rustCore_{ nullptr };

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
