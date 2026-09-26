#pragma once

#include <tuple>
#include <array>
#include <span>
#include <utility>
#include "AudioProcessorNode.h"
#include "CRTPProcessorNode.h"
#include "../core/RealtimePools.h"
#include "../dsp/core/DenormalGuards.h"
#include "../dsp/core/FastMath.h"

namespace audio_graph {

/**
 * @brief Cadena serial fija con despacho estático en tiempo de compilación (Reglas 5, 6, 14, 46, 47).
 * Concatena N nodos de procesamiento en serie sin sobrecarga de despacho virtual (vtable)
 * ni asignaciones dinámicas en el audio thread.
 * Permite que el compilador realice inlining agresivo y fusión de bucles.
 * Expone la interfaz AudioProcessorNode para integración modular transparente en Graph y ExecutionPlan.
 */
template <typename... Nodes>
class StaticSerialChain : public AudioProcessorNode {
public:
    static constexpr size_t ChainLength = sizeof...(Nodes);
    static_assert(ChainLength > 0, "StaticSerialChain debe contener al menos un nodo.");
    static constexpr size_t getChainLength() noexcept { return ChainLength; }

    enum Param : ParameterId {
        Mix = 1,
        OutputGain = 2
    };

    StaticSerialChain() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { Mix, "Mix", 1.0f, 0.0f, 1.0f, true };
        params_[1] = { OutputGain, "Gain", 1.0f, 0.0f, 2.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        const uint32_t channels = std::max(2u, spec.numOutputChannels);
        pingPong_[0].prepare(channels, spec.maximumBlockSize);
        pingPong_[1].prepare(channels, spec.maximumBlockSize);

        std::apply([&spec](auto&... n) {
            (n.prepare(spec), ...);
        }, nodes_);

        currentMix_ = targetMix_;
        currentGain_ = targetGain_;
    }

    void reset() override {
        std::apply([](auto&... n) {
            (n.reset(), ...);
        }, nodes_);
        currentMix_ = targetMix_;
        currentGain_ = targetGain_;
    }

    template <size_t Index>
    auto& getNode() noexcept {
        static_assert(Index < ChainLength, "Índice de nodo fuera de rango.");
        return std::get<Index>(nodes_);
    }

    template <size_t Index>
    const auto& getNode() const noexcept {
        static_assert(Index < ChainLength, "Índice de nodo fuera de rango.");
        return std::get<Index>(nodes_);
    }

    template <typename T>
    T& getNode() noexcept {
        return std::get<T>(nodes_);
    }

    template <typename T>
    const T& getNode() const noexcept {
        return std::get<T>(nodes_);
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case Mix:        targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            case OutputGain: targetGain_ = std::clamp(value, 0.0f, 2.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case Mix:        return targetMix_;
            case OutputGain: return targetGain_;
            default:         return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Container; }
    const char* getName() const override { return "Static Serial Chain"; }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard denormalGuard; // Regla 47

        if (context.numSamples == 0) return;

        const size_t numSamples = context.numSamples;
        const uint32_t numInCh = context.numInputChannels;
        const uint32_t numOutCh = context.numOutputChannels;
        const float alpha = 0.005f; // Anti-click smoothing (Regla 35)

        // Ejecutar la cadena serial estática a través de ping-pong de buffers contiguos alineados
        processChainHelper(context, std::make_index_sequence<ChainLength>{});

        // Aplicar mezcla Dry/Wet y Ganancia de salida suavizada
        const float mix = targetMix_;
        const float gain = targetGain_;
        const float* lastOutL = pingPong_[((ChainLength - 1) % 2)].getReadPointer(0);
        const float* lastOutR = pingPong_[((ChainLength - 1) % 2)].getReadPointer(1);

        for (size_t s = 0; s < numSamples; ++s) {
            currentGain_ += alpha * (gain - currentGain_);
            currentMix_ += alpha * (mix - currentMix_);

            const float rawInL = (numInCh > 0 && context.inputChannels[0]) ? context.inputChannels[0][s] : 0.0f;
            const float rawInR = (numInCh > 1 && context.inputChannels[1]) ? context.inputChannels[1][s] : rawInL;

            const float wetL = (lastOutL ? lastOutL[s] : 0.0f) * currentGain_;
            const float wetR = (lastOutR ? lastOutR[s] : 0.0f) * currentGain_;

            if (numOutCh > 0 && context.outputChannels[0]) {
                context.outputChannels[0][s] = (1.0f - currentMix_) * rawInL + currentMix_ * wetL;
            }
            if (numOutCh > 1 && context.outputChannels[1]) {
                context.outputChannels[1][s] = (1.0f - currentMix_) * rawInR + currentMix_ * wetR;
            }
        }
    }

private:
    template <size_t... Is>
    void processChainHelper(ProcessContext& context, std::index_sequence<Is...>) {
        (processSingleNode<Is>(context), ...);
    }

    template <size_t Index>
    void processSingleNode(ProcessContext& context) {
        auto& node = std::get<Index>(nodes_);
        const uint32_t channels = std::max(2u, spec_.numOutputChannels);

        const float* inPointers[2];
        float* outPointers[2];

        if constexpr (Index == 0) {
            // Primer nodo: lee directamente de context.inputChannels
            inPointers[0] = (context.numInputChannels > 0 && context.inputChannels[0])
                ? context.inputChannels[0] : nullptr;
            inPointers[1] = (context.numInputChannels > 1 && context.inputChannels[1])
                ? context.inputChannels[1] : inPointers[0];
        } else {
            // Nodos subsiguientes: leen del buffer ping-pong anterior
            const size_t prevBufIdx = (Index - 1) % 2;
            inPointers[0] = pingPong_[prevBufIdx].getReadPointer(0);
            inPointers[1] = pingPong_[prevBufIdx].getReadPointer(1);
        }

        // Buffer ping-pong de salida para este nodo
        const size_t outBufIdx = Index % 2;
        outPointers[0] = pingPong_[outBufIdx].getWritePointer(0);
        outPointers[1] = pingPong_[outBufIdx].getWritePointer(1);

        ProcessContext subCtx{
            inPointers,
            outPointers,
            context.numInputChannels,
            channels,
            context.numSamples,
            context.sidechainChannels,
            context.numSidechainChannels,
            context.audioRateModChannels,
            context.numAudioRateModChannels,
            context.bpm,
            context.ppqPosition,
            context.isPlaying
        };

        node.process(subCtx);
    }

    ProcessSpec spec_{ 44100.0, 512, 2, 2 };
    std::tuple<Nodes...> nodes_;
    std::array<PreallocatedBuffer, 2> pingPong_;

    float targetMix_{ 1.0f };
    float currentMix_{ 1.0f };
    float targetGain_{ 1.0f };
    float currentGain_{ 1.0f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 2> params_;
};

} // namespace audio_graph
