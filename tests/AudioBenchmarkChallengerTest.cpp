#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <cassert>
#include <chrono>
#include <memory>
#include <cstring>
#include <atomic>
#include <algorithm>

#include "../source/core/Types.h"
#include "../source/core/RealtimePools.h"
#include "../source/core/CpuProfiler.h"
#include "../source/graph/AudioProcessorNode.h"
#include "../source/graph/Graph.h"
#include "../source/graph/GraphExecutor.h"
#include "../source/dsp/PassthroughNode.h"
#include "../source/event/EventManager.h"
#include "../source/analysis/AnalysisEngine.h"
#include "../source/analysis/PitchTracker.h"
#include "../source/plugin/DualWorldEngine.h"

using namespace audio_graph;

static int gFailedChallenges = 0;
static int gPassedChallenges = 0;

#define CHALLENGE_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] " << msg << " (line " << __LINE__ << ")\n"; \
            gFailedChallenges++; \
        } else { \
            std::cout << "  [PASS] " << msg << "\n"; \
            gPassedChallenges++; \
        } \
    } while(0)

// ==============================================================================
// 1. Silence Gating (< -70 dBFS) & Snapshot Preservation Empirical Challenge
// ==============================================================================
void challengeSilenceGatingAndSnapshot() {
    std::cout << "\n======================================================\n";
    std::cout << "[CHALLENGE 1] Silence Gating (< -70 dBFS) & Snapshot Preservation\n";
    std::cout << "======================================================\n";

    constexpr double SampleRate = 48000.0;
    constexpr uint32_t BlockSize = 256;
    AnalysisEngine analysis;
    ProcessSpec spec{ SampleRate, BlockSize, 2, 2 };
    analysis.prepare(spec);

    std::vector<float> bufL(BlockSize, 0.0f);
    std::vector<float> bufR(BlockSize, 0.0f);
    const float* channels[2] = { bufL.data(), bufR.data() };

    // 1.1 Pure silence (0.0f): Silence gating active
    analysis.process(channels, 2, BlockSize);
    const auto& snap0 = analysis.getSnapshot();
    CHALLENGE_CHECK(snap0.energy == 0.0f, "Pure silence gives energy == 0");
    CHALLENGE_CHECK(snap0.pitchClarity == 0.0f, "Pure silence gives pitchClarity == 0");
    CHALLENGE_CHECK(snap0.clarity == 0.0f, "Pure silence gives clarity == 0");
    CHALLENGE_CHECK(!snap0.isTransient, "Pure silence gives isTransient == false");
    CHALLENGE_CHECK(snap0.onsetStrength == 0.0f, "Pure silence gives onsetStrength == 0");

    // 1.2 Boundary test: Input at -74 dBFS (amplitude = 0.0002f, RMS = 0.0002f < 0.0003f threshold)
    std::fill(bufL.begin(), bufL.end(), 0.0002f);
    std::fill(bufR.begin(), bufR.end(), 0.0002f);
    analysis.process(channels, 2, BlockSize);
    const auto& snapSub = analysis.getSnapshot();
    CHALLENGE_CHECK(snapSub.energy == 0.0f, "Sub-threshold (-74 dBFS) is gated (energy == 0)");
    CHALLENGE_CHECK(snapSub.pitchClarity == 0.0f, "Sub-threshold is gated (pitchClarity == 0)");

    // 1.3 Boundary test: Input at -66 dBFS (amplitude = 0.0005f, RMS = 0.0005f > 0.0003f threshold)
    std::fill(bufL.begin(), bufL.end(), 0.0005f);
    std::fill(bufR.begin(), bufR.end(), 0.0005f);
    // Feed 5 blocks (1280 samples > 1024 FFT size) so SpectralFeatureExtractor computes frame
    for (int b = 0; b < 5; ++b) {
        analysis.process(channels, 2, BlockSize);
    }
    const auto& snapSuper = analysis.getSnapshot();
    CHALLENGE_CHECK(snapSuper.energy > 0.0f, "Super-threshold (-66 dBFS) passes gate (energy > 0)");

    // 1.4 Warmup with 440.0 Hz active tone (Standard Ground Truth)
    constexpr float ToneHz = 440.0f;
    float phase = 0.0f;
    const float phaseInc = 2.0f * 3.14159265f * ToneHz / static_cast<float>(SampleRate);
    for (int b = 0; b < 60; ++b) {
        for (uint32_t s = 0; s < BlockSize; ++s) {
            float v = 0.5f * std::sin(phase);
            bufL[s] = v;
            bufR[s] = v;
            phase += phaseInc;
            if (phase >= 2.0f * 3.14159265f) phase -= 2.0f * 3.14159265f;
        }
        analysis.process(channels, 2, BlockSize);
    }

    const auto& snapTone = analysis.getSnapshot();
    float toneError = std::abs(snapTone.pitchHz - ToneHz);
    std::cout << "  Active 440 Hz tone detected: " << snapTone.pitchHz << " Hz (Error: " << toneError << " Hz, Clarity: " << snapTone.pitchClarity << ")\n";
    CHALLENGE_CHECK(toneError < 1.0f, "Active tone detected accurately (< 1.0 Hz error)");
    CHALLENGE_CHECK(snapTone.pitchClarity > 0.85f, "Active tone has high clarity (> 0.85)");
    float activeCentroid = snapTone.spectralCentroid;
    float activePitchHz = snapTone.pitchHz;

    // 1.5 Abrupt drop to total silence: verify snapshot preservation (Reglas 34 y 35)
    std::fill(bufL.begin(), bufL.end(), 0.0f);
    std::fill(bufR.begin(), bufR.end(), 0.0f);
    analysis.process(channels, 2, BlockSize);
    const auto& snapDrop = analysis.getSnapshot();
    CHALLENGE_CHECK(snapDrop.energy == 0.0f, "Post-active silence energy is 0");
    CHALLENGE_CHECK(snapDrop.pitchClarity == 0.0f, "Post-active silence clarity is 0");
    CHALLENGE_CHECK(snapDrop.pitchHz == activePitchHz, "Snapshot pitchHz preserved exactly across silence");
    CHALLENGE_CHECK(snapDrop.spectralCentroid == activeCentroid, "Snapshot spectralCentroid preserved exactly across silence");

    // 1.6 Silence-to-Active transition:
    // Run 10 silent blocks
    for (int i = 0; i < 10; ++i) {
        analysis.process(channels, 2, BlockSize);
    }
    // Feed block of active audio resuming from silence
    phase = 0.0f;
    for (uint32_t s = 0; s < BlockSize; ++s) {
        float v = 0.5f * std::sin(phase);
        bufL[s] = v;
        bufR[s] = v;
        phase += phaseInc;
    }
    analysis.process(channels, 2, BlockSize);
    const auto& snapResume = analysis.getSnapshot();
    CHALLENGE_CHECK(!std::isnan(snapResume.pitchHz), "Pitch remains clean and numeric on resume");
    CHALLENGE_CHECK(!std::isnan(snapResume.onsetStrength), "Onset strength remains clean and numeric on resume");
    CHALLENGE_CHECK(snapResume.energy > 0.0f || snapResume.onsetStrength > 0.0f, "Signal detected on resume from silence");

    // Sustained signal: after refractory period and adaptation, transient flag clears
    for (int i = 0; i < 15; ++i) {
        for (uint32_t s = 0; s < BlockSize; ++s) {
            float v = 0.5f * std::sin(phase);
            bufL[s] = v;
            bufR[s] = v;
            phase += phaseInc;
        }
        analysis.process(channels, 2, BlockSize);
    }
    const auto& snapSustained = analysis.getSnapshot();
    CHALLENGE_CHECK(!snapSustained.isTransient, "Sustained tone adapts so isTransient returns to false");
}

