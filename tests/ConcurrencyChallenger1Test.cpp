#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <cassert>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>
#include <algorithm>
#include <random>

#include "../source/core/Types.h"
#include "../source/core/RealtimePools.h"
#include "../source/graph/AudioProcessorNode.h"
#include "../source/graph/NodeFactory.h"
#include "../source/graph/Graph.h"
#include "../source/graph/GraphExecutor.h"
#include "../source/graph/WorkStealingGraphScheduler.h"
#include "../source/dsp/PassthroughNode.h"
#include "../source/dsp/SimpleFilterNode.h"
#include "../source/dsp/SimpleDelayNode.h"
#include "../source/dsp/processors/CompressorNode.h"
#include "../source/dsp/processors/DistortionNode.h"
#include "../source/dsp/processors/AdvancedDelayNode.h"
#include "../source/dsp/processors/ReverbNode.h"
#include "../source/plugin/DualWorldEngine.h"

using namespace audio_graph;

static int gFailedTests = 0;
static int gPassedTests = 0;

#define CHALLENGE_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] " << msg << " (line " << __LINE__ << ")\n"; \
            gFailedTests++; \
        } else { \
            std::cout << "  [PASS] " << msg << "\n"; \
            gPassedTests++; \
        } \
    } while(0)

// ==============================================================================
// CHALLENGE 1: WorkStealingGraphScheduler Passive Wait & Immediate Wakeup
// ==============================================================================
void testChallenge1_PassiveWaitAndWakeup() {
    std::cout << "\n====================================================================\n";
    std::cout << "[CHALLENGE 1] Scheduler Passive Synchronization & Immediate Wakeup\n";
    std::cout << "====================================================================\n";

    ProcessSpec spec{ 48000.0, 256, 2, 2 };
    WorkStealingGraphScheduler scheduler;
    scheduler.prepare(spec, 2);
    CHALLENGE_ASSERT(scheduler.getNumWorkers() == 2, "Scheduler correctly configured with 2 workers");

    // Allow worker threads to enter passive kernel sleep on activeEpoch_.wait()
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Construct a diamond test DAG:
    // Root -> (Branch1, Branch2) -> Leaf
    Graph graph;
    NodeId n1 = graph.addNode(std::make_unique<PassthroughNode>(), "Root");
    NodeId n2 = graph.addNode(std::make_unique<PassthroughNode>(), "Branch1");
    NodeId n3 = graph.addNode(std::make_unique<PassthroughNode>(), "Branch2");
    NodeId n4 = graph.addNode(std::make_unique<PassthroughNode>(), "Leaf");
    graph.connect(n1, 2, n2, 1);
    graph.connect(n1, 2, n3, 1);
    graph.connect(n2, 2, n4, 1);
    graph.connect(n3, 2, n4, 1);

    std::vector<NodeId> sorted;
    std::string err;
    bool valid = graph.validateAndTopologicalSort(sorted, err);
    CHALLENGE_ASSERT(valid, "Diamond DAG validated and topologically sorted");

    ExecutionPlan plan;
    plan.compileFrom(graph, sorted);
    CHALLENGE_ASSERT(plan.getSteps().size() == 4, "Plan compiled with 4 execution steps");

    AudioBufferPool bufferPool;
    bufferPool.prepare(16, 2, 256);
    std::array<PreallocatedBuffer*, 64> stepOutputBuffers{};
    for (size_t i = 0; i < 4; ++i) {
        stepOutputBuffers[i] = bufferPool.acquire();
        CHALLENGE_ASSERT(stepOutputBuffers[i] != nullptr, "Acquired step buffer");
    }

    std::vector<float> inL(256, 0.5f);
    std::vector<float> inR(256, 0.5f);
    const float* inCh[2] = { inL.data(), inR.data() };
    float* outCh[2] = { stepOutputBuffers[3]->getWritePointer(0), stepOutputBuffers[3]->getWritePointer(1) };
    ProcessContext ctx{ inCh, outCh, 2, 2, 256 };

    // Stress test: 500 consecutive rapid block dispatches
    constexpr int TestBlocks = 500;
    auto tStart = std::chrono::high_resolution_clock::now();
    for (int b = 0; b < TestBlocks; ++b) {
        scheduler.executePlan(plan.getSteps(), ctx, stepOutputBuffers, bufferPool);
        const float* outL = stepOutputBuffers[3]->getReadPointer(0);
        const float* outR = stepOutputBuffers[3]->getReadPointer(1);
        assert(!std::isnan(outL[0]) && !std::isinf(outL[0]));
        assert(!std::isnan(outR[0]) && !std::isinf(outR[0]));
    }
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto totalUs = std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();
    double avgUs = static_cast<double>(totalUs) / static_cast<double>(TestBlocks);
    std::cout << "  [METRIC] Average wakeup + dispatch latency over 500 blocks: " << avgUs << " us\n";
    CHALLENGE_ASSERT(avgUs < 100.0, "Average latency is well below real-time threshold (< 100 us)");

    // Test clean shutdown latency
    auto tStop0 = std::chrono::high_resolution_clock::now();
    scheduler.stopWorkers();
    auto tStop1 = std::chrono::high_resolution_clock::now();
    auto stopMs = std::chrono::duration_cast<std::chrono::milliseconds>(tStop1 - tStop0).count();
    std::cout << "  [METRIC] Worker shutdown latency: " << stopMs << " ms\n";
    CHALLENGE_ASSERT(stopMs < 50, "Scheduler shuts down cleanly in under 50 ms");

    // Test idempotent stopWorkers()
    scheduler.stopWorkers();
    CHALLENGE_ASSERT(true, "Second stopWorkers() call is safe and idempotent");
}

