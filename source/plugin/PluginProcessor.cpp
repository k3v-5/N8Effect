#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../dsp/PassthroughNode.h"
#include "../dsp/SimpleFilterNode.h"
#include "../dsp/SimpleDelayNode.h"
#include "../dsp/AllDspNodes.h"
#include "../graph/NodeFactory.h"

namespace audio_graph {

N8AudioProcessor::N8AudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "Parameters", createParameterLayout())
{
    dryParam_ = apvts_.getRawParameterValue("dry_level");
    wetParam_ = apvts_.getRawParameterValue("wet_level");

    const char* macroIds[8] = {
        "macro_texture", "macro_motion", "macro_space", "macro_color",
        "macro_chaos", "macro_density", "macro_energy", "macro_morph"
    };
    for (size_t i = 0; i < 8; ++i) {
        macroParams_[i] = apvts_.getRawParameterValue(macroIds[i]);
    }

    // Cargar la Cadena Maestra de 10 Efectos con bifurcación paralela y secuenciación por defecto
    PresetMetadata meta;
    std::array<float, 8> macros;
    if (presetManager_.loadFactoryPreset(0, graph_, meta, macros)) {
        dualWorldEngine_.setDryLevel(meta.dryLevel);
        dualWorldEngine_.setWetLevel(meta.wetLevel);
        if (auto* p = apvts_.getParameter("dry_level")) {
            p->setValueNotifyingHost(apvts_.getParameterRange("dry_level").convertTo0to1(meta.dryLevel));
        }
        if (auto* p = apvts_.getParameter("wet_level")) {
            p->setValueNotifyingHost(apvts_.getParameterRange("wet_level").convertTo0to1(meta.wetLevel));
        }
        recompilePlan();
    } else {
        auto nFilter = graph_.addNode(std::make_unique<SimpleFilterNode>(), "SVFFilter");
        auto nDelay = graph_.addNode(std::make_unique<SimpleDelayNode>(), "StereoDelay");
        graph_.connect(nFilter, 2, nDelay, 1);
        recompilePlan();
    }

    undoManager_.pushState(graph_);
}

juce::AudioProcessorValueTreeState::ParameterLayout N8AudioProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Regla 8: Parámetros con IDs únicos y estables
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "dry_level", 1 },
        "Dry Level",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        1.0f // Por defecto: DRY 100% puro (Regla 1)
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "wet_level", 1 },
        "Wet / Event Level",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        1.0f
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "filter_cutoff", 1 },
        "Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        2500.0f
    ));

    const char* macroIds[8] = {
        "macro_texture", "macro_motion", "macro_space", "macro_color",
        "macro_chaos", "macro_density", "macro_energy", "macro_morph"
    };
    const char* macroNames[8] = {
        "Texture", "Motion", "Space", "Color",
        "Chaos", "Density", "Energy", "Morph"
    };
    for (size_t i = 0; i < 8; ++i) {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ macroIds[i], 1 },
            macroNames[i],
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
            0.5f
        ));
    }

    return { params.begin(), params.end() };
}

void N8AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSpec_ = ProcessSpec{
        .sampleRate = sampleRate,
        .maximumBlockSize = static_cast<uint32_t>(samplesPerBlock),
        .numInputChannels = static_cast<uint32_t>(getTotalNumInputChannels()),
        .numOutputChannels = static_cast<uint32_t>(getTotalNumOutputChannels())
    };

    dualWorldEngine_.prepare(currentSpec_);
    testSynth_.prepare(sampleRate, samplesPerBlock);
    dualWorldEngine_.getExecutor().setProbeVisualizer(&probeVisualizerBuffer_);

    for (const auto& [_, inst] : graph_.getNodes()) {
        if (inst && inst->processor) {
            inst->processor->prepare(currentSpec_);
        }
    }
}

void N8AudioProcessor::releaseResources() {
    dualWorldEngine_.reset();
    testSynth_.reset();
}

void N8AudioProcessor::reset() {
    dualWorldEngine_.reset();
    testSynth_.reset();
    for (const auto& [_, inst] : graph_.getNodes()) {
        if (inst && inst->processor) {
            inst->processor->reset();
        }
    }
}

