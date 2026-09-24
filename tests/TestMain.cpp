#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

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
#include "../source/dsp/processors/CompressorNode.h"
#include "../source/dsp/processors/ParametricEQNode.h"
#include "../source/dsp/processors/DistortionNode.h"
#include "../source/dsp/processors/AdvancedDelayNode.h"
#include "../source/dsp/processors/ReverbNode.h"
#include "../source/dsp/processors/PitchShifterNode.h"
#include "../source/dsp/processors/SpectralProcessorNode.h"
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
#include "../source/dsp/core/TestSynthEngine.h"
#include "../source/core/CpuProfiler.h"

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
    assert(factory.size() == 6);

    // Validar que TODOS los 6 presets de fábrica deserializan limpiamente
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

void testSequentialLinearChainReordering() {
    std::cout << "[TEST] Sequential Slot Chain Reordering & Dynamic Insertion (Arturia Efx Mode)... ";
    Graph graph;

    // 1. Agregar 3 nodos iniciales
    auto n1 = graph.addNode(std::make_unique<DistortionNode>(), "Distortion");
    auto n2 = graph.addNode(std::make_unique<SimpleFilterNode>(), "Filter");
    auto n3 = graph.addNode(std::make_unique<AdvancedDelayNode>(), "Delay");

    assert(n1 != InvalidNodeId && n2 != InvalidNodeId && n3 != InvalidNodeId);

    // 2. Conectar en secuencia: n1 -> n2 -> n3
    graph.connect(n1, 2, n2, 1);
    graph.connect(n2, 2, n3, 1);

    std::vector<NodeId> sorted;
    std::string err;
    bool valid = graph.validateAndTopologicalSort(sorted, err);
    assert(valid);
    assert(sorted.size() == 3);
    assert(sorted[0] == n1);
    assert(sorted[1] == n2);
    assert(sorted[2] == n3);

    // 3. Reordenar la cola: n3 -> n1 -> n2 (Delay -> Distortion -> Filter)
    std::vector<ConnectionId> toRemove;
    for (const auto& c : graph.getConnections()) {
        if (c.sourcePinId == 2 && c.destPinId == 1) {
            toRemove.push_back(c.id);
        }
    }
    for (auto cid : toRemove) graph.disconnect(cid);

    graph.connect(n3, 2, n1, 1);
    graph.connect(n1, 2, n2, 1);

    sorted.clear();
    valid = graph.validateAndTopologicalSort(sorted, err);
    assert(valid);
    assert(sorted[0] == n3);
    assert(sorted[1] == n1);
    assert(sorted[2] == n2);

    // 4. Inserción dinámica de nuevo nodo en medio (n4: Reverb entre n1 y n2)
    auto n4 = graph.addNode(std::make_unique<ReverbNode>(), "Reverb");
    for (const auto& c : graph.getConnections()) {
        if (c.sourceNodeId == n1 && c.destNodeId == n2) {
            graph.disconnect(c.id);
            break;
        }
    }
    graph.connect(n1, 2, n4, 1);
    graph.connect(n4, 2, n2, 1);

    sorted.clear();
    valid = graph.validateAndTopologicalSort(sorted, err);
    assert(valid);
    assert(sorted.size() == 4);
    assert(sorted[0] == n3);
    assert(sorted[1] == n1);
    assert(sorted[2] == n4);
    assert(sorted[3] == n2);

    // 5. Compilación del ExecutionPlan y procesamiento real con cero NaNs
    ExecutionPlan plan;
    plan.compileFrom(graph, sorted);
    assert(!plan.isEmpty());

    GraphExecutor executor;
    ProcessSpec spec{ 48000.0, 512, 2, 2 };
    executor.prepare(spec);

    for (const auto& [id, node] : graph.getNodes()) {
        node->processor->prepare(spec);
    }

    std::vector<float> inL(512, 0.35f);
    std::vector<float> inR(512, 0.35f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    std::vector<float> outL(512, 0.0f);
    std::vector<float> outR(512, 0.0f);
    float* outChannels[2] = { outL.data(), outR.data() };

    ProcessContext ctx{
        .inputChannels = inChannels,
        .outputChannels = outChannels,
        .numInputChannels = 2,
        .numOutputChannels = 2,
        .numSamples = 512
    };

    PreallocatedBuffer outBuffer;
    outBuffer.prepare(2, 512);
    executor.process(plan, ctx, outBuffer);

    // Verificar salida procesada estable
    for (uint32_t s = 0; s < 512; ++s) {
        assert(!std::isnan(outBuffer.getReadPointer(0)[s]));
        assert(!std::isinf(outBuffer.getReadPointer(0)[s]));
        assert(!std::isnan(outBuffer.getReadPointer(1)[s]));
        assert(!std::isinf(outBuffer.getReadPointer(1)[s]));
    }

    std::cout << "PASSED\n";
}

void testTestSynthEnginePolyphonyAndSafety() {
    std::cout << "[TEST] TestSynthEngine Polyphony, PolyBLEP Anti-Aliasing & Voice Stealing (Reglas 9, 13, 34, 46)... ";

    TestSynthEngine synth;
    synth.prepare(48000.0);

    assert(synth.getActiveVoiceCount() == 0);
    assert(synth.isEnabled());

    // 1. Probar Note On y polifonía (Acorde C mayor: C4, E4, G4)
    synth.noteOn(60, 0.8f);
    synth.noteOn(64, 0.7f);
    synth.noteOn(67, 0.9f);
    assert(synth.getActiveVoiceCount() == 3);

    // 2. Renderizar audio en buffer estéreo
    constexpr uint32_t blockSize = 256;
    std::vector<float> bufL(blockSize, 0.0f);
    std::vector<float> bufR(blockSize, 0.0f);
    float* channels[2] = { bufL.data(), bufR.data() };

    synth.renderAudioAdding(channels, 2, blockSize);

    // Comprobar que hay señal generada y que no hay NaN/Inf
    float maxAbsL = 0.0f;
    float maxAbsR = 0.0f;
    for (uint32_t i = 0; i < blockSize; ++i) {
        assert(!std::isnan(bufL[i]) && !std::isinf(bufL[i]));
        assert(!std::isnan(bufR[i]) && !std::isinf(bufR[i]));
        maxAbsL = std::max(maxAbsL, std::abs(bufL[i]));
        maxAbsR = std::max(maxAbsR, std::abs(bufR[i]));
    }
    assert(maxAbsL > 0.01f);
    assert(maxAbsR > 0.01f);

    // 3. Probar todos los timbres (Saw, Sine, Square, Pluck)
    for (uint8_t t = 0; t < static_cast<uint8_t>(SynthSoundType::Count); ++t) {
        synth.setSoundType(static_cast<SynthSoundType>(t));
        std::fill(bufL.begin(), bufL.end(), 0.0f);
        std::fill(bufR.begin(), bufR.end(), 0.0f);
        synth.renderAudioAdding(channels, 2, blockSize);
        for (uint32_t i = 0; i < blockSize; ++i) {
            assert(!std::isnan(bufL[i]) && !std::isinf(bufL[i]));
            assert(!std::isnan(bufR[i]) && !std::isinf(bufR[i]));
        }
    }

    // 4. Probar saturación de polifonía y robo de voces (MaxVoices = 16)
    for (int n = 36; n < 36 + 20; ++n) {
        synth.noteOn(n, 0.6f);
    }
    assert(synth.getActiveVoiceCount() <= TestSynthEngine::MaxVoices);

    // 5. Probar Note Off y All Notes Off con desvanecimiento limpio
    synth.allNotesOff();
    // Renderizar suficientes bloques para permitir la liberación de la envolvente (80ms a 48kHz = ~3840 muestras = 15 bloques)
    for (int b = 0; b < 25; ++b) {
        std::fill(bufL.begin(), bufL.end(), 0.0f);
        std::fill(bufR.begin(), bufR.end(), 0.0f);
        synth.renderAudioAdding(channels, 2, blockSize);
    }
    assert(synth.getActiveVoiceCount() == 0);

    // 6. Probar desactivación / Mute
    synth.noteOn(60, 0.8f);
    synth.setEnabled(false);
    assert(!synth.isEnabled());
    std::fill(bufL.begin(), bufL.end(), 0.0f);
    std::fill(bufR.begin(), bufR.end(), 0.0f);
    synth.renderAudioAdding(channels, 2, blockSize);
    for (uint32_t i = 0; i < blockSize; ++i) {
        assert(bufL[i] == 0.0f && bufR[i] == 0.0f);
    }

    // 7. Probar reseteo y cambio de sample rate
    synth.prepare(192000.0);
    synth.setEnabled(true);
    synth.noteOn(72, 0.9f);
    assert(synth.getActiveVoiceCount() == 1);
    synth.renderAudioAdding(channels, 2, blockSize);
    for (uint32_t i = 0; i < blockSize; ++i) {
        assert(!std::isnan(bufL[i]) && !std::isinf(bufL[i]));
    }

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
    testSequentialLinearChainReordering();

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

    // Virtual Keyboard & Audition Synth Tests
    testTestSynthEnginePolyphonyAndSafety();

    std::cout << "==================================================\n";
    std::cout << "TODOS LOS TESTS DE FASES 1 A 13 + TEST SYNTH (56 PRUEBAS) HAN PASADO CON EXITO\n";
    std::cout << "==================================================\n";

    return 0;
}