// ==============================================================================
// 2. Measured Cycle Reduction (> 70%) Benchmark Challenge
// ==============================================================================
void challengeMeasuredCycleReduction() {
    std::cout << "\n======================================================\n";
    std::cout << "[CHALLENGE 2] Measured Cycle Reduction Benchmark (> 70%)\n";
    std::cout << "======================================================\n";

    constexpr double SampleRate = 48000.0;
    constexpr uint32_t BlockSize = 256;
    constexpr int Iterations = 500;

    AnalysisEngine analysis;
    ProcessSpec spec{ SampleRate, BlockSize, 2, 2 };
    analysis.prepare(spec);

    std::vector<float> activeL(BlockSize);
    std::vector<float> activeR(BlockSize);
    const float* activeChannels[2] = { activeL.data(), activeR.data() };

    float phase = 0.0f;
    const float phaseInc = 2.0f * 3.14159265f * 440.0f / static_cast<float>(SampleRate);
    auto fillSine = [&]() {
        for (uint32_t s = 0; s < BlockSize; ++s) {
            float v = 0.5f * std::sin(phase);
            activeL[s] = v;
            activeR[s] = v;
            phase += phaseInc;
            if (phase >= 2.0f * 3.14159265f) phase -= 2.0f * 3.14159265f;
        }
    };

    // Warmup
    for (int w = 0; w < 40; ++w) {
        fillSine();
        analysis.process(activeChannels, 2, BlockSize);
    }

    // Benchmark active blocks
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < Iterations; ++i) {
        fillSine();
        analysis.process(activeChannels, 2, BlockSize);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto activeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    // Benchmark silent blocks
    std::vector<float> silentL(BlockSize, 0.0f);
    std::vector<float> silentR(BlockSize, 0.0f);
    const float* silentChannels[2] = { silentL.data(), silentR.data() };

    auto t2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < Iterations; ++i) {
        analysis.process(silentChannels, 2, BlockSize);
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    auto silentNs = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count();

    double cycleReduction = 1.0 - (static_cast<double>(silentNs) / static_cast<double>(activeNs));
    double pct = cycleReduction * 100.0;

    std::cout << "  Active time: " << (activeNs / 1000.0) << " us (" << (activeNs / static_cast<double>(Iterations * 1000.0)) << " us/block)\n";
    std::cout << "  Silent time: " << (silentNs / 1000.0) << " us (" << (silentNs / static_cast<double>(Iterations * 1000.0)) << " us/block)\n";
    std::cout << "  Measured Cycle Reduction: " << std::fixed << std::setprecision(2) << pct << "%\n";

    CHALLENGE_CHECK(cycleReduction > 0.70, "Silence gating achieves > 70% cycle reduction");
    CHALLENGE_CHECK(cycleReduction > 0.90, "Silence gating achieves > 90% cycle reduction (target ~98%)");
}

