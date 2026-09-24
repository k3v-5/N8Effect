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
#include "../dsp/core/TestSynthEngine.h"

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
    PerformanceMetrics getPerformanceMetrics() const noexcept { return dualWorldEngine_.getPerformanceMetrics(); }
    void resetCpuOverload() noexcept { dualWorldEngine_.getCpuProfiler().resetOverload(); }

    juce::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState_; }
    TestSynthEngine& getTestSynth() noexcept { return testSynth_; }

    bool recompilePlan();
    NodeId addNodeToGraph(NodeType type, float x = 100.0f, float y = 100.0f);
    bool removeNodeFromGraph(NodeId id);
    ConnectionId connectNodes(NodeId srcNode, PinId srcPin, NodeId destNode, PinId destPin);

    // Operaciones sobre la Cola Secuencial (Reglas 4, 6, 28, 30)
    std::vector<NodeId> getLinearNodeChain() const;
    bool setLinearNodeChain(const std::vector<NodeId>& newOrder);
    NodeId insertNodeInLinearChain(NodeType type, int index = -1);
    bool removeNodeFromLinearChain(NodeId id);
    bool moveNodeInLinearChain(int fromIndex, int toIndex);

    const ProcessSpec& getCurrentSpec() const noexcept { return currentSpec_; }

private:
    ProcessSpec currentSpec_{ 44100.0, 512, 2, 2 };
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts_;
    DualWorldEngine dualWorldEngine_;
    Graph graph_;
    ExecutionPlan currentPlan_;
    PresetManager presetManager_;
    SceneManager sceneManager_;
    GraphUndoManager undoManager_;

    juce::MidiKeyboardState keyboardState_;
    TestSynthEngine testSynth_;

    // Punteros atómicos a parámetros para lectura ultra rápida en el hilo de audio
    std::atomic<float>* dryParam_{ nullptr };
    std::atomic<float>* wetParam_{ nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(N8AudioProcessor)
};

} // namespace audio_graph
