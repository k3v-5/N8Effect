# Project: N8Effect Extreme CPU & Multi-Instance Performance Optimization

## Architecture
Modular audio engine with decoupled Dual-World processing (Dry Signal vs Wet/Event World), dynamic Graph Engine with work-stealing multithreading, Universal Modulation, and real-time GUI telemetry.
The optimization architecture replaces thread spin-waiting with C++20 passive synchronization, introduces 3 adaptive concurrency modes, adds an ultra-fast silence gate (< -70 dB RMS) to analysis, vectorizes YIN pitch tracking via AVX2 FMA, bypasses idle event rendering and scratch buffers, and pauses audio-thread telemetry when the GUI editor is inactive.

## Feature Inventory
| # | Feature | Description | Milestone | Source |
|---|---------|-------------|-----------|--------|
| 1 | Scheduler Zero-Cost Passive Wait | Replace `_mm_pause()` in `WorkStealingGraphScheduler` with C++20 `std::atomic<uint64_t>::wait` / `notify_all` for 0.00% idle CPU | M1 | ORIGINAL_REQUEST R1 |
| 2 | 3 Concurrency Modes | Implement `SingleThreaded`, `SmartMultithreaded` (<=4 single, >4 multi), and `AlwaysMultithreaded` in `GraphExecutor` & `DualWorldEngine` | M1 | ORIGINAL_REQUEST R2 |
| 3 | Modal UI Concurrency Selector | Add 3-button segmented mode selector with ThemeManager styling in `EngineConfigModalComponent` and APVTS persistence | M1 | ORIGINAL_REQUEST R2 |
| 4 | AnalysisEngine Silence Gating | Bypass YIN and FFT when RMS < -70 dB (0.0003), retaining resting pitch/centroid snapshot without modulation clicks | M2 | ORIGINAL_REQUEST R3 |
| 5 | YIN AVX2 FMA Vectorization | Vectorize difference equation via `__m256` FMA and progressive CMNDF early valley exit (>70% cycle reduction) in `PitchTracker` | M2 | ORIGINAL_REQUEST R3 |
| 6 | Event Engine Idle Bypass | Early-return from `EventManager::render` and skip `eventBuffer_` copy in `DualWorldEngine` when active events are 0 | M2 | ORIGINAL_REQUEST R4 |
| 7 | EventCaptureBuffer memcpy Opt | Replace per-sample `% capacitySamples_` integer divisions with 1-2 `memcpy` chunks | M2 | ORIGINAL_REQUEST R4 |
| 8 | Telemetry Inactivity Gating | Introduce `editorActive_` flag to skip audio-thread telemetry generation when editor is closed/hidden | M3 | ORIGINAL_REQUEST R5 |
| 9 | Stale Telemetry Bug Eradication | Reset `EventTelemetryBuffer` upon editor reopen to prevent obsolete event burst display | M3 | ORIGINAL_REQUEST R5 |
| 10 | Editor Timer Inactivity Pause | Hook `visibilityChanged()` / `isShowing()` in `PluginEditor` to stop 30 Hz FFT/Waterfall timer when window is minimized/hidden | M3 | ORIGINAL_REQUEST R5 |
| 11 | Unit Tests & CPU Benchmarks | Add 5 new tests/benchmarks in `tests/TestMain.cpp` verifying concurrency modes, passive wait, silence gating, YIN speedup, and idle bypass | M4 | ORIGINAL_REQUEST R6 |
| 12 | Release Build & VST3 Deployment | Compile Release binaries (Tests, Standalone, VST3) and deploy VST3 bundle to `C:\Users\kevin.garrido\AppData\Local\Programs\Common\VST3\` | M4 | ORIGINAL_REQUEST R6 |

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| M1 | Concurrency & Scheduler Optimization | Features 1, 2, 3: C++20 passive wait in `WorkStealingGraphScheduler`, 3 modes in `GraphExecutor`/`DualWorldEngine`, UI in `EngineConfigModalComponent` | none | DONE |
| M2 | DSP Analysis & Event Engine Optimization | Features 4, 5, 6, 7: Silence gating in `AnalysisEngine`, AVX2 YIN in `PitchTracker`, idle event bypass in `EventManager`/`DualWorldEngine`, `EventCaptureBuffer` memcpy | none | IN_PROGRESS |
| M3 | Telemetry & Multi-Instance GUI Inactivity | Features 8, 9, 10: `editorActive_` gating, `EventTelemetryBuffer` reset on reopen, `PluginEditor` visibility hooks | M1 | PLANNED |
| M4 | Test Suite, Benchmarks & Release Deployment | Features 11, 12: 5 new tests/benchmarks in `TestMain.cpp`, 100% pass (121 tests), Release build, VST3 deployment | M1, M2, M3 | PLANNED |

## Interface Contracts
### `WorkStealingGraphScheduler` ↔ `GraphExecutor`
- `activeEpoch_`: `std::atomic<uint64_t>` incremented on each `executePlan()` and `stopWorkers()`.
- Workers wait on `activeEpoch_.wait(lastSeenEpoch)` with 0 CPU consumption when idle.
- Audio thread calls `activeEpoch_.notify_all()` upon dispatching work.

### `GraphExecutor` ↔ `DualWorldEngine` ↔ `EngineConfigModalComponent`
- `enum class ConcurrencyMode : uint8_t { SingleThreaded = 0, SmartMultithreaded = 1, AlwaysMultithreaded = 2 };` defined in `source/core/Types.h`.
- `GraphExecutor::setConcurrencyMode(ConcurrencyMode mode) noexcept;`
- `DualWorldEngine::setConcurrencyMode(ConcurrencyMode mode) noexcept;`
- `N8AudioProcessor::setConcurrencyMode(ConcurrencyMode mode) noexcept;`

### `AnalysisEngine` ↔ `PitchTracker` ↔ `SpectralFeatureExtractor`
- `AnalysisEngine::process()` calculates block RMS. If `< 0.0003f` (-70.45 dBFS), bypasses `pitchTracker_.process()` and `spectralExtractor_.process()`, maintaining last valid `pitchHz` and `spectralCentroid`, setting `clarity = 0.0f` and `energy = 0.0f`.

### `EventManager` ↔ `DualWorldEngine`
- `EventManager::getActiveEventCount()` checked before `eventBuffer_.copyFrom`.
- If 0 active events, `eventBuffer_` copy and `render()` zero-filling are bypassed.

### `N8AudioProcessor` ↔ `PluginEditor` ↔ Telemetry
- `std::atomic<bool> editorActive_{ false };` in `N8AudioProcessor`.
- Checked in `PluginProcessor::processBlock` and `DualWorldEngine::process` before writing to `visualizerBuffer_` or `eventManager_.getTelemetryBuffer()`.
- Editor constructor and `visibilityChanged()` set `editorActive_` and manage `startTimerHz(30)` / `stopTimer()`.

## Code Layout
- `source/core/Types.h` — `ConcurrencyMode` enum definition.
- `source/graph/WorkStealingGraphScheduler.h` — Worker thread passive synchronization.
- `source/graph/GraphExecutor.h` — 3-mode concurrency execution logic.
- `source/plugin/DualWorldEngine.h` — Concurrency mode passthrough, idle event bypass, telemetry gating.
- `source/plugin/PluginProcessor.h` / `.cpp` — APVTS concurrency parameter, `editorActive_` flag, visualizer write gating.
- `source/gui/EngineConfigModalComponent.h` / `.cpp` — 3-button concurrency UI selector.
- `source/gui/PluginEditor.h` / `.cpp` — Editor visibility listener, telemetry reset, timer pause.
- `source/analysis/AnalysisEngine.h` — Silence threshold RMS gating and snapshot preservation.
- `source/analysis/PitchTracker.h` — AVX2 FMA vectorization and progressive CMNDF early termination.
- `source/analysis/SpectralFeatureExtractor.h` — FFT execution bypass on silence.
- `source/event/EventManager.h` — Early return on 0 active events, telemetry write gating.
- `source/event/EventCaptureBuffer.h` — Chunked memcpy writing.
- `source/event/EventTelemetryBuffer.h` — `reset()` method for clearing stale queue items.
- `tests/TestMain.cpp` — 5 new unit tests and nanosecond benchmark suites.