bool N8AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainIn != juce::AudioChannelSet::mono() && mainIn != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.inputBuses.size() > 1) {
        const auto& sidechain = layouts.getChannelSet(true, 1);
        if (!sidechain.isDisabled() && sidechain != juce::AudioChannelSet::mono() && sidechain != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
}

void N8AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/) {
    if (isSuspended()) {
        buffer.clear();
        return;
    }

    struct AudioScope {
        std::atomic<bool>& flag;
        explicit AudioScope(std::atomic<bool>& f) noexcept : flag(f) { flag.store(true, std::memory_order_release); }
        ~AudioScope() noexcept { flag.store(false, std::memory_order_release); }
    } scope(isAudioThreadRunning_);

    juce::ScopedNoDenormals noDenormals; // Regla 9 y 34
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // Limpiar canales de salida no utilizados
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, numSamples);
    }

    const int bufferChannels = buffer.getNumChannels();
    const uint32_t synthChannels = static_cast<uint32_t>(std::min(bufferChannels, 2));

    // Inyectar señal de prueba del teclado visual si hay notas activas (Reglas 1, 9 y 17)
    if (testSynth_.hasActiveVoices()) {
        testSynth_.renderAndInject(buffer.getArrayOfWritePointers(), synthChannels, static_cast<uint32_t>(numSamples));
    }

    // Actualizar parámetros atómicos (sin lock)
    if (dryParam_ != nullptr) dualWorldEngine_.setDryLevel(dryParam_->load(std::memory_order_relaxed));
    if (wetParam_ != nullptr) dualWorldEngine_.setWetLevel(wetParam_->load(std::memory_order_relaxed));

    // Actualizar los 8 Macros globales para la ModulationMatrix (Reglas 7, 8 y 25)
    for (size_t i = 0; i < 8; ++i) {
        if (macroParams_[i] != nullptr) {
            dualWorldEngine_.getModulationEngine().getMacroManager().setMacro(
                static_cast<MacroManager::MacroIndex>(i),
                macroParams_[i]->load(std::memory_order_relaxed)
            );
        }
    }

    const uint32_t effectiveInChannels = testSynth_.hasActiveVoices()
        ? std::max(static_cast<uint32_t>(totalNumInputChannels), synthChannels)
        : static_cast<uint32_t>(totalNumInputChannels > 0 ? totalNumInputChannels : std::min(bufferChannels, 2));

    // Capturar bus de Sidechain del DAW host si está habilitado (Reglas 6, 13 y 37)
    auto scBus = getBusBuffer(buffer, true, 1);
    const float* const* scChannels = (scBus.getNumChannels() > 0) ? scBus.getArrayOfReadPointers() : nullptr;
    const uint32_t numScChannels = (scChannels != nullptr) ? static_cast<uint32_t>(scBus.getNumChannels()) : 0;

    // Obtener información de transporte del host si está disponible (Regla 37)
    ProcessContext context{
        .inputChannels = buffer.getArrayOfReadPointers(),
        .outputChannels = buffer.getArrayOfWritePointers(),
        .numInputChannels = effectiveInChannels,
        .numOutputChannels = static_cast<uint32_t>(totalNumOutputChannels),
        .numSamples = static_cast<uint32_t>(numSamples),
        .sidechainChannels = scChannels,
        .numSidechainChannels = numScChannels
    };

    if (auto* currentPlayHead = getPlayHead()) {
        if (auto posOpt = currentPlayHead->getPosition()) {
            if (posOpt->getBpm()) context.bpm = *posOpt->getBpm();
            if (posOpt->getPpqPosition()) context.ppqPosition = *posOpt->getPpqPosition();
            context.isPlaying = posOpt->getIsPlaying();
        }
    }

    // Procesamiento Dual World (Dry puro vs Event World con Modulación Universal)
    const auto* plan = activePlan_.load(std::memory_order_acquire);
    if (plan != nullptr) {
        dualWorldEngine_.process(*plan, context, buffer.getArrayOfWritePointers(), &graph_);
    }

    // Telemetría para el analizador visual (Reglas 9, 23 y 26)
    visualizerBuffer_.writeBlock(buffer.getReadPointer(0),
                                 buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : buffer.getReadPointer(0),
                                 static_cast<size_t>(numSamples));
}

juce::AudioProcessorEditor* N8AudioProcessor::createEditor() {
    return new N8AudioProcessorEditor(*this);
}

bool N8AudioProcessor::hasEditor() const {
    return true;
}

const juce::String N8AudioProcessor::getName() const {
    return "Audio Event Graph Engine";
}

bool N8AudioProcessor::acceptsMidi() const { return true; }
bool N8AudioProcessor::producesMidi() const { return false; }
bool N8AudioProcessor::isMidiEffect() const { return false; }
double N8AudioProcessor::getTailLengthSeconds() const { return 3.0; }

