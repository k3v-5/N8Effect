#include "PluginEditor.h"

namespace audio_graph {

N8AudioProcessorEditor::N8AudioProcessorEditor(N8AudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      stripView_(p),
      canvas_(p),
      palette_(),
      virtualKeyboard_(p.getKeyboardState(), p)
{
    setResizable(true, true);
    setResizeLimits(850, 550, 1920, 1200);
    setSize(980, 680);

    // 1. Título y estado
    titleLabel_.setText("AUDIO EVENT GRAPH ENGINE", juce::dontSendNotification);
    titleLabel_.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel_);

    statusLabel_.setText("Graph: Compiled & Active | EventPool: 0 active / 1024 slots", juce::dontSendNotification);
    statusLabel_.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    statusLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff00d2ff));
    addAndMakeVisible(statusLabel_);

    // 2. Controles Maestros Dry / Wet
    drySlider_.setSliderStyle(juce::Slider::LinearBar);
    drySlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 18);
    dryLabel_.setText("DRY", juce::dontSendNotification);
    dryLabel_.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    dryLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff8c96a5));
    addAndMakeVisible(drySlider_);
    addAndMakeVisible(dryLabel_);

    dryAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "dry_level", drySlider_);

    wetSlider_.setSliderStyle(juce::Slider::LinearBar);
    wetSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 18);
    wetLabel_.setText("WET", juce::dontSendNotification);
    wetLabel_.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    wetLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff00d2ff));
    addAndMakeVisible(wetSlider_);
    addAndMakeVisible(wetLabel_);

    wetAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "wet_level", wetSlider_);

    // 3. Preset Bar & Escenas
    presetBar_.setPresetList(processorRef.getPresetManager().getFactoryPresetNames());

    presetBar_.setOnPresetSelected([this](int index) {
        PresetMetadata meta;
        std::array<float, 8> macros;
        if (processorRef.getPresetManager().loadFactoryPreset(static_cast<size_t>(index), processorRef.getGraph(), meta, macros)) {
            processorRef.recompilePlan();
            processorRef.getUndoManager().pushState(processorRef.getGraph(), meta, macros);
            canvas_.rebuildFromGraph();
            stripView_.rebuild();
        }
    });

    presetBar_.setOnCaptureSceneA([this]() {
        std::array<float, 8> macros{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
        processorRef.getSceneManager().captureSceneA(processorRef.getGraph(), macros);
    });

    presetBar_.setOnCaptureSceneB([this]() {
        std::array<float, 8> macros{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
        processorRef.getSceneManager().captureSceneB(processorRef.getGraph(), macros);
    });

    presetBar_.setOnMorphChanged([this](float t) {
        processorRef.getSceneManager().setMorphFactor(t);
        std::array<float, 8> macros{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
        processorRef.getSceneManager().updateAndApply(processorRef.getGraph(), macros, 1.0f);
        canvas_.repaint();
        stripView_.repaint();
    });

    presetBar_.setOnUndoRequested([this]() {
        PresetMetadata meta;
        std::array<float, 8> macros;
        if (processorRef.getUndoManager().undo(processorRef.getGraph(), meta, macros)) {
            processorRef.recompilePlan();
            canvas_.rebuildFromGraph();
            stripView_.rebuild();
        }
    });

    presetBar_.setOnRedoRequested([this]() {
        PresetMetadata meta;
        std::array<float, 8> macros;
        if (processorRef.getUndoManager().redo(processorRef.getGraph(), meta, macros)) {
            processorRef.recompilePlan();
            canvas_.rebuildFromGraph();
            stripView_.rebuild();
        }
    });

    addAndMakeVisible(presetBar_);

    // 4. Selector de Modo de Vista (Estilo Arturia Efx) y Teclado
    viewModeStripBtn_.setButtonText(juce::CharPointer_UTF8("\xE2\x89\xA1 COLA SECUENCIAL")); // ≡ COLA SECUENCIAL
    viewModeStripBtn_.onClick = [this]() { setViewMode(ViewMode::SequentialSlots); };
    addAndMakeVisible(viewModeStripBtn_);

    viewModeGraphBtn_.setButtonText(juce::CharPointer_UTF8("\xE2\x9A\x8D GRAFO MODULAR")); // ⚍ GRAFO MODULAR
    viewModeGraphBtn_.onClick = [this]() { setViewMode(ViewMode::ModularGraph); };
    addAndMakeVisible(viewModeGraphBtn_);

    toggleKeyboardBtn_.setButtonText(juce::CharPointer_UTF8("\xF0\x9F\x8E\xB9 TECLADO")); // 🎹 TECLADO
    toggleKeyboardBtn_.setClickingTogglesState(true);
    toggleKeyboardBtn_.setToggleState(isKeyboardVisible_, juce::dontSendNotification);
    toggleKeyboardBtn_.onClick = [this]() {
        setKeyboardVisible(toggleKeyboardBtn_.getToggleState());
    };
    addAndMakeVisible(toggleKeyboardBtn_);

    // 5. Paleta lateral y Vistas
    palette_.setOnAddNodeRequested([this](NodeType type) {
        static int spawnCount = 0;
        const float x = 80.0f + static_cast<float>((spawnCount % 5) * 40);
        const float y = 60.0f + static_cast<float>((spawnCount % 5) * 30);
        spawnCount++;
        canvas_.addNodeAtPosition(type, x, y);
        stripView_.rebuild();
    });

    addChildComponent(palette_);
    addChildComponent(canvas_);
    addAndMakeVisible(stripView_);

    // 6. Teclado Virtual de Audición (Reglas 1, 9, 23, 24)
    virtualKeyboard_.onCloseRequested = [this]() {
        setKeyboardVisible(false);
    };
    addAndMakeVisible(virtualKeyboard_);

    hud_.setOnResetOverload([this]() {
        processorRef.resetCpuOverload();
    });
    addAndMakeVisible(hud_);

    setViewMode(ViewMode::SequentialSlots);
    setKeyboardVisible(true);
    startTimerHz(30); // 30 fps para actualizaciones de telemetría sin locks
}

void N8AudioProcessorEditor::setKeyboardVisible(bool visible) {
    isKeyboardVisible_ = visible;
    toggleKeyboardBtn_.setToggleState(visible, juce::dontSendNotification);
    virtualKeyboard_.setVisible(visible);
    if (visible) {
        toggleKeyboardBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00d2ff));
        toggleKeyboardBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff090910));
    } else {
        toggleKeyboardBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181824));
        toggleKeyboardBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
    }
    resized();
}