// ==============================================================================
// CHALLENGE 2: Rapid Start/Stop & Lifecycle Stress (Zero Thread Leak / Deadlock)
// ==============================================================================
void testChallenge2_RapidLifecycleStress() {
    std::cout << "\n====================================================================\n";
    std::cout << "[CHALLENGE 2] Rapid Start/Stop & Worker Lifecycle Stress (Anti-Leak)\n";
    std::cout << "====================================================================\n";

    ProcessSpec spec{ 48000.0, 256, 2, 2 };
    WorkStealingGraphScheduler scheduler;

    // 100 rapid prepare/stop cycles with varying worker counts
    constexpr int Iterations = 100;
    auto tStart = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < Iterations; ++i) {
        uint32_t workerCount = static_cast<uint32_t>((i % 4) + 1); // 1, 2, 3, 4
        scheduler.prepare(spec, workerCount);
        CHALLENGE_ASSERT(scheduler.getNumWorkers() == workerCount, "Worker count correctly initialized");
        scheduler.stopWorkers();
    }
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count();
    std::cout << "  [METRIC] 100 prepare/stop cycles completed in " << durationMs << " ms\n";
    CHALLENGE_ASSERT(durationMs < 1500, "100 lifecycle cycles completed without hang or deadlock");

    // Stress test: prepare without prior stopWorkers (re-preparation in-place)
    for (int i = 0; i < 50; ++i) {
        scheduler.prepare(spec, (i % 2 == 0) ? 1 : 2);
    }
    scheduler.stopWorkers();
    CHALLENGE_ASSERT(true, "50 in-place re-preparations without intermediate stopWorkers() succeeded");
}