int N8AudioProcessor::getNumPrograms() { return 1; }
int N8AudioProcessor::getCurrentProgram() { return 0; }
void N8AudioProcessor::setCurrentProgram(int) {}
const juce::String N8AudioProcessor::getProgramName(int) { return "Default"; }
void N8AudioProcessor::changeProgramName(int, const juce::String&) {}

void N8AudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    PresetMetadata meta{
        .schemaVersion = GraphSerializer::CurrentSchemaVersion,
        .name = "DAW Saved State",
        .dryLevel = dualWorldEngine_.getDryLevel(),
        .wetLevel = dualWorldEngine_.getWetLevel()
    };
    std::array<float, 8> macros{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    std::string json = GraphSerializer::serialize(graph_, meta, macros);
    destData.append(json.data(), json.size());
}

void N8AudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (data == nullptr || sizeInBytes <= 0) return;
    std::string json(reinterpret_cast<const char*>(data), sizeInBytes);
    if (!json.empty() && json.find('{') != std::string::npos) {
        PresetMetadata meta;
        std::array<float, 8> macros;
        std::string err;
        if (GraphSerializer::deserialize(json, graph_, meta, macros, err)) {
            dualWorldEngine_.setDryLevel(meta.dryLevel);
            dualWorldEngine_.setWetLevel(meta.wetLevel);
            if (auto* p = apvts_.getParameter("dry_level")) {
                p->setValueNotifyingHost(apvts_.getParameterRange("dry_level").convertTo0to1(meta.dryLevel));
            }
            if (auto* p = apvts_.getParameter("wet_level")) {
                p->setValueNotifyingHost(apvts_.getParameterRange("wet_level").convertTo0to1(meta.wetLevel));
            }
            recompilePlan();
        }
    }
}

bool N8AudioProcessor::recompilePlan() {
    for (const auto& [_, inst] : graph_.getNodes()) {
        if (inst && inst->processor) {
            inst->processor->prepare(currentSpec_);
        }
    }

    std::vector<NodeId> sorted;
    std::string err;
    if (graph_.validateAndTopologicalSort(sorted, err)) {
        // Double buffering thread-safe (Reglas 9, 26, 30)
        auto* current = activePlan_.load(std::memory_order_relaxed);
        ExecutionPlan* nextPlan = (current == &planA_) ? &planB_ : &planA_;
        nextPlan->compileFrom(graph_, sorted);
        activePlan_.store(nextPlan, std::memory_order_release);
        return true;
    }
    return false;
}

NodeId N8AudioProcessor::addNodeToGraph(NodeType type, float x, float y) {
    return executeSafeGraphMutation([this, type, x, y]() -> NodeId {
        auto proc = NodeFactory::getInstance().create(type);
        if (!proc) return InvalidNodeId;

        proc->prepare(currentSpec_);
        NodeId id = graph_.addNode(std::move(proc), "", x, y);
        if (recompilePlan()) {
            undoManager_.pushState(graph_);
        }
        return id;
    });
}

bool N8AudioProcessor::removeNodeFromGraph(NodeId id) {
    return executeSafeGraphMutation([this, id]() -> bool {
        bool res = graph_.removeNode(id);
        if (res) {
            recompilePlan();
            undoManager_.pushState(graph_);
        }
        return res;
    });
}

ConnectionId N8AudioProcessor::connectNodes(NodeId srcNode, PinId srcPin, NodeId destNode, PinId destPin) {
    return executeSafeGraphMutation([this, srcNode, srcPin, destNode, destPin]() -> ConnectionId {
        ConnectionId cid = graph_.connect(srcNode, srcPin, destNode, destPin);
        if (cid != 0) {
            if (!recompilePlan()) {
                graph_.disconnect(cid);
                return 0;
            }
            undoManager_.pushState(graph_);
        }
        return cid;
    });
}

bool N8AudioProcessor::disconnectConnection(ConnectionId cid) {
    return executeSafeGraphMutation([this, cid]() -> bool {
        if (graph_.disconnect(cid)) {
            recompilePlan();
            undoManager_.pushState(graph_);
            return true;
        }
        return false;
    });
}

