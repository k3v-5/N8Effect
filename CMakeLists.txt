cmake_minimum_required(VERSION 3.22)

project(N8Effect VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==============================================================================
# Target de Pruebas Unitarias del Núcleo (Ultra-rápido, sin dependencias pesadas)
# ==============================================================================
add_executable(N8Effect_Tests
    tests/TestMain.cpp
    source/core/Types.h
    source/core/RealtimePools.h
    source/core/CpuProfiler.h
    source/graph/AudioProcessorNode.h
    source/graph/NodeFactory.h
    source/graph/Graph.h
    source/graph/GraphExecutor.h
    source/dsp/PassthroughNode.h
    source/dsp/SimpleFilterNode.h
    source/dsp/SimpleDelayNode.h
    source/dsp/core/DelayLine.h
    source/dsp/core/BiquadFilter.h
    source/dsp/core/EnvelopeDetector.h
    source/dsp/core/SaturationFunctions.h
    source/dsp/core/FastMath.h
    source/dsp/core/DenormalGuards.h
    source/dsp/core/TestSynthEngine.h
    source/dsp/core/FFTEngine.h
    source/dsp/core/SpatialPannerCore.h
    source/dsp/processors/CompressorNode.h
    source/dsp/processors/ParametricEQNode.h
    source/dsp/processors/DistortionNode.h
    source/dsp/processors/AdvancedDelayNode.h
    source/dsp/processors/ReverbNode.h
    source/dsp/processors/PitchShifterNode.h
    source/dsp/processors/SpectralProcessorNode.h
    source/dsp/core/GrainPool.h
    source/dsp/core/LinkwitzRileyFilter.h
    source/dsp/processors/GranularNode.h
    source/dsp/processors/SpectralFreezeNode.h
    source/dsp/processors/ResonatorBankNode.h
    source/dsp/processors/GlitchNode.h
    source/dsp/processors/MultibandDynamicsNode.h
    source/dsp/processors/PhaserNode.h
    source/dsp/processors/ChorusNode.h
    source/dsp/processors/FlangerNode.h
    source/dsp/processors/RingModulatorNode.h
    source/dsp/processors/FrequencyShifterNode.h
    source/dsp/processors/TapeSaturationNode.h
    source/dsp/processors/MidSideEncoderNode.h
    source/dsp/processors/MidSideDecoderNode.h
    source/dsp/processors/SpatialPannerNode.h
    source/graph/ContainerNode.h
    source/dsp/processors/FeedbackContainerNode.h
    source/dsp/processors/EventContainerNode.h
    source/event/EventTypes.h
    source/event/EventCaptureBuffer.h
    source/event/AudioFragment.h
    source/event/Event.h
    source/event/EventPool.h
    source/event/EventManager.h
    source/modulation/ModulationTypes.h
    source/modulation/LFO.h
    source/modulation/EnvelopeGenerator.h
    source/modulation/StepSequencer.h
    source/modulation/AudioFollower.h
    source/modulation/MacroManager.h
    source/modulation/ModulationMatrix.h
    source/modulation/ModulationEngine.h
    source/preset/GraphSerializer.h
    source/preset/SceneManager.h
    source/preset/GraphUndoManager.h
    source/preset/PresetManager.h
    source/analysis/TransientDetector.h
    source/analysis/PitchTracker.h
    source/analysis/SpectralFeatureExtractor.h
    source/analysis/AnalysisEngine.h
    source/plugin/DualWorldEngine.h
)

target_include_directories(N8Effect_Tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

if (MSVC)
    target_compile_options(N8Effect_Tests PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(N8Effect_Tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ==============================================================================
# JUCE 8 Framework (Descarga y descompresión directa ZIP sin usar Git - Regla 0)
# ==============================================================================
include(FetchContent)

message(STATUS "Configurando JUCE 8 via descarga de archivo ZIP directo (No Git)...")

FetchContent_Declare(
    juce
    URL "https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.4.zip"
)

FetchContent_MakeAvailable(juce)

# ==============================================================================
# Target Plugin Principal (VST3 & Standalone)
# ==============================================================================
juce_add_plugin(N8Effect
    COMPANY_NAME "N8Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE N8Au
    PLUGIN_CODE N8Ef
    FORMATS VST3 Standalone
    PRODUCT_NAME "Audio Event Graph Engine"
)

target_sources(N8Effect PRIVATE
    source/core/Types.h
    source/core/RealtimePools.h
    source/core/CpuProfiler.h
    source/graph/AudioProcessorNode.h
    source/graph/NodeFactory.h
    source/graph/Graph.h
    source/graph/GraphExecutor.h
    source/dsp/PassthroughNode.h
    source/dsp/SimpleFilterNode.h
    source/dsp/SimpleDelayNode.h
    source/dsp/core/DelayLine.h
    source/dsp/core/BiquadFilter.h
    source/dsp/core/EnvelopeDetector.h
    source/dsp/core/SaturationFunctions.h
    source/dsp/core/FastMath.h
    source/dsp/core/DenormalGuards.h
    source/dsp/core/TestSynthEngine.h
    source/dsp/core/FFTEngine.h
    source/dsp/core/SpatialPannerCore.h
    source/dsp/processors/CompressorNode.h
    source/dsp/processors/ParametricEQNode.h
    source/dsp/processors/DistortionNode.h
    source/dsp/processors/AdvancedDelayNode.h
    source/dsp/processors/ReverbNode.h
    source/dsp/processors/PitchShifterNode.h
    source/dsp/processors/SpectralProcessorNode.h
    source/dsp/core/GrainPool.h
    source/dsp/core/LinkwitzRileyFilter.h
    source/dsp/processors/GranularNode.h
    source/dsp/processors/SpectralFreezeNode.h
    source/dsp/processors/ResonatorBankNode.h
    source/dsp/processors/GlitchNode.h
    source/dsp/processors/MultibandDynamicsNode.h
    source/dsp/processors/PhaserNode.h
    source/dsp/processors/ChorusNode.h
    source/dsp/processors/FlangerNode.h
    source/dsp/processors/RingModulatorNode.h
    source/dsp/processors/FrequencyShifterNode.h
    source/dsp/processors/TapeSaturationNode.h
    source/dsp/processors/MidSideEncoderNode.h
    source/dsp/processors/MidSideDecoderNode.h
    source/dsp/processors/SpatialPannerNode.h
    source/graph/ContainerNode.h
    source/dsp/processors/FeedbackContainerNode.h
    source/dsp/processors/EventContainerNode.h
    source/event/EventTypes.h
    source/event/EventCaptureBuffer.h
    source/event/AudioFragment.h
    source/event/Event.h
    source/event/EventPool.h
    source/event/EventManager.h
    source/modulation/ModulationTypes.h
    source/modulation/LFO.h
    source/modulation/EnvelopeGenerator.h
    source/modulation/StepSequencer.h
    source/modulation/AudioFollower.h
    source/modulation/MacroManager.h
    source/modulation/ModulationMatrix.h
    source/modulation/ModulationEngine.h
    source/gui/WireRenderer.h
    source/gui/ModulationSlider.h
    source/gui/NodeComponent.h
    source/gui/SequentialSlotComponent.h
    source/gui/SequentialStripComponent.h
    source/gui/SequentialStripComponent.cpp
    source/gui/NodePaletteComponent.h
    source/gui/PresetBarComponent.h
    source/gui/PerformanceHudComponent.h
    source/gui/VirtualKeyboardComponent.h
    source/gui/GraphCanvasComponent.h
    source/gui/GraphCanvasComponent.cpp
    source/preset/GraphSerializer.h
    source/preset/SceneManager.h
    source/preset/GraphUndoManager.h
    source/preset/PresetManager.h
    source/analysis/TransientDetector.h
    source/analysis/PitchTracker.h
    source/analysis/SpectralFeatureExtractor.h
    source/analysis/AnalysisEngine.h
    source/plugin/DualWorldEngine.h
    source/plugin/PluginProcessor.h
    source/plugin/PluginProcessor.cpp
    source/plugin/PluginEditor.h
    source/plugin/PluginEditor.cpp
)

target_include_directories(N8Effect PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

target_compile_definitions(N8Effect PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(N8Effect PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if (MSVC)
    target_compile_options(N8Effect PRIVATE /utf-8)
endif()
