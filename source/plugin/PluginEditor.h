#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "../gui/GraphCanvasComponent.h"
#include "../gui/SequentialStripComponent.h"
#include "../gui/NodePaletteComponent.h"
#include "../gui/PresetBarComponent.h"
#include "../gui/PerformanceHudComponent.h"
#include "../gui/VirtualKeyboardComponent.h"

namespace audio_graph {

class N8AudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    enum class ViewMode {
        SequentialSlots,
        ModularGraph
    };

    explicit N8AudioProcessorEditor(N8AudioProcessor&);
    ~N8AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void setViewMode(ViewMode mode);
    void setKeyboardVisible(bool visible);

private:
    void timerCallback() override;

    N8AudioProcessor& processorRef;

    ViewMode currentViewMode_{ ViewMode::SequentialSlots };
    bool isKeyboardVisible_{ true };

    // Subcomponentes de Vistas
    SequentialStripComponent stripView_;
    GraphCanvasComponent canvas_;
    NodePaletteComponent palette_;
    PresetBarComponent presetBar_;
    PerformanceHudComponent hud_;
    VirtualKeyboardComponent virtualKeyboard_;

    // Selector de Modo de Vista (Estilo Arturia Efx) y Teclado
    juce::TextButton viewModeStripBtn_;
    juce::TextButton viewModeGraphBtn_;
    juce::TextButton toggleKeyboardBtn_;

    // Cabecera Master
    juce::Label titleLabel_;
    juce::Label statusLabel_;
    juce::Slider drySlider_;
    juce::Slider wetSlider_;
    juce::Label dryLabel_;
    juce::Label wetLabel_;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryAttachment_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wetAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(N8AudioProcessorEditor)
};

} // namespace audio_graph