// ==============================================================================
// 3. YIN Pitch Accuracy & Multi-Frequency / Multi-Rate Clarity Challenge
// ==============================================================================
void challengeYinPitchAccuracy() {
    std::cout << "\n======================================================\n";
    std::cout << "[CHALLENGE 3] YIN Pitch Accuracy & Clarity Challenge\n";
    std::cout << "======================================================\n";

    // 3.1 440.0 Hz pure sine at 48000 Hz
    {
        PitchTracker tracker;
        tracker.prepare(48000.0);
        std::vector<float> block(256);
        float phase = 0.0f;
        const float phaseInc = 2.0f * 3.14159265f * 440.0f / 48000.0f;
        for (int b = 0; b < 50; ++b) {
            for (size_t s = 0; s < 256; ++s) {
                block[s] = 0.5f * std::sin(phase);
                phase += phaseInc;
                if (phase >= 2.0f * 3.14159265f) phase -= 2.0f * 3.14159265f;
            }
            tracker.process(block.data(), 256);
        }
        float pitch440 = tracker.getFundamentalHz();
        float err440 = std::abs(pitch440 - 440.0f);
        float clarity440 = tracker.getClarity();
        std::cout << "  440 Hz Test @ 48kHz: detected " << pitch440 << " Hz, error = " << err440 << " Hz, clarity = " << clarity440 << "\n";
        CHALLENGE_CHECK(err440 < 1.0f, "440 Hz pitch error < 1.0 Hz");
        CHALLENGE_CHECK(err440 < 0.1f, "440 Hz pitch error < 0.1 Hz (ultra-accurate)");
        CHALLENGE_CHECK(clarity440 > 0.85f, "440 Hz clarity > 0.85");
    }

    // 3.2 Octave sweep: 110 Hz, 220 Hz, 880 Hz, 1760 Hz
    const float testFreqs[] = { 110.0f, 220.0f, 880.0f, 1760.0f };
    for (float freq : testFreqs) {
        PitchTracker tracker;
        tracker.prepare(48000.0);
        std::vector<float> block(256);
        float phase = 0.0f;
        const float phaseInc = 2.0f * 3.14159265f * freq / 48000.0f;
        for (int b = 0; b < 60; ++b) {
            for (size_t s = 0; s < 256; ++s) {
                block[s] = 0.5f * std::sin(phase);
                phase += phaseInc;
                if (phase >= 2.0f * 3.14159265f) phase -= 2.0f * 3.14159265f;
            }
            tracker.process(block.data(), 256);
        }
        float pitch = tracker.getFundamentalHz();
        float err = std::abs(pitch - freq);
        float clarity = tracker.getClarity();
        float centsError = 1200.0f * std::abs(std::log2(pitch / freq));
        std::cout << "  " << freq << " Hz Test: detected " << pitch << " Hz, error = " << err << " Hz (" << centsError << " cents), clarity = " << clarity << "\n";
        CHALLENGE_CHECK(centsError < 15.0f, "Pitch error within musical pitch perception threshold (< 15 cents)");
        CHALLENGE_CHECK(clarity > 0.80f, "Clarity high across musical range");
    }

    // 3.3 Sample rate invariance: 44.1 kHz, 96 kHz
    const double testRates[] = { 44100.0, 96000.0 };
    for (double sr : testRates) {
        PitchTracker tracker;
        tracker.prepare(sr);
        std::vector<float> block(256);
        float phase = 0.0f;
        const float phaseInc = 2.0f * 3.14159265f * 440.0f / static_cast<float>(sr);
        for (int b = 0; b < 70; ++b) {
            for (size_t s = 0; s < 256; ++s) {
                block[s] = 0.5f * std::sin(phase);
                phase += phaseInc;
                if (phase >= 2.0f * 3.14159265f) phase -= 2.0f * 3.14159265f;
            }
            tracker.process(block.data(), 256);
        }
        float pitch = tracker.getFundamentalHz();
        float err = std::abs(pitch - 440.0f);
        std::cout << "  440 Hz @ " << sr << " Hz: detected " << pitch << " Hz, error = " << err << " Hz\n";
        CHALLENGE_CHECK(err < 1.0f, "Pitch error < 1.0 Hz at non-standard sample rate");
    }
}

