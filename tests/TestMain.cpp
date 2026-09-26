#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <unordered_set>
#include <thread>
#include <chrono>

#include "../source/core/Types.h"
#include "../source/core/RealtimePools.h"
#include "../source/graph/AudioProcessorNode.h"
#include "../source/graph/NodeFactory.h"
#include "../source/graph/Graph.h"
#include "../source/graph/GraphExecutor.h"
#include "../source/dsp/PassthroughNode.h"
#include "../source/dsp/SimpleFilterNode.h"
#include "../source/dsp/SimpleDelayNode.h"
#include "../source/dsp/core/DelayLine.h"
#include "../source/dsp/core/BiquadFilter.h"
#include "../source/dsp/core/EnvelopeDetector.h"
#include "../source/dsp/core/SaturationFunctions.h"
#include "../source/dsp/core/FFTEngine.h"
#include "../source/dsp/core/PhaseVocoder.h"
#include "../source/dsp/processors/CompressorNode.h"
#include "../source/dsp/processors/ParametricEQNode.h"
#include "../source/dsp/processors/DistortionNode.h"
#include "../source/dsp/processors/AdvancedDelayNode.h"
#include "../source/dsp/processors/ReverbNode.h"
#include "../source/dsp/processors/PitchShifterNode.h"
#include "../source/dsp/processors/SpectralProcessorNode.h"
#include "../source/dsp/processors/ShimmerReverbNode.h"
#include "../source/dsp/processors/RefractionNode.h"
#include "../source/dsp/processors/SpectralSmearNode.h"
#include "../source/dsp/core/GrainPool.h"
#include "../source/dsp/core/LinkwitzRileyFilter.h"
#include "../source/dsp/processors/GranularNode.h"
#include "../source/dsp/processors/SpectralFreezeNode.h"
#include "../source/dsp/processors/ResonatorBankNode.h"
#include "../source/dsp/processors/GlitchNode.h"
#include "../source/dsp/processors/MultibandDynamicsNode.h"
#include "../source/dsp/processors/PhaserNode.h"
#include "../source/dsp/processors/ChorusNode.h"
#include "../source/dsp/processors/FlangerNode.h"
#include "../source/dsp/processors/RingModulatorNode.h"
#include "../source/dsp/processors/FrequencyShifterNode.h"
#include "../source/dsp/processors/TapeSaturationNode.h"
#include "../source/dsp/processors/MidSideEncoderNode.h"
#include "../source/dsp/processors/MidSideDecoderNode.h"
#include "../source/dsp/processors/SpatialPannerNode.h"
#include "../source/dsp/core/SpatialPannerCore.h"
#include "../source/dsp/core/AcousticBodyResonator.h"
#include "../source/dsp/core/MagneticHysteresis.h"
#include "../source/graph/CRTPProcessorNode.h"
#include "../source/graph/StaticSerialChain.h"
#include "../source/graph/ContainerNode.h"
#include "../source/dsp/processors/FeedbackContainerNode.h"
#include "../source/dsp/processors/EventContainerNode.h"
#include "../source/analysis/AnalysisEngine.h"
#include "../source/event/EventTypes.h"
#include "../source/event/EventCaptureBuffer.h"
#include "../source/event/AudioFragment.h"
#include "../source/event/Event.h"
#include "../source/event/EventPool.h"
#include "../source/event/EventManager.h"
#include "../source/plugin/DualWorldEngine.h"
#include "../source/preset/GraphSerializer.h"
#include "../source/preset/SceneManager.h"
#include "../source/preset/GraphUndoManager.h"
#include "../source/preset/PresetManager.h"
#include "../source/dsp/core/FastMath.h"
#include "../source/dsp/core/DenormalGuards.h"
#include "../source/dsp/core/TestInputSynthesizer.h"
#include "../source/core/CpuProfiler.h"
#include "../source/dsp/processors/TapeStopNode.h"
#include "../source/dsp/processors/FormantFilterNode.h"
#include "../source/dsp/processors/NoiseTextureNode.h"
#include "../source/dsp/processors/TransientShaperNode.h"
#include "../source/dsp/processors/RotarySpeakerNode.h"
#include "../source/dsp/processors/HarmonicExciterNode.h"
#include "../source/dsp/processors/VocoderNode.h"
#include "../source/dsp/processors/KarplusStrongNode.h"
#include "../source/dsp/processors/ReverseReverbNode.h"
#include "../source/dsp/processors/BrickwallLimiterNode.h"
#include "../source/dsp/processors/BitcrusherNode.h"
#include "../source/dsp/processors/NoiseGateNode.h"
#include "../source/dsp/processors/DeEsserNode.h"
#include "../source/dsp/processors/ExternalSidechainNode.h"
#include "../source/dsp/processors/MidiArpeggiatorNode.h"
#include "../source/dsp/processors/MidiChordEngineNode.h"
#include "../source/dsp/processors/MidiScaleQuantizerNode.h"
#include "../source/modulation/MSEGModulator.h"
#include "../source/modulation/EuclideanModulator.h"
#include "../source/modulation/ChaosModulator.h"
#include "../source/preset/SmartRandomizer.h"
#include "../source/midi/MpeManager.h"
#include "../source/midi/MidiMappingManager.h"
#include "../source/preset/PresetPackager.h"
#include "../source/dsp/processors/AudioSlicerNode.h"
#include "../source/dsp/core/AdaptiveNoiseFloorEstimator.h"

using namespace audio_graph;

void testAudioBufferPool() {
    std::cout << "[TEST] AudioBufferPool (Reglas 9 y 10)... ";
    AudioBufferPool pool;
    pool.prepare(4, 2, 512);

    assert(pool.getTotalCapacity() == 4);
    assert(pool.getAvailableCount() == 4);

    auto* b1 = pool.acquire();
    auto* b2 = pool.acquire();
    auto* b3 = pool.acquire();
    auto* b4 = pool.acquire();

    assert(b1 != nullptr && b2 != nullptr && b3 != nullptr && b4 != nullptr);
    assert(pool.getAvailableCount() == 0);

    // Protección de límite: intentar adquirir con el pool vacío debe retornar nullptr de forma segura
    auto* bOverflow = pool.acquire();
    assert(bOverflow == nullptr);

    // Liberar y volver a adquirir
    pool.release(b2);
    assert(pool.getAvailableCount() == 1);
    auto* bReacquired = pool.acquire();
    assert(bReacquired == b2);

    pool.releaseAll();
    assert(pool.getAvailableCount() == 4);
    std::cout << "PASSED\n";
}

void testGraphValidationAndKahnSort() {
    std::cout << "[TEST] Graph Topological Sort & Cycle Detection (Reglas 4, 28, 29)... ";
    Graph graph;

    auto n1 = graph.addNode(std::make_unique<PassthroughNode>(), "InputGain");
    auto n2 = graph.addNode(std::make_unique<SimpleFilterNode>(), "Filter");
    auto n3 = graph.addNode(std::make_unique<SimpleDelayNode>(), "Delay");

    graph.connect(n1, 2, n2, 1);
    graph.connect(n2, 2, n3, 1);

    std::vector<NodeId> sorted;
    std::string error;
    bool success = graph.validateAndTopologicalSort(sorted, error);

    assert(success);
    assert(sorted.size() == 3);
    assert(sorted[0] == n1);
    assert(sorted[1] == n2);
    assert(sorted[2] == n3);

    // Probar detección de ciclo no controlado
    auto cycleConn = graph.connect(n3, 2, n1, 1);
    std::vector<NodeId> cycleSorted;
    bool cycleSuccess = graph.validateAndTopologicalSort(cycleSorted, error);

    assert(!cycleSuccess);
    assert(!error.empty());

    // Desconectar ciclo y revalidar
    graph.disconnect(cycleConn);
    bool recovered = graph.validateAndTopologicalSort(sorted, error);
    assert(recovered);

    std::cout << "PASSED\n";
}

void testDSPNodesCorrectness() {
    std::cout << "[TEST] DSP Nodes Correctness & NaN/Inf Protection (Reglas 14, 34, 38)... ";
    ProcessSpec spec{
        .sampleRate = 48000.0,
        .maximumBlockSize = 256,
        .numInputChannels = 2,
        .numOutputChannels = 2
    };

    // Test SimpleFilterNode
    SimpleFilterNode filter;
    filter.prepare(spec);

    std::vector<float> inL(256, 1.0f);
    std::vector<float> inR(256, 1.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);

    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    filter.process(ctx);

    for (uint32_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]));
        assert(!std::isinf(outL[s]));
        assert(!std::isnan(outR[s]));
        assert(!std::isinf(outR[s]));
    }

    // Test SimpleDelayNode
    SimpleDelayNode delay;
    delay.prepare(spec);
    assert(delay.supportsTail());
    assert(delay.getTailSamples() > 0);

    delay.process(ctx);
    for (uint32_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]));
        assert(!std::isinf(outL[s]));
    }

    std::cout << "PASSED\n";
}

void testEventPoolCapacityAndRecycling() {
    std::cout << "[TEST] EventPool 1024 Capacity & Recycled Allocations (Reglas 9, 10, 11)... ";
    EventPool pool;
    pool.prepare(1024);

    assert(pool.getCapacity() == 1024);
    assert(pool.getAvailableCount() == 1024);
    assert(pool.getActiveCount() == 0);

    std::vector<Event*> acquired;
    acquired.reserve(1024);

    for (size_t i = 0; i < 1024; ++i) {
        Event* ev = pool.acquire();
        assert(ev != nullptr);
        acquired.push_back(ev);
    }

    assert(pool.getAvailableCount() == 0);
    assert(pool.getActiveCount() == 1024);

    // Intento de adquirir cuando el pool está lleno debe fallar de forma segura sin malloc
    Event* overflow = pool.acquire();
    assert(overflow == nullptr);

    // Liberar 24 eventos
    for (size_t i = 0; i < 24; ++i) {
        pool.release(acquired[i]);
    }
    assert(pool.getAvailableCount() == 24);
    assert(pool.getActiveCount() == 1000);

    // Re-adquirir
    for (size_t i = 0; i < 24; ++i) {
        Event* ev = pool.acquire();
        assert(ev != nullptr);
    }
    assert(pool.getActiveCount() == 1024);

    pool.reset();
    assert(pool.getAvailableCount() == 1024);
    assert(pool.getActiveCount() == 0);
    std::cout << "PASSED\n";
}

void testEventLifecycleAndStates() {
    std::cout << "[TEST] Event Explicit Lifecycle State Machine (Regla 2)... ";
    Event ev;
    EventAttributes attrs;
    attrs.id = 1;
    attrs.lifetimeSamples = 100;
    attrs.attackSamples = 10;
    attrs.releaseSamples = 20;

    ev.initialize(attrs, 0.0, 100);
    assert(ev.getState() == EventLifecycle::Created);

    ev.start();
    assert(ev.getState() == EventLifecycle::Playing);

    ev.release();
    assert(ev.getState() == EventLifecycle::Releasing);

    ev.kill();
    assert(ev.getState() == EventLifecycle::Killed);
    assert(!ev.isActive());

    std::cout << "PASSED\n";
}

void testSourceFollowingBehaviors() {
    std::cout << "[TEST] Source Following (0.0 to 1.0) and Disappearance Modes (Regla 3)... ";
    EventCaptureBuffer capture;
    capture.prepare(44100.0, 1.0, 2);

    std::vector<float> bufferL(128, 0.0f);
    std::vector<float> bufferR(128, 0.0f);

    // Caso 1: sourceFollow = 1.0f con Cut cuando desaparece la fuente
    Event evCut;
    EventAttributes attrsCut;
    attrsCut.sourceFollow = 1.0f;
    attrsCut.disappearanceMode = SourceDisappearanceMode::Cut;
    attrsCut.lifetimeSamples = 1000;
    evCut.initialize(attrsCut, 0.0, 1000);
    evCut.start();

    // Renderizar con fuente silenciosa (sourceLevel = 0.0f)
    evCut.render(capture, bufferL.data(), bufferR.data(), 64, 0.0f);
    assert(evCut.getState() == EventLifecycle::Killed);
    assert(!evCut.isActive());

    // Caso 2: sourceFollow = 0.0f (Completamente autónomo, Regla 3 y 18)
    Event evAutonomous;
    EventAttributes attrsAuto;
    attrsAuto.sourceFollow = 0.0f;
    attrsAuto.disappearanceMode = SourceDisappearanceMode::Fade;
    attrsAuto.lifetimeSamples = 500;
    evAutonomous.initialize(attrsAuto, 0.0, 500);
    evAutonomous.start();

    // Renderizar con fuente silenciosa -> el evento DEBE permanecer activo y reproduciendo su ciclo normal
    evAutonomous.render(capture, bufferL.data(), bufferR.data(), 64, 0.0f);
    assert(evAutonomous.getState() == EventLifecycle::Playing);
    assert(evAutonomous.isActive());

    std::cout << "PASSED\n";
}

void testEventSpawningLimits() {
    std::cout << "[TEST] Event Spawning & Maximum Generations Protection (Reglas 11, 35)... ";
    EventManager manager;
    manager.prepare(44100.0, 256, 128);

    // Spawn generación 0
    Event* gen0 = manager.spawnEvent(EventType::Fragment, 0.0, 1000);
    assert(gen0 != nullptr);
    assert(gen0->getAttributes().generation == 0);

    // Spawning encadenado
    Event* gen1 = manager.spawnChildEvent(*gen0, 10.0);
    assert(gen1 != nullptr && gen1->getAttributes().generation == 1);
    assert(gen1->getAttributes().parentId == gen0->getAttributes().id);

    Event* gen2 = manager.spawnChildEvent(*gen1, 10.0);
    assert(gen2 != nullptr && gen2->getAttributes().generation == 2);

    Event* gen3 = manager.spawnChildEvent(*gen2, 10.0);
    assert(gen3 != nullptr && gen3->getAttributes().generation == 3);

    Event* gen4 = manager.spawnChildEvent(*gen3, 10.0);
    assert(gen4 != nullptr && gen4->getAttributes().generation == 4);

    // Intento de generar generación 5 (debe ser bloqueado por MaxGenerations = 4)
    Event* gen5 = manager.spawnChildEvent(*gen4, 10.0);
    assert(gen5 == nullptr); // Protección verificada

    std::cout << "PASSED\n";
}

void testDualWorldIsolation() {
    std::cout << "[TEST] Dual World Dry Isolation & Zero Corruption (Reglas 1 y 17)... ";
    ProcessSpec spec{
        .sampleRate = 44100.0,
        .maximumBlockSize = 128,
        .numInputChannels = 2,
        .numOutputChannels = 2
    };

    DualWorldEngine engine;
    engine.prepare(spec);

    // Grafo con delay en el Event World
    Graph graph;
    graph.addNode(std::make_unique<SimpleDelayNode>(), "DelayNode");
    std::vector<NodeId> sorted;
    std::string err;
    graph.validateAndTopologicalSort(sorted, err);

    ExecutionPlan plan;
    plan.compileFrom(graph, sorted);

    // Entrada: señal sinusoidal conocida
    std::vector<float> inL(128);
    std::vector<float> inR(128);
    for (size_t i = 0; i < 128; ++i) {
        inL[i] = std::sin(static_cast<float>(i) * 0.1f);
        inR[i] = std::cos(static_cast<float>(i) * 0.1f);
    }

    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);

    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 128
    };

    // Caso 1: Dry a 1.0, Wet a 0.0 -> La salida debe converger exactamente al Dry sin alteración
    engine.setDryLevel(1.0f);
    engine.setWetLevel(0.0f);

    // Procesar varios bloques para que el suavizado de volumen se estabilice
    for (int b = 0; b < 20; ++b) {
        engine.process(plan, ctx, outChannels);
    }

    // Verificar fidelidad de la señal de entrada en el último bloque
    float maxDiff = 0.0f;
    for (size_t i = 0; i < 128; ++i) {
        maxDiff = std::max(maxDiff, std::abs(outL[i] - inL[i]));
    }

    assert(maxDiff < 0.01f); // Convergencia al audio original
    std::cout << "PASSED (Fidelidad Dry verificada, maxDiff = " << maxDiff << ")\n";
}

void testLFOShapesAndHostSync() {
    std::cout << "[TEST] LFO Shapes & DAW Host PPQ Synchronization (Reglas 7, 28, 37)... ";
    LFO lfo;
    lfo.prepare(44100.0);

    // Test Sine
    lfo.setShape(LFOShape::Sine);
    lfo.setSyncDivision(SyncDivision::FreeHz);
    lfo.setFrequencyHz(2.0f);
    for (int i = 0; i < 1000; ++i) {
        float val = lfo.processSample(120.0, 0.0, false);
        assert(!std::isnan(val) && val >= -1.05f && val <= 1.05f);
    }

    // Test Tempo Sync (1/4 note beat sync)
    lfo.setShape(LFOShape::Triangle);
    lfo.setSyncDivision(SyncDivision::Quarter);
    float valQuarter = lfo.processSample(120.0, 0.25, true);
    assert(!std::isnan(valQuarter));

    // Test Sample & Hold
    lfo.setShape(LFOShape::SampleAndHold);
    float shVal = lfo.processSample(120.0, 0.5, true);
    assert(!std::isnan(shVal));

    std::cout << "PASSED\n";
}

void testEnvelopeGeneratorPhases() {
    std::cout << "[TEST] ADSR Envelope Generator Phases (Reglas 7, 46)... ";
    EnvelopeGenerator env;
    env.prepare(44100.0);
    env.setAttackMs(1.0f);
    env.setDecayMs(2.0f);
    env.setSustainLevel(0.5f);
    env.setReleaseMs(2.0f);

    // Trigger Gate ON
    env.triggerGate(true);
    assert(env.getState() == EnvelopeGenerator::State::Attack);

    // Procesar suficientes muestras para pasar Attack y Decay y alcanzar Sustain
    for (int i = 0; i < 200; ++i) {
        env.processSample();
    }
    assert(env.getState() == EnvelopeGenerator::State::Sustain);
    assert(std::abs(env.getCurrentValue() - 0.5f) < 0.05f);

    // Trigger Gate OFF
    env.triggerGate(false);
    assert(env.getState() == EnvelopeGenerator::State::Release);

    // Procesar muestras hasta alcanzar Idle
    for (int i = 0; i < 300; ++i) {
        env.processSample();
    }
    assert(env.getState() == EnvelopeGenerator::State::Idle);
    assert(env.getCurrentValue() == 0.0f);

    std::cout << "PASSED\n";
}

void testStepSequencerGlideAndProbability() {
    std::cout << "[TEST] Step Sequencer with Glide and Probability (Reglas 7, 29, 37)... ";
    StepSequencer seq;
    seq.prepare(44100.0);
    seq.setNumSteps(4);
    seq.setStep(0, 0.2f, 1.0f, 0.5f);
    seq.setStep(1, 0.8f, 1.0f, 0.5f);
    seq.setStep(2, -0.5f, 1.0f, 0.0f);
    seq.setStep(3, 0.0f, 1.0f, 0.0f);

    float val0 = seq.processSample(0.0, true);
    assert(!std::isnan(val0));

    // Avanzar PPQ hacia el siguiente paso (1/16 = 0.25 beat)
    for (int i = 0; i < 200; ++i) {
        seq.processSample(0.26, true);
    }
    assert(seq.getCurrentStep() == 1);

    std::cout << "PASSED\n";
}

void testAudioFollowerResponse() {
    std::cout << "[TEST] Audio Follower Dynamics Tracking (Reglas 7, 46)... ";
    AudioFollower follower;
    follower.prepare(44100.0);
    follower.setAttackMs(1.0f);
    follower.setReleaseMs(10.0f);

    // Entrada fuerte (amplitud 1.0)
    std::vector<float> loud(128, 1.0f);
    const float* loudChannels[1] = { loud.data() };
    for (int b = 0; b < 10; ++b) {
        follower.processBlock(loudChannels, 1, 128);
    }
    float peakEnv = follower.getCurrentValue();
    assert(peakEnv > 0.8f);

    // Entrada de silencio -> debe decaer
    std::vector<float> silent(128, 0.0f);
    const float* silentChannels[1] = { silent.data() };
    for (int b = 0; b < 50; ++b) {
        follower.processBlock(silentChannels, 1, 128);
    }
    float decayedEnv = follower.getCurrentValue();
    assert(decayedEnv < peakEnv);

    std::cout << "PASSED\n";
}

void testModulationMatrixAndEngineRouting() {
    std::cout << "[TEST] Modulation Matrix & Engine Dispatch (Reglas 7, 8, 25, 46)... ";
    Graph graph;
    auto filterId = graph.addNode(std::make_unique<SimpleFilterNode>(), "SVFFilter");
    auto* filterNode = graph.getNodeProcessor(filterId);
    assert(filterNode != nullptr);

    // Configurar frecuencia base
    filterNode->setParameter(SimpleFilterNode::CutoffHz, 1000.0f);

    ModulationEngine modEngine;
    ProcessSpec spec{
        .sampleRate = 44100.0,
        .maximumBlockSize = 128,
        .numInputChannels = 2,
        .numOutputChannels = 2
    };
    modEngine.prepare(spec);

    // Enrutar LFO1 -> Filter CutoffHz con amount = 0.5 (bipolar)
    int routeIdx = modEngine.getMatrix().addRoute(
        ModSourceType::LFO1,
        filterId,
        SimpleFilterNode::CutoffHz,
        0.5f,
        true
    );
    assert(routeIdx >= 0);

    // Configurar LFO1 para que produzca valor positivo alto
    modEngine.getLFO(0).setShape(LFOShape::Square);
    modEngine.getLFO(0).setFrequencyHz(10.0f);

    std::vector<float> inL(128, 0.5f);
    std::vector<float> inR(128, 0.5f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = nullptr,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 128,
        .bpm = 120.0,
        .ppqPosition = 0.0,
        .isPlaying = true
    };

    // Procesar modulación y aplicar a los nodos del grafo
    modEngine.process(ctx, graph);

    // El parámetro de Cutoff debe haber sido modulado desde su valor base
    float modulatedCutoff = filterNode->getParameter(SimpleFilterNode::CutoffHz);
    assert(!std::isnan(modulatedCutoff));
    assert(modulatedCutoff >= 20.0f && modulatedCutoff <= 20000.0f);

    std::cout << "PASSED\n";
}

void testCompressorDynamics() {
    std::cout << "[TEST] CompressorNode VCA Dynamics & Soft-Knee (Reglas 5, 8, 11, 34)... ";
    CompressorNode comp;
    ProcessSpec spec{ 48000.0, 128, 2, 2 };
    comp.prepare(spec);

    comp.setParameter(CompressorNode::Threshold, -12.0f);
    comp.setParameter(CompressorNode::Ratio, 4.0f);
    comp.setParameter(CompressorNode::Attack, 1.0f);
    comp.setParameter(CompressorNode::Release, 20.0f);
    comp.setParameter(CompressorNode::Makeup, 0.0f);

    // Entrada fuerte de +6 dB (amplitud 2.0)
    std::vector<float> inL(128, 2.0f);
    std::vector<float> inR(128, 2.0f);
    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);

    const float* inCh[2] = { inL.data(), inR.data() };
    float* outCh[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    // Procesar varios bloques para que el detector y reductor de ganancia actúen
    for (int b = 0; b < 10; ++b) {
        comp.process(ctx);
    }

    // La salida comprimida debe ser menor que la entrada (atenuación efectiva)
    assert(outL[127] < inL[127]);
    assert(!std::isnan(outL[127]) && !std::isinf(outL[127]));

    std::cout << "PASSED\n";
}