bool N8AudioProcessor::disconnectPin(NodeId nodeId, PinId pinId) {
    return executeSafeGraphMutation([this, nodeId, pinId]() -> bool {
        const auto connections = graph_.getConnections();
        bool any = false;
        for (const auto& c : connections) {
            if ((c.sourceNodeId == nodeId && c.sourcePinId == pinId) ||
                (c.destNodeId == nodeId && c.destPinId == pinId)) {
                graph_.disconnect(c.id);
                any = true;
            }
        }
        if (any) {
            recompilePlan();
            undoManager_.pushState(graph_);
        }
        return any;
    });
}

bool N8AudioProcessor::loadFactoryPreset(size_t index) {
    return executeSafeGraphMutation([this, index]() -> bool {
        PresetMetadata meta;
        std::array<float, 8> macros;
        if (presetManager_.loadFactoryPreset(index, graph_, meta, macros)) {
            dualWorldEngine_.setDryLevel(meta.dryLevel);
            dualWorldEngine_.setWetLevel(meta.wetLevel);

            if (auto* p = apvts_.getParameter("dry_level")) {
                p->setValueNotifyingHost(apvts_.getParameterRange("dry_level").convertTo0to1(meta.dryLevel));
            }
            if (auto* p = apvts_.getParameter("wet_level")) {
                p->setValueNotifyingHost(apvts_.getParameterRange("wet_level").convertTo0to1(meta.wetLevel));
            }

            recompilePlan();
            undoManager_.pushState(graph_, meta, macros);
            return true;
        }
        return false;
    });
}

bool N8AudioProcessor::loadPresetFromJson(const std::string& json) {
    return executeSafeGraphMutation([this, &json]() -> bool {
        PresetMetadata meta;
        std::array<float, 8> macros;
        std::string err;
        if (GraphSerializer::deserialize(json, graph_, meta, macros, err)) {
            dualWorldEngine_.setDryLevel(meta.dryLevel);
            dualWorldEngine_.setWetLevel(meta.wetLevel);

            if (auto* p = apvts_.getParameter("dry_level")) {
                p->setValueNotifyingHost(apvts_.getParameterRange("dry_level").convertTo0to1(meta.dryLevel));
            }
            if (auto* p = apvts_.getParameter("wet_level")) {
                p->setValueNotifyingHost(apvts_.getParameterRange("wet_level").convertTo0to1(meta.wetLevel));
            }

            recompilePlan();
            undoManager_.pushState(graph_, meta, macros);
            return true;
        }
        return false;
    });
}

bool N8AudioProcessor::undo(PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
    return executeSafeGraphMutation([this, &outMeta, &outMacros]() -> bool {
        if (undoManager_.undo(graph_, outMeta, outMacros)) {
            dualWorldEngine_.setDryLevel(outMeta.dryLevel);
            dualWorldEngine_.setWetLevel(outMeta.wetLevel);
            if (auto* p = apvts_.getParameter("dry_level")) {
                p->setValueNotifyingHost(apvts_.getParameterRange("dry_level").convertTo0to1(outMeta.dryLevel));
            }
            if (auto* p = apvts_.getParameter("wet_level")) {
                p->setValueNotifyingHost(apvts_.getParameterRange("wet_level").convertTo0to1(outMeta.wetLevel));
            }
            recompilePlan();
            return true;
        }
        return false;
    });
}

bool N8AudioProcessor::redo(PresetMetadata& outMeta, std::array<float, 8>& outMacros) {
    return executeSafeGraphMutation([this, &outMeta, &outMacros]() -> bool {
        if (undoManager_.redo(graph_, outMeta, outMacros)) {
            dualWorldEngine_.setDryLevel(outMeta.dryLevel);
            dualWorldEngine_.setWetLevel(outMeta.wetLevel);
            if (auto* p = apvts_.getParameter("dry_level")) {
                p->setValueNotifyingHost(apvts_.getParameterRange("dry_level").convertTo0to1(outMeta.dryLevel));
            }
            if (auto* p = apvts_.getParameter("wet_level")) {
                p->setValueNotifyingHost(apvts_.getParameterRange("wet_level").convertTo0to1(outMeta.wetLevel));
            }
            recompilePlan();
            return true;
        }
        return false;
    });
}

bool N8AudioProcessor::randomizeGraph(SmartRandomizer::RandomMode mode) {
    return executeSafeGraphMutation([this, mode]() -> bool {
        PresetMetadata meta;
        std::array<float, 8> macros{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
        if (SmartRandomizer::applyRandom(graph_, meta, macros, undoManager_, mode)) {
            recompilePlan();
            return true;
        }
        return false;
    });
}

} // namespace audio_graph

// Punto de entrada JUCE
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new audio_graph::N8AudioProcessor();
}
