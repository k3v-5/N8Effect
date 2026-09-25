#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../core/Types.h"
#include "../core/RealtimePools.h"
#include "../graph/Graph.h"
#include "../graph/GraphExecutor.h"
#include "../plugin/DualWorldEngine.h"
#include "../preset/GraphSerializer.h"
#include "../preset/SceneManager.h"
#include "../preset/PresetManager.h"
#include "../preset/GraphUndoManager.h"
#include "../preset/SmartRandomizer.h"
#include "../midi/MpeManager.h"
#include "../midi/MidiMappingManager.h"

#include "../dsp/core/TestInputSynthesizer.h"

namespace audio_graph {

class N8AudioProcessor : public juce::AudioProcessor {
public:
    N8AudioProcessor();
    ~N8AudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    DualWorldEngine& getDualWorldEngine() noexcept { return dualWorldEngine_; }
    Graph& getGraph() noexcept { return graph_; }
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts_; }
    PresetManager& getPresetManager() noexcept { return presetManager_; }
    SceneManager& getSceneManager() noexcept { return sceneManager_; }
    GraphUndoManager& getUndoManager() noexcept { return undoManager_; }
    TestInputSynthesizer& getTestSynthesizer() noexcept { return testSynth_; }
    PerformanceMetrics getPerformanceMetrics() const noexcept { return dualWorldEngine_.getPerformanceMetrics(); }
    void resetCpuOverload() noexcept { dualWorldEngine_.getCpuProfiler().resetOverload(); }

    MpeManager& getMpeManager() noexcept { return mpeManager_; }
    MidiMappingManager& getMidiMappingManager() noexcept { return midiMappingManager_; }

    AudioVisualizerBuffer& getVisualizerBuffer() noexcept { return visualizerBuffer_; }
    AudioVisualizerBuffer& getProbeVisualizerBuffer() noexcept { return probeVisualizerBuffer_; }
    void setProbeNodeId(NodeId id) noexcept { dualWorldEngine_.getExecutor().setProbeNodeId(id); }
    NodeId getProbeNodeId() const noexcept { return dualWorldEngine_.getExecutor().getProbeNodeId(); }

    bool recompilePlan();
    NodeId addNodeToGraph(NodeType type, float x = 100.0f, float y = 100.0f);
    bool removeNodeFromGraph(NodeId id);
    ConnectionId connectNodes(NodeId srcNode, PinId srcPin, NodeId destNode, PinId destPin);
    bool disconnectConnection(ConnectionId cid);
    bool disconnectPin(NodeId nodeId, PinId pinId);
    bool loadFactoryPreset(size_t index);
    bool loadPresetFromJson(const std::string& json);
    bool undo(PresetMetadata& outMeta, std::array<float, 8>& outMacros);
    bool redo(PresetMetadata& outMeta, std::array<float, 8>& outMacros);
    bool randomizeGraph(SmartRandomizer::RandomMode mode = SmartRandomizer::RandomMode::ModerateMutation);
    const ProcessSpec& getCurrentSpec() const noexcept { return currentSpec_; }

    void setNodeBypassed(NodeId id, bool bypassed);
    bool isNodeBypassed(NodeId id) const;

    // Métodos para la Tira Secuencial / Cola de Efectos (Estilo Arturia Efx MOTIONS)
    std::vector<NodeId> getLinearNodeChain();
    void moveNodeInLinearChain(int fromIdx, int toIdx);
    void removeNodeFromLinearChain(NodeId id);
    void insertNodeInLinearChain(NodeType type, int insertIndex);

private:
    std::atomic<bool> isAudioThreadRunning_{ false };

    template <typename Func>
    auto executeSafeGraphMutation(Func&& func) {
        suspendProcessing(true);
        while (isAudioThreadRunning_.load(std::memory_order_acquire)) {
            juce::Thread::sleep(1);
        }
        const juce::ScopedLock sl(getCallbackLock());
        auto result = func();
        suspendProcessing(false);
        return result;
    }

    ProcessSpec currentSpec_{ 44100.0, 512, 2, 2 };
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts_;
    DualWorldEngine dualWorldEngine_;
    Graph graph_;
    ExecutionPlan planA_;
    ExecutionPlan planB_;
    std::atomic<const ExecutionPlan*> activePlan_{ nullptr };
    PresetManager presetManager_;
    SceneManager sceneManager_;
    GraphUndoManager undoManager_;
    TestInputSynthesizer testSynth_;
    MpeManager mpeManager_;
    MidiMappingManager midiMappingManager_;

    // Telemetría visual lock-free para analizador de espectro, osciloscopio y goniometro (Reglas 9, 23, 26)
    AudioVisualizerBuffer visualizerBuffer_;
    AudioVisualizerBuffer probeVisualizerBuffer_;

    // Punteros atómicos a parámetros para lectura ultra rápida en el hilo de audio
    std::atomic<float>* dryParam_{ nullptr };
    std::atomic<float>* wetParam_{ nullptr };
    std::array<std::atomic<float>*, 8> macroParams_{ nullptr };

    // Orden lineal de efectos para SequentialStripComponent
    std::vector<NodeId> linearNodeOrder_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(N8AudioProcessor)
};

} // namespace audio_graph