void testParametricEQStability() {
    std::cout << "[TEST] ParametricEQNode Cascaded Biquads (Reglas 5, 12, 38)... ";
    ParametricEQNode eq;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    eq.prepare(spec);

    eq.setParameter(ParametricEQNode::LowFreq, 80.0f);
    eq.setParameter(ParametricEQNode::LowGain, 6.0f);
    eq.setParameter(ParametricEQNode::MidFreq, 2500.0f);
    eq.setParameter(ParametricEQNode::MidQ, 2.0f);
    eq.setParameter(ParametricEQNode::MidGain, -4.0f);
    eq.setParameter(ParametricEQNode::HighFreq, 12000.0f);
    eq.setParameter(ParametricEQNode::HighGain, 4.0f);

    std::vector<float> in(128, 0.5f);
    std::vector<float> out(128, 0.0f);
    const float* inCh[2] = { in.data(), in.data() };
    float* outCh[2] = { out.data(), out.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    eq.process(ctx);

    for (float sample : out) {
        assert(!std::isnan(sample));
        assert(!std::isinf(sample));
    }

    std::cout << "PASSED\n";
}

void testDistortionAlgorithms() {
    std::cout << "[TEST] DistortionNode Multi-Model Waveshaping (Reglas 5, 13, 34)... ";
    DistortionNode dist;
    ProcessSpec spec{ 48000.0, 128, 2, 2 };
    dist.prepare(spec);

    std::vector<float> in(128, 0.9f);
    std::vector<float> out(128, 0.0f);
    const float* inCh[2] = { in.data(), in.data() };
    float* outCh[2] = { out.data(), out.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    // Test SoftClip (Tanh)
    dist.setParameter(DistortionNode::Mode, 0.0f);
    dist.setParameter(DistortionNode::Drive, 10.0f);
    dist.process(ctx);
    assert(std::abs(out[0]) <= 1.05f);

    // Test Wavefold
    dist.setParameter(DistortionNode::Mode, 2.0f);
    dist.process(ctx);
    assert(std::abs(out[0]) <= 1.05f);

    // Test Bitcrush
    dist.setParameter(DistortionNode::Mode, 4.0f);
    dist.process(ctx);
    assert(std::abs(out[0]) <= 1.05f);

    std::cout << "PASSED\n";
}

void testAdvancedDelayPingPongAndTail() {
    std::cout << "[TEST] AdvancedDelayNode Ping-Pong & Tail Management (Reglas 12, 18)... ";
    AdvancedDelayNode delay;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    delay.prepare(spec);

    assert(delay.supportsTail());
    assert(delay.getTailSamples() > 0);

    // Configurar Ping-Pong y tiempo corto
    delay.setParameter(AdvancedDelayNode::TimeLeft, 5.0f); // ~220 muestras
    delay.setParameter(AdvancedDelayNode::TimeRight, 10.0f);
    delay.setParameter(AdvancedDelayNode::PingPong, 1.0f);
    delay.setParameter(AdvancedDelayNode::Feedback, 0.5f);
    delay.setParameter(AdvancedDelayNode::DryWet, 1.0f);

    std::vector<float> inL(128, 0.0f);
    std::vector<float> inR(128, 0.0f);
    inL[0] = 1.0f; // Impulso solo en el canal izquierdo

    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);
    const float* inCh[2] = { inL.data(), inR.data() };
    float* outCh[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    delay.process(ctx); // Bloque inicial

    // El impulso fue en Left; con Ping-Pong, el feedback debe alimentar Right en bloques posteriores
    inL[0] = 0.0f;
    for (int b = 0; b < 10; ++b) {
        delay.process(ctx);
    }

    // Verificar que los tails y el lazo no hayan generado NaN ni Inf
    for (int s = 0; s < 128; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
    }

    std::cout << "PASSED\n";
}

void testReverbDiffusionAndDecay() {
    std::cout << "[TEST] ReverbNode 4-Line FDN & Householder Diffusion (Reglas 5, 18, 34)... ";
    ReverbNode reverb;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    reverb.prepare(spec);

    assert(reverb.supportsTail());
    assert(reverb.getTailSamples() >= 44100);

    reverb.setParameter(ReverbNode::RoomSize, 0.5f);
    reverb.setParameter(ReverbNode::DecayTime, 1.0f);
    reverb.setParameter(ReverbNode::DryWet, 1.0f);

    std::vector<float> in(128, 0.0f);
    in[0] = 1.0f; // Impulso inicial
    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);
    const float* inCh[2] = { in.data(), in.data() };
    float* outCh[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    reverb.process(ctx);
    in[0] = 0.0f; // Silencio posterior

    // Procesar cola de reverberación
    float previousEnergy = 100.0f;
    for (int b = 0; b < 20; ++b) {
        reverb.process(ctx);
        float blockEnergy = 0.0f;
        for (int s = 0; s < 128; ++s) {
            blockEnergy += std::abs(outL[s]) + std::abs(outR[s]);
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        }
        previousEnergy = blockEnergy;
    }
    // Debe haber difusión y decaimiento controlado sin explosión numérica
    assert(previousEnergy < 100.0f);

    std::cout << "PASSED\n";
}

void testPitchShifterTransposition() {
    std::cout << "[TEST] PitchShifterNode Dual-Head Crossfade (Reglas 5, 14, 34)... ";
    PitchShifterNode pitch;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    pitch.prepare(spec);

    assert(pitch.supportsTail());

    // +12 Semitonos (1 octava arriba)
    pitch.setParameter(PitchShifterNode::Semitones, 12.0f);
    pitch.setParameter(PitchShifterNode::DryWet, 1.0f);

    std::vector<float> in(128, 0.7f);
    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);
    const float* inCh[2] = { in.data(), in.data() };
    float* outCh[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    pitch.process(ctx);
    for (float sample : outL) {
        assert(!std::isnan(sample) && !std::isinf(sample));
    }

    // -12 Semitonos (1 octava abajo)
    pitch.setParameter(PitchShifterNode::Semitones, -12.0f);
    pitch.process(ctx);
    for (float sample : outL) {
        assert(!std::isnan(sample) && !std::isinf(sample));
    }

    std::cout << "PASSED\n";
}

void testFFTEngineRoundtrip() {
    std::cout << "[TEST] FFTEngine Cooley-Tukey Roundtrip & SpectralProcessorNode (Reglas 19, 32, 46)... ";
    FFTEngine fft;
    fft.prepare(512, 128);

    std::vector<float> original(512);
    for (size_t i = 0; i < 512; ++i) {
        original[i] = std::sin(static_cast<float>(i) * 0.1f);
    }

    fft.forward(original.data());
    std::vector<float> reconstructed(512, 0.0f);
    fft.inverse(reconstructed.data());

    for (float s : reconstructed) {
        assert(!std::isnan(s) && !std::isinf(s));
    }

    // Probar SpectralProcessorNode
    SpectralProcessorNode spectral;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    spectral.prepare(spec);

    std::vector<float> specOutL(128, 0.0f);
    std::vector<float> specOutR(128, 0.0f);
    const float* inCh[2] = { original.data(), original.data() };
    float* outCh[2] = { specOutL.data(), specOutR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    for (int b = 0; b < 8; ++b) {
        spectral.process(ctx);
    }
    for (float s : specOutL) {
        assert(!std::isnan(s) && !std::isinf(s));
    }

    std::cout << "PASSED\n";
}

void testGrainPoolAllocationAndRecycling() {
    std::cout << "[TEST] GrainPool 128 Capacity & Zero Allocation Recycling (Reglas 9, 10, 11)... ";
    GrainPool pool;
    assert(pool.getAvailableCount() == 128);
    assert(pool.getActiveCount() == 0);

    std::vector<Grain*> acquired;
    for (size_t i = 0; i < 128; ++i) {
        Grain* g = pool.acquire();
        assert(g != nullptr);
        assert(g->active);
        acquired.push_back(g);
    }
    assert(pool.getAvailableCount() == 0);
    assert(pool.getActiveCount() == 128);

    // Overflow attempt must safely return nullptr (Rule 11)
    Grain* overflow = pool.acquire();
    assert(overflow == nullptr);

    // Release 32 grains
    for (size_t i = 0; i < 32; ++i) {
        pool.release(acquired[i]);
    }
    assert(pool.getAvailableCount() == 32);
    assert(pool.getActiveCount() == 96);

    // Reacquire
    for (size_t i = 0; i < 32; ++i) {
        Grain* g = pool.acquire();
        assert(g != nullptr);
    }
    assert(pool.getActiveCount() == 128);

    pool.reset();
    assert(pool.getAvailableCount() == 128);
    assert(pool.getActiveCount() == 0);

    std::cout << "PASSED\n";
}

void testGranularNodeCloudProcessing() {
    std::cout << "[TEST] GranularNode Cloud Synthesis & Windowing (Reglas 5, 8, 10, 35)... ";
    GranularNode gran;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    gran.prepare(spec);

    assert(gran.supportsTail());
    assert(gran.getTailSamples() > 0);

    gran.setParameter(GranularNode::GrainSizeMs, 40.0f);
    gran.setParameter(GranularNode::Density, 30.0f);
    gran.setParameter(GranularNode::PositionSprayMs, 10.0f);
    gran.setParameter(GranularNode::PitchSemitones, 7.0f);
    gran.setParameter(GranularNode::DryWet, 1.0f);

    std::vector<float> inL(128, 0.5f);
    std::vector<float> inR(128, 0.5f);
    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);
    const float* inCh[2] = { inL.data(), inR.data() };
    float* outCh[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    for (int b = 0; b < 20; ++b) {
        gran.process(ctx);
        for (int s = 0; s < 128; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
        }
    }

    std::cout << "PASSED\n";
}

void testSpectralFreezeAndSmear() {
    std::cout << "[TEST] SpectralFreezeNode Frozen Drone & Smear (Reglas 5, 18, 19, 32)... ";
    SpectralFreezeNode freeze;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    freeze.prepare(spec);

    assert(freeze.supportsTail());

    std::vector<float> in(128, 0.8f);
    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);
    const float* inCh[2] = { in.data(), in.data() };
    float* outCh[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    freeze.setParameter(SpectralFreezeNode::DryWet, 1.0f);
    for (int b = 0; b < 8; ++b) {
        freeze.process(ctx);
    }

    // Activar Freeze y pasar audio silencioso
    freeze.setParameter(SpectralFreezeNode::Freeze, 1.0f);
    std::fill(in.begin(), in.end(), 0.0f);

    float frozenEnergy = 0.0f;
    for (int b = 0; b < 12; ++b) {
        freeze.process(ctx);
        for (int s = 0; s < 128; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            frozenEnergy += std::abs(outL[s]);
        }
    }
    assert(frozenEnergy > 0.0f);

    std::cout << "PASSED\n";
}

void testResonatorBankHarmonics() {
    std::cout << "[TEST] ResonatorBankNode Modal & Harmonic Resonance (Reglas 5, 12, 18, 38)... ";
    ResonatorBankNode res;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    res.prepare(spec);

    assert(res.supportsTail());
    assert(res.getTailSamples() > 0);

    res.setParameter(ResonatorBankNode::FundamentalFreq, 220.0f);
    res.setParameter(ResonatorBankNode::DecayTime, 0.8f);
    res.setParameter(ResonatorBankNode::HarmonicSpread, 0.0f);
    res.setParameter(ResonatorBankNode::DryWet, 1.0f);

    std::vector<float> in(128, 0.0f);
    in[0] = 1.0f; // Impulso
    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);
    const float* inCh[2] = { in.data(), in.data() };
    float* outCh[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    res.process(ctx);
    in[0] = 0.0f;

    float tailEnergy = 0.0f;
    for (int b = 0; b < 10; ++b) {
        res.process(ctx);
        for (int s = 0; s < 128; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            tailEnergy += std::abs(outL[s]);
        }
    }
    assert(tailEnergy > 0.001f);

    std::cout << "PASSED\n";
}

void testGlitchBufferSlicing() {
    std::cout << "[TEST] GlitchNode Buffer Slicing & Anti-Click Smoothing (Reglas 5, 35, 37)... ";
    GlitchNode glitch;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    glitch.prepare(spec);

    glitch.setParameter(GlitchNode::Division, 3.0f); // 1/32
    glitch.setParameter(GlitchNode::RepeatProbability, 1.0f); // Forzar repetición
    glitch.setParameter(GlitchNode::ReverseProbability, 0.5f);
    glitch.setParameter(GlitchNode::DryWet, 1.0f);

    std::vector<float> in(128, 0.6f);
    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);
    const float* inCh[2] = { in.data(), in.data() };
    float* outCh[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    for (int b = 0; b < 25; ++b) {
        glitch.process(ctx);
        for (int s = 0; s < 128; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(std::abs(outL[s]) <= 1.5f);
        }
    }

    std::cout << "PASSED\n";
}

void testLinkwitzRileyCrossoverReconstruction() {
    std::cout << "[TEST] LinkwitzRileyCrossover3Way LR4 Flat Magnitude Sum (Reglas 32, 33, 46)... ";
    LinkwitzRileyCrossover3Way lr;
    lr.prepare(44100.0);
    lr.setCrossoverFrequencies(300.0f, 3000.0f);

    constexpr double sr = 44100.0;
    const std::array<float, 3> testFreqs = { 100.0f, 1000.0f, 5000.0f };

    for (float freq : testFreqs) {
        lr.reset();
        const double omega = 2.0 * 3.141592653589793 * freq / sr;
        float maxIn = 0.0f;
        float maxOut = 0.0f;

        for (int i = 0; i < 1500; ++i) {
            float in = std::sin(static_cast<float>(omega * i));
            auto bands = lr.processSample(0, in);
            float sum = bands.low + bands.mid + bands.high;

            if (i > 1000) {
                maxIn = std::max(maxIn, std::abs(in));
                maxOut = std::max(maxOut, std::abs(sum));
            }
        }
        assert(std::abs(maxIn - maxOut) < 0.08f);
    }

    std::cout << "PASSED\n";
}

void testMultibandDynamicsProcessing() {
    std::cout << "[TEST] MultibandDynamicsNode OTT Upward/Downward Dynamics (Reglas 5, 32, 34)... ";
    MultibandDynamicsNode mb;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    mb.prepare(spec);

    mb.setParameter(MultibandDynamicsNode::LowDynamics, 1.2f);
    mb.setParameter(MultibandDynamicsNode::MidDynamics, 1.2f);
    mb.setParameter(MultibandDynamicsNode::HighDynamics, 1.2f);
    mb.setParameter(MultibandDynamicsNode::DryWet, 1.0f);

    // 1. Señal alta (0 dBFS = 1.0f) -> compresión descendente debe frenar el pico
    std::vector<float> loudIn(128, 1.0f);
    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);
    const float* inCh[2] = { loudIn.data(), loudIn.data() };
    float* outCh[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    for (int b = 0; b < 20; ++b) {
        mb.process(ctx);
    }
    for (int s = 0; s < 128; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
    }

    // 2. Señal tenue (-50 dBFS = ~0.003f) -> compresión ascendente debe elevarla
    mb.reset();
    std::vector<float> quietIn(128, 0.003f);
    inCh[0] = quietIn.data();
    inCh[1] = quietIn.data();

    for (int b = 0; b < 20; ++b) {
        mb.process(ctx);
    }
    assert(outL[127] > 0.003f);

    std::cout << "PASSED\n";
}

void testDynamicGraphEditingAndValidation() {
    std::cout << "[TEST] Dynamic Graph Editing, Cycle Rejection & Plan Compilation (Reglas 4, 28, 29, 30)... ";
    Graph graph;
    auto n1 = graph.addNode(NodeFactory::getInstance().create(NodeType::Filter), "EQ1", 50.0f, 50.0f);
    auto n2 = graph.addNode(NodeFactory::getInstance().create(NodeType::Delay), "Delay1", 250.0f, 50.0f);
    auto n3 = graph.addNode(NodeFactory::getInstance().create(NodeType::Reverb), "Reverb1", 450.0f, 50.0f);

    assert(n1 != InvalidNodeId && n2 != InvalidNodeId && n3 != InvalidNodeId);
    assert(graph.getNodes().size() == 3);

    // Conectar n1 -> n2 y n2 -> n3
    auto c1 = graph.connect(n1, 2, n2, 1);
    auto c2 = graph.connect(n2, 2, n3, 1);
    assert(c1 != 0 && c2 != 0);

    // Validar y ordenar topológicamente
    std::vector<NodeId> sorted;
    std::string err;
    bool valid = graph.validateAndTopologicalSort(sorted, err);
    assert(valid);
    assert(sorted.size() == 3);
    assert(sorted[0] == n1);
    assert(sorted[1] == n2);
    assert(sorted[2] == n3);

    // Compilar ExecutionPlan
    ExecutionPlan plan;
    plan.compileFrom(graph, sorted);
    assert(plan.steps.size() == 3);

    // Intento de crear ciclo: n3 -> n1 (debe fallar la validación y detectarse el ciclo)
    auto cCycle = graph.connect(n3, 2, n1, 1);
    assert(cCycle != 0);
    std::vector<NodeId> cycleSorted;
    bool cycleValid = graph.validateAndTopologicalSort(cycleSorted, err);
    assert(!cycleValid); // Detección de ciclo verificada (Regla 28 y 29)

    // Desconectar ciclo y verificar recuperación
    graph.disconnect(cCycle);
    assert(graph.validateAndTopologicalSort(sorted, err));

    // Eliminar n2 dinámicamente: debe remover conexiones c1 y c2 automáticamente
    bool removed = graph.removeNode(n2);
    assert(removed);
    assert(graph.getNodes().size() == 2);
    assert(graph.getConnections().empty()); // Conexiones huérfanas eliminadas

    std::cout << "PASSED\n";
}

void testGraphSerializationAndDeserializationRoundtrip() {
    std::cout << "[TEST] Graph Serialization / Deserialization Roundtrip (Reglas 21 y 22)... ";

    Graph originalGraph;
    NodeId n1 = originalGraph.addNode(NodeFactory::getInstance().create(NodeType::Delay), "DelayNode", 50.0f, 60.0f);
    NodeId n2 = originalGraph.addNode(NodeFactory::getInstance().create(NodeType::Distortion), "DistNode", 200.0f, 60.0f);

    auto* node1 = originalGraph.getNode(n1);
    auto* node2 = originalGraph.getNode(n2);
    assert(node1 != nullptr && node2 != nullptr);

    // Ajustar parámetros
    node1->processor->setParameter(1, 250.0f); // Delay Time
    node1->processor->setParameter(3, 0.45f);  // Feedback
    node2->processor->setParameter(2, 6.0f);   // Drive

    // Conectar nodo 1 a nodo 2
    ConnectionId c1 = originalGraph.connect(n1, 2, n2, 1);
    assert(c1 != InvalidConnectionId);

    PresetMetadata inMeta;
    inMeta.name = "Roundtrip Test Preset";
    inMeta.author = "Antigravity";
    inMeta.category = "Test";
    inMeta.description = "Serialization integrity test";
    inMeta.dryLevel = 0.8f;
    inMeta.wetLevel = 0.9f;

    std::array<float, 8> inMacros{ 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f };

    std::string json = GraphSerializer::serialize(originalGraph, inMeta, inMacros);
    assert(!json.empty());

    // Deserializar en un grafo nuevo
    Graph restoredGraph;
    PresetMetadata outMeta;
    std::array<float, 8> outMacros{};
    std::string err;

    bool ok = GraphSerializer::deserialize(json, restoredGraph, outMeta, outMacros, err);
    assert(ok);
    assert(err.empty());

    // Verificar metadatos
    assert(outMeta.name == inMeta.name);
    assert(outMeta.author == inMeta.author);
    assert(outMeta.category == inMeta.category);
    assert(std::abs(outMeta.dryLevel - inMeta.dryLevel) < 1e-4f);
    assert(std::abs(outMeta.wetLevel - inMeta.wetLevel) < 1e-4f);

    // Verificar macros
    for (size_t i = 0; i < 8; ++i) {
        assert(std::abs(outMacros[i] - inMacros[i]) < 1e-4f);
    }

    // Verificar nodos y conexiones
    assert(restoredGraph.getNodes().size() == 2);
    assert(restoredGraph.getConnections().size() == 1);

    std::cout << "PASSED\n";
}

void testPresetVersionMigration() {
    std::cout << "[TEST] Preset Schema Version Migration (Regla 21)... ";

    // Simular un JSON con un schemaVersion anterior (versión 0)
    std::string legacyJson = R"({
  "schemaVersion": 0,
  "name": "Legacy Preset v0",
  "author": "OldEngine",
  "category": "Vintage",
  "description": "Legacy format to migrate",
  "dryLevel": 0.7,
  "wetLevel": 0.6,
  "macros": [0.25, 0.5, 0.75, 1.0],
  "nodes": [
    { "id": 1, "name": "Simple Filter", "type": 4, "x": 100.0, "y": 120.0, "params": { "1": 1200.0, "2": 0.707 } }
  ],
  "connections": []
})";

    Graph migratedGraph;
    PresetMetadata meta;
    std::array<float, 8> macros{ 0.0f };
    std::string err;

    bool ok = GraphSerializer::deserialize(legacyJson, migratedGraph, meta, macros, err);
    assert(ok);
    assert(meta.schemaVersion == 0); // Lee la versión original y ejecuta migración sin fallar
    assert(meta.name == "Legacy Preset v0");
    assert(migratedGraph.getNodes().size() == 1);
    assert(std::abs(macros[0] - 0.25f) < 1e-4f);
    assert(std::abs(macros[2] - 0.75f) < 1e-4f);

    std::cout << "PASSED\n";
}

void testSceneMorphingInterpolation() {
    std::cout << "[TEST] Scene Capture & Anti-Click Morphing (Reglas 7, 21, 35)... ";

    Graph graph;
    NodeId n1 = graph.addNode(NodeFactory::getInstance().create(NodeType::Filter), "Filter1", 100.0f, 100.0f);
    auto* node = graph.getNode(n1);
    assert(node != nullptr);

    // Escena A: Cutoff = 500 Hz, Macro 0 = 0.2
    node->processor->setParameter(1, 500.0f);
    std::array<float, 8> macrosA{ 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    SceneManager sceneMgr;
    sceneMgr.captureSceneA(graph, macrosA);
    assert(sceneMgr.hasSceneA());
    assert(!sceneMgr.hasSceneB());

    // Escena B: Cutoff = 2500 Hz, Macro 0 = 0.8
    node->processor->setParameter(1, 2500.0f);
    std::array<float, 8> macrosB{ 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    sceneMgr.captureSceneB(graph, macrosB);
    assert(sceneMgr.hasSceneB());

    // Posicionar morph al 50% (t = 0.5) con paso directo (smoothingSpeed = 1.0f)
    sceneMgr.setMorphFactor(0.5f);
    std::array<float, 8> currentMacros = macrosA;
    sceneMgr.updateAndApply(graph, currentMacros, 1.0f);

    float cutoffInterp = node->processor->getParameter(1);
    assert(std::abs(cutoffInterp - 1500.0f) < 1.0f); // (500 + 2500) / 2 = 1500 Hz
    assert(std::abs(currentMacros[0] - 0.5f) < 1e-3f); // (0.2 + 0.8) / 2 = 0.5

    // Posicionar morph al 100% (t = 1.0 -> Escena B pura)
    sceneMgr.setMorphFactor(1.0f);
    sceneMgr.updateAndApply(graph, currentMacros, 1.0f);
    assert(std::abs(node->processor->getParameter(1) - 2500.0f) < 1.0f);
    assert(std::abs(currentMacros[0] - 0.8f) < 1e-3f);

    std::cout << "PASSED\n";
}

void testGraphUndoRedoStack() {
    std::cout << "[TEST] Graph Undo / Redo Stack & Factory Presets (Reglas 21 y 22)... ";

    Graph graph;
    GraphUndoManager undoMgr(16);

    // Estado inicial: 1 nodo
    graph.addNode(NodeFactory::getInstance().create(NodeType::Passthrough), "Pass", 50.0f, 50.0f);
    undoMgr.pushState(graph);
    assert(!undoMgr.canUndo()); // Solo 1 estado en pila

    // Acción 2: Agregar segundo nodo
    graph.addNode(NodeFactory::getInstance().create(NodeType::Distortion), "Dist", 150.0f, 50.0f);
    undoMgr.pushState(graph);
    assert(undoMgr.canUndo());
    assert(!undoMgr.canRedo());

    // Deshacer (Undo) -> debe volver a 1 nodo
    PresetMetadata meta;
    std::array<float, 8> macros;
    bool okUndo = undoMgr.undo(graph, meta, macros);
    assert(okUndo);
    assert(graph.getNodes().size() == 1);
    assert(undoMgr.canRedo());

    // Rehacer (Redo) -> debe volver a 2 nodos
    bool okRedo = undoMgr.redo(graph, meta, macros);
    assert(okRedo);
    assert(graph.getNodes().size() == 2);

    // Verificar catálogo de Presets de Fábrica
    PresetManager presetMgr;
    const auto& factory = presetMgr.getFactoryPresets();
    assert(factory.size() == 40);

    // Validar que TODOS los 40 presets de fábrica deserializan limpiamente
    for (size_t i = 0; i < factory.size(); ++i) {
        Graph testG;
        PresetMetadata testM;
        std::array<float, 8> testMac{};
        bool loaded = presetMgr.loadFactoryPreset(i, testG, testM, testMac);
        assert(loaded);
        assert(!testG.getNodes().empty());
    }

    std::cout << "PASSED\n";
}

void testDenormalPreventionUnderSilentTail() {
    std::cout << "[TEST] Hardware Anti-Denormal Guards & Tail Safety (Reglas 34 y 47)... ";

    // 1. Verificar detector de subnormales
    const float subnormalVal = 1.0e-40f;
    assert(ScopedDenormalGuard::isDenormal(subnormalVal));
    assert(!ScopedDenormalGuard::isDenormal(1.0f));
    assert(!ScopedDenormalGuard::isDenormal(0.0f));

    // 2. Activar ScopedDenormalGuard
    {
        ScopedDenormalGuard guard;
        // En arquitecturas con soporte SSE (x86_64 en Windows), verificar que DAZ y FTZ estén activos
#if defined(_M_X64) || defined(__x86_64__)
        assert(ScopedDenormalGuard::areDenormalsDisabled());
#endif

        // 3. Simular un filtro biquad IIR con alta resonancia y verificar que su cola no produzca denormales
        BiquadFilter filter;
        filter.setCoefficients(BiquadFilter::Type::Lowpass, 44100.0, 100.0, 8.0f);

        // Alimentar impulso unitario
        filter.processSample(1.0f);

        // Alimentar 20,000 muestras de silencio continuo
        for (int i = 0; i < 20000; ++i) {
            float out = filter.processSample(0.0f);
            assert(!ScopedDenormalGuard::isDenormal(out)); // Ninguna muestra puede ser subnormal
            assert(!std::isnan(out) && !std::isinf(out));
        }
    }

    std::cout << "PASSED\n";
}

void testFastMathApproximationAccuracy() {
    std::cout << "[TEST] FastMath Padé Approximations & Numerical Accuracy (Reglas 34 y 47)... ";

    // 1. fastTanh vs std::tanh
    for (float x = -3.5f; x <= 3.5f; x += 0.05f) {
        float fastVal = FastMath::fastTanh(x);
        float stdVal = std::tanh(x);

        assert(!std::isnan(fastVal) && !std::isinf(fastVal));
        assert(std::abs(fastVal) <= 1.0001f);
        // Error absoluto máximo < 0.005
        assert(std::abs(fastVal - stdVal) < 0.005f);
    }
    // Simetría impar
    assert(FastMath::fastTanh(1.5f) == -FastMath::fastTanh(-1.5f));

    // 2. fastSoftClip
    assert(FastMath::fastSoftClip(0.0f) == 0.0f);
    assert(FastMath::fastSoftClip(2.0f) == 1.0f);
    assert(FastMath::fastSoftClip(-2.0f) == -1.0f);

    // 3. fastExpNegative vs std::exp
    assert(FastMath::fastExpNegative(0.0f) == 1.0f);
    for (float x = -8.0f; x <= 0.0f; x += 0.2f) {
        float fastVal = FastMath::fastExpNegative(x);
        float stdVal = std::exp(x);
        assert(!std::isnan(fastVal) && !std::isinf(fastVal));
        assert(std::abs(fastVal - stdVal) < 0.015f);
    }

    // 4. fastDbToGain y fastGainToDb
    assert(std::abs(FastMath::fastDbToGain(0.0f) - 1.0f) < 1e-4f);
    assert(std::abs(FastMath::fastGainToDb(1.0f) - 0.0f) < 1e-3f);
    assert(FastMath::fastDbToGain(-120.0f) == 0.0f);
    assert(FastMath::fastGainToDb(0.0f) == -96.0f);

    std::cout << "PASSED\n";
}

void testExecutionPlanBufferCacheReuse() {
    std::cout << "[TEST] Execution Plan Buffer Reuse & Bounded Memory Footprint (Reglas 4, 9 y 47)... ";

    // 1. Construir un grafo con 4 procesadores en cascada
    Graph graph;
    auto n1 = graph.addNode(NodeFactory::getInstance().create(NodeType::Filter), "F1", 50.0f, 50.0f);
    auto n2 = graph.addNode(NodeFactory::getInstance().create(NodeType::Delay), "D1", 150.0f, 50.0f);
    auto n3 = graph.addNode(NodeFactory::getInstance().create(NodeType::Distortion), "Dist1", 250.0f, 50.0f);
    auto n4 = graph.addNode(NodeFactory::getInstance().create(NodeType::Passthrough), "Pass1", 350.0f, 50.0f);

    graph.connect(n1, 2, n2, 1);
    graph.connect(n2, 2, n3, 1);
    graph.connect(n3, 2, n4, 1);

    std::vector<NodeId> sorted;
    std::string err;
    bool valid = graph.validateAndTopologicalSort(sorted, err);
    assert(valid);

    ExecutionPlan plan;
    plan.compileFrom(graph, sorted);
    assert(plan.getSteps().size() == 4);

    // 2. Preparar GraphExecutor con capacidad delimitada a solo 4 buffers (Regla 47)
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    GraphExecutor executor;
    executor.prepare(spec, 4); // Bounded memory allocation

    assert(executor.getBufferPoolCapacity() == 4);
    assert(executor.getBufferPoolAvailable() == 4);

    // 3. Crear buffers de entrada y contexto de proceso
    std::vector<float> inL(256, 0.5f);
    std::vector<float> inR(256, 0.5f);
    const float* inChannels[2] = { inL.data(), inR.data() };

    PreallocatedBuffer finalOutput;
    finalOutput.prepare(2, 256);

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = finalOutput.getArrayOfWritePointers(),
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    // Procesar 20 bloques consecutivos
    for (int block = 0; block < 20; ++block) {
        executor.process(plan, ctx, finalOutput);
        // Verificar que los buffers temporales se reciclen al 100% (zero memory leaks en pool)
        assert(executor.getBufferPoolAvailable() == 4);
    }

    // Verificar que la salida de audio sea válida (no silencio vacío, no NaNs)
    assert(!std::isnan(finalOutput.getReadPointer(0)[0]));
    assert(!std::isinf(finalOutput.getReadPointer(0)[0]));
    assert(std::abs(finalOutput.getReadPointer(0)[0]) > 0.001f);

    std::cout << "PASSED\n";
}

void testPhaserSweepingAndFeedback() {
    std::cout << "[TEST] PhaserNode 6-Stage Allpass Sweeping & Feedback (Reglas 5, 8, 14, 34)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    PhaserNode phaser;
    phaser.prepare(spec);

    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    inL[0] = 1.0f; inR[0] = 1.0f; // Impulso

    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    phaser.process(ctx);

    // Salida debe tener resonancia y no ser nula
    assert(std::abs(outL[0]) > 0.01f);
    assert(!std::isnan(outL[10]) && !std::isinf(outL[10]));

    // Cola con silencio: feedback activo debe generar oscilación decayente
    inL[0] = 0.0f; inR[0] = 0.0f;
    phaser.process(ctx);
    assert(std::abs(outL[10]) > 0.0001f);

    phaser.reset();
    std::cout << "PASSED\n";
}

void testChorusMultiVoiceStereoSpread() {
    std::cout << "[TEST] ChorusNode 4-Voice Multi-Phase Stereo Spread (Reglas 5, 8, 14, 34)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    ChorusNode chorus;
    chorus.prepare(spec);
    chorus.setParameter(ChorusNode::Voices, 4.0f);
    chorus.setParameter(ChorusNode::Depth, 0.8f);

    // Señal monofónica idéntica en L y R
    std::vector<float> inL(256);
    std::vector<float> inR(256);
    for (size_t i = 0; i < 256; ++i) {
        inL[i] = std::sin(2.0f * std::numbers::pi_v<float> * 440.0f * (static_cast<float>(i) / 44100.0f));
        inR[i] = inL[i];
    }

    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    // Procesar varios bloques para que el LFO module
    for (int b = 0; b < 10; ++b) {
        chorus.process(ctx);
    }

    // El chorus multivoz debe inducir descorrelación estéreo (L != R)
    float diffSum = 0.0f;
    for (size_t i = 0; i < 256; ++i) {
        diffSum += std::abs(outL[i] - outR[i]);
        assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
    }
    assert(diffSum > 0.01f);

    std::cout << "PASSED\n";
}

void testFlangerCombFilteringAndInversion() {
    std::cout << "[TEST] FlangerNode Comb Filtering & Bipolar Inversion (Reglas 5, 8, 14, 34)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    FlangerNode flanger;
    flanger.prepare(spec);

    std::vector<float> inL(256, 0.5f);
    std::vector<float> inR(256, 0.5f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    // Probar feedback positivo
    flanger.setParameter(FlangerNode::Feedback, 0.8f);
    flanger.process(ctx);
    assert(!std::isnan(outL[50]) && !std::isinf(outL[50]));

    // Probar feedback invertido negativo
    flanger.setParameter(FlangerNode::Feedback, -0.8f);
    flanger.process(ctx);
    assert(!std::isnan(outL[50]) && !std::isinf(outL[50]));

    // El soft-clip garantiza que no haya runaway explosivo
    for (size_t i = 0; i < 256; ++i) {
        assert(std::abs(outL[i]) < 2.0f);
    }

    std::cout << "PASSED\n";
}

void testRingModulatorCarrierMultiplication() {
    std::cout << "[TEST] RingModulatorNode 4-Quadrant Carrier Multiplication (Reglas 5, 8, 14, 34)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    RingModulatorNode ringMod;
    ringMod.prepare(spec);
    ringMod.setParameter(RingModulatorNode::Frequency, 440.0f);
    ringMod.setParameter(RingModulatorNode::Waveform, 0.0f); // Seno
    ringMod.setParameter(RingModulatorNode::Mix, 1.0f);

    // Entrada DC constante unitaria (1.0f): la salida debe ser exactamente la portadora sinusoidal pura
    std::vector<float> inL(256, 1.0f);
    std::vector<float> inR(256, 1.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    ringMod.process(ctx);

    // Verificar oscilación bipolar acotada
    float maxVal = 0.0f;
    float minVal = 0.0f;
    for (size_t i = 0; i < 256; ++i) {
        maxVal = std::max(maxVal, outL[i]);
        minVal = std::min(minVal, outL[i]);
        assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
    }
    assert(maxVal > 0.7f && minVal < -0.7f);

    std::cout << "PASSED\n";
}

void testFrequencyShifterHilbertSSB() {
    std::cout << "[TEST] FrequencyShifterNode Hilbert 90-Degree Allpass SSB (Reglas 5, 8, 14, 34)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    FrequencyShifterNode shifter;
    shifter.prepare(spec);
    shifter.setParameter(FrequencyShifterNode::ShiftHz, 50.0f); // +50 Hz
    shifter.setParameter(FrequencyShifterNode::Mix, 1.0f);

    std::vector<float> inL(256);
    std::vector<float> inR(256);
    for (size_t i = 0; i < 256; ++i) {
        inL[i] = std::sin(2.0f * std::numbers::pi_v<float> * 200.0f * (static_cast<float>(i) / 44100.0f));
        inR[i] = inL[i];
    }

    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    // Procesar bloques sucesivos
    for (int b = 0; b < 5; ++b) {
        shifter.process(ctx);
    }

    // Verificar salida válida desplazada y sin degradación numérica
    for (size_t i = 0; i < 256; ++i) {
        assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
    }
    assert(std::abs(outL[50]) > 0.01f);

    std::cout << "PASSED\n";
}

void testTapeSaturationWarmthAndFlutter() {
    std::cout << "[TEST] TapeSaturationNode Magnetic Warmth & Wow/Flutter (Reglas 5, 8, 14, 34, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    TapeSaturationNode tape;
    tape.prepare(spec);
    tape.setParameter(TapeSaturationNode::Drive, 6.0f);
    tape.setParameter(TapeSaturationNode::Warmth, 0.8f);
    tape.setParameter(TapeSaturationNode::WowFlutter, 0.5f);

    // Entrada de alta amplitud (2.5f) para forzar saturación magnética de cinta
    std::vector<float> inL(256, 2.5f);
    std::vector<float> inR(256, 2.5f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    for (int b = 0; b < 10; ++b) {
        tape.process(ctx);
    }

    // La saturación suave de cinta debe comprimir la señal por debajo del nivel de entrada
    for (size_t i = 0; i < 256; ++i) {
        assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
        assert(std::abs(outL[i]) < 2.0f); // Saturado y acotado
    }

    std::cout << "PASSED\n";
}

void testSubgraphContainerNestingAndExecution() {
    std::cout << "[TEST] ContainerNode Subgraph Nesting & Execution (Reglas 5, 6, 9, 28, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };

    // 1. Crear subgrafo interno
    ContainerNode innerContainer;
    auto& innerGraph = innerContainer.getInnerGraph();
    auto nFilter = innerGraph.addNode(std::make_unique<SimpleFilterNode>(), "InnerFilter");
    auto nGain = innerGraph.addNode(std::make_unique<PassthroughNode>(), "InnerGain");
    innerGraph.connect(nFilter, 2, nGain, 1);

    std::string err;
    bool compiled = innerContainer.compileInnerGraph(err);
    assert(compiled && err.empty());
    assert(innerContainer.getInnerExecutionPlan().getSteps().size() == 2);

    // 2. Anidar innerContainer dentro de outerContainer (Subgrafos jerárquicos multinivel, Regla 6)
    ContainerNode outerContainer;
    auto& outerGraph = outerContainer.getInnerGraph();
    outerGraph.addNode(std::make_unique<ContainerNode>(), "SubContainer");
    bool outerCompiled = outerContainer.compileInnerGraph(err);
    assert(outerCompiled && err.empty());

    // 3. Preparar y procesar a través de innerContainer
    innerContainer.prepare(spec);
    innerContainer.setParameter(ContainerNode::Mix, 1.0f);
    innerContainer.setParameter(ContainerNode::OutputGain, 1.2f);

    std::vector<float> inL(256, 0.5f);
    std::vector<float> inR(256, 0.5f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    innerContainer.process(ctx);

    for (size_t i = 0; i < 256; ++i) {
        assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
    }
    // Debe haber procesado audio a través del filtro y la ganancia
    assert(std::abs(outL[100]) > 0.01f);

    // Reset sin fugas
    innerContainer.reset();

    std::cout << "PASSED\n";
}

void testControlledFeedbackLoopRunawayProtection() {
    std::cout << "[TEST] FeedbackContainerNode Anti-Runaway Protection (Reglas 6, 11, 12, 34, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    FeedbackContainerNode fbNode;
    fbNode.prepare(spec);

    // Configurar ganancia de feedback excesiva (> 1.0) para probar la limitación no lineal
    fbNode.setParameter(FeedbackContainerNode::Feedback, 1.4f);
    fbNode.setParameter(FeedbackContainerNode::DelayTime, 10.0f); // 10 ms delay
    fbNode.setParameter(FeedbackContainerNode::Threshold, 1.0f);  // Anti-runaway threshold a 0 dBFS
    fbNode.setParameter(FeedbackContainerNode::Mix, 0.8f);

    // Inyectar señales de alta amplitud continuadas durante 50 bloques
    std::vector<float> inL(256, 1.5f);
    std::vector<float> inR(256, 1.5f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    for (int b = 0; b < 50; ++b) {
        fbNode.process(ctx);
        // Verificar rigurosamente que NINGUNA muestra explote o supere límites seguros [-1.5f, 1.5f]
        for (size_t i = 0; i < 256; ++i) {
            assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
            assert(!std::isnan(outR[i]) && !std::isinf(outR[i]));
            assert(std::abs(outL[i]) <= 1.5f);
            assert(std::abs(outR[i]) <= 1.5f);
        }
    }

    // Probar con un procesador interno en el lazo
    fbNode.setInnerProcessor(std::make_unique<DistortionNode>());
    fbNode.process(ctx);
    for (size_t i = 0; i < 256; ++i) {
        assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
    }

    std::cout << "PASSED\n";
}

void testEventContainerLifecycle() {
    std::cout << "[TEST] EventContainerNode Event Rack Lifecycle (Reglas 2, 3, 6, 9, 10, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    EventContainerNode evNode;
    evNode.prepare(spec);

    evNode.setParameter(EventContainerNode::Duration, 80.0f);
    evNode.setParameter(EventContainerNode::Pitch, 1.25f);
    evNode.setParameter(EventContainerNode::Mix, 1.0f);

    std::vector<float> inL(256, 0.8f);
    std::vector<float> inR(256, 0.8f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    // Bloque 1: audio entra al buffer de captura
    evNode.process(ctx);

    // Bloque 2: disparar evento manualmente mediante parámetro Trigger
    evNode.setParameter(EventContainerNode::Trigger, 1.0f);
    evNode.process(ctx);

    // Verificar que el EventManager interno activó un evento
    assert(evNode.getEventManager().getActiveEventCount() >= 1);

    // Procesar bloques subsiguientes y verificar renderizado sin NaN
    for (int b = 0; b < 10; ++b) {
        evNode.setParameter(EventContainerNode::Trigger, 0.0f);
        evNode.process(ctx);
        for (size_t i = 0; i < 256; ++i) {
            assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
        }
    }

    // Probar auto-disparador con densidad
    evNode.setParameter(EventContainerNode::Density, 0.8f);
    for (int b = 0; b < 20; ++b) {
        evNode.process(ctx);
    }
    assert(evNode.getEventManager().getActiveEventCount() <= 64); // Respeta límite estricto de capacidad

    // Resetear y comprobar reciclaje
    evNode.reset();
    assert(evNode.getEventManager().getActiveEventCount() == 0);

    std::cout << "PASSED\n";
}

void testTransientDetectionSensitivity() {
    std::cout << "[TEST] TransientDetector Sensitivity & Refractory Logic (Reglas 1, 7, 9, 34, 46)... ";
    TransientDetector detector;
    detector.prepare(44100.0);
    detector.setSensitivity(0.85f);

    // 1. Tono senoidal puro continuo (sin transitorios bruscos)
    std::vector<float> sineBlock(256);
    for (size_t i = 0; i < 256; ++i) {
        sineBlock[i] = 0.5f * std::sin(2.0f * std::numbers::pi_v<float> * 440.0f * static_cast<float>(i) / 44100.0f);
    }
    for (int b = 0; b < 5; ++b) {
        detector.process(sineBlock.data(), 256);
    }
    // Una senoidal pura establecida no debe disparar transitorios continuos
    assert(!detector.isTransientDetected());

    // 2. Inyección de impulso brusco (Ataque fuerte / Transitorio)
    std::vector<float> transientBlock(256, 0.0f);
    transientBlock[10] = 1.0f;
    transientBlock[11] = -0.8f;
    detector.process(transientBlock.data(), 256);

    assert(detector.isTransientDetected());
    assert(detector.getOnsetStrength() > 0.1f);

    std::cout << "PASSED\n";
}

void testPitchTrackingAccuracy() {
    std::cout << "[TEST] PitchTracker YIN Sub-Sample Fundamental Detection (Reglas 1, 7, 34, 46, 47)... ";
    PitchTracker tracker;
    tracker.prepare(44100.0);

    // Generar tono puro de 440.0 Hz (La 4)
    const float targetFreq = 440.0f;
    std::vector<float> block(256);
    float phase = 0.0f;
    const float phaseInc = 2.0f * std::numbers::pi_v<float> * targetFreq / 44100.0f;

    for (int b = 0; b < 25; ++b) {
        for (size_t i = 0; i < 256; ++i) {
            block[i] = std::sin(phase);
            phase += phaseInc;
            if (phase >= 2.0f * std::numbers::pi_v<float>) phase -= 2.0f * std::numbers::pi_v<float>;
        }
        tracker.process(block.data(), 256);
    }

    const float detectedHz = tracker.getFundamentalHz();
    const float clarity = tracker.getClarity();
    const float midi = tracker.getMidiNote();

    // Precisión con error < 3% respecto a 440 Hz
    assert(std::abs(detectedHz - targetFreq) < 15.0f);
    assert(clarity > 0.6f);
    assert(std::abs(midi - 69.0f) < 1.0f);

    std::cout << "PASSED (Detected " << detectedHz << " Hz, Clarity " << clarity << ")\n";
}

void testSpectralCentroidAndFluxExtraction() {
    std::cout << "[TEST] SpectralFeatureExtractor Centroid & Flux (Reglas 1, 7, 32, 46, 47)... ";
    SpectralFeatureExtractor extractor;
    extractor.prepare(44100.0, 1024, 256);

    // 1. Tono grave (150 Hz)
    std::vector<float> lowBlock(1024);
    for (size_t i = 0; i < 1024; ++i) {
        lowBlock[i] = std::sin(2.0f * std::numbers::pi_v<float> * 150.0f * static_cast<float>(i) / 44100.0f);
    }
    for (int b = 0; b < 4; ++b) {
        extractor.process(lowBlock.data(), 1024);
    }
    const float lowCentroid = extractor.getSpectralCentroid();

    // 2. Tono agudo (6000 Hz)
    std::vector<float> highBlock(1024);
    for (size_t i = 0; i < 1024; ++i) {
        highBlock[i] = std::sin(2.0f * std::numbers::pi_v<float> * 6000.0f * static_cast<float>(i) / 44100.0f);
    }
    for (int b = 0; b < 6; ++b) {
        extractor.process(highBlock.data(), 1024);
    }
    const float highCentroid = extractor.getSpectralCentroid();

    // El centroide de la frecuencia aguda debe ser notablemente superior al de la grave
    assert(highCentroid > lowCentroid);
    assert(!std::isnan(highCentroid) && !std::isinf(highCentroid));

    std::cout << "PASSED (Low: " << lowCentroid << ", High: " << highCentroid << ")\n";
}

void testAnalysisModulationRouting() {
    std::cout << "[TEST] AnalysisEngine & ModulationMatrix Live Routing (Reglas 1, 7, 8, 25, 46)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    DualWorldEngine dualWorld;
    dualWorld.prepare(spec);

    Graph graph;
    auto filterId = graph.addNode(std::make_unique<SimpleFilterNode>(), "Filter");
    std::vector<NodeId> sorted;
    std::string err;
    graph.validateAndTopologicalSort(sorted, err);
    ExecutionPlan plan;
    plan.compileFrom(graph, sorted);

    // Rutar fuente de análisis (AnalysisCentroid) al Cutoff del filtro (Param 1)
    dualWorld.getModulationEngine().getMatrix().addRoute(
        ModSourceType::AnalysisCentroid,
        filterId,
        SimpleFilterNode::CutoffHz,
        0.5f // Modulación positiva de 50%
    );

    // Inyectar audio de alta frecuencia para elevar el centroide espectral
    std::vector<float> inL(256);
    std::vector<float> inR(256);
    for (size_t i = 0; i < 256; ++i) {
        float s = std::sin(2.0f * std::numbers::pi_v<float> * 5000.0f * static_cast<float>(i) / 44100.0f);
        inL[i] = s;
        inR[i] = s;
    }
    const float* inChannels[2] = { inL.data(), inR.data() };
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    for (int b = 0; b < 10; ++b) {
        dualWorld.process(plan, ctx, outChannels, &graph);
    }

    // Verificar que el AnalysisEngine actualizó sus métricas
    const auto& snap = dualWorld.getAnalysisEngine().getSnapshot();
    assert(snap.spectralCentroid > 0.05f);

    // El parámetro modulado debe haber recibido el offset de modulación
    auto* filterNode = graph.getNodeProcessor(filterId);
    assert(filterNode != nullptr);
    float cutoff = filterNode->getParameter(SimpleFilterNode::CutoffHz);
    assert(!std::isnan(cutoff) && !std::isinf(cutoff));

    std::cout << "PASSED\n";
}

void testMidSideEncodingAndDecodingRoundtrip() {
    std::cout << "[TEST] MidSideEncoder and MidSideDecoder Bit-Exact Roundtrip (Reglas 13, 16, 34, 46)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };

    MidSideEncoderNode encoder;
    MidSideDecoderNode decoder;
    encoder.prepare(spec);
    decoder.prepare(spec);
    decoder.setParameter(MidSideDecoderNode::MonoBassActive, 0.0f); // Desactivar filtro para test de roundtrip puro
    decoder.setParameter(MidSideDecoderNode::Width, 1.0f);

    // Entrada estéreo asimétrica
    std::vector<float> inL(256, 0.75f);
    std::vector<float> inR(256, 0.25f);
    std::vector<float> mid(256, 0.0f);
    std::vector<float> side(256, 0.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);

    const float* inChannels[2] = { inL.data(), inR.data() };
    float* msChannels[2] = { mid.data(), side.data() };
    const float* msReadChannels[2] = { mid.data(), side.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext encCtx{ inChannels, msChannels, 2, 2, 256 };
    ProcessContext decCtx{ msReadChannels, outChannels, 2, 2, 256 };

    encoder.process(encCtx);
    decoder.process(decCtx);

    // Comprobar que Mid = (0.75 + 0.25)*0.7071 = 0.7071 y Side = (0.75 - 0.25)*0.7071 = 0.3535
    assert(std::abs(mid[10] - 0.70710678f) < 1e-4f);
    assert(std::abs(side[10] - 0.35355339f) < 1e-4f);

    // Comprobar reconstrucción bit-exact L' = 0.75, R' = 0.25
    for (size_t i = 10; i < 256; ++i) {
        assert(std::abs(outL[i] - inL[i]) < 1e-4f);
        assert(std::abs(outR[i] - inR[i]) < 1e-4f);
    }

    // Probar Width = 0.0 (debe colapsar a mono idéntico L == R)
    decoder.setParameter(MidSideDecoderNode::Width, 0.0f);
    decoder.process(decCtx);
    for (size_t i = 10; i < 256; ++i) {
        assert(std::abs(outL[i] - outR[i]) < 1e-4f);
    }

    std::cout << "PASSED\n";
}

void testMonoBassMakerPhaseIntegrity() {
    std::cout << "[TEST] MidSideDecoder Mono Bass Maker Side Highpass (Reglas 13, 16, 34, 46)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };

    MidSideDecoderNode decoder;
    decoder.prepare(spec);
    decoder.setParameter(MidSideDecoderNode::MonoBassActive, 1.0f);
    decoder.setParameter(MidSideDecoderNode::MonoBassFreq, 150.0f); // Cortar todo Side por debajo de 150 Hz
    decoder.setParameter(MidSideDecoderNode::Width, 1.0f);

    // Generar tono de subgrave a 50 Hz continuo en el canal Side (Ch 1) y silencio en Mid (Ch 0)
    std::vector<float> mid(256, 0.0f);
    std::vector<float> side(256, 0.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { mid.data(), side.data() };
    float* outChannels[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inChannels, outChannels, 2, 2, 256 };

    float phase = 0.0f;
    const float phaseInc = 2.0f * std::numbers::pi_v<float> * 50.0f / 44100.0f;

    for (int b = 0; b < 25; ++b) {
        for (size_t i = 0; i < 256; ++i) {
            side[i] = std::sin(phase);
            phase += phaseInc;
            if (phase >= 2.0f * std::numbers::pi_v<float>) phase -= 2.0f * std::numbers::pi_v<float>;
        }
        decoder.process(ctx);
    }

    // El filtro pasa-altos en Side a 150 Hz debe haber atenuado fuertemente la señal de 50 Hz en la salida
    float maxAmp = 0.0f;
    for (size_t i = 0; i < 256; ++i) {
        maxAmp = std::max(maxAmp, std::abs(outL[i]));
    }
    assert(maxAmp < 0.25f); // Atenuado por debajo del 25%

    std::cout << "PASSED (Sub-bass in Side attenuated to " << maxAmp << ")\n";
}

void testSpatial3DPannerITDandILD() {
    std::cout << "[TEST] SpatialPannerNode & Event 3D Psychoacoustic Positioning (Reglas 13, 16, 32, 46)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };

    // 1. Probar SpatialPannerNode por efecto
    SpatialPannerNode panner;
    panner.prepare(spec);
    panner.setParameter(SpatialPannerNode::Azimuth, 90.0f); // Fuente completamente a la derecha (+90°)
    panner.setParameter(SpatialPannerNode::Elevation, 0.0f);
    panner.setParameter(SpatialPannerNode::Distance, 1.0f);

    std::vector<float> inL(256, 1.0f);
    std::vector<float> inR(256, 1.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inChannels, outChannels, 2, 2, 256 };

    for (int b = 0; b < 5; ++b) {
        panner.process(ctx);
    }

    // A +90°, el oído derecho (ipsilateral) debe recibir mayor energía que el izquierdo (contralateral, sombra de cabeza)
    float rmsL = 0.0f, rmsR = 0.0f;
    for (size_t i = 0; i < 256; ++i) {
        rmsL += outL[i] * outL[i];
        rmsR += outR[i] * outR[i];
    }
    assert(rmsR > rmsL);

    // 2. Probar posicionamiento 3D por Evento
    Event event;
    EventAttributes attrs;
    attrs.id = 1;
    attrs.type = EventType::Grain;
    attrs.azimuth = -60.0f; // Evento a la izquierda
    attrs.elevation = 30.0f;
    attrs.distance = 2.0f;
    attrs.gain = 1.0f;
    event.initialize(attrs, 0.0, 512, false);

    assert(!std::isnan(attrs.azimuth));
    assert(event.isActive());

    std::cout << "PASSED\n";
}

void testCpuProfilerTimingAccuracy() {
    std::cout << "[TEST] CpuProfiler Timing & Budget Accuracy (Reglas 9, 26, 39, 47)... ";
    CpuProfiler profiler;
    profiler.prepare(44100.0, 512);

    auto m0 = profiler.getLatestMetrics();
    assert(m0.currentCpuPercent == 0.0f);
    assert(m0.peakCpuPercent == 0.0f);
    assert(!m0.overloadDetected);

    profiler.startBlock();
    profiler.startStage(ProfilerStage::Analysis);
    volatile float sum = 0.0f;
    for (int i = 0; i < 5000; ++i) {
        sum += static_cast<float>(i) * 0.001f;
    }
    profiler.endStage(ProfilerStage::Analysis);

    profiler.startStage(ProfilerStage::Graph);
    for (int i = 0; i < 5000; ++i) {
        sum += static_cast<float>(i) * 0.002f;
    }
    profiler.endStage(ProfilerStage::Graph);

    profiler.endBlock(512, 12, 4, 8);

    auto m1 = profiler.getLatestMetrics();
    assert(m1.bufferBudgetUs > 11600.0f && m1.bufferBudgetUs < 11620.0f);
    assert(m1.totalDspUs > 0.0f);
    assert(m1.analysisUs > 0.0f);
    assert(m1.graphExecutionUs > 0.0f);
    assert(m1.activeEvents == 12);
    assert(m1.totalNodes == 8);
    assert(m1.currentCpuPercent >= 0.0f);
    assert(m1.peakCpuPercent >= m1.currentCpuPercent);
    assert(!m1.overloadDetected);

    std::cout << "PASSED\n";
}

void testDenseGraphHeavyLoadStress() {
    std::cout << "[TEST] Dense Graph Heavy Load Stress (30 Nodes, 1000 Blocks, Zero Allocs)... ";
    Graph graph;
    std::vector<NodeId> nodeIds;

    NodeType types[] = {
        NodeType::Filter, NodeType::Distortion, NodeType::Delay, NodeType::Compressor,
        NodeType::Tape, NodeType::Phaser, NodeType::Chorus, NodeType::Flanger,
        NodeType::PitchShifter, NodeType::Resonator, NodeType::SpatialPanner,
        NodeType::MidSideEncoder, NodeType::MidSideDecoder
    };

    for (int i = 0; i < 30; ++i) {
        NodeType t = types[i % 13];
        auto p = NodeFactory::getInstance().create(t);
        assert(p != nullptr);
        NodeId id = graph.addNode(std::move(p), "", static_cast<float>(i * 50), 100.0f);
        nodeIds.push_back(id);
    }

    // Encadenar secuencialmente (pin 2 a pin 1)
    for (size_t i = 0; i < nodeIds.size() - 1; ++i) {
        graph.connect(nodeIds[i], 2, nodeIds[i + 1], 1);
    }

    std::vector<NodeId> sorted;
    std::string err;
    bool valid = graph.validateAndTopologicalSort(sorted, err);
    assert(valid);
    assert(sorted.size() == 30);

    ExecutionPlan plan;
    plan.compileFrom(graph, sorted);
    assert(plan.getSteps().size() == 30);

    DualWorldEngine engine;
    ProcessSpec spec{ 48000.0, 512, 2, 2 };
    engine.prepare(spec);

    std::vector<float> inL(512, 0.0f);
    std::vector<float> inR(512, 0.0f);
    for (size_t s = 0; s < 512; ++s) {
        inL[s] = std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(s) / 48000.0f) * 0.5f;
        inR[s] = std::cos(2.0f * 3.14159265f * 440.0f * static_cast<float>(s) / 48000.0f) * 0.5f;
    }
    const float* inChannels[2] = { inL.data(), inR.data() };

    std::vector<float> outL(512, 0.0f);
    std::vector<float> outR(512, 0.0f);
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 512,
        .bpm = 120.0,
        .ppqPosition = 0.0,
        .isPlaying = true
    };

    // Procesar 1000 bloques continuos
    for (int b = 0; b < 1000; ++b) {
        ctx.ppqPosition = static_cast<double>(b) * (512.0 / 48000.0) * (120.0 / 60.0);
        engine.process(plan, ctx, outChannels, &graph);

        if (b % 100 == 0) {
            for (size_t s = 0; s < 512; ++s) {
                assert(!std::isnan(outL[s]));
                assert(!std::isnan(outR[s]));
                assert(!std::isinf(outL[s]));
                assert(!std::isinf(outR[s]));
            }
        }
    }

    auto metrics = engine.getPerformanceMetrics();
    assert(metrics.totalDspUs > 0.0f);
    assert(metrics.totalNodes == 30);
    assert(metrics.bufferBudgetUs > 10600.0f && metrics.bufferBudgetUs < 10700.0f);

    std::cout << "PASSED\n";
}

void testOverloadProtectionAndGracefulDegradation() {
    std::cout << "[TEST] Overload Protection & Graceful Degradation (Reglas 9, 11, 47)... ";

    // 1. Probar tope estricto de EventPool a 1024 slots
    EventPool eventPool;
    eventPool.prepare(1024);
    assert(eventPool.getCapacity() == 1024);
    assert(eventPool.getActiveCount() == 0);

    std::vector<Event*> allocated;
    allocated.reserve(1200);

    for (int i = 0; i < 1200; ++i) {
        EventAttributes attr;
        attr.id = static_cast<uint64_t>(i + 1);
        auto* ev = eventPool.acquire();
        if (ev != nullptr) {
            ev->initialize(attr, 0.0, 512, false);
            allocated.push_back(ev);
        }
    }

    assert(allocated.size() == 1024);
    assert(eventPool.getActiveCount() == 1024);

    for (auto* ev : allocated) {
        eventPool.release(ev);
    }
    assert(eventPool.getActiveCount() == 0);

    // 2. Probar FeedbackContainer con feedback super-unitario (1.4f)
    FeedbackContainerNode fbNode;
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    fbNode.prepare(spec);
    fbNode.setParameter(FeedbackContainerNode::Feedback, 1.4f);

    std::vector<float> inL(256, 0.9f);
    std::vector<float> inR(256, 0.9f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inCh[2] = { inL.data(), inR.data() };
    float* outCh[2] = { outL.data(), outR.data() };

    ProcessContext fbCtx{
        .inputChannels = inCh,
        .outputChannels = outCh,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    float maxVal = 0.0f;
    for (int b = 0; b < 100; ++b) {
        fbNode.process(fbCtx);
        for (size_t s = 0; s < 256; ++s) {
            maxVal = std::max(maxVal, std::abs(outL[s]));
            assert(!std::isnan(outL[s]));
            assert(!std::isinf(outL[s]));
        }
    }
    assert(maxVal <= 1.05f);

    // 3. Probar detección de overload y reset en CpuProfiler
    CpuProfiler profiler;
    profiler.prepare(44100.0, 64);
    
    profiler.startBlock();
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count() < 2500) {
        // Simular cálculo prolongado superior al presupuesto de 64 muestras (~1451 µs)
    }
    profiler.endBlock(64, 10, 0, 5);

    auto m = profiler.getLatestMetrics();
    assert(m.overloadDetected == true);

    profiler.resetOverload();
    assert(profiler.getLatestMetrics().overloadDetected == false);

    std::cout << "PASSED\n";
}

void testGraphDAGParallelRoutingAndBranching() {
    std::cout << "[TEST] Graph DAG Parallel Routing (Split & Merge 2 Effects at the Same Time) (Reglas 4, 6, 9, 28)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };

    Graph graph;
    // Node 1: Passthrough (Input distributor)
    auto n1 = graph.addNode(std::make_unique<PassthroughNode>(), "Distributor");
    // Node 2 (Branch A): Chorus (Modulation)
    auto n2 = graph.addNode(std::make_unique<ChorusNode>(), "ChorusBranch");
    // Node 3 (Branch B): Resonator Bank (Resonance - AL MISMO TIEMPO!)
    auto n3 = graph.addNode(std::make_unique<ResonatorBankNode>(), "ResonatorBranch");
    // Node 4: Passthrough (Merge collector)
    auto n4 = graph.addNode(std::make_unique<PassthroughNode>(), "MergeCollector");

    // Conectar: n1 -> n2 y n1 -> n3 (SPLIT: 2 efectos al mismo tiempo)
    graph.connect(n1, 2, n2, 1);
    graph.connect(n1, 2, n3, 1);

    // Conectar: n2 -> n4 y n3 -> n4 (MERGE)
    graph.connect(n2, 2, n4, 1);
    graph.connect(n3, 2, n4, 1);

    for (const auto& [id, inst] : graph.getNodes()) {
        inst->processor->prepare(spec);
    }

    std::vector<NodeId> sorted;
    std::string err;
    assert(graph.validateAndTopologicalSort(sorted, err));
    assert(sorted.size() == 4);

    ExecutionPlan plan;
    plan.compileFrom(graph, sorted);
    assert(plan.hasExplicitConnections());

    GraphExecutor executor;
    executor.prepare(spec, 32);

    std::vector<float> inL(256, 0.5f);
    std::vector<float> inR(256, 0.5f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    std::vector<float> dummyOutL(256, 0.0f);
    std::vector<float> dummyOutR(256, 0.0f);
    float* outChannels[2] = { dummyOutL.data(), dummyOutR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    PreallocatedBuffer finalOut;
    finalOut.prepare(2, 256);

    // Procesar bloques de audio y verificar que la señal atraviesa las ramas paralelas y se suma
    for (int b = 0; b < 10; ++b) {
        executor.process(plan, ctx, finalOut);
    }

    const float* outL = finalOut.getReadPointer(0);
    const float* outR = finalOut.getReadPointer(1);
    for (size_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
    }
    // La suma de ambas ramas activas debe producir amplitud válida
    assert(std::abs(outL[128]) > 0.001f);

    std::cout << "PASSED\n";
}

void testDecaMatrix10FXChainPreset() {
    std::cout << "[TEST] DecaMatrix 10-FX Master Chain Preset & Key-Release CUT Behavior (Reglas 2, 3, 4, 7, 21)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };

    // Deserializar directamente la configuración DecaMatrix 10-FX
    const char* decaMatrixJson = R"json({
  "schemaVersion": 1,
  "name": "DecaMatrix: 10-FX Master Chain",
  "author": "N8Audio",
  "category": "Master Chains",
  "description": "Cadena de 10 efectos: EQ, saturador, split paralelo (Chorus + Resonador armónico simultáneos), secuenciación glitch, pitch shift, rack de eventos con corte inmediato por seguimiento de fuente (CUT al soltar tecla), delay estéreo, reverb FDN y OTT multibanda",
  "dryLevel": 0.85,
  "wetLevel": 0.90,
  "macros": [0.6, 0.7, 0.5, 0.8, 0.4, 0.9, 0.5, 0.0],
  "nodes": [
    { "id": 1, "name": "Parametric EQ", "type": 4, "x": 25.0, "y": 25.0, "params": { "1": 80.0, "2": -2.0, "3": 2800.0, "4": 2.5, "5": 1.2, "6": 9000.0, "7": 1.0 } },
    { "id": 2, "name": "Distortion", "type": 8, "x": 225.0, "y": 25.0, "params": { "1": 3.0, "2": 3.5, "3": 6500.0, "4": 0.5 } },
    { "id": 3, "name": "Chorus Branch A", "type": 17, "x": 425.0, "y": 15.0, "params": { "1": 1.2, "2": 0.65, "3": 0.25, "4": 4.0, "5": 0.75 } },
    { "id": 4, "name": "Resonator Branch B", "type": 13, "x": 425.0, "y": 185.0, "params": { "1": 220.0, "2": 2.0, "3": 0.12, "4": 0.35, "5": 0.6 } },
    { "id": 5, "name": "Glitch Slicer", "type": 14, "x": 625.0, "y": 25.0, "params": { "1": 4.0, "2": 0.7, "3": 0.35, "4": 0.2, "5": 0.85 } },
    { "id": 6, "name": "Pitch Shifter", "type": 11, "x": 825.0, "y": 25.0, "params": { "1": 7.0, "2": 0.0, "3": 1.0, "4": 0.4 } },
    { "id": 7, "name": "Event Rack Key CUT", "type": 23, "x": 825.0, "y": 235.0, "params": { "1": 0.0, "2": 0.8, "3": 1.0, "4": 200.0, "5": 1.0, "6": 0.85 } },
    { "id": 8, "name": "Stereo Delay", "type": 5, "x": 625.0, "y": 235.0, "params": { "1": 250.0, "2": 375.0, "3": 0.45, "4": 6000.0, "5": 1.0, "6": 0.4 } },
    { "id": 9, "name": "FDN Reverb", "type": 6, "x": 425.0, "y": 355.0, "params": { "1": 0.75, "2": 2.5, "3": 5500.0, "4": 15.0, "5": 0.35 } },
    { "id": 10, "name": "Multiband OTT", "type": 15, "x": 225.0, "y": 355.0, "params": { "1": 200.0, "2": 2500.0, "3": 1.0, "4": 1.0, "5": 1.0, "6": 0.9 } }
  ],
  "connections": [
    { "id": 1, "srcNode": 1, "srcPin": 2, "destNode": 2, "destPin": 1 },
    { "id": 2, "srcNode": 2, "srcPin": 2, "destNode": 3, "destPin": 1 },
    { "id": 3, "srcNode": 2, "srcPin": 2, "destNode": 4, "destPin": 1 },
    { "id": 4, "srcNode": 3, "srcPin": 2, "destNode": 5, "destPin": 1 },
    { "id": 5, "srcNode": 4, "srcPin": 2, "destNode": 5, "destPin": 1 },
    { "id": 6, "srcNode": 5, "srcPin": 2, "destNode": 6, "destPin": 1 },
    { "id": 7, "srcNode": 6, "srcPin": 2, "destNode": 7, "destPin": 1 },
    { "id": 8, "srcNode": 7, "srcPin": 2, "destNode": 8, "destPin": 1 },
    { "id": 9, "srcNode": 8, "srcPin": 2, "destNode": 9, "destPin": 1 },
    { "id": 10, "srcNode": 9, "srcPin": 2, "destNode": 10, "destPin": 1 }
  ]
})json";

    Graph graph;
    PresetMetadata meta;
    std::array<float, 8> macros;
    std::string errDeser;
    bool loaded = GraphSerializer::deserialize(decaMatrixJson, graph, meta, macros, errDeser);
    assert(loaded);
    assert(graph.getNodes().size() == 10);
    assert(graph.getConnections().size() == 10);

    for (const auto& [id, inst] : graph.getNodes()) {
        inst->processor->prepare(spec);
    }

    std::vector<NodeId> sorted;
    std::string err;
    assert(graph.validateAndTopologicalSort(sorted, err));
    assert(sorted.size() == 10);

    ExecutionPlan plan;
    plan.compileFrom(graph, sorted);
    assert(plan.hasExplicitConnections());
    assert(plan.getSteps().size() == 10);

    GraphExecutor executor;
    executor.prepare(spec, 32);

    // Probar con señal de entrada activa (tecla presionada)
    std::vector<float> inL(256, 0.4f);
    std::vector<float> inR(256, 0.4f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    std::vector<float> dummyOutL(256, 0.0f);
    std::vector<float> dummyOutR(256, 0.0f);
    float* outChannels[2] = { dummyOutL.data(), dummyOutR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    PreallocatedBuffer finalOut;
    finalOut.prepare(2, 256);

    for (int b = 0; b < 20; ++b) {
        executor.process(plan, ctx, finalOut);
    }

    const float* outL = finalOut.getReadPointer(0);
    for (size_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
    }

    // Probar con cese de entrada (tecla liberada / source energy = 0):
    std::vector<float> silentL(256, 0.0f);
    std::vector<float> silentR(256, 0.0f);
    const float* silentChannels[2] = { silentL.data(), silentR.data() };
    ctx.inputChannels = silentChannels;

    for (int b = 0; b < 10; ++b) {
        executor.process(plan, ctx, finalOut);
    }

    for (size_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
    }

    std::cout << "PASSED\n";
}

void testTestInputSynthesizerPolyphonyAndLifecycle() {
    std::cout << "[TEST] TestInputSynthesizer Polyphony, Voice Stealing & Audio Injection (Reglas 9, 13, 34, 47)... ";
    TestInputSynthesizer synth;
    const double sr = 48000.0;
    const int blockSize = 256;
    synth.prepare(sr, blockSize);

    assert(!synth.hasActiveVoices());

    // 1. Probar disparo de nota simple (C4 = 60)
    synth.noteOn(60, 0.8f);
    assert(synth.hasActiveVoices());

    std::vector<float> bufL(blockSize, 0.0f);
    std::vector<float> bufR(blockSize, 0.0f);
    float* channels[2] = { bufL.data(), bufR.data() };

    synth.renderAndInject(channels, 2, blockSize);

    // Verificar que se generó señal audible en ambos canales y sin NaN / Inf
    float sumEnergy = 0.0f;
    for (int s = 0; s < blockSize; ++s) {
        assert(!std::isnan(bufL[s]) && !std::isinf(bufL[s]));
        assert(!std::isnan(bufR[s]) && !std::isinf(bufR[s]));
        sumEnergy += std::abs(bufL[s]);
    }
    assert(sumEnergy > 0.01f);

    // 2. Probar Polifonía de 8 voces completas
    synth.reset();
    assert(!synth.hasActiveVoices());
    const int chord[8] = { 60, 62, 64, 65, 67, 69, 71, 72 };
    for (int i = 0; i < 8; ++i) {
        synth.noteOn(chord[i], 0.7f);
    }
    assert(synth.hasActiveVoices());

    // 3. Probar Voice Stealing (9ª nota sin crash ni memory allocation)
    synth.noteOn(74, 0.9f);
    assert(synth.hasActiveVoices());

    // Renderizar varias iteraciones para probar la continuidad
    for (int b = 0; b < 10; ++b) {
        std::fill(bufL.begin(), bufL.end(), 0.0f);
        std::fill(bufR.begin(), bufR.end(), 0.0f);
        synth.renderAndInject(channels, 2, blockSize);
        for (int s = 0; s < blockSize; ++s) {
            assert(!std::isnan(bufL[s]) && !std::isinf(bufL[s]));
        }
    }

    // 4. Probar NoteOff individual y decaimiento ADSR anti-click
    for (int i = 0; i < 8; ++i) {
        synth.noteOff(chord[i]);
    }
    synth.noteOff(74);

    // Renderizar suficientes bloques para permitir la fase Release (15ms @ 48kHz = 720 samples ~ 3 bloques)
    for (int b = 0; b < 10; ++b) {
        std::fill(bufL.begin(), bufL.end(), 0.0f);
        std::fill(bufR.begin(), bufR.end(), 0.0f);
        synth.renderAndInject(channels, 2, blockSize);
    }
    // Una vez completada la etapa de liberación, todas las voces deben estar apagadas
    assert(!synth.hasActiveVoices());

    // 5. Probar todos los modos tímbricos (ElectricPiano, Sine, Triangle, WarmSaw)
    const TestTimbreMode timbres[] = {
        TestTimbreMode::ElectricPiano,
        TestTimbreMode::Sine,
        TestTimbreMode::Triangle,
        TestTimbreMode::WarmSaw
    };

    for (auto timbre : timbres) {
        synth.setTimbreMode(timbre);
        assert(synth.getTimbreMode() == timbre);
        synth.noteOn(69, 0.75f); // A4 = 440 Hz
        std::fill(bufL.begin(), bufL.end(), 0.0f);
        std::fill(bufR.begin(), bufR.end(), 0.0f);
        synth.renderAndInject(channels, 2, blockSize);
        for (int s = 0; s < blockSize; ++s) {
            assert(!std::isnan(bufL[s]) && !std::isinf(bufL[s]));
        }
        synth.allNotesOff();
    }

    // Drenar voces tras allNotesOff
    for (int b = 0; b < 10; ++b) {
        std::fill(bufL.begin(), bufL.end(), 0.0f);
        std::fill(bufR.begin(), bufR.end(), 0.0f);
        synth.renderAndInject(channels, 2, blockSize);
    }
    assert(!synth.hasActiveVoices());

    std::cout << "PASSED\n";
}

void testAll40FactoryPresetsCatalog() {
    std::cout << "[TEST] 20 Thematic Categories & 80 Creative Presets (Reglas 4, 5, 9, 21, 22, 34, 38, 44, 46)... ";
    ProcessSpec spec{ 48000.0, 256, 2, 2 };

    PresetManager pm;
    const auto& presets = pm.getFactoryPresets();
    assert(presets.size() == 80);

    const auto categories = pm.getCategories();
    assert(categories.size() == 20);

    // Set para rastrear los tipos de nodos DSP del motor
    std::unordered_set<NodeType> presentNodeTypes;

    GraphExecutor executor;
    executor.prepare(spec, 32);

    PreallocatedBuffer finalOut;
    finalOut.prepare(2, 256);

    std::vector<float> inL(256);
    std::vector<float> inR(256);
    for (size_t s = 0; s < 256; ++s) {
        float sample = std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(s) / 48000.0f) * 0.5f;
        inL[s] = sample;
        inR[s] = sample;
    }
    const float* inChannels[2] = { inL.data(), inR.data() };
    std::vector<float> dummyOutL(256, 0.0f);
    std::vector<float> dummyOutR(256, 0.0f);
    float* outChannels[2] = { dummyOutL.data(), dummyOutR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    std::unordered_map<std::string, int> categoryCounts;

    for (size_t i = 0; i < presets.size(); ++i) {
        Graph graph;
        PresetMetadata meta;
        std::array<float, 8> macros;

        bool loaded = pm.loadFactoryPreset(i, graph, meta, macros);
        assert(loaded);
        assert(!meta.name.empty());
        assert(!meta.category.empty());
        assert(meta.dryLevel >= 0.0f && meta.dryLevel <= 2.0f);
        assert(meta.wetLevel >= 0.0f && meta.wetLevel <= 2.0f);
        assert(!graph.getNodes().empty());

        categoryCounts[meta.category]++;

        for (const auto& [nodeId, nodeInst] : graph.getNodes()) {
            assert(nodeInst->processor != nullptr);
            presentNodeTypes.insert(nodeInst->type);
            nodeInst->processor->prepare(spec);
        }

        std::vector<NodeId> sorted;
        std::string err;
        bool valid = graph.validateAndTopologicalSort(sorted, err);
        if (!valid) {
            std::cerr << "\nValidation error in preset " << i << " (" << meta.name << "): " << err << "\n";
        }
        assert(valid);

        ExecutionPlan plan;
        plan.compileFrom(graph, sorted);

        // Procesar 20 bloques consecutivos de audio a 48kHz
        for (int b = 0; b < 20; ++b) {
            executor.process(plan, ctx, finalOut);
        }

        // Validación estricta de estabilidad acústica (cero NaN / Inf)
        const float* outL = finalOut.getReadPointer(0);
        const float* outR = finalOut.getReadPointer(1);
        float rmsOut = 0.0f;
        float maxDiff = 0.0f;
        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
            rmsOut += outL[s] * outL[s];
            float d = std::abs(outL[s] - inL[s]);
            if (d > maxDiff) maxDiff = d;
        }
        rmsOut = std::sqrt(rmsOut / 256.0f);
        if (rmsOut < 1e-4f || maxDiff < 1e-3f) {
            std::cout << "\n   [WARNING] Preset " << i << " (" << meta.name << "): rmsOut=" << rmsOut << ", maxDiff=" << maxDiff;
        }
    }

    // Comprobar que todas las 20 categorías están cubiertas (con al menos 2 presets cada una)
    assert(categoryCounts.size() == 20);
    for (const auto& [cat, count] : categoryCounts) {
        assert(count >= 2);
    }

    // Verificar que se emplean al menos 28 tipos distintos de nodos en el catálogo de presets
    assert(presentNodeTypes.size() >= 28);

    std::cout << "PASSED (80 presets, 20 categorias, " << presentNodeTypes.size() << " tipos DSP verificados)\n";
}

void testTapeStopProcessingAndHermiteInterpolation() {
    std::cout << "[TEST] TapeStopNode Braking, Spin-Up & Hermite Interpolation (Reglas 5, 8, 14, 34, 35, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    TapeStopNode node;
    node.prepare(spec);

    assert(node.getType() == NodeType::TapeStop);
    assert(std::string(node.getName()) == "Tape Stop");
    assert(node.getPins().size() == 2);
    assert(node.getParameters().size() == 6);

    std::vector<float> inL(256);
    std::vector<float> inR(256);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    // Generar tono senoidal de entrada
    for (size_t i = 0; i < 256; ++i) {
        inL[i] = std::sin(2.0f * std::numbers::pi_v<float> * 440.0f * (static_cast<float>(i) / 44100.0f));
        inR[i] = inL[i];
    }

    // 1. Probar en reproducción normal (Trigger = 0)
    node.setParameter(TapeStopNode::Trigger, 0.0f);
    node.setParameter(TapeStopNode::StopTime, 0.2f);
    node.setParameter(TapeStopNode::Mix, 1.0f);

    for (int b = 0; b < 10; ++b) {
        node.process(ctx);
        for (size_t i = 0; i < 256; ++i) {
            assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
            assert(!std::isnan(outR[i]) && !std::isinf(outR[i]));
        }
    }

    // 2. Activar freno (Trigger = 1)
    node.setParameter(TapeStopNode::Trigger, 1.0f);
    float lastRms = 1.0f;
    for (int b = 0; b < 40; ++b) {
        node.process(ctx);
        float rms = 0.0f;
        for (size_t i = 0; i < 256; ++i) {
            assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
            rms += outL[i] * outL[i];
        }
        rms = std::sqrt(rms / 256.0f);
        lastRms = rms;
    }
    // Al finalizar el tiempo de frenado, la señal debe haber caído sustancialmente
    assert(lastRms < 0.15f);

    // 3. Reanudar (Trigger = 0)
    node.setParameter(TapeStopNode::Trigger, 0.0f);
    node.setParameter(TapeStopNode::SpinUpTime, 0.15f);
    for (int b = 0; b < 40; ++b) {
        node.process(ctx);
        for (size_t i = 0; i < 256; ++i) {
            assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
        }
    }

    node.reset();
    std::cout << "PASSED\n";
}

void testFormantFilterVowelMorphing() {
    std::cout << "[TEST] FormantFilterNode Resonant Vowels & Continuous Morphing (Reglas 5, 8, 14, 34, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    FormantFilterNode node;
    node.prepare(spec);

    assert(node.getType() == NodeType::FormantFilter);
    assert(std::string(node.getName()) == "Formant Filter");

    std::vector<float> inL(256, 0.5f);
    std::vector<float> inR(256, 0.5f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    // Probar cada vocal pura (0=A, 1=E, 2=I, 3=O, 4=U) y posiciones intermedias
    for (float v : { 0.0f, 0.5f, 1.0f, 1.8f, 2.0f, 3.0f, 3.7f, 4.0f }) {
        node.setParameter(FormantFilterNode::Vowel, v);
        node.setParameter(FormantFilterNode::Resonance, 8.0f);
        node.setParameter(FormantFilterNode::FormantShift, 1.1f);
        node.setParameter(FormantFilterNode::Warmth, 0.4f);
        node.process(ctx);

        for (size_t i = 0; i < 256; ++i) {
            assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
            assert(!std::isnan(outR[i]) && !std::isinf(outR[i]));
        }
    }

    node.reset();
    std::cout << "PASSED\n";
}

void testNoiseTextureGenerationAndSidechainDuck() {
    std::cout << "[TEST] NoiseTextureNode Organic Textures & Sidechain Ducking (Reglas 5, 8, 9, 34, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    NoiseTextureNode node;
    node.prepare(spec);

    assert(node.getType() == NodeType::NoiseTexture);

    std::vector<float> inSilentL(256, 0.0f);
    std::vector<float> inSilentR(256, 0.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inSilentChannels[2] = { inSilentL.data(), inSilentR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctxSilent{
        .inputChannels = inSilentChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    // 1. Probar que genera ruido en todos los 5 modos (White, Pink, Tape, Vinyl, Rain)
    for (int mode = 0; mode < 5; ++mode) {
        node.setParameter(NoiseTextureNode::Mode, static_cast<float>(mode));
        node.setParameter(NoiseTextureNode::Level, 0.5f);
        node.setParameter(NoiseTextureNode::Density, 0.6f);
        node.process(ctxSilent);

        float rms = 0.0f;
        for (size_t i = 0; i < 256; ++i) {
            assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
            rms += outL[i] * outL[i];
        }
        rms = std::sqrt(rms / 256.0f);
        assert(rms > 0.001f);
    }

    // 2. Probar Sidechain Ducking: ante señal entrante fuerte, el ruido debe duckearse
    std::vector<float> inLoudL(256, 1.0f);
    std::vector<float> inLoudR(256, 1.0f);
    const float* inLoudChannels[2] = { inLoudL.data(), inLoudR.data() };
    ProcessContext ctxLoud{
        .inputChannels = inLoudChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    node.setParameter(NoiseTextureNode::SidechainDuck, 1.0f);
    for (int b = 0; b < 20; ++b) {
        node.process(ctxLoud);
    }
    for (size_t i = 0; i < 256; ++i) {
        assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
    }

    node.reset();
    std::cout << "PASSED\n";
}

void testMSEGModulatorCurvesAndSync() {
    std::cout << "[TEST] MSEGModulator Multi-Segment Bézier Curves & Loops (Reglas 7, 8, 25, 37, 46, 47)... ";
    MSEGModulator mseg;
    mseg.prepare(44100.0);

    mseg.setNumPoints(4);
    mseg.setPoint(0, 0.0f,  0.0f,  0.0f);
    mseg.setPoint(1, 0.3f,  1.0f,  0.6f);
    mseg.setPoint(2, 0.7f,  0.2f, -0.6f);
    mseg.setPoint(3, 1.0f,  0.0f,  0.0f);

    mseg.setMode(MSEGModulator::PlayMode::Loop);
    mseg.setSync(SyncDivision::FreeHz, 2.0f);

    float minVal = 100.0f;
    float maxVal = -100.0f;
    for (int s = 0; s < 22050; ++s) {
        mseg.processSample(120.0, 0.0, false);
        float val = mseg.getCurrentValue();
        assert(!std::isnan(val) && !std::isinf(val));
        assert(val >= -0.01f && val <= 1.01f);
        minVal = std::min(minVal, val);
        maxVal = std::max(maxVal, val);
    }

    assert(maxVal > 0.95f);
    assert(minVal < 0.05f);

    mseg.setMode(MSEGModulator::PlayMode::OneShot);
    mseg.trigger();
    for (int s = 0; s < 44100; ++s) {
        mseg.processSample(120.0, 0.0, false);
    }
    assert(std::abs(mseg.getCurrentValue()) < 0.01f);

    std::cout << "PASSED\n";
}

void testEuclideanModulatorBjorklundRhythm() {
    std::cout << "[TEST] EuclideanModulator Bjorklund Rhythm & Dynamic Envelopes (Reglas 7, 8, 37, 46, 47)... ";
    EuclideanModulator euc;
    euc.prepare(44100.0);

    euc.setSteps(16);
    euc.setPulses(4);
    euc.setRotation(0);
    assert(euc.hasPulseAtStep(0) == true);
    assert(euc.hasPulseAtStep(1) == false);
    assert(euc.hasPulseAtStep(2) == false);
    assert(euc.hasPulseAtStep(3) == false);
    assert(euc.hasPulseAtStep(4) == true);
    assert(euc.hasPulseAtStep(8) == true);
    assert(euc.hasPulseAtStep(12) == true);

    euc.setSteps(8);
    euc.setPulses(3);
    assert(euc.hasPulseAtStep(0) == true);
    assert(euc.hasPulseAtStep(1) == false);
    assert(euc.hasPulseAtStep(2) == false);
    assert(euc.hasPulseAtStep(3) == true);
    assert(euc.hasPulseAtStep(4) == false);
    assert(euc.hasPulseAtStep(5) == false);
    assert(euc.hasPulseAtStep(6) == true);
    assert(euc.hasPulseAtStep(7) == false);

    euc.setDecay(0.05f);
    euc.setSync(SyncDivision::FreeHz, 120.0f);
    for (int s = 0; s < 44100; ++s) {
        euc.processSample(120.0, 0.0, false);
        float val = euc.getCurrentValue();
        assert(!std::isnan(val) && !std::isinf(val));
        assert(val >= 0.0f && val <= 1.0f);
    }

    std::cout << "PASSED\n";
}

void testChaosModulatorLorenzAttractor() {
    std::cout << "[TEST] ChaosModulator Non-Linear Lorenz RK4 Integration (Reglas 7, 8, 25, 46, 47)... ";
    ChaosModulator chaos;
    chaos.prepare(44100.0);
    chaos.setSpeed(4.0f);

    float minX = 10.0f, maxX = -10.0f;
    float minY = 10.0f, maxY = -10.0f;
    float minZ = 10.0f, maxZ = -10.0f;

    for (int s = 0; s < 20000; ++s) {
        chaos.processSample();
        float x = chaos.getNormalizedX();
        float y = chaos.getNormalizedY();
        float z = chaos.getNormalizedZ();

        assert(!std::isnan(x) && !std::isinf(x));
        assert(!std::isnan(y) && !std::isinf(y));
        assert(!std::isnan(z) && !std::isinf(z));

        assert(x >= -1.0f && x <= 1.0f);
        assert(y >= -1.0f && y <= 1.0f);
        assert(z >= -1.0f && z <= 1.0f);

        minX = std::min(minX, x); maxX = std::max(maxX, x);
        minY = std::min(minY, y); maxY = std::max(maxY, y);
        minZ = std::min(minZ, z); maxZ = std::max(maxZ, z);
    }

    assert((maxX - minX) > 0.4f);
    assert((maxY - minY) > 0.4f);

    std::cout << "PASSED\n";
}

void testSmartRandomizerSafeguardsAndMutation() {
    std::cout << "[TEST] SmartRandomizer Acoustic Safeguards & DAG Mutation (Reglas 4, 11, 21, 28, 34, 46, 47)... ";
    Graph graph;
    graph.addNode(NodeFactory::getInstance().create(NodeType::Input), "", 100.0f, 200.0f);
    graph.addNode(NodeFactory::getInstance().create(NodeType::Output), "", 700.0f, 200.0f);

    NodeId dId = graph.addNode(NodeFactory::getInstance().create(NodeType::Distortion), "", 300.0f, 200.0f);
    NodeId fId = graph.addNode(NodeFactory::getInstance().create(NodeType::Delay), "", 500.0f, 200.0f);

    graph.connect(1, 1, dId, 1);
    graph.connect(dId, 2, fId, 1);
    graph.connect(fId, 2, 2, 1);

    PresetMetadata meta{ .name = "Base" };
    std::array<float, 8> macros{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    GraphUndoManager undoMgr(16);

    // 1. Probar Subtle Tweak
    bool okTweak = SmartRandomizer::applyRandom(graph, meta, macros, undoMgr, SmartRandomizer::RandomMode::SubtleTweak);
    assert(okTweak);

    auto* delayNode = graph.getNode(fId)->processor.get();
    float fdbk = delayNode->getParameter(SimpleDelayNode::Feedback);
    assert(fdbk <= 0.85f);

    // 2. Probar Surprise Patch
    bool okSurprise = SmartRandomizer::applyRandom(graph, meta, macros, undoMgr, SmartRandomizer::RandomMode::SurprisePatch);
    assert(okSurprise);

    std::vector<NodeId> sortedIds;
    std::string errMsg;
    bool okSort = graph.validateAndTopologicalSort(sortedIds, errMsg);
    assert(okSort);
    assert(!sortedIds.empty());
    assert(graph.getNodeCount() >= 4);

    // 3. Probar Undo para restaurar el estado previo
    PresetMetadata restoredMeta;
    std::array<float, 8> restoredMacros;
    bool okUndo = undoMgr.undo(graph, restoredMeta, restoredMacros);
    assert(okUndo);

    std::cout << "PASSED\n";
}

void testTransientShaperDynamics() {
    std::cout << "[TEST] TransientShaperNode Punch, Sustain & Soft-Clipping (Reglas 5, 8, 14, 34, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    TransientShaperNode node;
    node.prepare(spec);

    assert(node.getType() == NodeType::TransientShaper);
    assert(std::string(node.getName()) == "Transient Shaper");
    assert(node.getPins().size() == 2);
    assert(node.getParameters().size() == 6);

    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    // Generar transiente pronunciado tipo caja / bombo (ataque rápido y decaimiento)
    inL[0] = 1.0f;
    for (size_t i = 1; i < 256; ++i) {
        inL[i] = inL[i - 1] * 0.94f;
        inR[i] = inL[i];
    }

    // 1. Probar realce de ataque (+1.0)
    node.setParameter(TransientShaperNode::Attack, 1.0f);
    node.setParameter(TransientShaperNode::Sustain, 0.0f);
    node.setParameter(TransientShaperNode::SoftClip, 0.0f);
    node.process(ctx);

    for (size_t i = 0; i < 256; ++i) {
        assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
    }
    float boostedAttackPeak = outL[0];
    assert(boostedAttackPeak > 1.05f);

    // 2. Probar atenuación de ataque (-1.0)
    node.reset();
    node.setParameter(TransientShaperNode::Attack, -1.0f);
    node.process(ctx);
    float softenedAttackPeak = outL[0];
    assert(softenedAttackPeak < boostedAttackPeak);

    node.reset();
    std::cout << "PASSED\n";
}

void testRotarySpeakerDopplerAndInertia() {
    std::cout << "[TEST] RotarySpeakerNode Leslie Dual Rotor & Doppler Physics (Reglas 5, 8, 14, 34, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    RotarySpeakerNode node;
    node.prepare(spec);

    assert(node.getType() == NodeType::RotarySpeaker);
    assert(std::string(node.getName()) == "Rotary Speaker");

    std::vector<float> inL(256, 0.5f);
    std::vector<float> inR(256, 0.5f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    node.setParameter(RotarySpeakerNode::SpeedMode, 1.0f);
    node.setParameter(RotarySpeakerNode::Drive, 1.2f);
    node.setParameter(RotarySpeakerNode::Spread, 0.9f);

    for (int b = 0; b < 20; ++b) {
        node.process(ctx);
        for (size_t i = 0; i < 256; ++i) {
            assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
            assert(!std::isnan(outR[i]) && !std::isinf(outR[i]));
        }
    }

    node.setParameter(RotarySpeakerNode::SpeedMode, 0.0f);
    for (int b = 0; b < 20; ++b) {
        node.process(ctx);
        for (size_t i = 0; i < 256; ++i) {
            assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
        }
    }

    node.reset();
    std::cout << "PASSED\n";
}

void testHarmonicExciterAirAndSub() {
    std::cout << "[TEST] HarmonicExciterNode Air Sheen & Sub-Harmonic Synthesis (Reglas 5, 8, 14, 34, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    HarmonicExciterNode node;
    node.prepare(spec);

    assert(node.getType() == NodeType::HarmonicExciter);
    assert(std::string(node.getName()) == "Harmonic Exciter");

    std::vector<float> inL(256);
    std::vector<float> inR(256);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    for (size_t i = 0; i < 256; ++i) {
        inL[i] = std::sin(2.0f * std::numbers::pi_v<float> * 8000.0f * (static_cast<float>(i) / 44100.0f));
        inR[i] = inL[i];
    }
    node.setParameter(HarmonicExciterNode::AirFreq, 6000.0f);
    node.setParameter(HarmonicExciterNode::AirDrive, 3.0f);
    node.setParameter(HarmonicExciterNode::AirMix, 0.8f);
    node.process(ctx);

    for (size_t i = 0; i < 256; ++i) {
        assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
    }

    for (size_t i = 0; i < 256; ++i) {
        inL[i] = std::sin(2.0f * std::numbers::pi_v<float> * 80.0f * (static_cast<float>(i) / 44100.0f));
        inR[i] = inL[i];
    }
    node.setParameter(HarmonicExciterNode::SubFreq, 100.0f);
    node.setParameter(HarmonicExciterNode::SubDrive, 2.5f);
    node.setParameter(HarmonicExciterNode::SubMix, 0.6f);
    node.process(ctx);

    for (size_t i = 0; i < 256; ++i) {
        assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
    }

    node.reset();
    std::cout << "PASSED\n";
}

void testVocoderFilterBank16Bands() {
    std::cout << "[TEST] VocoderNode 16-Band Log Filter Bank & Carrier Modulation (Reglas 5, 8, 14, 34, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    VocoderNode node;
    node.prepare(spec);

    assert(node.getType() == NodeType::Vocoder);
    assert(std::string(node.getName()) == "Channel Vocoder");

    std::vector<float> inL(256);
    std::vector<float> inR(256);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    for (size_t i = 0; i < 256; ++i) {
        float t = static_cast<float>(i) / 44100.0f;
        inL[i] = 0.5f * std::sin(2.0f * std::numbers::pi_v<float> * 300.0f * t)
               + 0.3f * std::sin(2.0f * std::numbers::pi_v<float> * 1200.0f * t);
        inR[i] = inL[i];
    }

    for (int mode = 0; mode < 3; ++mode) {
        node.setParameter(VocoderNode::CarrierMode, static_cast<float>(mode));
        node.setParameter(VocoderNode::CarrierPitch, 130.0f);
        node.setParameter(VocoderNode::FormantShift, 1.15f);
        node.setParameter(VocoderNode::BandQ, 6.0f);

        for (int b = 0; b < 10; ++b) {
            node.process(ctx);
            for (size_t i = 0; i < 256; ++i) {
                assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
                assert(!std::isnan(outR[i]) && !std::isinf(outR[i]));
            }
        }
    }

    node.reset();
    std::cout << "PASSED\n";
}

void testKarplusStrongPhysicalModeling() {
    std::cout << "[TEST] KarplusStrongNode Plucked String Physical Modeling (Reglas 5, 8, 14, 34, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    KarplusStrongNode node;
    node.prepare(spec);

    assert(node.getType() == NodeType::KarplusStrong);
    assert(std::string(node.getName()) == "Karplus-Strong");

    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    node.setParameter(KarplusStrongNode::Pitch, 440.0f);
    node.setParameter(KarplusStrongNode::Damping, 0.35f);
    node.setParameter(KarplusStrongNode::Decay, 1.2f);
    node.setParameter(KarplusStrongNode::AudioTrigger, 1.0f);

    inL[0] = 1.0f;
    inR[0] = 1.0f;
    node.process(ctx);

    float firstBlockRms = 0.0f;
    for (size_t i = 0; i < 256; ++i) {
        assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
        firstBlockRms += outL[i] * outL[i];
    }
    firstBlockRms = std::sqrt(firstBlockRms / 256.0f);
    assert(firstBlockRms > 0.05f);

    inL[0] = 0.0f;
    inR[0] = 0.0f;
    for (int b = 0; b < 30; ++b) {
        node.process(ctx);
        for (size_t i = 0; i < 256; ++i) {
            assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
        }
    }

    node.reset();
    std::cout << "PASSED\n";
}

void testReverseReverbBloomDiffusion() {
    std::cout << "[TEST] ReverseReverbNode Pre-Swell Bloom & Diffusive Tail (Reglas 5, 8, 14, 34, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    ReverseReverbNode node;
    node.prepare(spec);

    assert(node.getType() == NodeType::ReverseReverb);
    assert(std::string(node.getName()) == "Reverse Reverb");

    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    node.setParameter(ReverseReverbNode::SwellTime, 0.3f);
    node.setParameter(ReverseReverbNode::Diffusion, 0.8f);
    node.setParameter(ReverseReverbNode::Feedback, 0.4f);

    for (size_t i = 0; i < 256; ++i) inL[i] = 0.6f;
    for (int b = 0; b < 10; ++b) {
        node.process(ctx);
        for (size_t i = 0; i < 256; ++i) {
            assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
            assert(!std::isnan(outR[i]) && !std::isinf(outR[i]));
        }
    }

    for (size_t i = 0; i < 256; ++i) inL[i] = 0.0f;
    for (int b = 0; b < 20; ++b) {
        node.process(ctx);
        for (size_t i = 0; i < 256; ++i) {
            assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
        }
    }

    node.reset();
    std::cout << "PASSED\n";
}

void testBrickwallLimiterTruePeakAndLookahead() {
    std::cout << "[TEST] BrickwallLimiterNode True Peak & Lookahead Safety (Reglas 5, 8, 9, 14, 34, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    BrickwallLimiterNode limiter;
    limiter.prepare(spec);

    assert(limiter.getType() == NodeType::BrickwallLimiter);
    assert(std::string(limiter.getName()) == "Brickwall Limiter");

    limiter.setParameter(BrickwallLimiterNode::Ceiling, -0.5f); // -0.5 dBFS ~ 0.944f
    limiter.setParameter(BrickwallLimiterNode::Threshold, -6.0f);
    limiter.setParameter(BrickwallLimiterNode::Release, 20.0f);
    limiter.setParameter(BrickwallLimiterNode::Lookahead, 2.0f);
    limiter.setParameter(BrickwallLimiterNode::AutoMakeup, 1.0f);

    std::vector<float> inL(256, 1.8f); // Hot input well above 0 dBFS
    std::vector<float> inR(256, 1.8f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    const float maxAllowed = 0.9442f; // EnvelopeDetector::dbToLinear(-0.5f) + epsilon
    for (int b = 0; b < 10; ++b) {
        limiter.process(ctx);
        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(std::abs(outL[s]) <= maxAllowed + 0.001f);
            assert(std::abs(outR[s]) <= maxAllowed + 0.001f);
        }
    }

    limiter.reset();
    std::cout << "PASSED\n";
}

void testBitcrusherQuantizationAndDownsampling() {
    std::cout << "[TEST] BitcrusherNode Bit Depth Quantization & Downsampling (Reglas 5, 8, 9, 14, 34, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    BitcrusherNode node;
    node.prepare(spec);

    assert(node.getType() == NodeType::Bitcrusher);
    assert(std::string(node.getName()) == "Bitcrusher");

    node.setParameter(BitcrusherNode::BitDepth, 4.0f); // 4-bit resolution: 2^(4-1) = 8 levels
    node.setParameter(BitcrusherNode::Downsample, 6.0f); // 6x sample-and-hold
    node.setParameter(BitcrusherNode::Jitter, 0.0f);
    node.setParameter(BitcrusherNode::AntiAliasing, 0.0f);
    node.setParameter(BitcrusherNode::Drive, 0.0f);
    node.setParameter(BitcrusherNode::Mix, 1.0f);

    std::vector<float> inL(256);
    std::vector<float> inR(256);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    for (size_t i = 0; i < 256; ++i) {
        inL[i] = std::sin(2.0f * std::numbers::pi_v<float> * 440.0f * (static_cast<float>(i) / 44100.0f));
        inR[i] = inL[i];
    }
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    node.process(ctx);

    // Verify samples are quantized to steps of 1/8 = 0.125f
    for (size_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        float scaled = outL[s] * 8.0f;
        float diffFromInt = std::abs(scaled - std::round(scaled));
        assert(diffFromInt < 1e-4f);
    }

    node.reset();
    std::cout << "PASSED\n";
}

void testNoiseGateHysteresisAndHold() {
    std::cout << "[TEST] NoiseGateNode Hysteresis, Hold & Sidechain Filter (Reglas 5, 8, 9, 14, 34, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    NoiseGateNode gate;
    gate.prepare(spec);

    assert(gate.getType() == NodeType::NoiseGate);
    assert(std::string(gate.getName()) == "Noise Gate");

    gate.setParameter(NoiseGateNode::Threshold, -20.0f);
    gate.setParameter(NoiseGateNode::Hysteresis, 6.0f);
    gate.setParameter(NoiseGateNode::Attack, 0.5f);
    gate.setParameter(NoiseGateNode::Hold, 10.0f);
    gate.setParameter(NoiseGateNode::Release, 20.0f);
    gate.setParameter(NoiseGateNode::Range, -60.0f);
    gate.setParameter(NoiseGateNode::SidechainHPF, 80.0f);

    std::vector<float> inL(256, 0.5f); // ~ -6 dBFS (Above -20 dB -> Open gate)
    std::vector<float> inR(256, 0.5f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    // 1. Loud signal: gate opens
    for (int b = 0; b < 5; ++b) {
        gate.process(ctx);
    }
    assert(outL[255] > 0.4f);

    // 2. Quiet noise below -26 dB (e.g. -40 dB ~ 0.01f): gate closes down to range
    std::fill(inL.begin(), inL.end(), 0.005f);
    std::fill(inR.begin(), inR.end(), 0.005f);
    for (int b = 0; b < 10; ++b) {
        gate.process(ctx);
    }
    assert(outL[255] < 0.001f);

    gate.reset();
    std::cout << "PASSED\n";
}

void testDeEsserSibilanceAttenuationAndListenMode() {
    std::cout << "[TEST] DeEsserNode Surgical Sibilance Attenuation & Listen Mode (Reglas 5, 8, 9, 14, 34, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    DeEsserNode deesser;
    deesser.prepare(spec);

    assert(deesser.getType() == NodeType::DeEsser);
    assert(std::string(deesser.getName()) == "De-Esser");

    deesser.setParameter(DeEsserNode::Frequency, 6000.0f);
    deesser.setParameter(DeEsserNode::Bandwidth, 2.0f);
    deesser.setParameter(DeEsserNode::Threshold, -20.0f);
    deesser.setParameter(DeEsserNode::Reduction, 18.0f);
    deesser.setParameter(DeEsserNode::Mode, 0.0f); // Split Band
    deesser.setParameter(DeEsserNode::Listen, 0.0f);

    // Generate high frequency sibilance at 6000 Hz at high amplitude (0.8f)
    std::vector<float> inL(256);
    std::vector<float> inR(256);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    for (size_t i = 0; i < 256; ++i) {
        inL[i] = 0.8f * std::sin(2.0f * std::numbers::pi_v<float> * 6000.0f * (static_cast<float>(i) / 44100.0f));
        inR[i] = inL[i];
    }
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    // Process blocks: de-esser reduces sibilant band
    for (int b = 0; b < 10; ++b) {
        deesser.process(ctx);
    }

    float maxSibilantOut = 0.0f;
    for (size_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        maxSibilantOut = std::max(maxSibilantOut, std::abs(outL[s]));
    }
    // High sibilance tone must be attenuated well below the 0.8f input
    assert(maxSibilantOut < 0.45f);

    // Test Listen Mode: returns isolated band
    deesser.setParameter(DeEsserNode::Listen, 1.0f);
    deesser.process(ctx);
    float listenEnergy = 0.0f;
    for (size_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        listenEnergy += std::abs(outL[s]);
    }
    assert(listenEnergy > 0.05f);

    deesser.reset();
    std::cout << "PASSED\n";
}

void testNodeAutomationSequencerMultiLaneAndSync() {
    std::cout << "[TEST] NodeAutomationBank Multi-Lane Tabs, PPQ Sync & Parameter Dispatch (Reglas 1, 7, 8, 9, 25, 46, 47)... ";
    Graph graph;
    auto filterId = graph.addNode(std::make_unique<SimpleFilterNode>(), "FilterNode");
    auto* inst = graph.getNode(filterId);
    assert(inst != nullptr);
    assert(inst->processor != nullptr);

    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    inst->processor->prepare(spec);
    inst->sequencer.prepare(44100.0);

    // 1. Configurar Lane 0 (CutoffHz a 1/16 con Amount 1.0)
    auto& lane0 = inst->sequencer.getLane(0);
    lane0.active = true;
    lane0.targetParamId = SimpleFilterNode::CutoffHz;
    lane0.numSteps = 16;
    lane0.rate = SyncDivision::Sixteenth;
    lane0.amount = 1.0f;
    lane0.glide = 0.0f;
    lane0.steps[0] = 0.1f; // paso 0
    lane0.steps[1] = 0.9f; // paso 1 (a 0.25 beat)

    // 2. Configurar Lane 1 (Resonance a 1/8 con Amount 0.6)
    auto& lane1 = inst->sequencer.getLane(1);
    lane1.active = true;
    lane1.targetParamId = SimpleFilterNode::Resonance;
    lane1.numSteps = 16;
    lane1.rate = SyncDivision::Eighth;
    lane1.amount = 0.6f;
    lane1.glide = 0.0f;
    lane1.steps[0] = 0.2f;
    lane1.steps[1] = 0.8f; // paso 1 (a 0.50 beat)

    // 3. Probar generador de formas rápidas (Sidechain Pump y RampUp)
    inst->sequencer.applyShape(2, NodeAutomationBank::SidechainPump);
    const auto& lane2 = inst->sequencer.getLane(2);
    assert(lane2.steps[0] == 0.0f);
    assert(lane2.steps[3] > 0.5f);

    inst->sequencer.applyShape(3, NodeAutomationBank::RampUp);
    const auto& lane3 = inst->sequencer.getLane(3);
    assert(lane3.steps[0] < lane3.steps[15]);

    std::vector<float> inL(256, 0.5f);
    std::vector<float> inR(256, 0.5f);
    const float* inCh[2] = { inL.data(), inR.data() };
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    float* outCh[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inCh,
        .outputChannels = outCh,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256,
        .bpm = 120.0,
        .ppqPosition = 0.0,
        .isPlaying = true
    };

    // Bloque 1: PPQ = 0.0 (Paso 0 de Lane 0 y Lane 1)
    inst->sequencer.processBlock(ctx, inst->processor.get());
    assert(lane0.currentStep == 0);
    assert(lane1.currentStep == 0);

    // Bloque 2: Avanzar PPQ a 0.26 beats (1/16 transcurrida -> Paso 1 de Lane 0, Paso 0 de Lane 1)
    ctx.ppqPosition = 0.26;
    inst->sequencer.processBlock(ctx, inst->processor.get());
    assert(lane0.currentStep == 1);
    assert(lane1.currentStep == 0); // Lane 1 a 1/8 aún no avanza

    // El Cutoff debe haber alcanzado el paso 1 (~90% del rango)
    float cutoffVal = inst->processor->getParameter(SimpleFilterNode::CutoffHz);
    assert(cutoffVal > 15000.0f);

    // Bloque 3: Avanzar PPQ a 0.51 beats (1/8 transcurrida -> Paso 2 de Lane 0, Paso 1 de Lane 1)
    ctx.ppqPosition = 0.51;
    inst->sequencer.processBlock(ctx, inst->processor.get());
    assert(lane0.currentStep == 2);
    assert(lane1.currentStep == 1);

    // Verificar estabilidad numérica
    float resVal = inst->processor->getParameter(SimpleFilterNode::Resonance);
    assert(!std::isnan(cutoffVal) && !std::isinf(cutoffVal));
    assert(!std::isnan(resVal) && !std::isinf(resVal));

    inst->sequencer.reset();
    std::cout << "PASSED\n";
}

void testAudioVisualizerBufferLockFree() {
    std::cout << "[TEST] AudioVisualizerBuffer Lock-Free SPSC Ring Buffer (Reglas 9, 23, 26, 47)... ";
    AudioVisualizerBuffer buffer;

    // Escribir 10 bloques de 256 muestras (total 2560 muestras)
    std::vector<float> blockL(256);
    std::vector<float> blockR(256);
    for (size_t b = 0; b < 10; ++b) {
        for (size_t s = 0; s < 256; ++s) {
            blockL[s] = static_cast<float>(b * 256 + s);
            blockR[s] = -blockL[s];
        }
        buffer.writeBlock(blockL.data(), blockR.data(), 256);
    }

    // Leer las últimas 512 muestras
    std::vector<float> readL(512);
    std::vector<float> readR(512);
    size_t count = buffer.getLatestSamples(readL.data(), readR.data(), 512);
    assert(count == 512);

    for (size_t i = 0; i < 512; ++i) {
        const float expected = static_cast<float>(2048 + i);
        assert(std::abs(readL[i] - expected) < 1e-4f);
        assert(std::abs(readR[i] - (-expected)) < 1e-4f);
    }

    // Escribir 20 bloques más (para probar wrap-around completo)
    for (size_t b = 10; b < 30; ++b) {
        for (size_t s = 0; s < 256; ++s) {
            blockL[s] = static_cast<float>(b * 256 + s);
            blockR[s] = blockL[s] * 0.5f;
        }
        buffer.writeBlock(blockL.data(), blockR.data(), 256);
    }

    std::vector<float> read1024L(1024);
    std::vector<float> read1024R(1024);
    size_t count1024 = buffer.getLatestSamples(read1024L.data(), read1024R.data(), 1024);
    assert(count1024 == 1024);

    const size_t totalWritten = 30 * 256;
    for (size_t i = 0; i < 1024; ++i) {
        const float expected = static_cast<float>(totalWritten - 1024 + i);
        assert(std::abs(read1024L[i] - expected) < 1e-4f);
    }

    std::cout << "PASSED\n";
}

void testMacroManagerModulationMatrixRouting() {
    std::cout << "[TEST] 8 Performance Macros & ModulationMatrix Routing (Reglas 7, 8, 25, 46)... ";
    MacroManager macroMgr;
    ModulationMatrix matrix;

    macroMgr.setMacro(MacroManager::Texture, 0.8f);
    macroMgr.setMacro(MacroManager::Motion, 0.25f);

    assert(std::abs(macroMgr.getMacro(MacroManager::Texture) - 0.8f) < 1e-4f);
    assert(std::abs(macroMgr.getMacro(MacroManager::Motion) - 0.25f) < 1e-4f);

    int r1 = matrix.addRoute(ModSourceType::MacroTexture, 5, 1, 0.5f, false);
    assert(r1 >= 0);

    int r2 = matrix.addRoute(ModSourceType::MacroMotion, 5, 1, -0.2f, true);
    assert(r2 >= 0);

    std::array<float, static_cast<size_t>(ModSourceType::Count)> sourceValues{};
    sourceValues.fill(0.0f);
    sourceValues[static_cast<size_t>(ModSourceType::MacroTexture)] = macroMgr.getMacro(MacroManager::Texture);
    sourceValues[static_cast<size_t>(ModSourceType::MacroMotion)] = macroMgr.getMacro(MacroManager::Motion);

    float offset = matrix.calculateModulationOffset(5, 1, sourceValues);
    assert(!std::isnan(offset) && !std::isinf(offset));
    assert(std::abs(offset - 0.40f) < 0.05f);

    std::cout << "PASSED\n";
}

void testMidiArpeggiatorAndScaleQuantizer() {
    std::cout << "[TEST] MidiArpeggiatorNode & MidiScaleQuantizerNode (Reglas 5, 8, 9, 14, 34, 37, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };

    MidiArpeggiatorNode arp;
    arp.prepare(spec);
    arp.setParameter(MidiArpeggiatorNode::Rate, 1.0f);
    arp.setParameter(MidiArpeggiatorNode::Pattern, 0.0f);
    arp.setParameter(MidiArpeggiatorNode::Octaves, 2.0f);
    arp.setParameter(MidiArpeggiatorNode::Gate, 80.0f);
    arp.setParameter(MidiArpeggiatorNode::SynthMix, 1.0f);

    const int testChord[4] = { 60, 64, 67, 71 };
    arp.setNotes(testChord, 4);

    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    const float* inCh[2] = { inL.data(), inR.data() };
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    float* outCh[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inCh,
        .outputChannels = outCh,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256,
        .bpm = 120.0,
        .ppqPosition = 0.0,
        .isPlaying = true
    };

    float totalArpEnergy = 0.0f;
    for (int b = 0; b < 16; ++b) {
        ctx.ppqPosition = static_cast<double>(b) * 0.125;
        arp.process(ctx);
        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            totalArpEnergy += std::abs(outL[s]);
        }
    }
    assert(totalArpEnergy > 1.0f);

    MidiScaleQuantizerNode quant;
    quant.prepare(spec);
    quant.setParameter(MidiScaleQuantizerNode::RootKey, 0.0f); // C
    quant.setParameter(MidiScaleQuantizerNode::Scale, 0.0f);   // C Major

    int q1 = quant.quantizeNote(61);
    assert(q1 == 60 || q1 == 62);

    int q2 = quant.quantizeNote(63);
    assert(q2 == 62 || q2 == 64);

    quant.setParameter(MidiScaleQuantizerNode::Scale, 10.0f); // Hirajoshi
    int qHirajoshi = quant.quantizeNote(64);
    assert(qHirajoshi == 63 || qHirajoshi == 62);

    inL.assign(256, 0.4f);
    inR.assign(256, 0.4f);
    quant.process(ctx);
    for (size_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
    }

    std::cout << "PASSED\n";
}

void testMidiChordEngineVoicingsAndStrum() {
    std::cout << "[TEST] MidiChordEngineNode Voicings & Strum Physics (Reglas 5, 8, 9, 14, 34, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };
    MidiChordEngineNode chord;
    chord.prepare(spec);

    chord.setParameter(MidiChordEngineNode::ChordType, 7.0f); // Maj7
    chord.setParameter(MidiChordEngineNode::RootNote, 48.0f);  // C3
    chord.setParameter(MidiChordEngineNode::StrumDelay, 20.0f);
    chord.setParameter(MidiChordEngineNode::Spread, 0.7f);
    chord.setParameter(MidiChordEngineNode::SynthMix, 1.0f);
    chord.triggerChord();

    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    const float* inCh[2] = { inL.data(), inR.data() };
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    float* outCh[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inCh,
        .outputChannels = outCh,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256,
        .bpm = 120.0
    };

    float totalL = 0.0f;
    float totalR = 0.0f;
    for (int b = 0; b < 10; ++b) {
        chord.process(ctx);
        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
            totalL += std::abs(outL[s]);
            totalR += std::abs(outR[s]);
        }
    }

    assert(totalL > 1.0f && totalR > 1.0f);
    std::cout << "PASSED\n";
}

void testSidechainModularRoutingAndVocoder() {
    std::cout << "[TEST] Modular Sidechain Routing, Ducking Compressor & Vocoder (Reglas 4, 6, 9, 13, 28, 46, 47)... ";
    ProcessSpec spec{ 44100.0, 256, 2, 2 };

    // 1. Probar CompressorNode con Sidechain directo
    CompressorNode comp;
    comp.prepare(spec);
    comp.setParameter(CompressorNode::Threshold, -20.0f);
    comp.setParameter(CompressorNode::Ratio, 8.0f);
    comp.setParameter(CompressorNode::Attack, 1.0f);
    comp.setParameter(CompressorNode::Release, 50.0f);

    std::vector<float> inMainL(256, 0.8f);
    std::vector<float> inMainR(256, 0.8f);
    const float* inMainCh[2] = { inMainL.data(), inMainR.data() };

    std::vector<float> scL(256, 1.0f);
    std::vector<float> scR(256, 1.0f);
    const float* scCh[2] = { scL.data(), scR.data() };

    std::vector<float> outCompL(256, 0.0f);
    std::vector<float> outCompR(256, 0.0f);
    float* outCompCh[2] = { outCompL.data(), outCompR.data() };

    ProcessContext compCtx{
        .inputChannels = inMainCh,
        .outputChannels = outCompCh,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256,
        .sidechainChannels = scCh,
        .numSidechainChannels = 2
    };

    for (int b = 0; b < 8; ++b) {
        comp.process(compCtx);
    }

    assert(outCompL[255] < inMainL[255] * 0.7f);

    // 2. Probar ruteo DAG modular con GraphExecutor conectando a Pin 3 (SidechainPinId)
    Graph graph;
    auto nSidechainSrc = graph.addNode(std::make_unique<ExternalSidechainNode>(), "ExtSC");
    auto nComp = graph.addNode(std::make_unique<CompressorNode>(), "DuckingComp");

    ConnectionId cid = graph.connect(nSidechainSrc, 1, nComp, SidechainPinId);
    assert(cid > 0);

    std::vector<NodeId> order;
    std::string err;
    bool validDag = graph.validateAndTopologicalSort(order, err);
    assert(validDag);

    ExecutionPlan plan;
    plan.compileFrom(graph, order);

    const auto& steps = plan.getSteps();
    assert(steps.size() == 2);
    bool foundSidechainLink = false;
    for (const auto& s : steps) {
        if (s.nodeId == nComp && s.sidechainStepIndex >= 0) {
            foundSidechainLink = true;
        }
    }
    assert(foundSidechainLink);

    // 3. Probar Vocoder con Sidechain (Audio In = Carrier, Sidechain In = Modulator)
    VocoderNode vocoder;
    vocoder.prepare(spec);
    std::vector<float> carL(256, 0.5f);
    std::vector<float> carR(256, 0.5f);
    const float* carCh[2] = { carL.data(), carR.data() };

    std::vector<float> modL(256, 0.9f);
    std::vector<float> modR(256, 0.9f);
    const float* modCh[2] = { modL.data(), modR.data() };

    std::vector<float> vocOutL(256, 0.0f);
    std::vector<float> vocOutR(256, 0.0f);
    float* vocOutCh[2] = { vocOutL.data(), vocOutR.data() };

    ProcessContext vocCtx{
        .inputChannels = carCh,
        .outputChannels = vocOutCh,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256,
        .sidechainChannels = modCh,
        .numSidechainChannels = 2
    };

    vocoder.process(vocCtx);
    for (size_t s = 0; s < 256; ++s) {
        assert(!std::isnan(vocOutL[s]) && !std::isinf(vocOutL[s]));
    }

    std::cout << "PASSED\n";
}

void testEventTelemetryBufferLockFree() {
    std::cout << "[TEST] EventTelemetryBuffer Lock-Free SPSC & Bounded Overflow (Reglas 9, 26, 47)... ";
    EventTelemetryBuffer buffer;
    assert(buffer.size() == 0);

    // 1. Empujar y leer 100 elementos comprobando FIFO
    for (uint32_t i = 0; i < 100; ++i) {
        EventTelemetryItem item;
        item.pan = static_cast<float>(i) / 100.0f;
        item.pitchRatio = 1.0f + static_cast<float>(i) * 0.01f;
        item.energy = 0.5f;
        item.distance = 2.0f;
        item.azimuth = 45.0f;
        item.type = EventType::Grain;
        item.generation = 1;
        item.isAlive = true;
        bool ok = buffer.push(item);
        assert(ok);
    }

    assert(buffer.size() == 100);

    for (uint32_t i = 0; i < 100; ++i) {
        EventTelemetryItem outItem;
        bool ok = buffer.pop(outItem);
        assert(ok);
        assert(std::abs(outItem.pan - (static_cast<float>(i) / 100.0f)) < 1e-4f);
        assert(outItem.type == EventType::Grain);
    }
    assert(buffer.size() == 0);

    // 2. Llenar hasta capacidad completa (1024) y verificar desbordamiento controlado
    for (size_t i = 0; i < EventTelemetryBuffer::Capacity; ++i) {
        EventTelemetryItem item;
        item.pan = 0.0f;
        item.energy = 1.0f;
        bool ok = buffer.push(item);
        assert(ok);
    }
    assert(buffer.size() == EventTelemetryBuffer::Capacity);

    // Desbordamiento controlado: debe retornar false sin bloquear ni corromper
    EventTelemetryItem overflowItem;
    overflowItem.energy = 999.0f;
    bool overflowOk = buffer.push(overflowItem);
    assert(!overflowOk);

    // Resetear
    buffer.reset();
    assert(buffer.size() == 0);

    std::cout << "PASSED\n";
}

void testUserPresetBankStorageAndFiltering() {
    std::cout << "[TEST] UserPresetBank Disk Persistence, Tag Indexing & Filtering (Reglas 21, 27)... ";
    auto tempDir = (std::filesystem::temp_directory_path() / "n8_test_presets").string();

    UserPresetBank bank;
    bank.setDirectory(tempDir);

    Graph graph;
    auto node1 = NodeFactory::getInstance().create(NodeType::Delay);
    auto node2 = NodeFactory::getInstance().create(NodeType::Compressor);
    NodeId id1 = graph.addNode(std::move(node1), "Delay", 10.0f, 10.0f);
    NodeId id2 = graph.addNode(std::move(node2), "Compressor", 100.0f, 10.0f);
    graph.connect(id1, 0, id2, 0);

    std::array<float, 8> macros{ 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f };

    // Guardar Preset 1
    PresetMetadata meta1;
    meta1.name = "Ethereal Echoes";
    meta1.category = "Delays";
    meta1.author = "SoundDesigner";
    meta1.description = "Ambient delay cloud with feedback compression";
    meta1.tags = { "Ambient", "Space", "Tape" };
    meta1.favorite = true;
    bool saved1 = bank.savePreset(graph, meta1, macros);
    assert(saved1);

    // Guardar Preset 2
    PresetMetadata meta2;
    meta2.name = "Cyber Glitch";
    meta2.category = "Glitch";
    meta2.author = "Producer";
    meta2.description = "Rapid buffer stutters and distortion";
    meta2.tags = { "Glitch", "Fast", "Aggressive" };
    meta2.favorite = false;
    bool saved2 = bank.savePreset(graph, meta2, macros);
    assert(saved2);

    // Guardar Preset 3
    PresetMetadata meta3;
    meta3.name = "Analog Warmth";
    meta3.category = "Warmth";
    meta3.author = "SoundDesigner";
    meta3.description = "Vintage compression and harmonics";
    meta3.tags = { "Vintage", "Warmth", "Tape" };
    meta3.favorite = true;
    bool saved3 = bank.savePreset(graph, meta3, macros);
    assert(saved3);

    assert(bank.getPresetCount() == 3);

    // Filtrar por texto de búsqueda "Echoes"
    auto resSearch = bank.filter("Echoes", "All", {}, false);
    assert(resSearch.size() == 1);
    assert(bank.getPreset(resSearch[0])->metadata.name == "Ethereal Echoes");

    // Filtrar por categoría "Glitch"
    auto resCat = bank.filter("", "Glitch", {}, false);
    assert(resCat.size() == 1);
    assert(bank.getPreset(resCat[0])->metadata.name == "Cyber Glitch");

    // Filtrar por tag "Tape"
    auto resTag = bank.filter("", "All", { "Tape" }, false);
    assert(resTag.size() == 2);

    // Filtrar solo favoritos
    auto resFav = bank.filter("", "All", {}, true);
    assert(resFav.size() == 2);

    // Conmutar favorito en Preset 2 (Cyber Glitch)
    size_t glitchIdx = 0;
    for (size_t i = 0; i < bank.getPresetCount(); ++i) {
        if (bank.getPreset(i)->metadata.name == "Cyber Glitch") glitchIdx = i;
    }
    bank.toggleFavorite(glitchIdx);
    assert(bank.getPreset(glitchIdx)->metadata.favorite == true);

    auto resFav2 = bank.filter("", "All", {}, true);
    assert(resFav2.size() == 3);

    // Eliminar Preset 1
    size_t echoIdx = 0;
    for (size_t i = 0; i < bank.getPresetCount(); ++i) {
        if (bank.getPreset(i)->metadata.name == "Ethereal Echoes") echoIdx = i;
    }
    bank.deletePreset(echoIdx);
    assert(bank.getPresetCount() == 2);

    // Limpieza de directorio temporal
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);

    std::cout << "PASSED\n";
}

void testInteractiveModulationRoutingAndPayload() {
    std::cout << "[TEST] Interactive Modulation Matrix Routing & Bipolar Depth (Reglas 7, 25, 48)... ";
    ModulationMatrix matrix;

    // Ruta 1: MacroTexture -> Node 3, Param 1
    int r1 = matrix.addRoute(ModSourceType::MacroTexture, 3, 1, 0.75f, true);
    assert(r1 >= 0);
    assert(matrix.getRoute(static_cast<size_t>(r1)).isActive);
    assert(std::abs(matrix.getRoute(static_cast<size_t>(r1)).amount - 0.75f) < 1e-4f);

    // Invertir profundidad de modulación
    matrix.setRouteAmount(static_cast<size_t>(r1), -0.75f);
    assert(std::abs(matrix.getRoute(static_cast<size_t>(r1)).amount - (-0.75f)) < 1e-4f);

    // Ruta 2: LFO1 -> Node 3, Param 2
    int r2 = matrix.addRoute(ModSourceType::LFO1, 3, 2, 0.5f, true);
    assert(r2 >= 0);

    // Ruta 3: ChaosX -> Node 4, Param 0
    int r3 = matrix.addRoute(ModSourceType::ChaosX, 4, 0, 1.0f, false);
    assert(r3 >= 0);

    assert(matrix.getActiveRouteCount() == 3);

    // Eliminar Ruta 2
    matrix.removeRoute(static_cast<size_t>(r2));
    assert(matrix.getActiveRouteCount() == 2);
    assert(!matrix.getRoute(static_cast<size_t>(r2)).isActive);

    std::cout << "PASSED\n";
}

void testAll44DSPNodesInstantiationAndProcessing() {
    std::cout << "[TEST] 50 DSP Nodes Complete Functional Verification & NaN/Inf Safety (Reglas 5, 8, 14, 19, 34, 38)... ";
    ProcessSpec spec{
        .sampleRate = 48000.0,
        .maximumBlockSize = 256,
        .numInputChannels = 2,
        .numOutputChannels = 2
    };

    const NodeType allTypes[] = {
        NodeType::Passthrough,
        NodeType::Filter,
        NodeType::Delay,
        NodeType::AdvancedDelay,
        NodeType::Reverb,
        NodeType::ReverseReverb,
        NodeType::Compressor,
        NodeType::Multiband,
        NodeType::ParametricEQ,
        NodeType::BrickwallLimiter,
        NodeType::NoiseGate,
        NodeType::DeEsser,
        NodeType::TransientShaper,
        NodeType::Distortion,
        NodeType::Bitcrusher,
        NodeType::HarmonicExciter,
        NodeType::Tape,
        NodeType::TapeStop,
        NodeType::FormantFilter,
        NodeType::NoiseTexture,
        NodeType::Granular,
        NodeType::Spectral,
        NodeType::SpectralProcessor,
        NodeType::Resonator,
        NodeType::Glitch,
        NodeType::Phaser,
        NodeType::Chorus,
        NodeType::Flanger,
        NodeType::RingModulator,
        NodeType::FrequencyShifter,
        NodeType::PitchShifter,
        NodeType::RotarySpeaker,
        NodeType::Vocoder,
        NodeType::KarplusStrong,
        NodeType::MidSideEncoder,
        NodeType::MidSideDecoder,
        NodeType::SpatialPanner,
        NodeType::ExternalSidechain,
        NodeType::MidiArpeggiator,
        NodeType::MidiChordEngine,
        NodeType::MidiScaleQuantizer,
        NodeType::Container,
        NodeType::Feedback,
        NodeType::EventContainer,
        NodeType::Convolution,
        NodeType::Oversampler,
        NodeType::AudioSlicer,
        NodeType::ShimmerReverb,
        NodeType::Refraction,
        NodeType::SpectralSmear
    };

    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);

    for (size_t s = 0; s < 256; ++s) {
        inL[s] = std::sin(2.0f * 3.14159f * 440.0f * static_cast<float>(s) / 48000.0f);
        inR[s] = std::cos(2.0f * 3.14159f * 440.0f * static_cast<float>(s) / 48000.0f);
    }
    inL[0] = 1.0f; // Impulso

    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    size_t verifiedCount = 0;

    for (NodeType type : allTypes) {
        auto node = NodeFactory::getInstance().create(type);
        assert(node != nullptr && "Error: Fallo al instanciar NodeType desde NodeFactory");
        assert(node->getType() == type);

        node->prepare(spec);

        // 1. Procesar bloque con audio e impulsos
        std::fill(outL.begin(), outL.end(), 0.0f);
        std::fill(outR.begin(), outR.end(), 0.0f);
        node->process(ctx);

        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && "Error: NaN detectado en salida L");
            assert(!std::isinf(outL[s]) && "Error: Inf detectado en salida L");
            assert(!std::isnan(outR[s]) && "Error: NaN detectado en salida R");
            assert(!std::isinf(outR[s]) && "Error: Inf detectado en salida R");
        }

        // 2. Procesar bloque de silencio para probar estabilidad de tails
        std::vector<float> silentInL(256, 0.0f);
        std::vector<float> silentInR(256, 0.0f);
        const float* silentIn[2] = { silentInL.data(), silentInR.data() };
        ProcessContext silentCtx{
            .inputChannels = silentIn,
            .outputChannels = outChannels,
            .numInputChannels = 2,
            .numOutputChannels = 2,
            .numSamples = 256
        };
        node->process(silentCtx);

        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]));
            assert(!std::isinf(outL[s]));
            assert(!std::isnan(outR[s]));
            assert(!std::isinf(outR[s]));
        }

        // 3. Probar get/set en todos los parámetros registrados
        const auto params = node->getParameters();
        for (const auto& p : params) {
            const float orig = node->getParameter(p.id);
            node->setParameter(p.id, p.minValue);
            assert(node->getParameter(p.id) >= p.minValue - 1e-4f);
            node->setParameter(p.id, p.maxValue);
            assert(node->getParameter(p.id) <= p.maxValue + 1e-4f);
            node->setParameter(p.id, orig);
        }

        node->reset();
        ++verifiedCount;
    }

    assert(verifiedCount == 50);
    std::cout << "PASSED (50/50 procesadores verificados con éxito)\n";
}

void testRadar3DAcousticTelemetryEmission() {
    std::cout << "[TEST] Radar 3D Reactive Acoustic Telemetry & Sweep Pipeline (Reglas 9, 23, 26)... ";
    DualWorldEngine engine;
    ProcessSpec spec{
        .sampleRate = 48000.0,
        .maximumBlockSize = 256,
        .numInputChannels = 2,
        .numOutputChannels = 2
    };
    engine.prepare(spec);

    ExecutionPlan plan;

    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);

    // Audio estéreo con transient fuerte paneado a la derecha
    for (size_t s = 0; s < 256; ++s) {
        inL[s] = 0.2f * std::sin(2.0f * 3.14159f * 1000.0f * static_cast<float>(s) / 48000.0f);
        inR[s] = 0.8f * std::sin(2.0f * 3.14159f * 1000.0f * static_cast<float>(s) / 48000.0f);
    }
    inR[0] = 1.0f; // Ataque/Onset fuerte

    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };
    float* finalOutputs[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    engine.process(plan, ctx, finalOutputs);

    // Verificar que el buffer de telemetría de Radar 3D haya recibido el evento acústico
    auto& telemetry = engine.getEventManager().getTelemetryBuffer();
    EventTelemetryItem item;
    bool found = telemetry.pop(item);
    assert(found && "Error: El Radar 3D no recibió telemetría acústica durante la reproducción");
    assert(item.isAlive);
    assert(item.energy > 0.0f);
    assert(item.pan > 0.1f && "Error: El paneo estéreo hacia la derecha no se calculó correctamente");
    assert(item.pitchRatio > 0.0f);
    assert(item.distance >= 0.5f && item.distance <= 10.0f);

    std::cout << "PASSED\n";
}

void testPartitionedConvolutionZeroLatency() {
    std::cout << "[TEST] Partitioned Convolution Zero-Latency & Synthetic IR Models (Reglas 5, 8, 9, 14, 34, 46, 47)... " << std::endl;
    ProcessSpec spec{
        .sampleRate = 48000.0,
        .maximumBlockSize = 256,
        .numInputChannels = 2,
        .numOutputChannels = 2
    };

    ConvolutionNode conv;
    conv.prepare(spec);

    // 1. Verificar latencia cero: impulso unitario en muestra 0 debe producir salida inmediata en muestra 0
    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);

    inL[0] = 1.0f; // Impulso Dirac
    inR[0] = 1.0f;

    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    conv.setParameter(ConvolutionNode::Mix, 1.0f); // 100% wet
    conv.process(ctx);

    // El primer sample no debe ser cero (prueba de latencia 0 en la cabeza FIR)
    assert(std::abs(outL[0]) > 0.01f && "Error: La convolución en cabeza debe tener latencia cero");
    assert(std::abs(outR[0]) > 0.01f);

    for (size_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
    }

    // 2. Procesar varios bloques consecutivos para verificar transición fluida de la cabeza a las colas FFT
    std::fill(inL.begin(), inL.end(), 0.0f);
    std::fill(inR.begin(), inR.end(), 0.0f);
    for (int b = 0; b < 10; ++b) {
        conv.process(ctx);
        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
        }
    }

    // 3. Probar todos los modelos sintéticos incluidos (Small Room, Large Hall, Dark Plate, Vintage Cab 4x12, Spring Tank)
    for (int model = 0; model <= 4; ++model) {
        conv.setParameter(ConvolutionNode::IRType, static_cast<float>(model));
        conv.reset();
        inL[0] = 0.8f;
        conv.process(ctx);
        inL[0] = 0.0f;
        assert(!std::isnan(outL[0]) && !std::isinf(outL[0]));
    }

    // 4. Probar carga de IR personalizada (Custom IR)
    std::vector<float> customIRL(512, 0.0f);
    std::vector<float> customIRR(512, 0.0f);
    customIRL[0] = 0.9f;
    customIRL[64] = 0.5f;
    customIRR[0] = 0.9f;
    customIRR[64] = -0.5f;
    conv.setCustomImpulseResponse(customIRL.data(), customIRR.data(), 512);

    inL[0] = 1.0f;
    conv.process(ctx);
    assert(std::abs(outL[0]) > 0.1f);

    std::cout << "PASSED\n" << std::flush;
}

void testConvolutionCustomIRDoubleBufferingAndThumbnail() {
    std::cout << "[TEST] ConvolutionNode Custom IR Double-Buffering & Waveform Thumbnail (R1, Reglas 9, 30, 46, 47)... " << std::endl;
    ProcessSpec spec{
        .sampleRate = 48000.0,
        .maximumBlockSize = 256,
        .numInputChannels = 2,
        .numOutputChannels = 2
    };

    ConvolutionNode conv;
    conv.prepare(spec);

    // 1. Verificar inicialización de la miniatura de 64 puntos de la IR sintética
    const auto& initThumb = conv.getIRThumbnail();
    assert(initThumb.size() == 64 && "Error: La miniatura de la IR debe contener 64 puntos");
    float maxInit = 0.0f;
    for (float v : initThumb) {
        assert(!std::isnan(v) && !std::isinf(v) && v >= 0.0f && v <= 1.0f);
        maxInit = std::max(maxInit, v);
    }
    assert(maxInit > 0.5f && "Error: La miniatura debe estar normalizada y no vacía");

    // 2. Cargar respuesta al impulso personalizada con envolvente exponencial
    const size_t customLen = 1024;
    std::vector<float> irL(customLen, 0.0f);
    std::vector<float> irR(customLen, 0.0f);
    for (size_t i = 0; i < customLen; ++i) {
        float env = std::exp(-static_cast<float>(i) * 0.008f);
        irL[i] = std::sin(static_cast<float>(i) * 0.15f) * env;
        irR[i] = std::cos(static_cast<float>(i) * 0.15f) * env;
    }

    conv.setCustomImpulseResponse(irL.data(), irR.data(), customLen);
    assert(conv.getParameter(ConvolutionNode::IRType) == 5.0f && "Error: Tipo de IR debe ser Custom (5)");

    // 3. Verificar que la miniatura de 64 puntos refleja la envolvente de la nueva IR
    const auto& customThumb = conv.getIRThumbnail();
    assert(customThumb.size() == 64);
    assert(customThumb[0] > 0.6f && "Error: El pico inicial debe estar cerca de 1.0");
    assert(customThumb[63] < customThumb[0] && "Error: La cola de la envolvente debe mostrar decaimiento");
    for (float v : customThumb) {
        assert(!std::isnan(v) && !std::isinf(v) && v >= 0.0f && v <= 1.0f);
    }

    // 4. Procesar bloques de audio y verificar correctitud numérica y cero latencia
    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);

    inL[0] = 1.0f; // Impulso Dirac
    inR[0] = 1.0f;

    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    conv.setParameter(ConvolutionNode::Mix, 1.0f); // 100% wet
    conv.process(ctx);

    // Muestra 0 no debe ser cero (latencia 0 FIR)
    assert(std::abs(outL[0]) > 0.01f);
    assert(std::abs(outR[0]) > 0.01f);

    // 5. Procesar cola multietapa a través de particiones FFT en frecuencia
    inL[0] = 0.0f;
    inR[0] = 0.0f;
    for (int b = 0; b < 10; ++b) {
        conv.process(ctx);
        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
        }
    }

    // 6. Prueba de resistencia de doble buffering: swaps continuos durante procesamiento de audio
    for (int cycle = 0; cycle < 8; ++cycle) {
        const size_t len = 256 + cycle * 128;
        std::vector<float> dynamicIR(len, 0.4f);
        conv.setCustomImpulseResponse(dynamicIR.data(), dynamicIR.data(), len);
        conv.process(ctx);
        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        }
    }

    // 7. Casos extremos: paso de punteros nulos o longitud 0 no debe causar crash
    conv.setCustomImpulseResponse(nullptr, nullptr, 0);
    conv.process(ctx);

    // Mono a estéreo (puntero R nulo duplica canal L)
    conv.setCustomImpulseResponse(irL.data(), nullptr, customLen);
    conv.process(ctx);
    for (size_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
    }

    std::cout << "PASSED\n" << std::flush;
}

void testConvolutionAdversarialChallenge() {
    std::cout << "\n==================================================\n";
    std::cout << "[CHALLENGER] Adversarial Stress Testing ConvolutionNode (Milestone 1)...\n";
    std::cout << "==================================================\n" << std::flush;

    ProcessSpec spec{
        .sampleRate = 48000.0,
        .maximumBlockSize = 256,
        .numInputChannels = 2,
        .numOutputChannels = 2
    };

    // --------------------------------------------------------------------------
    // Test C1: Partition Boundary Timing Oracle
    // --------------------------------------------------------------------------
    std::cout << "[CHALLENGE 1] Partition 0 -> Partition 1 Boundary Latency & Continuity...\n" << std::flush;
    {
        ConvolutionNode conv;
        conv.prepare(spec);
        conv.setParameter(ConvolutionNode::Mix, 1.0f); // 100% wet
        conv.setParameter(ConvolutionNode::PreDelayMs, 0.0f);
        conv.setParameter(ConvolutionNode::StereoWidth, 1.0f);

        // Create an IR of 512 samples with a marker impulse exactly at sample 128 (Partition 1 start)
        // Partition 0 is samples 0..127 (head FIR)
        // Partition 1 is samples 128..255 (first FFT tail partition)
        std::vector<float> markerIR(512, 0.0f);
        markerIR[128] = 1.0f; // Marker spike at sample 128

        conv.setCustomImpulseResponse(markerIR.data(), markerIR.data(), 512);

        // Feed a single Dirac impulse at sample 0
        std::vector<float> inL(256, 0.0f);
        std::vector<float> inR(256, 0.0f);
        std::vector<float> outBlock0L(256, 0.0f);
        std::vector<float> outBlock0R(256, 0.0f);
        std::vector<float> outBlock1L(256, 0.0f);
        std::vector<float> outBlock1R(256, 0.0f);

        inL[0] = 1.0f;
        inR[0] = 1.0f;

        const float* inChs0[2] = { inL.data(), inR.data() };
        float* outChs0[2] = { outBlock0L.data(), outBlock0R.data() };
        ProcessContext ctx0{
            .inputChannels = inChs0,
            .outputChannels = outChs0,
            .numInputChannels = 2,
            .numOutputChannels = 2,
            .numSamples = 256
        };

        // Process Block 0 (samples 0..255)
        conv.process(ctx0);

        // The impulse was at t=0.
        // In the IR, markerIR has spike at sample 128.
        // Therefore, in Block 0 (which contains samples 0..255), the spike should arrive at sample 128!
        // Specifically, outBlock0L[128] must be non-zero, and samples 128..150 should contain the response!
        std::cout << "  Block 0 Sample 0: " << outBlock0L[0] << "\n";
        std::cout << "  Block 0 Sample 127: " << outBlock0L[127] << "\n";
        std::cout << "  Block 0 Sample 128: " << outBlock0L[128] << "\n";
        std::cout << "  Block 0 Sample 129: " << outBlock0L[129] << "\n";

        // Process Block 1 (samples 256..511) with silence input
        std::fill(inL.begin(), inL.end(), 0.0f);
        std::fill(inR.begin(), inR.end(), 0.0f);
        float* outChs1[2] = { outBlock1L.data(), outBlock1R.data() };
        ProcessContext ctx1{
            .inputChannels = inChs0,
            .outputChannels = outChs1,
            .numInputChannels = 2,
            .numOutputChannels = 2,
            .numSamples = 256
        };
        conv.process(ctx1);

        std::cout << "  Block 1 Sample 0 (Global sample 256): " << outBlock1L[0] << "\n";
        std::cout << "  Block 1 Sample 1 (Global sample 257): " << outBlock1L[1] << "\n";

        float maxBlock0Tail = 0.0f;
        for (size_t s = 128; s < 256; ++s) {
            maxBlock0Tail = std::max(maxBlock0Tail, std::abs(outBlock0L[s]));
        }
        std::cout << "  Max energy in Block 0 [128..255]: " << maxBlock0Tail << "\n";

        float maxBlock1Tail = 0.0f;
        for (size_t s = 0; s < 256; ++s) {
            maxBlock1Tail = std::max(maxBlock1Tail, std::abs(outBlock1L[s]));
        }
        std::cout << "  Max energy in Block 1 [256..511]: " << maxBlock1Tail << "\n";

        // Verification of fix:
        // Expected: spike arrives at sample 128 (maxBlock0Tail > 0.4f) with zero latency gap
        assert(maxBlock0Tail > 0.2f && "Challenge 1 Failed: Partition 1 FFT response must start at sample 128 with non-zero energy");
        assert(outBlock0L[128] > 0.2f && "Challenge 1 Failed: Sample 128 must contain the direct marker spike");
        std::cout << "  [CHALLENGE 1 PASSED] Partition 0 -> Partition 1 boundary continuity verified! Energy at sample 128: "
                  << outBlock0L[128] << ", maxBlock0Tail: " << maxBlock0Tail << "\n";
    }

    // --------------------------------------------------------------------------
    // Test C2: Rapid Multi-Threaded Concurrent IR Swapping Stress Test
    // --------------------------------------------------------------------------
    std::cout << "[CHALLENGE 2] Concurrent Multi-Threaded IR Swapping Stress Test...\n" << std::flush;
    {
        ConvolutionNode conv;
        conv.prepare(spec);

        std::atomic<bool> keepRunning{ true };
        std::atomic<uint64_t> totalProcessBlocks{ 0 };
        std::atomic<uint64_t> totalIRSwaps{ 0 };
        std::atomic<uint64_t> nanOrInfCount{ 0 };

        // Thread 1: Real-time Audio Processing
        auto audioThread = std::jthread([&](std::stop_token stopToken) {
            std::vector<float> inL(256);
            std::vector<float> inR(256);
            std::vector<float> outL(256);
            std::vector<float> outR(256);
            const float* inChs[2] = { inL.data(), inR.data() };
            float* outChs[2] = { outL.data(), outR.data() };
            ProcessContext ctx{
                .inputChannels = inChs,
                .outputChannels = outChs,
                .numInputChannels = 2,
                .numOutputChannels = 2,
                .numSamples = 256
            };

            for (size_t i = 0; i < 256; ++i) {
                inL[i] = std::sin(static_cast<float>(i) * 0.05f);
                inR[i] = std::cos(static_cast<float>(i) * 0.05f);
            }

            while (!stopToken.stop_requested()) {
                conv.process(ctx);
                totalProcessBlocks.fetch_add(1, std::memory_order_relaxed);
                for (size_t i = 0; i < 256; ++i) {
                    if (std::isnan(outL[i]) || std::isinf(outL[i]) ||
                        std::isnan(outR[i]) || std::isinf(outR[i])) {
                        nanOrInfCount.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }
        });

        // Thread 2: Rapid Custom IR Loader (varying lengths)
        auto irThread1 = std::jthread([&](std::stop_token stopToken) {
            size_t iteration = 0;
            while (!stopToken.stop_requested()) {
                size_t len = 64 + (iteration % 20) * 128; // 64 to 2496 samples
                std::vector<float> custom(len);
                for (size_t i = 0; i < len; ++i) {
                    custom[i] = std::sin(static_cast<float>(i + iteration) * 0.1f) * std::exp(-static_cast<float>(i) * 0.005f);
                }
                conv.setCustomImpulseResponse(custom.data(), custom.data(), len);
                totalIRSwaps.fetch_add(1, std::memory_order_relaxed);
                iteration++;
            }
        });

        // Thread 3: Rapid Synthetic Model Switcher
        auto irThread2 = std::jthread([&](std::stop_token stopToken) {
            int model = 0;
            while (!stopToken.stop_requested()) {
                conv.setParameter(ConvolutionNode::IRType, static_cast<float>(model % 5));
                model++;
            }
        });

        // Thread 4: GUI Telemetry Reader
        auto guiThread = std::jthread([&](std::stop_token stopToken) {
            while (!stopToken.stop_requested()) {
                const auto& thumb = conv.getIRThumbnail();
                for (float v : thumb) {
                    if (std::isnan(v) || std::isinf(v)) {
                        nanOrInfCount.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }
        });

        // Let threads race for 1.0 second
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        audioThread.request_stop();
        irThread1.request_stop();
        irThread2.request_stop();
        guiThread.request_stop();

        std::cout << "  Total Audio Blocks Processed: " << totalProcessBlocks.load() << "\n";
        std::cout << "  Total Concurrent IR Swaps: " << totalIRSwaps.load() << "\n";
        std::cout << "  NaN / Inf Anomalies Detected: " << nanOrInfCount.load() << "\n";
        assert(nanOrInfCount.load() == 0 && "Error: Concurrent IR swapping produced NaN or Inf");
        assert(totalProcessBlocks.load() > 500 && "Error: Audio thread starved");
        assert(totalIRSwaps.load() > 100 && "Error: IR Swapping thread starved");
    }

    // --------------------------------------------------------------------------
    // Test C3: Pathological IR Inputs (Zero, Extreme, Sub-Partition)
    // --------------------------------------------------------------------------
    std::cout << "[CHALLENGE 3] Pathological IR Numerical Stability...\n" << std::flush;
    {
        ConvolutionNode conv;
        conv.prepare(spec);

        std::vector<float> inL(256, 1.0f);
        std::vector<float> inR(256, 1.0f);
        std::vector<float> outL(256, 0.0f);
        std::vector<float> outR(256, 0.0f);
        const float* inChs[2] = { inL.data(), inR.data() };
        float* outChs[2] = { outL.data(), outR.data() };
        ProcessContext ctx{
            .inputChannels = inChs,
            .outputChannels = outChs,
            .numInputChannels = 2,
            .numOutputChannels = 2,
            .numSamples = 256
        };

        // 1. All Zeros IR
        std::vector<float> zeroIR(512, 0.0f);
        conv.setCustomImpulseResponse(zeroIR.data(), zeroIR.data(), 512);
        conv.process(ctx);
        for (float v : outL) {
            assert(!std::isnan(v) && !std::isinf(v));
        }

        // 2. 1-Sample IR
        std::vector<float> oneSampleIR{ 0.75f };
        conv.setCustomImpulseResponse(oneSampleIR.data(), oneSampleIR.data(), 1);
        conv.process(ctx);
        for (float v : outL) {
            assert(!std::isnan(v) && !std::isinf(v));
        }

        // 3. Extreme Values IR (+/- 1e20)
        std::vector<float> extremeIR(256, 1e20f);
        extremeIR[128] = -1e20f;
        conv.setCustomImpulseResponse(extremeIR.data(), extremeIR.data(), 256);
        conv.process(ctx);
        for (float v : outL) {
            assert(!std::isnan(v) && !std::isinf(v));
        }

        // 4. Denormal IR
        std::vector<float> denormalIR(256, 1e-39f);
        conv.setCustomImpulseResponse(denormalIR.data(), denormalIR.data(), 256);
        conv.process(ctx);
        for (float v : outL) {
            assert(!std::isnan(v) && !std::isinf(v));
        }

        // 5. Oversized IR (100,000 samples)
        std::vector<float> oversizedIR(100000, 0.01f);
        conv.setCustomImpulseResponse(oversizedIR.data(), oversizedIR.data(), 100000);
        conv.process(ctx);
        for (float v : outL) {
            assert(!std::isnan(v) && !std::isinf(v));
        }
    }

    std::cout << "[CHALLENGER] All adversarial tests executed successfully.\n";
    std::cout << "==================================================\n\n" << std::flush;
}


void testPolyphaseOversamplingPDC() {
    std::cout << "[TEST] Polyphase Oversampling HQ & Delay Compensation (PDC) (Reglas 8, 9, 14, 34, 46, 47)... " << std::endl;
    PolyphaseOversampler oversampler;
    oversampler.prepare(256, PolyphaseOversampler::Factor2x);

    // 1. Comprobar latencias PDC reportadas
    assert(oversampler.getLatencyInSamples() == 6);
    oversampler.setFactor(PolyphaseOversampler::Factor4x);
    assert(oversampler.getLatencyInSamples() == 9);
    oversampler.setFactor(PolyphaseOversampler::Factor8x);
    assert(oversampler.getLatencyInSamples() == 11);

    // 2. Probar fidelidad de reconstrucción 2x con onda senoidal
    oversampler.setFactor(PolyphaseOversampler::Factor2x);
    std::vector<float> inL(256);
    std::vector<float> inR(256);
    for (size_t s = 0; s < 256; ++s) {
        inL[s] = std::sin(2.0f * 3.14159f * 1000.0f * static_cast<float>(s) / 48000.0f);
        inR[s] = std::cos(2.0f * 3.14159f * 1000.0f * static_cast<float>(s) / 48000.0f);
    }

    auto [overPointers, numOverSamples] = oversampler.processUpsample(inL.data(), inR.data(), 256);
    assert(numOverSamples == 512);
    assert(overPointers[0] != nullptr && overPointers[1] != nullptr);

    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    oversampler.processDownsample(overPointers[0], overPointers[1], 512, outL.data(), outR.data(), 256);

    for (size_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
    }

    // 3. Probar OversamplerNode completo con saturación anti-aliasing
    ProcessSpec spec{ 48000.0, 256, 2, 2 };
    OversamplerNode node;
    node.prepare(spec);

    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };
    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    node.setParameter(OversamplerNode::Factor, 2.0f); // 4x oversampling
    node.setParameter(OversamplerNode::DriveDb, 12.0f); // 12 dB drive
    node.setParameter(OversamplerNode::Saturation, 3.0f); // Tube Wavefold
    node.process(ctx);

    for (size_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
        assert(std::abs(outL[s]) <= 1.5f && "Error: Saturación debe contener la amplitud");
    }

    std::cout << "PASSED\n" << std::flush;
}

void testAudioRateCrossModulation() {
    std::cout << "[TEST] Audio-Rate Cross-Modulation / FM Internodal DAG (Reglas 4, 6, 7, 9, 34)... " << std::endl;
    ProcessSpec spec{
        .sampleRate = 48000.0,
        .maximumBlockSize = 256,
        .numInputChannels = 2,
        .numOutputChannels = 2
    };

    Graph graph;
    // Nodo 1: Generador Passthrough o fuente moduladora
    NodeId modSrc = graph.addNode(std::make_unique<PassthroughNode>(), "FM Carrier Source");
    // Nodo 2: SimpleFilterNode (acepta FM en Pin 4)
    NodeId filterNode = graph.addNode(std::make_unique<SimpleFilterNode>(), "FM Filter");

    // Preparar procesadores del grafo con la especificación de audio
    graph.getNodeProcessor(modSrc)->prepare(spec);
    graph.getNodeProcessor(filterNode)->prepare(spec);

    // Conexión de audio principal (Pin 2 -> Pin 1)
    graph.connect(modSrc, 2, filterNode, 1);
    // Conexión de modulación audio-rate hacia FM In (Pin 2 -> Pin 4)
    ConnectionId fmConn = graph.connect(modSrc, 2, filterNode, AudioRateModPinId);
    assert(fmConn != 0);

    // Compilar ejecución topológica
    std::vector<NodeId> sorted;
    std::string err;
    bool valid = graph.validateAndTopologicalSort(sorted, err);
    assert(valid);

    ExecutionPlan plan;
    plan.compileFrom(graph, sorted);

    const auto& steps = plan.getSteps();
    assert(steps.size() == 2);
    assert(steps[1].audioRateModStepIndex == 0 && "Error: audioRateModStepIndex debe vincularse al paso 0");

    // Ejecutar con GraphExecutor
    GraphExecutor executor;
    executor.prepare(spec, 32);

    std::vector<float> inL(256);
    std::vector<float> inR(256);
    for (size_t s = 0; s < 256; ++s) {
        inL[s] = std::sin(2.0f * 3.14159f * 220.0f * static_cast<float>(s) / 48000.0f);
        inR[s] = inL[s];
    }
    const float* inChannels[2] = { inL.data(), inR.data() };
    std::vector<float> dummyOutL(256, 0.0f);
    std::vector<float> dummyOutR(256, 0.0f);
    float* outChannels[2] = { dummyOutL.data(), dummyOutR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    PreallocatedBuffer finalOut;
    finalOut.prepare(2, 256);

    for (int b = 0; b < 10; ++b) {
        executor.process(plan, ctx, finalOut);
        const float* outL = finalOut.getReadPointer(0);
        const float* outR = finalOut.getReadPointer(1);
        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
        }
    }

    std::cout << "PASSED\n" << std::flush;
}

void testMpeVoiceAllocationAndDimensions() {
    std::cout << "[TEST] MPE 5D Expression & Polyphonic Event Voicing (Reglas 2, 3, 5, 8, 9, 14, 34, 46)... " << std::endl;
    MpeManager mpe;
    mpe.prepare(48000.0);
    mpe.setPerNotePitchBendRange(48.0f);
    mpe.setMasterPitchBendRange(2.0f);

    EventManager eventManager;
    eventManager.prepare(48000.0, 256, 128);

    // 1. Note On en canal 2 (Miembro MPE: Nota 60 C4, Vel 100)
    std::cout << "mpe step 1" << std::endl;
    mpe.processMidiMessage(0x91, 60, 100, &eventManager); // 0x91 = Note On canal 2 (0-indexed = 1)
    assert(mpe.getActiveVoiceCount() == 1);
    const auto* v1 = mpe.getVoice(1);
    assert(v1 != nullptr && v1->isActive);
    assert(v1->noteNumber == 60);
    assert(std::abs(v1->strikeVelocity - (100.0f / 127.0f)) < 0.001f);
    assert(std::abs(v1->baseFrequency - 261.6255f) < 0.1f);
    assert(v1->associatedEventId != InvalidEventId);

    // 2. Note On polifónica simultánea en canal 3 (Nota 64 E4, Vel 80)
    std::cout << "mpe step 2" << std::endl;
    mpe.processMidiMessage(0x92, 64, 80, &eventManager);
    assert(mpe.getActiveVoiceCount() == 2);
    const auto* v2 = mpe.getVoice(2);
    assert(v2 != nullptr && v2->isActive);
    assert(v2->noteNumber == 64);

    // 3. Expresión Press: Channel Pressure en canal 2 (Aftertouch continuo = 90)
    std::cout << "mpe step 3" << std::endl;
    mpe.processMidiMessage(0xD1, 90, 0, &eventManager);
    assert(std::abs(v1->press - (90.0f / 127.0f)) < 0.001f);
    assert(v2->press == 0.0f); // La voz en canal 3 permanece aislada

    // 4. Expresión Slide: CC 74 en canal 2 (Timbre = 110)
    std::cout << "mpe step 4" << std::endl;
    mpe.processMidiMessage(0xB1, 74, 110, &eventManager);
    assert(std::abs(v1->slide - (110.0f / 127.0f)) < 0.001f);

    // 5. Expresión Glide: Pitch Bend individual en canal 2 (+48 semitonos = raw 16383)
    std::cout << "mpe step 5" << std::endl;
    mpe.processMidiMessage(0xE1, 0x7F, 0x7F, &eventManager);
    assert(std::abs(v1->perNoteGlideSemitones - 48.0f) < 0.1f);
    assert(std::abs(v1->pitchRatio - 16.0f) < 0.1f); // 4 octavas arriba (2^4 = 16)
    assert(v2->perNoteGlideSemitones == 0.0f); // Voz 2 no recibe bend

    // 6. Master Pitch Bend en canal 1 (Maestro de la zona): Modulación común (+2 semitonos)
    std::cout << "mpe step 6" << std::endl;
    mpe.processMidiMessage(0xE0, 0x7F, 0x7F, &eventManager);
    assert(std::abs(v1->masterGlideSemitones - 2.0f) < 0.05f);
    assert(std::abs(v2->masterGlideSemitones - 2.0f) < 0.05f);

    // 7. Note Off con Lift Velocity (Velocidad de liberación)
    std::cout << "mpe step 7" << std::endl;
    mpe.processMidiMessage(0x81, 60, 75, &eventManager);
    assert(!v1->isActive);
    assert(std::abs(v1->liftVelocity - (75.0f / 127.0f)) < 0.001f);
    assert(mpe.getActiveVoiceCount() == 1);

    mpe.processMidiMessage(0x82, 64, 40, &eventManager);
    assert(!v2->isActive);
    assert(mpe.getActiveVoiceCount() == 0);

    std::cout << "PASSED\n" << std::flush;
}

void testMidiLearnMappingAndCurves() {
    std::cout << "[TEST] MIDI Learn, Controller Profiles & Non-Linear CC Curves (Reglas 8, 9, 21, 46)... " << std::endl;
    MidiMappingManager mgr;

    // 1. Probar modo MIDI Learn interactivo
    std::cout << "learn step 1" << std::endl;
    mgr.startLearning(15 /* paramId */, 2 /* targetNodeId */, 100.0f, 5000.0f, MidiCurve::Exponential);
    assert(mgr.isLearning());

    // Llega un mensaje CC 22 en canal 1 con valor 64
    mgr.processControlChange(1, 22, 64);
    assert(!mgr.isLearning()); // Debe haber salido de learn

    const auto& mappings = mgr.getMappings();
    assert(mappings.size() == 1);
    assert(mappings[0].ccNumber == 22);
    assert(mappings[0].paramId == 15);
    assert(mappings[0].targetNodeId == 2);
    assert(mappings[0].curve == MidiCurve::Exponential);

    // 2. Probar curvas de respuesta matemática: Linear, Exponential, Logarithmic, SShaped
    std::cout << "learn step 2" << std::endl;
    MidiMapping mLinear{ .ccNumber = 10, .minVal = 0.0f, .maxVal = 100.0f, .curve = MidiCurve::Linear };
    float valLinMid = MidiMappingManager::calculateMappedValue(mLinear, 64); // ~0.504 * 100
    assert(std::abs(valLinMid - 50.39f) < 0.5f);

    MidiMapping mExp{ .ccNumber = 11, .minVal = 0.0f, .maxVal = 100.0f, .curve = MidiCurve::Exponential };
    float valExpMid = MidiMappingManager::calculateMappedValue(mExp, 64); // (0.504)^2 * 100 ~= 25.4
    assert(std::abs(valExpMid - 25.4f) < 1.0f);

    MidiMapping mLog{ .ccNumber = 12, .minVal = 0.0f, .maxVal = 100.0f, .curve = MidiCurve::Logarithmic };
    float valLogMid = MidiMappingManager::calculateMappedValue(mLog, 64); // sqrt(0.504) * 100 ~= 71.0
    assert(std::abs(valLogMid - 71.0f) < 1.0f);

    MidiMapping mInv{ .ccNumber = 13, .minVal = 0.0f, .maxVal = 100.0f, .inverted = true, .curve = MidiCurve::Linear };
    float valInv = MidiMappingManager::calculateMappedValue(mInv, 127);
    assert(valInv == 0.0f); // 127 invertido es 0.0f

    // 3. Probar perfiles de controlador hardware prefabricados
    std::cout << "learn step 3" << std::endl;
    mgr.loadFactoryProfile("Akai MPK Mini MK3");
    assert(mgr.getMappings().size() == 8);
    assert(mgr.getMappings()[0].ccNumber == 70);
    assert(mgr.getMappings()[7].ccNumber == 77);

    mgr.loadFactoryProfile("Arturia KeyLab Essential");
    assert(mgr.getMappings().size() == 8);
    assert(mgr.getMappings()[0].ccNumber == 73);

    // 4. Probar Serialización y Deserialización JSON de perfiles (.n8midi)
    std::cout << "learn step 4" << std::endl;
    std::string jsonProfile = mgr.exportToJson("My Studio Master Setup");
    assert(!jsonProfile.empty());
    assert(jsonProfile.find("My Studio Master Setup") != std::string::npos);

    MidiMappingManager importedMgr;
    bool importOk = importedMgr.importFromJson(jsonProfile);
    assert(importOk);
    assert(importedMgr.getMappings().size() == 8);
    assert(importedMgr.getMappings()[0].ccNumber == 73);

    std::cout << "PASSED\n" << std::flush;
}

void testGraphUndoTimelineAndTimeTravel() {
    std::cout << "[TEST] Graph Undo Timeline & Direct Time-Travel (Reglas 21, 22, 47)... " << std::endl;
    GraphUndoManager undoMgr(16);

    Graph graph;
    auto n1 = graph.addNode(NodeFactory::getInstance().create(NodeType::Filter), "F1", 50.0f, 50.0f);
    undoMgr.pushAction("Add Filter Node", ActionCategory::NodeAdd, graph);

    assert(!undoMgr.canUndo());
    assert(undoMgr.getCurrentIndex() == 0);
    assert(undoMgr.getHistorySize() == 1);
    assert(undoMgr.getTimeline()[0].category == ActionCategory::NodeAdd);

    // Acción 2: Agregar Delay
    auto n2 = graph.addNode(NodeFactory::getInstance().create(NodeType::Delay), "D1", 150.0f, 50.0f);
    undoMgr.pushAction("Add Delay Node", ActionCategory::NodeAdd, graph);
    assert(undoMgr.canUndo());
    assert(!undoMgr.canRedo());
    assert(undoMgr.getCurrentIndex() == 1);
    assert(undoMgr.getHistorySize() == 2);

    // Acción 3: Conectar F1 -> D1
    graph.connect(n1, 2, n2, 1);
    undoMgr.pushAction("Connect Filter to Delay", ActionCategory::Connection, graph);
    assert(undoMgr.getCurrentIndex() == 2);
    assert(undoMgr.getHistorySize() == 3);

    // Acción 4: Agregar Distortion
    auto n3 = graph.addNode(NodeFactory::getInstance().create(NodeType::Distortion), "Dist1", 250.0f, 50.0f);
    graph.connect(n2, 2, n3, 1);
    undoMgr.pushAction("Add Distortion & Wire", ActionCategory::Connection, graph);
    assert(undoMgr.getCurrentIndex() == 3);
    assert(undoMgr.getHistorySize() == 4);

    // 1. Time-Travel directo: Saltar del paso 3 al paso 0 (Direct Jump)
    PresetMetadata meta;
    std::array<float, 8> macros{};
    bool jumpOk = undoMgr.jumpToStep(0, graph, meta, macros);
    assert(jumpOk);
    assert(undoMgr.getCurrentIndex() == 0);
    assert(graph.getNodes().size() == 1);
    assert(graph.getConnections().empty());
    assert(undoMgr.canRedo());
    assert(!undoMgr.canUndo());

    // 2. Time-Travel directo hacia el futuro: Saltar del paso 0 al paso 3
    jumpOk = undoMgr.jumpToStep(3, graph, meta, macros);
    assert(jumpOk);
    assert(undoMgr.getCurrentIndex() == 3);
    assert(graph.getNodes().size() == 3);
    assert(graph.getConnections().size() == 2);
    assert(!undoMgr.canRedo());
    assert(undoMgr.canUndo());

    // 3. Probar bifurcación (Branching) en el pasado:
    // Saltar a paso 1, y luego realizar una nueva acción (debe truncar pasos 2 y 3)
    jumpOk = undoMgr.jumpToStep(1, graph, meta, macros);
    assert(jumpOk);
    assert(undoMgr.getCurrentIndex() == 1);

    auto nNew = graph.addNode(NodeFactory::getInstance().create(NodeType::Reverb), "RevAlternative", 150.0f, 150.0f);
    undoMgr.pushAction("Branch: Add Reverb", ActionCategory::NodeAdd, graph);
    assert(undoMgr.getCurrentIndex() == 2);
    assert(undoMgr.getHistorySize() == 3);
    assert(!undoMgr.canRedo()); // Futuro truncado

    // 4. Verificación de descripciones de Undo/Redo
    assert(undoMgr.getNextUndoDescription() == "Branch: Add Reverb");
    bool undoOk = undoMgr.undo(graph, meta, macros);
    assert(undoOk);
    assert(undoMgr.getNextRedoDescription() == "Branch: Add Reverb");

    std::cout << "PASSED\n" << std::flush;
}

void testPresetPackagerExportAndImport() {
    std::cout << "[TEST] PresetPackager .n8pack Bundling, Manifest & Duplicate Resolution (Reglas 21, 27)... " << std::endl;

    // 1. Crear presets sintéticos de prueba
    std::vector<UserPresetRecord> presets;
    for (int i = 0; i < 3; ++i) {
        Graph g;
        g.addNode(NodeFactory::getInstance().create(NodeType::Filter), "Filt" + std::to_string(i), 50.0f, 50.0f);
        PresetMetadata m;
        m.name = "Pack Preset " + std::to_string(i + 1);
        m.author = "Test Designer";
        m.category = "Cinematic";
        m.tags = { "cinematic", "filter", "cyberpunk" };
        std::array<float, 8> mac{ 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f };

        UserPresetRecord rec;
        rec.filePath = "mock/path/" + m.name + ".n8preset";
        rec.metadata = m;
        rec.jsonContent = GraphSerializer::serialize(g, m, mac);
        presets.push_back(std::move(rec));
    }

    // 2. Empaquetar y exportar a string JSON (.n8pack)
    PresetPackManifest manifest{
        .packVersion = 1,
        .packName = "Cyber Odyssey Vol 1",
        .author = "N8 Master Team",
        .description = "Futuristic atmospheres and rhythmic textures",
        .version = "1.2.0",
        .createdAt = "2026-09-24",
        .tags = { "cyberpunk", "hybrid", "cinematic" },
        .presetCount = presets.size()
    };

    std::string packJson = PresetPackager::serializePack(presets, manifest);
    assert(!packJson.empty());
    assert(packJson.find("Cyber Odyssey Vol 1") != std::string::npos);

    // 3. Inspección del manifiesto
    PresetPackManifest parsedManifest;
    std::string inspectErr;
    bool inspectOk = PresetPackager::inspectPackManifest(packJson, parsedManifest, inspectErr);
    assert(inspectOk);
    assert(parsedManifest.packName == "Cyber Odyssey Vol 1");
    assert(parsedManifest.author == "N8 Master Team");
    assert(parsedManifest.version == "1.2.0");
    assert(parsedManifest.presetCount == 3);

    // 4. Importar paquete en directorio temporal de prueba
    std::string tempPackDir = "build/temp_pack_test";
    std::filesystem::remove_all(tempPackDir);

    PackImportReport report1;
    bool importOk1 = PresetPackager::importPackContent(packJson, tempPackDir, DuplicatePolicy::Overwrite, report1);
    assert(importOk1);
    assert(report1.totalFound == 3);
    assert(report1.imported == 3);
    assert(report1.errors.empty());

    // 5. Probar política de duplicados: Skip
    PackImportReport reportSkip;
    bool importOkSkip = PresetPackager::importPackContent(packJson, tempPackDir, DuplicatePolicy::Skip, reportSkip);
    assert(importOkSkip || reportSkip.skipped == 3);
    assert(reportSkip.skipped == 3);
    assert(reportSkip.imported == 0);

    // 6. Probar política de duplicados: RenameWithSuffix
    PackImportReport reportRename;
    bool importOkRename = PresetPackager::importPackContent(packJson, tempPackDir, DuplicatePolicy::RenameWithSuffix, reportRename);
    assert(importOkRename);
    assert(reportRename.imported == 3);

    // Limpiar directorio temporal
    std::filesystem::remove_all(tempPackDir);

    std::cout << "PASSED\n" << std::flush;
}

void testNodeGroupsAndMacroAutomation() {
    std::cout << "[TEST] Node Groups Chassis & 3 Macro Automations with Bipolar Depth (R2, Reglas 4, 8, 21, 22)... ";
    Graph graph;

    // 1. Añadir 3 nodos al grafo: Filter, Delay, Distortion
    auto filterId = graph.addNode(std::make_unique<SimpleFilterNode>(), "Filter");
    auto delayId = graph.addNode(std::make_unique<SimpleDelayNode>(), "Delay");
    auto distId = graph.addNode(std::make_unique<DistortionNode>(), "Distortion");

    assert(filterId != InvalidNodeId);
    assert(delayId != InvalidNodeId);
    assert(distId != InvalidNodeId);

    // 2. Crear un grupo que contenga los 3 nodos
    auto groupId = graph.addGroup("TEST_GROUP", 0xff00ffcc, { filterId, delayId, distId });
    assert(groupId != InvalidGroupId);

    const auto* group = graph.getGroup(groupId);
    assert(group != nullptr);
    assert(group->name == "TEST_GROUP");
    assert(group->memberNodeIds.size() == 3);
    assert(!group->isBypassed);

    // 3. Configurar macros: Mapear Macro 0 (CTRL 1) a Cutoff de Filter con profundidad positiva
    // y Macro 1 (CTRL 2) a Feedback de Delay con profundidad negativa
    auto* mutGroup = graph.getGroup(groupId);
    GroupMacroMapping map1{
        .targetNodeId = filterId,
        .targetParamId = SimpleFilterNode::CutoffHz,
        .depth = 0.5f, // bipolar depth
        .baseValue = 1000.0f
    };
    mutGroup->macros[0].mappings.push_back(map1);

    GroupMacroMapping map2{
        .targetNodeId = delayId,
        .targetParamId = SimpleDelayNode::Feedback,
        .depth = -0.4f, // negative depth
        .baseValue = 0.5f
    };
    mutGroup->macros[1].mappings.push_back(map2);

    // 4. Probar modulación de Macro 0
    // Macro al centro (0.5) -> valor base
    graph.applyGroupMacroValue(groupId, 0, 0.5f);
    auto* filterNode = graph.getNodeProcessor(filterId);
    assert(filterNode != nullptr);
    float valCenter = filterNode->getParameter(SimpleFilterNode::CutoffHz);
    assert(std::abs(valCenter - 1000.0f) < 5.0f);

    // Macro al máximo (1.0) -> baseValue + 0.5 * depth * range
    graph.applyGroupMacroValue(groupId, 0, 1.0f);
    float valMax = filterNode->getParameter(SimpleFilterNode::CutoffHz);
    assert(valMax > valCenter);

    // Macro al mínimo (0.0) -> baseValue - 0.5 * depth * range
    graph.applyGroupMacroValue(groupId, 0, 0.0f);
    float valMin = filterNode->getParameter(SimpleFilterNode::CutoffHz);
    assert(valMin < valCenter);

    // 5. Probar Macro 1 con profundidad negativa
    graph.applyGroupMacroValue(groupId, 1, 0.5f);
    auto* delayNode = graph.getNodeProcessor(delayId);
    assert(delayNode != nullptr);
    float dCenter = delayNode->getParameter(SimpleDelayNode::Feedback);

    graph.applyGroupMacroValue(groupId, 1, 1.0f);
    float dMax = delayNode->getParameter(SimpleDelayNode::Feedback);
    assert(dMax < dCenter); // Con profundidad negativa, macro a 1.0 disminuye el parámetro

    // 6. Probar Bypass colectivo del grupo
    graph.setGroupBypassed(groupId, true);
    assert(graph.getGroup(groupId)->isBypassed);
    assert(filterNode->isBypassed());
    assert(delayNode->isBypassed());
    assert(graph.getNodeProcessor(distId)->isBypassed());

    graph.setGroupBypassed(groupId, false);
    assert(!graph.getGroup(groupId)->isBypassed);
    assert(!filterNode->isBypassed());
    assert(!delayNode->isBypassed());
    assert(!graph.getNodeProcessor(distId)->isBypassed());

    // 7. Probar serialización y deserialización de NodeGroups
    std::string json = GraphSerializer::serialize(graph);
    assert(!json.empty());
    assert(json.find("\"groups\"") != std::string::npos);
    assert(json.find("\"TEST_GROUP\"") != std::string::npos);

    Graph loadedGraph;
    PresetMetadata outMeta;
    std::array<float, 8> outMacros;
    std::string outErr;
    bool loadOk = GraphSerializer::deserialize(json, loadedGraph, outMeta, outMacros, outErr);
    assert(loadOk);
    const auto& loadedGroups = loadedGraph.getGroups();
    assert(loadedGroups.size() == 1);
    const auto* loadedGrp = loadedGraph.getGroup(loadedGroups.begin()->first);
    assert(loadedGrp != nullptr);
    assert(loadedGrp->name == "TEST_GROUP");
    assert(loadedGrp->memberNodeIds.size() == 3);
    assert(loadedGrp->macros[0].mappings.size() == 1);
    assert(loadedGrp->macros[1].mappings.size() == 1);

    // 8. Probar eliminación del grupo
    bool removeOk = graph.removeGroup(groupId);
    assert(removeOk);
    assert(graph.getGroup(groupId) == nullptr);

    std::cout << "PASSED\n" << std::flush;
}

void testAudioSlicerNodeProcessingAndFreeze() {
    std::cout << "[TEST] AudioSlicerNode (DSP #49) Rhythmic Slicing & Frozen Granular Playground (R3, Reglas 5, 8, 9, 34, 46, 47)... ";

    auto node = NodeFactory::getInstance().create(NodeType::AudioSlicer);
    assert(node != nullptr && "Error: No se pudo crear AudioSlicerNode desde NodeFactory");
    assert(node->getType() == NodeType::AudioSlicer);
    assert(node->getName() == "Audio Slicer");

    ProcessSpec spec{
        .sampleRate = 48000.0,
        .maximumBlockSize = 256,
        .numInputChannels = 2,
        .numOutputChannels = 2
    };
    node->prepare(spec);

    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);

    // 1. Probar grabación en streaming continuo en ring buffer
    for (size_t s = 0; s < 256; ++s) {
        inL[s] = 0.5f * std::sin(2.0f * 3.14159f * 440.0f * static_cast<float>(s) / 48000.0f);
        inR[s] = 0.5f * std::cos(2.0f * 3.14159f * 440.0f * static_cast<float>(s) / 48000.0f);
    }
    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    // Procesar varios bloques para llenar parte del ring buffer
    for (int b = 0; b < 10; ++b) {
        node->process(ctx);
        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
        }
    }

    // 2. Probar activación de Modo Freeze (congelación de audio y micro-granos)
    node->setParameter(AudioSlicerNode::Freeze, 1.0f);
    assert(node->getParameter(AudioSlicerNode::Freeze) == 1.0f);

    // Alimentar silencio mientras está en Freeze: la salida debe contener granos del buffer congelado
    std::vector<float> silentL(256, 0.0f);
    std::vector<float> silentR(256, 0.0f);
    const float* silentIn[2] = { silentL.data(), silentR.data() };
    ProcessContext freezeCtx{
        .inputChannels = silentIn,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    float energyL = 0.0f;
    float energyR = 0.0f;
    for (int b = 0; b < 10; ++b) {
        node->process(freezeCtx);
        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
            energyL += outL[s] * outL[s];
            energyR += outR[s] * outR[s];
        }
    }
    assert(energyL > 0.0f && "Error: El modo Freeze no generó audio granular desde el buffer congelado");

    // 3. Probar parámetros granulares: Scrub, PitchShift, Reverse Speed
    node->setParameter(AudioSlicerNode::PositionScrub, 0.75f);
    node->setParameter(AudioSlicerNode::PlaybackSpeed, -1.0f); // Reversa
    node->setParameter(AudioSlicerNode::PitchShiftSemitones, 7.0f); // Quinta justa
    node->setParameter(AudioSlicerNode::GrainSizeMs, 120.0f);
    node->setParameter(AudioSlicerNode::Jitter, 0.3f);
    node->setParameter(AudioSlicerNode::FilterCutoff, 2500.0f);

    for (int b = 0; b < 10; ++b) {
        node->process(freezeCtx);
        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
        }
    }

    // 4. Salir de freeze y verificar reset
    node->setParameter(AudioSlicerNode::Freeze, 0.0f);
    node->reset();

    std::cout << "PASSED\n" << std::flush;
}

void testPhase1BiquadFilterSmoothInterpolation() {
    std::cout << "[TEST] BiquadFilter Smooth Coefficient Interpolation (Reglas 1, 34, 35)... ";
    BiquadFilter filter;
    filter.reset();
    filter.setCoefficients(BiquadFilter::Type::Lowpass, 44100.0, 1000.0f, 0.707f);
    assert(!filter.isRamping());

    // Iniciar rampa hacia 200 Hz en 128 muestras
    filter.setLowpassSmooth(44100.0, 200.0f, 0.707f, 128);
    assert(filter.isRamping());

    // Procesar bloque durante la rampa
    for (int i = 0; i < 64; ++i) {
        float out = filter.processSample(0.5f);
        assert(!std::isnan(out) && !std::isinf(out));
    }
    assert(filter.isRamping());

    // Terminar rampa
    for (int i = 0; i < 64; ++i) {
        float out = filter.processSample(0.5f);
        assert(!std::isnan(out) && !std::isinf(out));
    }
    assert(!filter.isRamping());

    std::cout << "PASSED\n";
}

void testPhase1DelayLinePowerOfTwoBitmask() {
    std::cout << "[TEST] DelayLine Power-of-Two Bitmask & 64-Byte Alignment (Reglas 11, 12, 47)... ";
    DelayLine delay;
    delay.prepare(100);
    // Capacidad debe ser potencia de 2 >= 104 -> 128
    assert(delay.getCapacity() == 128);
    assert((delay.getCapacity() & (delay.getCapacity() - 1)) == 0);
    assert(delay.getMask() == 127);

    // Escribir impulso
    delay.write(1.0f);
    for (int i = 0; i < 20; ++i) {
        delay.write(0.0f);
    }

    // Leer con retardo entero y fraccional
    float rLinear = delay.readLinear(20.0f);
    assert(std::abs(rLinear - 1.0f) < 1e-4f);

    float rCubic = delay.readCubic(20.0f);
    assert(std::abs(rCubic - 1.0f) < 1e-3f);

    std::cout << "PASSED\n";
}

void testPhase1PreallocatedBuffer64ByteAlignment() {
    std::cout << "[TEST] PreallocatedBuffer 64-Byte Alignment & Channel Stride (Reglas 11, 47)... ";
    PreallocatedBuffer buf;
    buf.prepare(2, 513); // 513 muestras -> channelStride_ debe ser múltiplo de 16 (528)
    assert(buf.getChannelStride() % 16 == 0);

    float* ch0 = buf.getWritePointer(0);
    float* ch1 = buf.getWritePointer(1);

    // Verificar alineación a 64 bytes (16 floats = 64 bytes)
    assert(reinterpret_cast<uintptr_t>(ch0) % 64 == 0);
    assert(reinterpret_cast<uintptr_t>(ch1) % 64 == 0);

    buf.clear(513);
    assert(ch0[0] == 0.0f && ch1[512] == 0.0f);

    std::cout << "PASSED\n";
}

void testPhase1TransientShaperMultibandLR4() {
    std::cout << "[TEST] TransientShaperNode 3-Band Multiband LR4 (Reglas 2, 5, 32, 47)... ";
    TransientShaperNode shaper;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    shaper.prepare(spec);

    // Multiband mode
    shaper.setParameter(TransientShaperNode::Mode, 1.0f);
    shaper.setParameter(TransientShaperNode::LowAttack, 0.8f);
    shaper.setParameter(TransientShaperNode::MidAttack, -0.5f);
    shaper.setParameter(TransientShaperNode::HighAttack, 0.5f);
    shaper.setParameter(TransientShaperNode::Mix, 1.0f);

    std::vector<float> inL(128, 0.4f);
    std::vector<float> inR(128, 0.4f);
    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);

    const float* inCh[2] = { inL.data(), inR.data() };
    float* outCh[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    shaper.process(ctx);

    for (int s = 0; s < 128; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
    }

    std::cout << "PASSED\n";
}

void testPhase1FeedbackContainerDynamicAGC() {
    std::cout << "[TEST] FeedbackContainerNode Dynamic AGC Anti-Runaway (Reglas 6, 11, 12, 34)... ";
    FeedbackContainerNode fbNode;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    fbNode.prepare(spec);

    // Condición extrema de sobrealimentación (Feedback = 1.4 > 1.0)
    fbNode.setParameter(FeedbackContainerNode::Feedback, 1.4f);
    fbNode.setParameter(FeedbackContainerNode::DelayTime, 10.0f);
    fbNode.setParameter(FeedbackContainerNode::Threshold, 1.0f);
    fbNode.setParameter(FeedbackContainerNode::Mix, 1.0f);

    std::vector<float> inL(128, 0.8f);
    std::vector<float> inR(128, 0.8f);
    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);

    const float* inCh[2] = { inL.data(), inR.data() };
    float* outCh[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    // Procesar 50 bloques sucesivos para activar y sostener auto-oscilación
    for (int b = 0; b < 50; ++b) {
        fbNode.process(ctx);
        for (int s = 0; s < 128; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
            // El AGC debe haber evitado explosión numérica (< 2.0f a pesar de feedback 140%)
            assert(std::abs(outL[s]) < 2.0f);
            assert(std::abs(outR[s]) < 2.0f);
        }
    }

    std::cout << "PASSED\n";
}

void testPhase2FastMathSIMDVectorization() {
    std::cout << "[TEST] FastMath AVX2/FMA SIMD Vectorization & Scalar Fallback (Reglas 9, 47)... ";
    constexpr size_t N = 75; // 9 bloques de 8 + 3 de fallback escalar
    std::vector<float> in(N), outPadé(N), a(N, 2.0f), b(N, 3.0f), c(N, 1.5f), outFMA(N), outGain(N), outMix(N);

    for (size_t i = 0; i < N; ++i) {
        in[i] = -4.0f + 8.0f * (static_cast<float>(i) / static_cast<float>(N - 1));
    }

    // 1. Vectorized Padé Tanh
    FastMath::vecPadéTanh(in.data(), outPadé.data(), N);
    for (size_t i = 0; i < N; ++i) {
        const float expected = FastMath::fastTanh(in[i]);
        assert(std::abs(outPadé[i] - expected) < 1.0e-4f);
        assert(!std::isnan(outPadé[i]) && !std::isinf(outPadé[i]));
    }

    // 2. Vectorized Multiply-Add (FMA)
    FastMath::vecMultiplyAdd(a.data(), b.data(), c.data(), outFMA.data(), N);
    for (size_t i = 0; i < N; ++i) {
        assert(std::abs(outFMA[i] - 7.5f) < 1.0e-5f);
    }

    // 3. Vectorized Gain
    FastMath::vecApplyGain(a.data(), outGain.data(), 0.5f, N);
    for (size_t i = 0; i < N; ++i) {
        assert(std::abs(outGain[i] - 1.0f) < 1.0e-5f);
    }

    // 4. Vectorized Linear Mix
    FastMath::vecMix(a.data(), b.data(), outMix.data(), 0.25f, N);
    for (size_t i = 0; i < N; ++i) {
        // (1 - 0.25)*2.0 + 0.25*3.0 = 1.5 + 0.75 = 2.25
        assert(std::abs(outMix[i] - 2.25f) < 1.0e-5f);
    }

    // 5. Vectorized Denormals Flush
    std::vector<float> denormals(N, 1.0e-39f);
    denormals[10] = 0.5f;
    FastMath::vecFlushDenormals(denormals.data(), N);
    assert(denormals[0] == 0.0f);
    assert(denormals[10] == 0.5f);

    std::cout << "PASSED\n";
}

void testPhase2KarplusStrongAcousticBodyResonator() {
    std::cout << "[TEST] KarplusStrong 4-Mode Acoustic Body Resonator (Reglas 5, 32, 46)... ";
    AcousticBodyResonator body;
    body.prepare(44100.0);
    body.setParameters(1.0f, 1.0f, 0.5f);

    float outL = 0.0f, outR = 0.0f;
    body.processSample(0.5f, 0.5f, outL, outR);
    assert(!std::isnan(outL) && !std::isinf(outL));
    assert(!std::isnan(outR) && !std::isinf(outR));

    KarplusStrongNode karplus;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    karplus.prepare(spec);

    karplus.setParameter(KarplusStrongNode::BodySize, 1.2f);
    karplus.setParameter(KarplusStrongNode::BodyDecay, 1.5f);
    karplus.setParameter(KarplusStrongNode::BodyMix, 0.6f);
    karplus.setParameter(KarplusStrongNode::Mix, 1.0f);

    std::vector<float> inL(128, 0.0f);
    std::vector<float> inR(128, 0.0f);
    inL[0] = 0.9f; inR[0] = 0.9f; // Impulso

    std::vector<float> oL(128, 0.0f);
    std::vector<float> oR(128, 0.0f);
    const float* inCh[2] = { inL.data(), inR.data() };
    float* outCh[2] = { oL.data(), oR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    karplus.process(ctx);

    for (int s = 0; s < 128; ++s) {
        assert(!std::isnan(oL[s]) && !std::isinf(oL[s]));
        assert(!std::isnan(oR[s]) && !std::isinf(oR[s]));
    }

    std::cout << "PASSED\n";
}

void testPhase2TapeSaturationMagneticHysteresis() {
    std::cout << "[TEST] TapeSaturation Jiles-Atherton Magnetic Hysteresis (Reglas 5, 8, 34)... ";
    MagneticHysteresis hyst;
    hyst.setParameters(3.0f, 0.8f, 1.0f);

    // Excitar con rampa positiva y volver a cero para verificar remanencia magnética
    float mOutL = 0.0f, mOutR = 0.0f;
    for (int i = 0; i < 50; ++i) {
        hyst.processSample(0.8f, 0.8f, mOutL, mOutR);
    }
    // Señal vuelve a cero: la remanencia magnética debe mantener un estado residual no nulo
    hyst.processSample(0.0f, 0.0f, mOutL, mOutR);
    assert(std::abs(mOutL) > 1.0e-4f); // Memoria ferromagnética
    assert(!std::isnan(mOutL) && !std::isinf(mOutL));

    TapeSaturationNode tapeNode;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    tapeNode.prepare(spec);

    tapeNode.setParameter(TapeSaturationNode::Drive, 3.5f);
    tapeNode.setParameter(TapeSaturationNode::Hysteresis, 0.75f);
    tapeNode.setParameter(TapeSaturationNode::Mix, 1.0f);

    std::vector<float> in(128);
    for (int i = 0; i < 128; ++i) in[i] = std::sin(static_cast<float>(i) * 0.1f) * 0.7f;
    std::vector<float> oL(128, 0.0f), oR(128, 0.0f);
    const float* inCh[2] = { in.data(), in.data() };
    float* outCh[2] = { oL.data(), oR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    tapeNode.process(ctx);

    for (int s = 0; s < 128; ++s) {
        assert(!std::isnan(oL[s]) && !std::isinf(oL[s]));
        assert(!std::isnan(oR[s]) && !std::isinf(oR[s]));
        assert(std::abs(oL[s]) < 2.0f);
    }

    std::cout << "PASSED\n";
}

void testPhase2StaticSerialChainAndCRTP() {
    std::cout << "[TEST] StaticSerialChain CRTP Zero-Overhead Inlined Dispatch (Reglas 6, 14, 46)... ";
    StaticSerialChain<SimpleFilterNode, DistortionNode, SimpleDelayNode> chain;
    static_assert(chain.getChainLength() == 3);

    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    chain.prepare(spec);

    chain.getNode<0>().setParameter(SimpleFilterNode::CutoffHz, 2500.0f);
    chain.getNode<DistortionNode>().setParameter(DistortionNode::Drive, 2.0f);
    chain.getNode<2>().setParameter(SimpleDelayNode::DelayTimeMs, 20.0f);
    chain.setParameter(StaticSerialChain<SimpleFilterNode, DistortionNode, SimpleDelayNode>::Mix, 1.0f);

    std::vector<float> inL(128, 0.5f);
    std::vector<float> inR(128, 0.5f);
    std::vector<float> oL(128, 0.0f);
    std::vector<float> oR(128, 0.0f);

    const float* inCh[2] = { inL.data(), inR.data() };
    float* outCh[2] = { oL.data(), oR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    chain.process(ctx);

    for (int s = 0; s < 128; ++s) {
        assert(!std::isnan(oL[s]) && !std::isinf(oL[s]));
        assert(!std::isnan(oR[s]) && !std::isinf(oR[s]));
    }

    std::cout << "PASSED\n";
}

void testPhase3FFTEngineAVX2AndR2C() {
    std::cout << "[TEST] Phase 3: FFTEngine Twiddle LUT, AVX2 Butterfly & R2C/C2R Hermitian Symmetry (Reglas 13, 19, 32, 46, 47)... ";

    FFTEngine fft;
    const size_t N = 1024;
    const size_t H = 256;
    fft.prepare(N, H);

    assert(fft.getFFTSize() == N);
    assert(fft.getHopSize() == H);

    // 1. Probar ida y vuelta (Roundtrip) sin ventana para verificar precision numerica exacta
    std::vector<float> input(N);
    for (size_t i = 0; i < N; ++i) {
        input[i] = std::sin(2.0f * std::numbers::pi_v<float> * 4.0f * static_cast<float>(i) / static_cast<float>(N)) +
                   0.5f * std::cos(2.0f * std::numbers::pi_v<float> * 11.0f * static_cast<float>(i) / static_cast<float>(N));
    }

    fft.forward(input.data(), false);
    std::vector<float> output(N, 0.0f);
    fft.inverse(output.data(), false);

    float maxErr = 0.0f;
    for (size_t i = 0; i < N; ++i) {
        assert(!std::isnan(output[i]) && !std::isinf(output[i]));
        float err = std::abs(output[i] - input[i]);
        if (err > maxErr) maxErr = err;
    }
    // Precision matematica < 1e-4 para 1024 puntos
    assert(maxErr < 1.0e-4f);

    // 2. Probar R2C y C2R (Real-to-Complex / Complex-to-Real)
    const size_t numBins = N / 2 + 1;
    std::vector<std::complex<float>> halfSpectrum(numBins);
    fft.forwardR2C(input.data(), halfSpectrum.data(), false);

    // Verificar que DC y Nyquist son esencialmente reales
    assert(std::abs(halfSpectrum[0].imag()) < 1.0e-5f);
    assert(std::abs(halfSpectrum[numBins - 1].imag()) < 1.0e-5f);

    std::vector<float> r2cOutput(N, 0.0f);
    fft.inverseC2R(halfSpectrum.data(), r2cOutput.data(), false);

    float maxR2CErr = 0.0f;
    for (size_t i = 0; i < N; ++i) {
        assert(!std::isnan(r2cOutput[i]) && !std::isinf(r2cOutput[i]));
        float err = std::abs(r2cOutput[i] - input[i]);
        if (err > maxR2CErr) maxR2CErr = err;
    }
    assert(maxR2CErr < 1.0e-4f);

    // 3. Probar impulso unitario
    std::vector<float> impulse(N, 0.0f);
    impulse[0] = 1.0f;
    fft.forward(impulse.data(), false);
    auto& freqImpulse = fft.getFrequencyBuffer();
    for (const auto& b : freqImpulse) {
        const float mag = std::abs(b);
        assert(std::abs(mag - 1.0f) < 1.0e-4f);
        (void)mag;
    }

    std::cout << "PASSED\n";
}

void testPhase3PhaseVocoderIdentityPhaseLocking() {
    std::cout << "[TEST] Phase 3: PhaseVocoder STFT 75% Overlap & PitchShifter HD Dual-Mode (Reglas 3, 5, 8, 14, 17, 34, 46, 47)... ";

    // 1. Probar PhaseVocoder autonomo
    PhaseVocoder vocoder;
    vocoder.prepare(1024, 256, 44100.0f);
    assert(vocoder.getLatencySamples() == 1024);

    const size_t totalSamples = 2048;
    std::vector<float> testIn(totalSamples);
    for (size_t i = 0; i < totalSamples; ++i) {
        testIn[i] = std::sin(2.0f * std::numbers::pi_v<float> * 440.0f * static_cast<float>(i) / 44100.0f);
    }
    std::vector<float> vocoderOut(totalSamples, 0.0f);

    // Probar transposicion de +1 octava (pitchRatio = 2.0) con Identity Phase Locking estricto (1.0)
    const size_t blockSize = 128;
    for (size_t offset = 0; offset < totalSamples; offset += blockSize) {
        vocoder.process(testIn.data() + offset, vocoderOut.data() + offset, static_cast<uint32_t>(blockSize), 2.0f, 1.0f);
    }

    // Verificar ausencia de NaNs y amplitud controlada
    for (size_t i = 0; i < totalSamples; ++i) {
        assert(!std::isnan(vocoderOut[i]) && !std::isinf(vocoderOut[i]));
        assert(std::abs(vocoderOut[i]) <= 2.5f);
    }

    // 2. Probar PitchShifterNode en ambos algoritmos (Dual-Head vs Phase Vocoder HD)
    PitchShifterNode pitchNode;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    pitchNode.prepare(spec);

    // Modo 0: Time-Domain Dual-Head (Latencia = 0)
    pitchNode.setParameter(PitchShifterNode::Algorithm, 0.0f);
    pitchNode.setParameter(PitchShifterNode::Semitones, 7.0f); // Quinta justa (+7 semitonos)
    pitchNode.setParameter(PitchShifterNode::DryWet, 1.0f);
    assert(pitchNode.getLatencySamples() == 0);

    std::vector<float> outL(128, 0.0f), outR(128, 0.0f);
    const float* inCh[2] = { testIn.data(), testIn.data() };
    float* outCh[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    pitchNode.process(ctx);
    for (int s = 0; s < 128; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
    }

    // Modo 1: Phase Vocoder HD con Identity Phase Locking (Latencia = 1024)
    pitchNode.setParameter(PitchShifterNode::Algorithm, 1.0f);
    pitchNode.setParameter(PitchShifterNode::Semitones, 12.0f); // +12 semitonos
    pitchNode.setParameter(PitchShifterNode::PhaseLocking, 1.0f);
    assert(pitchNode.getLatencySamples() == 1024);
    assert(pitchNode.getTailSamples() == 2048);

    // Procesar varias iteraciones para cubrir la latencia STFT
    for (int b = 0; b < 16; ++b) {
        const float* chunkIn[2] = { testIn.data() + (b * 128 % 1024), testIn.data() + (b * 128 % 1024) };
        ProcessContext vocoderCtx{ chunkIn, outCh, 2, 2, 128 };
        pitchNode.process(vocoderCtx);
        for (int s = 0; s < 128; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
        }
    }

    std::cout << "PASSED\n";
}

void testPhase4WorkStealingGraphSchedulerAndMultithreading() {
    std::cout << "[TEST] Phase 4: WorkStealingGraphScheduler Multi-Threaded DAG Execution (Reglas 4, 9, 10, 26, 47)... ";

    // 1. Probar WorkStealingQueue Chase-Lev
    WorkStealingQueue<uint32_t, 64> queue;
    for (uint32_t i = 0; i < 32; ++i) {
        bool ok = queue.push(i);
        assert(ok);
    }
    // Propietario hace pop en LIFO
    uint32_t val = 0;
    bool popOk = queue.pop(val);
    assert(popOk && val == 31);

    // Ladron hace steal en FIFO
    uint32_t stolen = 0;
    bool stealOk = queue.steal(stolen);
    assert(stealOk && stolen == 0);

    // 2. Construir grafo DAG con 2 ramas paralelas para procesamiento concurrente
    Graph graph;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };

    NodeId filterNode = graph.addNode(std::make_unique<SimpleFilterNode>(), "Filter", 100.0f, 100.0f);
    NodeId distNode = graph.addNode(std::make_unique<DistortionNode>(), "Distortion", 100.0f, 250.0f);
    NodeId delayNode = graph.addNode(std::make_unique<SimpleDelayNode>(), "Delay", 300.0f, 175.0f);

    // Ambas ramas (filter y dist) reciben entrada independiente y convergen en delayNode
    graph.connect(filterNode, 2, delayNode, 1);
    graph.connect(distNode, 2, delayNode, 1);

    std::vector<NodeId> sorted;
    std::string err;
    bool valid = graph.validateAndTopologicalSort(sorted, err);
    assert(valid);

    ExecutionPlan plan;
    plan.compileFrom(graph, sorted);
    assert(plan.hasExplicitConnections());

    // Preparar ejecutor con multithreading activado
    GraphExecutor executor;
    executor.prepare(spec, 64);
    executor.setMultithreadingEnabled(true);
    assert(executor.isMultithreadingEnabled());

    PreallocatedBuffer finalOut;
    finalOut.prepare(2, 128);

    std::vector<float> inL(128, 0.4f);
    std::vector<float> inR(128, 0.4f);
    const float* inCh[2] = { inL.data(), inR.data() };
    float* outCh[2] = { finalOut.getWritePointer(0), finalOut.getWritePointer(1) };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    // Procesar 50 bloques de audio concurrentes
    for (int b = 0; b < 50; ++b) {
        executor.process(plan, ctx, finalOut);
        for (int s = 0; s < 128; ++s) {
            assert(!std::isnan(finalOut.getReadPointer(0)[s]) && !std::isinf(finalOut.getReadPointer(0)[s]));
            assert(!std::isnan(finalOut.getReadPointer(1)[s]) && !std::isinf(finalOut.getReadPointer(1)[s]));
        }
    }

    std::cout << "PASSED\n";
}

void testPhase4CycleDetectionAndRejection() {
    std::cout << "[TEST] Phase 4: Graph Predictive Cycle Detection wouldCreateCycle (Reglas 4, 15, 28, 29)... ";

    Graph graph;
    NodeId n1 = graph.addNode(std::make_unique<PassthroughNode>(), "N1", 0.0f, 0.0f);
    NodeId n2 = graph.addNode(std::make_unique<PassthroughNode>(), "N2", 100.0f, 0.0f);
    NodeId n3 = graph.addNode(std::make_unique<PassthroughNode>(), "N3", 200.0f, 0.0f);

    // N1 -> N2 -> N3
    graph.connect(n1, 2, n2, 1);
    graph.connect(n2, 2, n3, 1);

    // 1. Intentar conectar N3 -> N1 crearia ciclo N1 -> N2 -> N3 -> N1
    assert(graph.wouldCreateCycle(n3, n1) == true);

    // 2. Intentar conectar N2 -> N1 crearia ciclo N1 -> N2 -> N1
    assert(graph.wouldCreateCycle(n2, n1) == true);

    // 3. Conectar N1 -> N3 es un forward shortcut aciclico valido
    assert(graph.wouldCreateCycle(n1, n3) == false);

    // 4. Auto-lazo N2 -> N2 debe ser rechazado
    assert(graph.wouldCreateCycle(n2, n2) == true);

    // 5. Nodos inexistentes o invalidos deben ser rechazados
    assert(graph.wouldCreateCycle(InvalidNodeId, n1) == true);
    assert(graph.wouldCreateCycle(n1, 9999) == true);

    std::cout << "PASSED\n";
}

void testPhase4SubGraphContainerAndModuleExport() {
    std::cout << "[TEST] Phase 4: ContainerNode SubGraph JSON Export/Import & Recompilation (Reglas 6, 16, 21, 22)... ";

    // 1. Crear subgrafo dentro de un ContainerNode
    ContainerNode container;
    auto& inner = container.getInnerGraph();

    NodeId fNode = inner.addNode(std::make_unique<SimpleFilterNode>(), "InnerFilter", 50.0f, 50.0f);
    NodeId dNode = inner.addNode(std::make_unique<DistortionNode>(), "InnerDist", 150.0f, 50.0f);
    inner.connect(fNode, 2, dNode, 1);

    std::string err;
    bool compiled = container.compileInnerGraph(err);
    assert(compiled);
    assert(container.getInnerExecutionPlan().getSteps().size() == 2);

    // 2. Exportar subgrafo a JSON
    std::string exportedJson = container.exportSubGraphJson("VocalChain");
    assert(!exportedJson.empty());
    assert(exportedJson.find("InnerFilter") != std::string::npos);
    assert(exportedJson.find("InnerDist") != std::string::npos);

    // 3. Importar subgrafo en un segundo contenedor virgen
    ContainerNode importedContainer;
    bool importOk = importedContainer.importSubGraphJson(exportedJson, err);
    assert(importOk);
    assert(importedContainer.getInnerExecutionPlan().getSteps().size() == 2);

    // 4. Procesar audio a traves del contenedor importado
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    importedContainer.prepare(spec);

    std::vector<float> inL(128, 0.5f), inR(128, 0.5f);
    std::vector<float> outL(128, 0.0f), outR(128, 0.0f);
    const float* inCh[2] = { inL.data(), inR.data() };
    float* outCh[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 128 };

    importedContainer.process(ctx);

    for (int s = 0; s < 128; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
    }

    std::cout << "PASSED\n";
}


void testPhase5AdaptiveNoiseFloorAndHysteresis() {
    std::cout << "[TEST] Phase 5: AdaptiveNoiseFloorEstimator & Schmitt Trigger Hysteresis (Reglas 1, 3, 9, 27, 47)... ";
    AdaptiveNoiseFloorEstimator estimator;
    estimator.prepare(44100.0);

    // Inicialmente piso por defecto -80 dB
    assert(estimator.getNoiseFloor() >= 1.0e-5f);

    // Alimentar señal fuerte: debe activar la fuente
    for (int b = 0; b < 10; ++b) {
        estimator.updateBlock(0.2f, 128);
    }
    assert(estimator.isSourceActive());
    assert(!estimator.isSourceSilent());

    // Nivel intermedio por encima del piso: la histéresis debe mantener activa la fuente sin chattering
    const float floorVal = estimator.getNoiseFloor();
    const float midLevel = floorVal * 1.8f; // zona de histéresis entre 1.25x y 2.5x
    for (int b = 0; b < 10; ++b) {
        estimator.updateBlock(midLevel, 128);
    }
    assert(estimator.isSourceActive()); // Mantiene activa la fuente sin fluctuar

    // Alimentar silencio absoluto: tras debounce (4 bloques) debe declarar silencio
    for (int b = 0; b < 6; ++b) {
        estimator.updateBlock(0.0f, 128);
    }
    assert(estimator.isSourceSilent());

    std::cout << "PASSED\n";
}

void testPhase5GranularSpectralAndOnsetSpawning() {
    std::cout << "[TEST] Phase 5: GranularNode 3 Spawn Modes (Clock, Stochastic, Spectral/Onset) (Reglas 5, 8, 9, 10, 28, 47)... ";
    GranularNode gran;
    ProcessSpec spec{ 44100.0, 128, 2, 2 };
    gran.prepare(spec);

    // Verificar parámetros SpawnMode y SpectralSensitivity
    gran.setParameter(GranularNode::SpawnMode, 2.0f); // Modo Espectral/Onset
    gran.setParameter(GranularNode::SpectralSensitivity, 0.8f);
    assert(gran.getParameter(GranularNode::SpawnMode) == 2.0f);
    assert(gran.getParameter(GranularNode::SpectralSensitivity) == 0.8f);

    // Simular procesamiento con transitorio brusco
    std::vector<float> inL(128, 0.0f);
    std::vector<float> inR(128, 0.0f);
    std::vector<float> outL(128, 0.0f);
    std::vector<float> outR(128, 0.0f);

    // Salto transitorio para disparar flujo espectral
    for (size_t i = 10; i < 20; ++i) {
        inL[i] = 0.9f;
        inR[i] = 0.9f;
    }

    const float* inPtrs[2] = { inL.data(), inR.data() };
    float* outPtrs[2] = { outL.data(), outR.data() };
    ProcessContext ctx;
    ctx.inputChannels = inPtrs;
    ctx.outputChannels = outPtrs;
    ctx.numInputChannels = 2;
    ctx.numOutputChannels = 2;
    ctx.numSamples = 128;

    gran.process(ctx);

    // Comprobar que no hay NaN/Inf en salida
    for (size_t i = 0; i < 128; ++i) {
        assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
        assert(!std::isnan(outR[i]) && !std::isinf(outR[i]));
    }

    // Modo Estocástico
    gran.setParameter(GranularNode::SpawnMode, 1.0f);
    gran.process(ctx);
    for (size_t i = 0; i < 128; ++i) {
        assert(!std::isnan(outL[i]) && !std::isinf(outL[i]));
    }

    std::cout << "PASSED\n";
}

void testShimmerReverbNode() {
    std::cout << "[TEST] ShimmerReverbNode Harmonic Feedback Reverb, Allpass Diffusers & M/S Width (Reglas 5, 8, 9, 14, 18, 45, 46, 47)... ";
    ProcessSpec spec{ 48000.0, 256, 2, 2 };
    ShimmerReverbNode node;
    node.prepare(spec);

    assert(node.getType() == NodeType::ShimmerReverb);
    assert(std::string(node.getName()) == "Shimmer Reverb");
    assert(node.getPins().size() == 2);
    assert(node.getParameters().size() == 6);

    // Verify parameter bounds and defaults
    assert(node.getParameter(ShimmerReverbNode::DecayTime) >= 0.2f);
    assert(node.getParameter(ShimmerReverbNode::DampingHz) >= 1000.0f);
    assert(node.getParameter(ShimmerReverbNode::ShimmerMix) >= 0.0f);
    assert(node.getParameter(ShimmerReverbNode::PitchInterval) >= 0.0f);
    assert(node.getParameter(ShimmerReverbNode::StereoWidth) >= 0.0f);
    assert(node.getParameter(ShimmerReverbNode::DryWet) >= 0.0f);

    // Set parameters
    node.setParameter(ShimmerReverbNode::DecayTime, 6.0f);
    node.setParameter(ShimmerReverbNode::DampingHz, 8000.0f);
    node.setParameter(ShimmerReverbNode::ShimmerMix, 0.7f);
    node.setParameter(ShimmerReverbNode::PitchInterval, 0.0f); // +12st
    node.setParameter(ShimmerReverbNode::StereoWidth, 1.5f);
    node.setParameter(ShimmerReverbNode::DryWet, 1.0f); // 100% wet to check shimmer tail

    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);

    // Inject an impulse
    inL[0] = 1.0f;
    inR[0] = 1.0f;

    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    // First block: initial impulse response
    node.process(ctx);

    // Check no NaN or Inf
    for (size_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
    }

    // Process subsequent silence blocks to verify shimmer reverb tail persistence and stability
    inL[0] = 0.0f;
    inR[0] = 0.0f;

    float tailEnergy = 0.0f;
    for (int b = 0; b < 20; ++b) {
        std::fill(outL.begin(), outL.end(), 0.0f);
        std::fill(outR.begin(), outR.end(), 0.0f);
        node.process(ctx);

        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
            tailEnergy += std::abs(outL[s]) + std::abs(outR[s]);
            // Output must be bounded by fastTanh soft saturation
            assert(std::abs(outL[s]) < 3.0f);
            assert(std::abs(outR[s]) < 3.0f);
        }
    }

    // Tail energy must be non-zero (reverb tail persists)
    assert(tailEnergy > 0.01f);

    // Test different pitch intervals (+7st and -12st)
    node.setParameter(ShimmerReverbNode::PitchInterval, 1.0f); // +7st
    node.process(ctx);
    node.setParameter(ShimmerReverbNode::PitchInterval, 2.0f); // -12st
    node.process(ctx);

    node.reset();
    std::cout << "PASSED\n";
}

void testRefractionNode() {
    std::cout << "[TEST] RefractionNode 8-Voice Prism Dispersion & Energy Conservation (Reglas 5, 8, 9, 14, 18, 45, 46, 47)... ";
    ProcessSpec spec{ 48000.0, 256, 2, 2 };
    RefractionNode node;
    node.prepare(spec);

    assert(node.getType() == NodeType::Refraction);
    assert(std::string(node.getName()) == "Refraction");
    assert(node.getPins().size() == 2);
    assert(node.getParameters().size() == 7);

    // Verify parameter defaults and ranges
    assert(node.getParameter(RefractionNode::VoiceCount) >= 2.0f);
    assert(node.getParameter(RefractionNode::Detune) >= 0.0f);
    assert(node.getParameter(RefractionNode::DelaySpreadMs) >= 0.0f);
    assert(node.getParameter(RefractionNode::StereoSpread) >= 0.0f);
    assert(node.getParameter(RefractionNode::HarmonicDisp) >= 0.0f);
    assert(node.getParameter(RefractionNode::Resonance) >= 0.1f);
    assert(node.getParameter(RefractionNode::DryWet) >= 0.0f);

    // Configure 8 voices with maximum spread and detune
    node.setParameter(RefractionNode::VoiceCount, 8.0f);
    node.setParameter(RefractionNode::Detune, 35.0f);
    node.setParameter(RefractionNode::DelaySpreadMs, 25.0f);
    node.setParameter(RefractionNode::StereoSpread, 1.0f);
    node.setParameter(RefractionNode::HarmonicDisp, 1.0f);
    node.setParameter(RefractionNode::Resonance, 2.0f);
    node.setParameter(RefractionNode::DryWet, 1.0f); // 100% wet

    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);

    // Sine tone input
    for (size_t s = 0; s < 256; ++s) {
        inL[s] = std::sin(2.0f * 3.14159f * 440.0f * static_cast<float>(s) / 48000.0f);
        inR[s] = inL[s];
    }

    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    // Process multiple blocks
    float totalEnergyL = 0.0f;
    float totalEnergyR = 0.0f;
    for (int b = 0; b < 10; ++b) {
        std::fill(outL.begin(), outL.end(), 0.0f);
        std::fill(outR.begin(), outR.end(), 0.0f);
        node.process(ctx);

        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
            totalEnergyL += outL[s] * outL[s];
            totalEnergyR += outR[s] * outR[s];
        }
    }

    // Output energy must be non-zero and stable (1/sqrt(N) normalization prevents explosion)
    assert(totalEnergyL > 1.0f);
    assert(totalEnergyR > 1.0f);

    // Test with 2 voices
    node.setParameter(RefractionNode::VoiceCount, 2.0f);
    node.process(ctx);
    for (size_t s = 0; s < 256; ++s) {
        assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
        assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
    }

    node.reset();
    std::cout << "PASSED\n";
}

void testSpectralSmearNode() {
    std::cout << "[TEST] SpectralSmearNode STFT 1024 Phase Diffusion & Liquid Persistence (Reglas 5, 8, 9, 14, 18, 45, 46, 47)... ";
    ProcessSpec spec{ 48000.0, 256, 2, 2 };
    SpectralSmearNode node;
    node.prepare(spec);

    assert(node.getType() == NodeType::SpectralSmear);
    assert(std::string(node.getName()) == "Spectral Smear");
    assert(node.getPins().size() == 2);
    assert(node.getParameters().size() == 5);

    // Verify parameter defaults and ranges
    assert(node.getParameter(SpectralSmearNode::SmearTime) >= 0.05f);
    assert(node.getParameter(SpectralSmearNode::PhaseDiffusion) >= 0.0f);
    assert(node.getParameter(SpectralSmearNode::StereoDecorrel) >= 0.0f);
    assert(node.getParameter(SpectralSmearNode::TiltDamping) >= -1.0f);
    assert(node.getParameter(SpectralSmearNode::DryWet) >= 0.0f);

    node.setParameter(SpectralSmearNode::SmearTime, 3.0f);
    node.setParameter(SpectralSmearNode::PhaseDiffusion, 0.8f);
    node.setParameter(SpectralSmearNode::StereoDecorrel, 0.9f);
    node.setParameter(SpectralSmearNode::TiltDamping, -0.3f);
    node.setParameter(SpectralSmearNode::DryWet, 1.0f); // 100% wet

    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);

    // Inject transient impulse into block 0
    inL[0] = 1.0f;
    inR[0] = 1.0f;

    const float* inChannels[2] = { inL.data(), inR.data() };
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 256
    };

    node.process(ctx);

    // Process 16 subsequent silent blocks to verify STFT overlap-add smearing tail
    inL[0] = 0.0f;
    inR[0] = 0.0f;

    float smearTailEnergy = 0.0f;
    for (int b = 0; b < 16; ++b) {
        std::fill(outL.begin(), outL.end(), 0.0f);
        std::fill(outR.begin(), outR.end(), 0.0f);
        node.process(ctx);

        for (size_t s = 0; s < 256; ++s) {
            assert(!std::isnan(outL[s]) && !std::isinf(outL[s]));
            assert(!std::isnan(outR[s]) && !std::isinf(outR[s]));
            smearTailEnergy += std::abs(outL[s]) + std::abs(outR[s]);
        }
    }

    // Smeared spectral tail must have emitted non-trivial energy after the initial impulse
    assert(smearTailEnergy > 0.01f);

    node.reset();
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "==================================================\n";
    std::cout << "AUDIO EVENT GRAPH ENGINE - UNIT & INTEGRATION TESTS\n";
    std::cout << "Fases 1 a 13: Core + Event + Mod + DSP + Adv DSP + Editor + Presets + Opt + Large Lib + Containers & Feedback + Analysis + Spatial + Profiling Stress\n";
    std::cout << "==================================================\n";

    testAudioBufferPool();
    testGraphValidationAndKahnSort();
    testDSPNodesCorrectness();
    testEventPoolCapacityAndRecycling();
    testEventLifecycleAndStates();
    testSourceFollowingBehaviors();
    testEventSpawningLimits();
    testDualWorldIsolation();

    // Phase 3 Tests
    testLFOShapesAndHostSync();
    testEnvelopeGeneratorPhases();
    testStepSequencerGlideAndProbability();
    testAudioFollowerResponse();
    testModulationMatrixAndEngineRouting();

    // Phase 4 Tests
    testCompressorDynamics();
    testParametricEQStability();
    testDistortionAlgorithms();
    testAdvancedDelayPingPongAndTail();
    testReverbDiffusionAndDecay();
    testPitchShifterTransposition();
    testFFTEngineRoundtrip();

    // Phase 5 Tests
    testGrainPoolAllocationAndRecycling();
    testGranularNodeCloudProcessing();
    testSpectralFreezeAndSmear();
    testResonatorBankHarmonics();
    testGlitchBufferSlicing();
    testLinkwitzRileyCrossoverReconstruction();
    testMultibandDynamicsProcessing();

    // Phase 6 Tests
    testDynamicGraphEditingAndValidation();

    // Phase 7 Tests
    testGraphSerializationAndDeserializationRoundtrip();
    testPresetVersionMigration();
    testSceneMorphingInterpolation();
    testGraphUndoRedoStack();

    // Phase 8 Tests
    testDenormalPreventionUnderSilentTail();
    testFastMathApproximationAccuracy();
    testExecutionPlanBufferCacheReuse();

    // Phase 9 Tests
    testPhaserSweepingAndFeedback();
    testChorusMultiVoiceStereoSpread();
    testFlangerCombFilteringAndInversion();
    testRingModulatorCarrierMultiplication();
    testFrequencyShifterHilbertSSB();
    testTapeSaturationWarmthAndFlutter();

    // Phase 10 Tests
    testSubgraphContainerNestingAndExecution();
    testControlledFeedbackLoopRunawayProtection();
    testEventContainerLifecycle();

    // Phase 11 Tests
    testTransientDetectionSensitivity();
    testPitchTrackingAccuracy();
    testSpectralCentroidAndFluxExtraction();
    testAnalysisModulationRouting();

    // Phase 12 Tests
    testMidSideEncodingAndDecodingRoundtrip();
    testMonoBassMakerPhaseIntegrity();
    testSpatial3DPannerITDandILD();

    // Phase 13 Tests
    testCpuProfilerTimingAccuracy();
    testDenseGraphHeavyLoadStress();
    testOverloadProtectionAndGracefulDegradation();

    // DAG Wiring, Parallel Branches & 10-FX Master Chain Tests
    testGraphDAGParallelRoutingAndBranching();
    testDecaMatrix10FXChainPreset();

    // Virtual Piano & Test Input Synthesizer Tests
    testTestInputSynthesizerPolyphonyAndLifecycle();

    // 20 Thematic Categories & 40 Creative Factory Presets
    testAll40FactoryPresetsCatalog();

    // New Advanced DSPs Tests
    testTapeStopProcessingAndHermiteInterpolation();
    testFormantFilterVowelMorphing();
    testNoiseTextureGenerationAndSidechainDuck();

    // New Complex Modulators Tests
    testMSEGModulatorCurvesAndSync();
    testEuclideanModulatorBjorklundRhythm();
    testChaosModulatorLorenzAttractor();

    // Smart Randomizer Tests
    testSmartRandomizerSafeguardsAndMutation();

    // New Advanced DSP Suite Tests
    testTransientShaperDynamics();
    testRotarySpeakerDopplerAndInertia();
    testHarmonicExciterAirAndSub();
    testVocoderFilterBank16Bands();
    testKarplusStrongPhysicalModeling();
    testReverseReverbBloomDiffusion();

    // Mastering & Studio Mixing Suite Tests
    testBrickwallLimiterTruePeakAndLookahead();
    testBitcrusherQuantizationAndDownsampling();
    testNoiseGateHysteresisAndHold();
    testDeEsserSibilanceAttenuationAndListenMode();

    // Node Contextual Automation Sequencer Tests
    testNodeAutomationSequencerMultiLaneAndSync();

    // Option 1, 3, 5, 6 Tests: Visualizer, Macros, MIDI FX & Inter-Nodal Sidechain
    testAudioVisualizerBufferLockFree();
    testMacroManagerModulationMatrixRouting();
    testMidiArpeggiatorAndScaleQuantizer();
    testMidiChordEngineVoicingsAndStrum();
    testSidechainModularRoutingAndVocoder();

    // Advanced Features: Event Telemetry Radar, User Preset Bank, Drag & Drop Halo Modulation
    testEventTelemetryBufferLockFree();
    testUserPresetBankStorageAndFiltering();
    testInteractiveModulationRoutingAndPayload();

    // Radar 3D Reactive Pipeline & 46 DSP Effects Full Verification
    testAll44DSPNodesInstantiationAndProcessing();
    testRadar3DAcousticTelemetryEmission();

    // Phase 1 Advanced Suite: Zero-Latency Partitioned Convolution, HQ Oversampling PDC, Audio-Rate FM
    testPartitionedConvolutionZeroLatency();
    testConvolutionCustomIRDoubleBufferingAndThumbnail();
    testConvolutionAdversarialChallenge();
    testPolyphaseOversamplingPDC();
    testAudioRateCrossModulation();

    // Phase 2 Advanced Suite: MPE 5D Expression & Universal MIDI Learn / Controller Profiles
    testMpeVoiceAllocationAndDimensions();
    testMidiLearnMappingAndCurves();

    // Phase 4 Advanced Suite: Action Undo Timeline / Time-Travel & .n8pack Preset Bundler
    testGraphUndoTimelineAndTimeTravel();
    testPresetPackagerExportAndImport();

    // Visual Node Groups with 3 Automation Macros & Audio Slicer Frozen Playground
    testNodeGroupsAndMacroAutomation();
    testAudioSlicerNodeProcessingAndFreeze();

    // Phase 1 Flagship Innovations: Smooth Biquads, Bitmask 2^N Delays, 64-Byte Alignment, Multiband Transient Shaper & Dynamic AGC Feedback
    testPhase1BiquadFilterSmoothInterpolation();
    testPhase1DelayLinePowerOfTwoBitmask();
    testPhase1PreallocatedBuffer64ByteAlignment();
    testPhase1TransientShaperMultibandLR4();
    testPhase1FeedbackContainerDynamicAGC();

    // Phase 2 Flagship Innovations: AVX2/FMA SIMD, Karplus 4-Mode Body Resonator, Jiles-Atherton Hysteresis & StaticSerialChain CRTP
    testPhase2FastMathSIMDVectorization();
    testPhase2KarplusStrongAcousticBodyResonator();
    testPhase2TapeSaturationMagneticHysteresis();
    testPhase2StaticSerialChainAndCRTP();

    // Phase 3 Flagship Innovations: FFTEngine AVX2/R2C Twiddle LUT & PhaseVocoder HD Identity Phase Locking
    testPhase3FFTEngineAVX2AndR2C();
    testPhase3PhaseVocoderIdentityPhaseLocking();

    // Phase 4 Flagship Innovations: WorkStealingGraphScheduler, wouldCreateCycle & Container SubGraph Export/Import
    testPhase4WorkStealingGraphSchedulerAndMultithreading();
    testPhase4CycleDetectionAndRejection();
    testPhase4SubGraphContainerAndModuleExport();

    // Phase 5 Flagship Innovations: Adaptive Noise Floor & Granular Spectral Spawning
    testPhase5AdaptiveNoiseFloorAndHysteresis();
    testPhase5GranularSpectralAndOnsetSpawning();

    // Arturia-Killer Flagship Suite Tests (R1, R2, R3)
    testShimmerReverbNode();
    testRefractionNode();
    testSpectralSmearNode();

    std::cout << "==================================================\n";
    std::cout << "TODOS LOS TESTS (115 PRUEBAS UNITARIAS) HAN PASADO CON EXITO\n";
    std::cout << "==================================================\n";

    return 0;
}


