#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "../gui/GraphCanvasComponent.h"
#include "../gui/NodePaletteComponent.h"
#include "../gui/PresetBarComponent.h"
#include "../gui/PerformanceHudComponent.h"
#include "../gui/MinimalistLookAndFeel.h"

#include "../gui/VirtualPianoComponent.h"
#include "../gui/EngineConfigModalComponent.h"
#include "../gui/NodeAutomationSequencerComponent.h"
#include "../gui/AudioVisualizerComponent.h"
#include "../gui/MacroDashboardComponent.h"

namespace audio_graph {

class N8AudioProcessorEditor : public juce::AudioProcessorEditor,
                               public juce::DragAndDropContainer,
                               private juce::Timer {
public:
    explicit N8AudioProcessorEditor(N8AudioProcessor&);
    ~N8AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    void timerCallback() override;

    N8AudioProcessor& processorRef;
    MinimalistLookAndFeel lookAndFeel_;

    // Subcomponentes del Graph Editor
    PresetBarComponent presetBar_;
    NodePaletteComponent palette_;
    GraphCanvasComponent canvas_;
    PerformanceHudComponent hud_;
    VirtualPianoComponent piano_;
    NodeAutomationSequencerComponent sequencerLane_;
    AudioVisualizerComponent visualizer_;
    MacroDashboardComponent macroDashboard_;
    EngineConfigModalComponent configModal_;

    bool isPianoVisible_{ true };
    bool isSeqVisible_{ true };
    bool isVisVisible_{ true };
    bool isMacrosVisible_{ true };
    bool isHudVisible_{ true };
    bool isConfigModalVisible_{ false };
    std::unique_ptr<juce::FileChooser> fileChooser_;

    // Cabecera Master
    juce::Label titleLabel_;
    juce::TextButton configBtn_;
    juce::Slider drySlider_;
    juce::Slider wetSlider_;
    juce::Label dryLabel_;
    juce::Label wetLabel_;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wetAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(N8AudioProcessorEditor)
};

} // namespace audio_graph