void N8AudioProcessorEditor::setViewMode(ViewMode mode) {
    currentViewMode_ = mode;
    const bool isStrip = (mode == ViewMode::SequentialSlots);

    stripView_.setVisible(isStrip);
    canvas_.setVisible(!isStrip);
    palette_.setVisible(!isStrip);

    if (isStrip) {
        viewModeStripBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00d2ff));
        viewModeStripBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff090910));
        viewModeGraphBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181824));
        viewModeGraphBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        stripView_.rebuild();
    } else {
        viewModeStripBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181824));
        viewModeStripBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        viewModeGraphBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00d2ff));
        viewModeGraphBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff090910));
        canvas_.rebuildFromGraph();
    }
    resized();
}

N8AudioProcessorEditor::~N8AudioProcessorEditor() {
    stopTimer();
}

void N8AudioProcessorEditor::timerCallback() {
    const auto metrics = processorRef.getPerformanceMetrics();
    hud_.updateMetrics(metrics);

    juce::String status = "Graph: " + juce::String(static_cast<int>(metrics.totalNodes)) + " Nodes | " +
                          "EventPool: " + juce::String(static_cast<int>(metrics.activeEvents)) + " active / 1024 slots | 0 mallocs (Lock-Free)";
    statusLabel_.setText(status, juce::dontSendNotification);

    presetBar_.setUndoRedoEnabled(processorRef.getUndoManager().canUndo(),
                                  processorRef.getUndoManager().canRedo());
}

void N8AudioProcessorEditor::paint(juce::Graphics& g) {
    // Fondo de barra superior
    g.setColour(juce::Colour(0xff12121c));
    g.fillRect(0, 0, getWidth(), 84);

    // Divisor de barra superior
    g.setColour(juce::Colour(0xff222236));
    g.drawHorizontalLine(83, 0.0f, static_cast<float>(getWidth()));
}

void N8AudioProcessorEditor::resized() {
    auto area = getLocalBounds();

    // 1. Cabecera superior
    auto header = area.removeFromTop(56).reduced(12, 6);

    auto masterSlidersArea = header.removeFromRight(220);
    auto dryRow = masterSlidersArea.removeFromTop(20);
    dryLabel_.setBounds(dryRow.removeFromLeft(36));
    drySlider_.setBounds(dryRow);

    masterSlidersArea.removeFromTop(4);
    auto wetRow = masterSlidersArea.removeFromTop(20);
    wetLabel_.setBounds(wetRow.removeFromLeft(36));
    wetSlider_.setBounds(wetRow);

    header.removeFromRight(12);
    auto hudArea = header.removeFromRight(230);
    hud_.setBounds(hudArea);

    header.removeFromRight(12);
    titleLabel_.setBounds(header.removeFromTop(24));
    statusLabel_.setBounds(header.removeFromTop(18));

    // 2. Barra de Presets, Escenas, Selector de Vistas y Botón Teclado
    auto barArea = area.removeFromTop(28);
    auto modeSwitcherArea = barArea.removeFromRight(375);
    toggleKeyboardBtn_.setBounds(modeSwitcherArea.removeFromRight(100).reduced(2, 2));
    modeSwitcherArea.removeFromRight(6);
    viewModeStripBtn_.setBounds(modeSwitcherArea.removeFromLeft(132).reduced(2, 2));
    viewModeGraphBtn_.setBounds(modeSwitcherArea.reduced(2, 2));
    presetBar_.setBounds(barArea);

    // 3. Teclado Virtual en el borde inferior si está visible (alto: ~104px)
    if (isKeyboardVisible_) {
        virtualKeyboard_.setBounds(area.removeFromBottom(104).reduced(8, 2));
    }

    // 4. Área de Trabajo Principal (Dual View Arturia Efx)
    if (currentViewMode_ == ViewMode::SequentialSlots) {
        stripView_.setBounds(area);
    } else {
        palette_.setBounds(area.removeFromLeft(150));
        canvas_.setBounds(area);
    }
}

} // namespace audio_graph
