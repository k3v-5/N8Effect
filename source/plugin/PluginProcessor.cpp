#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../dsp/PassthroughNode.h"
#include "../dsp/SimpleFilterNode.h"
#include "../dsp/SimpleDelayNode.h"
#include "../graph/NodeFactory.h"

namespace audio_graph {

N8AudioProcessor::N8AudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "Parameters", createParameterLayout())
{
    dryParam_ = apvts_.getRawParameterValue("dry_level");
    wetParam_ = apvts_.getRawParameterValue("wet_level");

    // Construcción del grafo por defecto para el Event World
    auto nFilter = graph_.addNode(std::make_unique<SimpleFilterNode>(), "SVFFilter");
    auto nDelay = graph_.addNode(std::make_unique<SimpleDelayNode>(), "StereoDelay");
    graph_.connect(nFilter, 2, nDelay, 1);

    std::vector<NodeId> sorted;
    std::string err;
    if (graph_.validateAndTopologicalSort(sorted, err)) {
        currentPlan_.compileFrom(graph_, sorted);
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
    testSynth_.prepare(sampleRate);

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
    keyboardState_.reset();
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

    return true;
}

void N8AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals; // Regla 9 y 34
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // 1. Integrar eventos del teclado virtual en el buffer MIDI (Reglas 1, 9, 23)
    keyboardState_.processNextMidiBuffer(midiMessages, 0, numSamples, true);

    // 2. Procesar mensajes MIDI en el sintetizador de pruebas
    for (const auto metadata : midiMessages) {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            testSynth_.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        } else if (msg.isNoteOff()) {
            testSynth_.noteOff(msg.getNoteNumber());
        } else if (msg.isAllNotesOff() || msg.isAllSoundOff()) {
            testSynth_.allNotesOff();
        } else if (msg.isPitchWheel()) {
            const float bend = static_cast<float>(msg.getPitchWheelValue() - 8192) / 8192.0f * 2.0f;
            testSynth_.setPitchBend(bend);
        }
    }

    // 3. Limpiar canales de salida no utilizados
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, numSamples);
    }

    // 4. Inyectar audio del sintetizador de prueba en el buffer de entrada para audicionar el grafo
    const uint32_t activeChannels = static_cast<uint32_t>(std::max(totalNumInputChannels, std::min(totalNumOutputChannels, 2)));
    if (activeChannels > 0) {
        testSynth_.renderAudioAdding(buffer.getArrayOfWritePointers(), activeChannels, static_cast<uint32_t>(numSamples));
    }

    // Actualizar parámetros atómicos (sin lock)
    if (dryParam_ != nullptr) dualWorldEngine_.setDryLevel(dryParam_->load(std::memory_order_relaxed));
    if (wetParam_ != nullptr) dualWorldEngine_.setWetLevel(wetParam_->load(std::memory_order_relaxed));

    // Obtener información de transporte del host si está disponible (Regla 37)
    ProcessContext context{
        .inputChannels = const_cast<const float**>(buffer.getArrayOfReadPointers()),
        .outputChannels = buffer.getArrayOfWritePointers(),
        .numInputChannels = activeChannels,
        .numOutputChannels = static_cast<uint32_t>(totalNumOutputChannels),
        .numSamples = static_cast<uint32_t>(numSamples)
    };

    if (auto* currentPlayHead = getPlayHead()) {
        if (auto posOpt = currentPlayHead->getPosition()) {
            if (posOpt->getBpm()) context.bpm = *posOpt->getBpm();
            if (posOpt->getPpqPosition()) context.ppqPosition = *posOpt->getPpqPosition();
            context.isPlaying = posOpt->getIsPlaying();
        }
    }

    // Procesamiento Dual World (Dry puro vs Event World con Modulación Universal)
    dualWorldEngine_.process(currentPlan_, context, buffer.getArrayOfWritePointers(), &graph_);
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
            recompilePlan();
        }
    }
}

bool N8AudioProcessor::recompilePlan() {
    std::vector<NodeId> sorted;
    std::string err;
    if (graph_.validateAndTopologicalSort(sorted, err)) {
        ExecutionPlan newPlan;
        newPlan.compileFrom(graph_, sorted);
        currentPlan_ = std::move(newPlan);
        return true;
    }
    return false;
}