// ==============================================================================
// 4. EventManager & DualWorldEngine Idle Bypass Bit-Exact Fidelity Challenge
// ==============================================================================
void challengeEventManagerIdleBypassBitExact() {
    std::cout << "\n======================================================\n";
    std::cout << "[CHALLENGE 4] EventManager & DualWorldEngine Idle Bypass Bit-Exact Fidelity\n";
    std::cout << "======================================================\n";

    // 4.1 EventManager::render with activeEvents == 0
    EventManager em;
    em.prepare(48000.0, 256);
    CHALLENGE_CHECK(em.getActiveEventCount() == 0, "Initial activeEventCount == 0");

    std::vector<float> originalL(256);
    std::vector<float> originalR(256);
    for (size_t s = 0; s < 256; ++s) {
        originalL[s] = 3.14159265f * static_cast<float>(s + 1) * 0.0123f;
        originalR[s] = -2.7182818f * static_cast<float>(s + 1) * 0.0456f;
    }

    std::vector<float> testL = originalL;
    std::vector<float> testR = originalR;

    // Call render with idle events
    em.render(testL.data(), testR.data(), 256, 1.0f, -1);

    // Verify 100% bit-exact preservation using memcmp
    int cmpL = std::memcmp(testL.data(), originalL.data(), 256 * sizeof(float));
    int cmpR = std::memcmp(testR.data(), originalR.data(), 256 * sizeof(float));
    CHALLENGE_CHECK(cmpL == 0, "EventManager::render left buffer bit-for-bit identical when idle");
    CHALLENGE_CHECK(cmpR == 0, "EventManager::render right buffer bit-for-bit identical when idle");

    // 4.2 DualWorldEngine Idle Event Bypass
    DualWorldEngine engine;
    ProcessSpec spec{ 48000.0, 256, 2, 2 };
    engine.prepare(spec);

    Graph graph;
    graph.addNode(std::make_unique<PassthroughNode>(), "Pass");
    std::vector<NodeId> sorted;
    std::string err;
    bool valid = graph.validateAndTopologicalSort(sorted, err);
    assert(valid);
    ExecutionPlan plan;
    plan.compileFrom(graph, sorted);

    engine.setDryLevel(0.0f);
    engine.setWetLevel(1.0f);

    std::vector<float> inL(256);
    std::vector<float> inR(256);
    for (size_t s = 0; s < 256; ++s) {
        inL[s] = 0.35f * std::sin(static_cast<float>(s) * 0.05f);
        inR[s] = 0.35f * std::cos(static_cast<float>(s) * 0.05f);
    }
    const float* inCh[2] = { inL.data(), inR.data() };
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    float* outCh[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inCh, outCh, 2, 2, 256 };

    // Let volume smoothing stabilize
    for (int b = 0; b < 60; ++b) {
        engine.process(plan, ctx, outCh);
    }

    CHALLENGE_CHECK(engine.getEventManager().getActiveEventCount() == 0, "Engine event count is 0");
    float maxDiffIdle = 0.0f;
    for (size_t s = 0; s < 256; ++s) {
        maxDiffIdle = std::max(maxDiffIdle, std::abs(outL[s] - inL[s]));
        maxDiffIdle = std::max(maxDiffIdle, std::abs(outR[s] - inR[s]));
    }
    std::cout << "  DualWorldEngine idle passthrough max diff: " << maxDiffIdle << "\n";
    CHALLENGE_CHECK(maxDiffIdle < 1e-4f, "DualWorldEngine passthrough fidelity during event idle bypass");

    // 4.3 Dynamic activation & deactivation lifecycle
    engine.getEventManager().captureInputAudio(inCh, 2, 256);
    Event* ev = engine.getEventManager().spawnEvent(EventType::Fragment, 256.0, 512, 1.0f, 1.0f);
    CHALLENGE_CHECK(ev != nullptr, "Spawned event successfully");
    CHALLENGE_CHECK(engine.getEventManager().getActiveEventCount() == 1, "Active event count == 1");

    // Process block with active event -> bypass inactive, event audio renders
    std::vector<float> eventOutL(256, 0.0f);
    std::vector<float> eventOutR(256, 0.0f);
    engine.getEventManager().render(eventOutL.data(), eventOutR.data(), 256, 0.5f, 0);
    float eventEnergy = 0.0f;
    for (size_t s = 0; s < 256; ++s) {
        eventEnergy += std::abs(eventOutL[s]) + std::abs(eventOutR[s]);
    }
    std::cout << "  DEBUG: writePos=" << engine.getEventManager().getCaptureBuffer().getWritePosition()
              << " capacity=" << engine.getEventManager().getCaptureBuffer().getCapacitySamples()
              << " evActive=" << ev->isActive()
              << " evState=" << static_cast<int>(ev->getState())
              << " read0=" << engine.getEventManager().getCaptureBuffer().readSample(0, 0.0)
              << " read100=" << engine.getEventManager().getCaptureBuffer().readSample(0, 100.0)
              << " read256=" << engine.getEventManager().getCaptureBuffer().readSample(0, 256.0) << "\n";
    std::cout << "  Event render energy with 1 active event: " << eventEnergy << " (first samples: " << eventOutL[0] << ", " << eventOutL[100] << ", " << eventOutL[250] << ")\n";
    CHALLENGE_CHECK(eventEnergy > 0.0f, "Event engine renders non-zero energy when active");

    // Kill event and render to force return to pool
    ev->kill();
    engine.getEventManager().render(eventOutL.data(), eventOutR.data(), 256, 0.5f, -1);
    CHALLENGE_CHECK(engine.getEventManager().getActiveEventCount() == 0, "Dead event safely returned to pool (count == 0)");

    // Test that bit-exact idle bypass immediately reactivates
    testL = originalL;
    testR = originalR;
    engine.getEventManager().render(testL.data(), testR.data(), 256, 1.0f, -1);
    cmpL = std::memcmp(testL.data(), originalL.data(), 256 * sizeof(float));
    cmpR = std::memcmp(testR.data(), originalR.data(), 256 * sizeof(float));
    CHALLENGE_CHECK(cmpL == 0, "Bit-exact idle bypass re-activates immediately after event destruction (L)");
    CHALLENGE_CHECK(cmpR == 0, "Bit-exact idle bypass re-activates immediately after event destruction (R)");
}