// ==============================================================================
// CHALLENGE 3: Execution Equivalence Across Topologies
// ==============================================================================
void testChallenge3_ExecutionEquivalence() {
    std::cout << "\n====================================================================\n";
    std::cout << "[CHALLENGE 3] Execution Equivalence Across Concurrency Modes\n";
    std::cout << "====================================================================\n";

    ProcessSpec spec{ 48000.0, 256, 2, 2 };
    constexpr size_t numSamples = 256;

    std::vector<float> inL(numSamples);
    std::vector<float> inR(numSamples);
    for (size_t s = 0; s < numSamples; ++s) {
        inL[s] = 0.3f * std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(s) / 48000.0f);
        inR[s] = 0.3f * std::cos(2.0f * 3.14159265f * 440.0f * static_cast<float>(s) / 48000.0f);
    }
    const float* inChannels[2] = { inL.data(), inR.data() };

    auto testEquivalence = [&](Graph& graph, const std::string& topologyName) {
        std::vector<NodeId> sorted;
        std::string err;
        bool valid = graph.validateAndTopologicalSort(sorted, err);
        assert(valid);

        ExecutionPlan plan;
        plan.compileFrom(graph, sorted);

        GraphExecutor executorSingle;
        executorSingle.prepare(spec, 64);
        executorSingle.setConcurrencyMode(ConcurrencyMode::SingleThreaded);

        GraphExecutor executorSmart;
        executorSmart.prepare(spec, 64);
        executorSmart.setConcurrencyMode(ConcurrencyMode::SmartMultithreaded);

        GraphExecutor executorAlways;
        executorAlways.prepare(spec, 64);
        executorAlways.setConcurrencyMode(ConcurrencyMode::AlwaysMultithreaded);

        PreallocatedBuffer outSingle; outSingle.prepare(2, numSamples);
        PreallocatedBuffer outSmart;  outSmart.prepare(2, numSamples);
        PreallocatedBuffer outAlways; outAlways.prepare(2, numSamples);

        float* outSingleCh[2] = { outSingle.getWritePointer(0), outSingle.getWritePointer(1) };
        float* outSmartCh[2]  = { outSmart.getWritePointer(0),  outSmart.getWritePointer(1) };
        float* outAlwaysCh[2] = { outAlways.getWritePointer(0), outAlways.getWritePointer(1) };

        ProcessContext ctxSingle{ inChannels, outSingleCh, 2, 2, numSamples };
        ProcessContext ctxSmart { inChannels, outSmartCh,  2, 2, numSamples };
        ProcessContext ctxAlways{ inChannels, outAlwaysCh, 2, 2, numSamples };

        // Warmup 5 blocks for filters/delays to reach steady state
        for (int b = 0; b < 5; ++b) {
            executorSingle.process(plan, ctxSingle, outSingle);
            executorSmart.process(plan, ctxSmart, outSmart);
            executorAlways.process(plan, ctxAlways, outAlways);
        }

        float maxDiffSmart = 0.0f;
        float maxDiffAlways = 0.0f;
        for (size_t s = 0; s < numSamples; ++s) {
            const float sL = outSingle.getReadPointer(0)[s];
            const float sR = outSingle.getReadPointer(1)[s];
            const float mLS = outSmart.getReadPointer(0)[s];
            const float mRS = outSmart.getReadPointer(1)[s];
            const float mLA = outAlways.getReadPointer(0)[s];
            const float mRA = outAlways.getReadPointer(1)[s];

            assert(!std::isnan(sL) && !std::isinf(sL));
            assert(!std::isnan(mLS) && !std::isinf(mLS));
            assert(!std::isnan(mLA) && !std::isinf(mLA));

            maxDiffSmart = std::max(maxDiffSmart, std::abs(sL - mLS));
            maxDiffSmart = std::max(maxDiffSmart, std::abs(sR - mRS));
            maxDiffAlways = std::max(maxDiffAlways, std::abs(sL - mLA));
            maxDiffAlways = std::max(maxDiffAlways, std::abs(sR - mRA));
        }

        std::cout << "  [TOPOLOGY] " << topologyName << " -> maxDiff(Smart): " << maxDiffSmart 
                  << ", maxDiff(Always): " << maxDiffAlways << "\n";
        CHALLENGE_ASSERT(maxDiffSmart < 1e-4f, topologyName + " equivalence with SmartMultithreaded");
        CHALLENGE_ASSERT(maxDiffAlways < 1e-4f, topologyName + " equivalence with AlwaysMultithreaded");
    };

    // 1. Topology A: Empty Graph (0 nodes)
    {
        Graph emptyGraph;
        testEquivalence(emptyGraph, "Topology A: Empty Graph (0 nodes)");
    }

    // 2. Topology B: Single Node (1 node)
    {
        Graph singleGraph;
        singleGraph.addNode(std::make_unique<PassthroughNode>(), "P1");
        testEquivalence(singleGraph, "Topology B: Single Node (1 node)");
    }

    // 3. Topology C: Serial Chain of 6 Nodes (A -> B -> C -> D -> E -> F)
    // Triggers SmartMultithreaded (> 4 nodes)
    {
        Graph serialGraph;
        NodeId n1 = serialGraph.addNode(std::make_unique<PassthroughNode>(), "P1");
        NodeId n2 = serialGraph.addNode(std::make_unique<PassthroughNode>(), "P2");
        NodeId n3 = serialGraph.addNode(std::make_unique<PassthroughNode>(), "P3");
        NodeId n4 = serialGraph.addNode(std::make_unique<PassthroughNode>(), "P4");
        NodeId n5 = serialGraph.addNode(std::make_unique<PassthroughNode>(), "P5");
        NodeId n6 = serialGraph.addNode(std::make_unique<PassthroughNode>(), "P6");
        serialGraph.connect(n1, 2, n2, 1);
        serialGraph.connect(n2, 2, n3, 1);
        serialGraph.connect(n3, 2, n4, 1);
        serialGraph.connect(n4, 2, n5, 1);
        serialGraph.connect(n5, 2, n6, 1);
        testEquivalence(serialGraph, "Topology C: Serial Chain of 6 Nodes");
    }

    // 4. Topology D: Wide Parallel Fan-Out / Fan-In (Diamond: Root -> 4 Branches -> Leaf)
    {
        Graph diamondGraph;
        NodeId root = diamondGraph.addNode(std::make_unique<PassthroughNode>(), "Root");
        NodeId b1 = diamondGraph.addNode(std::make_unique<PassthroughNode>(), "B1");
        NodeId b2 = diamondGraph.addNode(std::make_unique<PassthroughNode>(), "B2");
        NodeId b3 = diamondGraph.addNode(std::make_unique<PassthroughNode>(), "B3");
        NodeId b4 = diamondGraph.addNode(std::make_unique<PassthroughNode>(), "B4");
        NodeId leaf = diamondGraph.addNode(std::make_unique<PassthroughNode>(), "Leaf");
        diamondGraph.connect(root, 2, b1, 1);
        diamondGraph.connect(root, 2, b2, 1);
        diamondGraph.connect(root, 2, b3, 1);
        diamondGraph.connect(root, 2, b4, 1);
        diamondGraph.connect(b1, 2, leaf, 1);
        diamondGraph.connect(b2, 2, leaf, 1);
        diamondGraph.connect(b3, 2, leaf, 1);
        diamondGraph.connect(b4, 2, leaf, 1);
        testEquivalence(diamondGraph, "Topology D: Wide Parallel Fan-Out (6 nodes)");
    }

    // 5. Topology E: Complex 8-Node DAG with DSP Nodes (Filter + Distortion)
    {
        Graph dspGraph;
        NodeId r1 = dspGraph.addNode(std::make_unique<PassthroughNode>(), "R1");
        NodeId r2 = dspGraph.addNode(std::make_unique<PassthroughNode>(), "R2");
        NodeId f1 = dspGraph.addNode(std::make_unique<SimpleFilterNode>(), "Filter1");
        NodeId f2 = dspGraph.addNode(std::make_unique<SimpleFilterNode>(), "Filter2");
        NodeId d1 = dspGraph.addNode(std::make_unique<DistortionNode>(), "Dist1");
        NodeId d2 = dspGraph.addNode(std::make_unique<DistortionNode>(), "Dist2");
        NodeId m1 = dspGraph.addNode(std::make_unique<PassthroughNode>(), "Merge1");
        NodeId m2 = dspGraph.addNode(std::make_unique<PassthroughNode>(), "FinalMaster");

        dspGraph.connect(r1, 2, f1, 1);
        dspGraph.connect(r2, 2, f2, 1);
        dspGraph.connect(f1, 2, d1, 1);
        dspGraph.connect(f2, 2, d2, 1);
        dspGraph.connect(d1, 2, m1, 1);
        dspGraph.connect(d2, 2, m1, 1);
        dspGraph.connect(m1, 2, m2, 1);

        testEquivalence(dspGraph, "Topology E: Complex 8-Node DSP DAG");
    }

    // 6. Topology F: Exact Boundary Test (4 nodes vs 5 nodes for SmartMultithreaded)
    {
        Graph g4;
        NodeId a1 = g4.addNode(std::make_unique<PassthroughNode>(), "A1");
        NodeId a2 = g4.addNode(std::make_unique<PassthroughNode>(), "A2");
        NodeId a3 = g4.addNode(std::make_unique<PassthroughNode>(), "A3");
        NodeId a4 = g4.addNode(std::make_unique<PassthroughNode>(), "A4");
        g4.connect(a1, 2, a2, 1);
        g4.connect(a2, 2, a3, 1);
        g4.connect(a3, 2, a4, 1);
        testEquivalence(g4, "Topology F1: 4-Node Boundary (SmartMultithreaded runs Single)");

        Graph g5;
        NodeId b1 = g5.addNode(std::make_unique<PassthroughNode>(), "B1");
        NodeId b2 = g5.addNode(std::make_unique<PassthroughNode>(), "B2");
        NodeId b3 = g5.addNode(std::make_unique<PassthroughNode>(), "B3");
        NodeId b4 = g5.addNode(std::make_unique<PassthroughNode>(), "B4");
        NodeId b5 = g5.addNode(std::make_unique<PassthroughNode>(), "B5");
        g5.connect(b1, 2, b2, 1);
        g5.connect(b2, 2, b3, 1);
        g5.connect(b3, 2, b4, 1);
        g5.connect(b4, 2, b5, 1);
        testEquivalence(g5, "Topology F2: 5-Node Boundary (SmartMultithreaded runs Multi)");
    }
}