NodeId N8AudioProcessor::addNodeToGraph(NodeType type, float x, float y) {
    auto proc = NodeFactory::getInstance().create(type);
    if (!proc) return InvalidNodeId;

    proc->prepare(currentSpec_);
    NodeId id = graph_.addNode(std::move(proc), "", x, y);
    if (recompilePlan()) {
        undoManager_.pushState(graph_);
    }
    return id;
}

bool N8AudioProcessor::removeNodeFromGraph(NodeId id) {
    bool res = graph_.removeNode(id);
    if (res) {
        recompilePlan();
        undoManager_.pushState(graph_);
    }
    return res;
}

ConnectionId N8AudioProcessor::connectNodes(NodeId srcNode, PinId srcPin, NodeId destNode, PinId destPin) {
    ConnectionId cid = graph_.connect(srcNode, srcPin, destNode, destPin);
    if (cid != 0) {
        if (!recompilePlan()) {
            graph_.disconnect(cid);
            return 0;
        }
        undoManager_.pushState(graph_);
    }
    return cid;
}

std::vector<NodeId> N8AudioProcessor::getLinearNodeChain() const {
    std::vector<NodeId> sorted;
    std::string err;
    if (graph_.validateAndTopologicalSort(sorted, err)) {
        return sorted;
    }
    std::vector<NodeId> ids;
    for (const auto& [id, _] : graph_.getNodes()) {
        ids.push_back(id);
    }
    return ids;
}

bool N8AudioProcessor::setLinearNodeChain(const std::vector<NodeId>& newOrder) {
    // 1. Desconectar enlaces de audio estéreo previos entre nodos
    std::vector<ConnectionId> toRemove;
    for (const auto& c : graph_.getConnections()) {
        if (c.sourcePinId == 2 && c.destPinId == 1) {
            toRemove.push_back(c.id);
        }
    }
    for (ConnectionId cid : toRemove) {
        graph_.disconnect(cid);
    }

    // 2. Conectar en serie secuencial: newOrder[i] Pin 2 -> newOrder[i+1] Pin 1
    if (newOrder.size() > 1) {
        for (size_t i = 0; i < newOrder.size() - 1; ++i) {
            graph_.connect(newOrder[i], 2, newOrder[i + 1], 1);
        }
    }

    // 3. Alinear en el canvas 2D para sincronía visual absoluta
    for (size_t i = 0; i < newOrder.size(); ++i) {
        if (auto* node = graph_.getNode(newOrder[i])) {
            node->posX = 80.0f + static_cast<float>(i) * 230.0f;
            node->posY = 140.0f;
        }
    }

    // 4. Recompilar plan seguro (Safe Swap) y registrar Undo
    bool ok = recompilePlan();
    if (ok) {
        undoManager_.pushState(graph_);
    }
    return ok;
}

NodeId N8AudioProcessor::insertNodeInLinearChain(NodeType type, int index) {
    auto proc = NodeFactory::getInstance().create(type);
    if (!proc) return InvalidNodeId;

    proc->prepare(currentSpec_);
    NodeId id = graph_.addNode(std::move(proc), "", 100.0f, 100.0f);
    if (id == InvalidNodeId) return InvalidNodeId;

    auto chain = getLinearNodeChain();
    std::erase(chain, id);

    if (index < 0 || index >= static_cast<int>(chain.size())) {
        chain.push_back(id);
    } else {
        chain.insert(chain.begin() + index, id);
    }

    setLinearNodeChain(chain);
    return id;
}

bool N8AudioProcessor::removeNodeFromLinearChain(NodeId id) {
    auto chain = getLinearNodeChain();
    std::erase(chain, id);
    bool res = graph_.removeNode(id);
    if (res) {
        setLinearNodeChain(chain);
    }
    return res;
}

bool N8AudioProcessor::moveNodeInLinearChain(int fromIndex, int toIndex) {
    auto chain = getLinearNodeChain();
    if (fromIndex < 0 || fromIndex >= static_cast<int>(chain.size()) ||
        toIndex < 0 || toIndex >= static_cast<int>(chain.size()) ||
        fromIndex == toIndex) {
        return false;
    }

    NodeId nodeToMove = chain[static_cast<size_t>(fromIndex)];
    chain.erase(chain.begin() + fromIndex);
    chain.insert(chain.begin() + toIndex, nodeToMove);

    return setLinearNodeChain(chain);
}

} // namespace audio_graph

// Punto de entrada JUCE
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new audio_graph::N8AudioProcessor();
}