// ==============================================================================
// 5. Telemetry Inactivity Gating & Stale Bug Eradication Challenge
// ==============================================================================
void challengeTelemetryGating() {
    std::cout << "\n======================================================\n";
    std::cout << "[CHALLENGE 5] Telemetry Inactivity Gating & Stale Item Eradication\n";
    std::cout << "======================================================\n";

    DualWorldEngine engine;
    ProcessSpec spec{ 48000.0, 256, 2, 2 };
    engine.prepare(spec);

    ExecutionPlan plan;

    // Synthesize loud, aggressive transient audio
    std::vector<float> inL(256, 0.0f);
    std::vector<float> inR(256, 0.0f);
    for (size_t s = 0; s < 256; ++s) {
        inL[s] = 0.5f * std::sin(2.0f * 3.14159f * 1000.0f * static_cast<float>(s) / 48000.0f);
        inR[s] = 0.9f * std::sin(2.0f * 3.14159f * 1000.0f * static_cast<float>(s) / 48000.0f);
    }
    inR[0] = 1.0f; // Strong transient spike

    const float* inChannels[2] = { inL.data(), inR.data() };
    std::vector<float> outL(256, 0.0f);
    std::vector<float> outR(256, 0.0f);
    float* outChannels[2] = { outL.data(), outR.data() };
    ProcessContext ctx{ inChannels, outChannels, 2, 2, 256 };

    auto& telemetry = engine.getEventManager().getTelemetryBuffer();

    // 5.1 Editor Inactive: Gating MUST prevent ANY items from entering the SPSC queue
    engine.setEditorActive(false);
    CHALLENGE_CHECK(!engine.isEditorActive(), "Engine reports editorActive == false");
    CHALLENGE_CHECK(!engine.getEventManager().isEditorActive(), "EventManager reports editorActive == false");

    telemetry.reset();
    EventTelemetryItem item;
    CHALLENGE_CHECK(!telemetry.pop(item), "Telemetry queue is initially empty");

    // Process 20 blocks with strong transient signal while editor is inactive
    for (int b = 0; b < 20; ++b) {
        engine.process(plan, ctx, outChannels);
    }

    CHALLENGE_CHECK(!telemetry.pop(item), "Telemetry queue remains strictly 0 items when editor is inactive");

    // Also spawn active event and call render with editor inactive
    engine.getEventManager().captureInputAudio(inChannels, 2, 256);
    Event* ev = engine.getEventManager().spawnEvent(EventType::Transient, 0.0, 512, 1.0f, 1.0f);
    CHALLENGE_CHECK(ev != nullptr, "Spawned event while editor inactive");
    std::vector<float> scratchL(256, 0.0f);
    std::vector<float> scratchR(256, 0.0f);
    engine.getEventManager().render(scratchL.data(), scratchR.data(), 256, 0.8f, -1);

    CHALLENGE_CHECK(!telemetry.pop(item), "EventManager::render emits 0 telemetry items when editor is inactive");

    // 5.2 Editor Active: Telemetry flows reactively
    engine.setEditorActive(true);
    CHALLENGE_CHECK(engine.isEditorActive(), "Engine reports editorActive == true");
    CHALLENGE_CHECK(engine.getEventManager().isEditorActive(), "EventManager reports editorActive == true");

    engine.process(plan, ctx, outChannels);
    bool poppedActive = telemetry.pop(item);
    CHALLENGE_CHECK(poppedActive, "Telemetry successfully enqueues when editor is active");
    CHALLENGE_CHECK(item.isAlive, "Telemetry item has isAlive == true");
    CHALLENGE_CHECK(item.energy > 0.0f, "Telemetry item has non-zero energy");

    // 5.3 Stale Telemetry Bug Eradication: reset() on editor reopen
    for (int i = 0; i < 15; ++i) {
        telemetry.push(item);
    }
    CHALLENGE_CHECK(telemetry.pop(item), "Items pushed into queue");

    // Reopen editor simulation: call reset()
    telemetry.reset();
    CHALLENGE_CHECK(!telemetry.pop(item), "All stale items purged after reset() - 0 items remaining");
}

int main() {
    std::cout << "======================================================\n";
    std::cout << "N8EFFECT CHALLENGER 2: AUDIO & BENCHMARK EMPIRICAL SUITE\n";
    std::cout << "======================================================\n";

    challengeSilenceGatingAndSnapshot();
    challengeMeasuredCycleReduction();
    challengeYinPitchAccuracy();
    challengeEventManagerIdleBypassBitExact();
    challengeTelemetryGating();

    std::cout << "\n======================================================\n";
    std::cout << "CHALLENGER 2 SUMMARY:\n";
    std::cout << "  Passed Checks: " << gPassedChallenges << "\n";
    std::cout << "  Failed Checks: " << gFailedChallenges << "\n";
    if (gFailedChallenges == 0) {
        std::cout << "VERDICT: APPROVE (All empirical challenges verified!)\n";
    } else {
        std::cerr << "VERDICT: REQUEST_CHANGES (" << gFailedChallenges << " challenges failed!)\n";
    }
    std::cout << "======================================================\n";

    return gFailedChallenges;
}