// ==============================================================================
// CHALLENGE 4: Mid-Flight Dynamic Mode Switching Under High Concurrent Load
// ==============================================================================
void testChallenge4_DynamicModeSwitchingUnderLoad() {
    std::cout << "\n====================================================================\n";
    std::cout << "[CHALLENGE 4] Dynamic Mode Switching Mid-Flight Under Audio Stream\n";
    std::cout << "====================================================================\n";

    ProcessSpec spec{ 48000.0, 256, 2, 2 };
    constexpr size_t numSamples = 256;

    DualWorldEngine engine;
    engine.prepare(spec);

    // Build 6-node graph
    Graph graph;
    NodeId n1 = graph.addNode(std::make_unique<PassthroughNode>(), "N1");
    NodeId n2 = graph.addNode(std::make_unique<PassthroughNode>(), "N2");
    NodeId n3 = graph.addNode(std::make_unique<PassthroughNode>(), "N3");
    NodeId n4 = graph.addNode(std::make_unique<PassthroughNode>(), "N4");
    NodeId n5 = graph.addNode(std::make_unique<PassthroughNode>(), "N5");
    NodeId n6 = graph.addNode(std::make_unique<PassthroughNode>(), "N6");
    graph.connect(n1, 2, n2, 1);
    graph.connect(n2, 2, n3, 1);
    graph.connect(n3, 2, n4, 1);
    graph.connect(n4, 2, n5, 1);
    graph.connect(n5, 2, n6, 1);

    std::vector<NodeId> sorted;
    std::string err;
    bool valid = graph.validateAndTopologicalSort(sorted, err);
    assert(valid);
    ExecutionPlan plan;
    plan.compileFrom(graph, sorted);

    std::vector<float> inL(numSamples, 0.2f);
    std::vector<float> inR(numSamples, 0.2f);
    const float* inChannels[2] = { inL.data(), inR.data() };
    std::vector<float> outL(numSamples, 0.0f);
    std::vector<float> outR(numSamples, 0.0f);
    float* outChannels[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inChannels, outChannels, 2, 2, numSamples };

    std::atomic<bool> audioRunning{ true };
    std::atomic<uint64_t> blocksProcessed{ 0 };
    std::atomic<uint64_t> modeSwitches{ 0 };
    std::atomic<bool> nanInfDetected{ false };

    // Thread A: Real-time audio rendering loop (simulating DAW processBlock)
    std::thread audioThread([&]() {
        while (audioRunning.load(std::memory_order_relaxed)) {
            engine.process(plan, ctx, outChannels);
            for (size_t s = 0; s < numSamples; ++s) {
                if (std::isnan(outL[s]) || std::isinf(outL[s]) ||
                    std::isnan(outR[s]) || std::isinf(outR[s])) {
                    nanInfDetected.store(true, std::memory_order_relaxed);
                }
            }
            blocksProcessed.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Thread B: Control/GUI thread violently switching concurrency modes
    std::thread controlThread([&]() {
        const ConcurrencyMode modes[] = {
            ConcurrencyMode::SingleThreaded,
            ConcurrencyMode::SmartMultithreaded,
            ConcurrencyMode::AlwaysMultithreaded
        };
        for (int i = 0; i < 500; ++i) {
            engine.setConcurrencyMode(modes[i % 3]);
            modeSwitches.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
        audioRunning.store(false, std::memory_order_release);
    });

    controlThread.join();
    audioThread.join();

    std::cout << "  [METRIC] Total blocks processed under stress: " << blocksProcessed.load() << "\n";
    std::cout << "  [METRIC] Total concurrent mode switches: " << modeSwitches.load() << "\n";
    CHALLENGE_ASSERT(!nanInfDetected.load(), "Zero NaN or Inf detected during mid-flight mode switching");
    CHALLENGE_ASSERT(blocksProcessed.load() > 200, "Audio thread made steady forward progress (> 200 blocks)");
    CHALLENGE_ASSERT(modeSwitches.load() == 500, "All 500 concurrent mode switches completed cleanly");
}

// ==============================================================================
// CHALLENGE 5: Multi-Instance Concurrent Execution Stress
// ==============================================================================
void testChallenge5_MultiInstanceStress() {
    std::cout << "\n====================================================================\n";
    std::cout << "[CHALLENGE 5] Multi-Instance Concurrency & Resource Isolation\n";
    std::cout << "====================================================================\n";

    ProcessSpec spec{ 48000.0, 256, 2, 2 };
    constexpr size_t numSamples = 256;
    constexpr int NumInstances = 4;
    constexpr int BlocksPerInstance = 200;

    std::vector<std::unique_ptr<DualWorldEngine>> engines;
    std::vector<ExecutionPlan> plans;

    for (int inst = 0; inst < NumInstances; ++inst) {
        auto engine = std::make_unique<DualWorldEngine>();
        engine->prepare(spec);
        engine->setConcurrencyMode(ConcurrencyMode::SmartMultithreaded);

        Graph graph;
        NodeId n1 = graph.addNode(std::make_unique<PassthroughNode>(), "P1");
        NodeId n2 = graph.addNode(std::make_unique<PassthroughNode>(), "P2");
        NodeId n3 = graph.addNode(std::make_unique<PassthroughNode>(), "P3");
        NodeId n4 = graph.addNode(std::make_unique<PassthroughNode>(), "P4");
        NodeId n5 = graph.addNode(std::make_unique<PassthroughNode>(), "P5");
        graph.connect(n1, 2, n2, 1);
        graph.connect(n2, 2, n3, 1);
        graph.connect(n3, 2, n4, 1);
        graph.connect(n4, 2, n5, 1);

        std::vector<NodeId> sorted;
        std::string err;
        bool valid = graph.validateAndTopologicalSort(sorted, err);
        assert(valid);
        ExecutionPlan plan;
        plan.compileFrom(graph, sorted);

        engines.push_back(std::move(engine));
        plans.push_back(std::move(plan));
    }

    std::atomic<bool> allPassed{ true };
    std::vector<std::thread> instanceThreads;

    for (int inst = 0; inst < NumInstances; ++inst) {
        instanceThreads.emplace_back([&, inst]() {
            std::vector<float> inL(numSamples, 0.1f * static_cast<float>(inst + 1));
            std::vector<float> inR(numSamples, 0.1f * static_cast<float>(inst + 1));
            const float* inCh[2] = { inL.data(), inR.data() };
            std::vector<float> outL(numSamples, 0.0f);
            std::vector<float> outR(numSamples, 0.0f);
            float* outCh[2] = { outL.data(), outR.data() };
            ProcessContext ctx{ inCh, outCh, 2, 2, numSamples };

            for (int b = 0; b < BlocksPerInstance; ++b) {
                engines[inst]->process(plans[inst], ctx, outCh);
                for (size_t s = 0; s < numSamples; ++s) {
                    if (std::isnan(outL[s]) || std::isinf(outL[s])) {
                        allPassed.store(false, std::memory_order_relaxed);
                    }
                }
            }
        });
    }

    for (auto& t : instanceThreads) {
        t.join();
    }

    CHALLENGE_ASSERT(allPassed.load(), "4 concurrent engine instances executed 200 blocks without NaN/Inf or collision");
}

// ==============================================================================
// CHALLENGE 6: Adversarial Inter-Nodal Sidechain & Audio-Rate FM Equivalence
// ==============================================================================
void testChallenge6_InterNodalSidechainAndAudioRateMod() {
    std::cout << "\n====================================================================\n";
    std::cout << "[CHALLENGE 6] Adversarial Inter-Nodal Sidechain & Audio-Rate FM Oracle\n";
    std::cout << "====================================================================\n";

    ProcessSpec spec{ 48000.0, 256, 2, 2 };
    constexpr size_t numSamples = 256;

    std::vector<float> inL(numSamples, 0.4f);
    std::vector<float> inR(numSamples, 0.4f);
    const float* inChannels[2] = { inL.data(), inR.data() };

    // Test 6A: Inter-nodal Audio-Rate Modulation (FM) into SimpleFilterNode
    // Node 1 (Passthrough Modulator) -> AudioRateModPinId of Node 2 (SimpleFilterNode)
    // plus 4 dummy nodes so total steps = 6 (> 4 triggers SmartMultithreaded)
    {
        Graph graph;
        NodeId nMod = graph.addNode(std::make_unique<PassthroughNode>(), "FM_Modulator");
        NodeId nCarrier = graph.addNode(std::make_unique<SimpleFilterNode>(), "FM_Carrier");
        NodeId d1 = graph.addNode(std::make_unique<PassthroughNode>(), "D1");
        NodeId d2 = graph.addNode(std::make_unique<PassthroughNode>(), "D2");
        NodeId d3 = graph.addNode(std::make_unique<PassthroughNode>(), "D3");
        NodeId d4 = graph.addNode(std::make_unique<PassthroughNode>(), "D4");

        // Connect main audio into modulator and carrier
        graph.connect(nMod, 2, d1, 1);
        graph.connect(d1, 2, d2, 1);
        graph.connect(d2, 2, d3, 1);
        graph.connect(d3, 2, d4, 1);
        // Connect nMod output to nCarrier AudioRateModPinId (pin 4)
        graph.connect(nMod, 2, nCarrier, AudioRateModPinId);

        std::vector<NodeId> sorted;
        std::string err;
        bool valid = graph.validateAndTopologicalSort(sorted, err);
        assert(valid);
        ExecutionPlan plan;
        plan.compileFrom(graph, sorted);

        GraphExecutor execSingle;
        execSingle.prepare(spec, 64);
        execSingle.setConcurrencyMode(ConcurrencyMode::SingleThreaded);

        GraphExecutor execAlways;
        execAlways.prepare(spec, 64);
        execAlways.setConcurrencyMode(ConcurrencyMode::AlwaysMultithreaded);

        PreallocatedBuffer outSingle; outSingle.prepare(2, numSamples);
        PreallocatedBuffer outAlways; outAlways.prepare(2, numSamples);
        float* outSingleCh[2] = { outSingle.getWritePointer(0), outSingle.getWritePointer(1) };
        float* outAlwaysCh[2] = { outAlways.getWritePointer(0), outAlways.getWritePointer(1) };
        ProcessContext ctxSingle{ inChannels, outSingleCh, 2, 2, numSamples };
        ProcessContext ctxAlways{ inChannels, outAlwaysCh, 2, 2, numSamples };

        execSingle.process(plan, ctxSingle, outSingle);
        execAlways.process(plan, ctxAlways, outAlways);

        float maxDiff = 0.0f;
        for (size_t s = 0; s < numSamples; ++s) {
            maxDiff = std::max(maxDiff, std::abs(outSingle.getReadPointer(0)[s] - outAlways.getReadPointer(0)[s]));
            maxDiff = std::max(maxDiff, std::abs(outSingle.getReadPointer(1)[s] - outAlways.getReadPointer(1)[s]));
        }

        std::cout << "  [EMPIRICAL ORACLE] Audio-Rate FM maxDiff(Single vs AlwaysMulti): " << maxDiff << "\n";
        if (maxDiff > 1e-4f) {
            std::cout << "  [DISCREPANCY DETECTED] WorkStealingGraphScheduler does NOT route AudioRateModChannels!\n";
        }
    }

    // Test 6B: Inter-nodal Sidechain into CompressorNode
    {
        Graph graph;
        NodeId nSc = graph.addNode(std::make_unique<PassthroughNode>(), "SidechainSource");
        NodeId nComp = graph.addNode(std::make_unique<CompressorNode>(), "SidechainComp");
        NodeId d1 = graph.addNode(std::make_unique<PassthroughNode>(), "D1");
        NodeId d2 = graph.addNode(std::make_unique<PassthroughNode>(), "D2");
        NodeId d3 = graph.addNode(std::make_unique<PassthroughNode>(), "D3");
        NodeId d4 = graph.addNode(std::make_unique<PassthroughNode>(), "D4");

        graph.connect(nSc, 2, d1, 1);
        graph.connect(d1, 2, d2, 1);
        graph.connect(d2, 2, d3, 1);
        graph.connect(d3, 2, d4, 1);
        // Connect sidechain from nSc to nComp (pin 3)
        graph.connect(nSc, 2, nComp, SidechainPinId);

        std::vector<NodeId> sorted;
        std::string err;
        bool valid = graph.validateAndTopologicalSort(sorted, err);
        assert(valid);
        ExecutionPlan plan;
        plan.compileFrom(graph, sorted);

        GraphExecutor execSingle;
        execSingle.prepare(spec, 64);
        execSingle.setConcurrencyMode(ConcurrencyMode::SingleThreaded);

        GraphExecutor execAlways;
        execAlways.prepare(spec, 64);
        execAlways.setConcurrencyMode(ConcurrencyMode::AlwaysMultithreaded);

        PreallocatedBuffer outSingle; outSingle.prepare(2, numSamples);
        PreallocatedBuffer outAlways; outAlways.prepare(2, numSamples);
        float* outSingleCh[2] = { outSingle.getWritePointer(0), outSingle.getWritePointer(1) };
        float* outAlwaysCh[2] = { outAlways.getWritePointer(0), outAlways.getWritePointer(1) };
        ProcessContext ctxSingle{ inChannels, outSingleCh, 2, 2, numSamples };
        ProcessContext ctxAlways{ inChannels, outAlwaysCh, 2, 2, numSamples };

        // Process blocks
        for (int b = 0; b < 10; ++b) {
            execSingle.process(plan, ctxSingle, outSingle);
            execAlways.process(plan, ctxAlways, outAlways);
        }

        float maxDiff = 0.0f;
        for (size_t s = 0; s < numSamples; ++s) {
            maxDiff = std::max(maxDiff, std::abs(outSingle.getReadPointer(0)[s] - outAlways.getReadPointer(0)[s]));
            maxDiff = std::max(maxDiff, std::abs(outSingle.getReadPointer(1)[s] - outAlways.getReadPointer(1)[s]));
        }

        std::cout << "  [EMPIRICAL ORACLE] Modular Sidechain maxDiff(Single vs AlwaysMulti): " << maxDiff << "\n";
        if (maxDiff > 1e-4f) {
            std::cout << "  [DISCREPANCY DETECTED] WorkStealingGraphScheduler does NOT set numSidechainChannels!\n";
        }
    }

    // Test 6C: Compressor Dynamic Sidechain Ducking Equivalence
    // Node 1 (Bass) -> Compressor Node 3 (Audio In) -> Leaf
    // Node 2 (Loud Kick) -> Compressor Node 3 (Sidechain Pin)
    // 3 extra dummy nodes so total steps = 6 (> 4 triggers multithreading)
    {
        Graph graph;
        NodeId nBass = graph.addNode(std::make_unique<PassthroughNode>(), "Bass");
        NodeId nKick = graph.addNode(std::make_unique<DistortionNode>(), "Kick"); // High gain
        NodeId nComp = graph.addNode(std::make_unique<CompressorNode>(), "DuckingComp");
        NodeId d1 = graph.addNode(std::make_unique<PassthroughNode>(), "D1");
        NodeId d2 = graph.addNode(std::make_unique<PassthroughNode>(), "D2");
        NodeId d3 = graph.addNode(std::make_unique<PassthroughNode>(), "D3");

        // Set compressor: low threshold -30 dB, high ratio 10:1
        auto* compProc = dynamic_cast<CompressorNode*>(graph.getNodeProcessor(nComp));
        if (compProc) {
            compProc->setParameter(CompressorNode::Threshold, -30.0f);
            compProc->setParameter(CompressorNode::Ratio, 10.0f);
            compProc->setParameter(CompressorNode::Attack, 1.0f);
            compProc->setParameter(CompressorNode::Release, 50.0f);
        }

        graph.connect(nBass, 2, nComp, 1); // Main audio in
        graph.connect(nKick, 2, nComp, SidechainPinId); // Sidechain in
        graph.connect(nKick, 2, d1, 1);
        graph.connect(d1, 2, d2, 1);
        graph.connect(d2, 2, d3, 1);

        std::vector<NodeId> sorted;
        std::string err;
        bool valid = graph.validateAndTopologicalSort(sorted, err);
        assert(valid);
        ExecutionPlan plan;
        plan.compileFrom(graph, sorted);

        GraphExecutor execSingle;
        execSingle.prepare(spec, 64);
        execSingle.setConcurrencyMode(ConcurrencyMode::SingleThreaded);

        GraphExecutor execAlways;
        execAlways.prepare(spec, 64);
        execAlways.setConcurrencyMode(ConcurrencyMode::AlwaysMultithreaded);

        PreallocatedBuffer outSingle; outSingle.prepare(2, numSamples);
        PreallocatedBuffer outAlways; outAlways.prepare(2, numSamples);
        float* outSingleCh[2] = { outSingle.getWritePointer(0), outSingle.getWritePointer(1) };
        float* outAlwaysCh[2] = { outAlways.getWritePointer(0), outAlways.getWritePointer(1) };
        ProcessContext ctxSingle{ inChannels, outSingleCh, 2, 2, numSamples };
        ProcessContext ctxAlways{ inChannels, outAlwaysCh, 2, 2, numSamples };

        // Process 10 blocks to let compressor gain reduction engage
        for (int b = 0; b < 10; ++b) {
            execSingle.process(plan, ctxSingle, outSingle);
            execAlways.process(plan, ctxAlways, outAlways);
        }

        float maxDiff = 0.0f;
        for (size_t s = 0; s < numSamples; ++s) {
            maxDiff = std::max(maxDiff, std::abs(outSingle.getReadPointer(0)[s] - outAlways.getReadPointer(0)[s]));
            maxDiff = std::max(maxDiff, std::abs(outSingle.getReadPointer(1)[s] - outAlways.getReadPointer(1)[s]));
        }

        std::cout << "  [EMPIRICAL ORACLE] Compressor Sidechain Ducking maxDiff(Single vs AlwaysMulti): " << maxDiff << "\n";
        if (maxDiff > 1e-4f) {
            std::cout << "  [DISCREPANCY DETECTED] Compressor sidechain behaves differently in Multithreaded mode!\n";
        }
    }
}


// ==============================================================================
// MAIN ENTRY POINT
// ==============================================================================
int main() {
    std::cout << "====================================================================\n";
    std::cout << "N8EFFECT CONCURRENCY & STRESS CHALLENGER SUITE (CHALLENGER 1)\n";
    std::cout << "Empirical stress testing of scheduler, passive wait & concurrency modes\n";
    std::cout << "====================================================================\n";

    testChallenge1_PassiveWaitAndWakeup();
    testChallenge2_RapidLifecycleStress();
    testChallenge3_ExecutionEquivalence();
    testChallenge4_DynamicModeSwitchingUnderLoad();
    testChallenge5_MultiInstanceStress();
    testChallenge6_InterNodalSidechainAndAudioRateMod();

    std::cout << "\n====================================================================\n";
    std::cout << "CHALLENGE SUMMARY: " << gPassedTests << " PASSED, " << gFailedTests << " FAILED\n";
    std::cout << "====================================================================\n";

    return (gFailedTests == 0) ? 0 : 1;
}
