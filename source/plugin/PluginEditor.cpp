#include "PluginEditor.h"

namespace audio_graph {

N8AudioProcessorEditor::N8AudioProcessorEditor(N8AudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      palette_(),
      canvas_(p),
      visualizer_(p),
      macroDashboard_(p),
      presetDrawer_(p)
{
    // Asegurar que el modo Standalone tenga siempre la entrada desmuteada (Regla 1 y 17)
    {
        juce::PropertiesFile::Options opts;
        opts.applicationName = "Audio Event Graph Engine";
        opts.filenameSuffix = ".settings";
        opts.osxLibrarySubFolder = "Application Support";
        opts.folderName = "";

        juce::ApplicationProperties appProps;
        appProps.setStorageParameters(opts);
        if (auto* userSettings = appProps.getUserSettings()) {
            userSettings->setValue("shouldMuteInput", false);
            userSettings->saveIfNeeded();
        }
    }

    setResizable(true, true);
    setResizeLimits(900, 560, 1920, 1200);
    setSize(1200, 840);

    setLookAndFeel(&lookAndFeel_);
    ThemeManager::getInstance().applyToLookAndFeel(lookAndFeel_);
    ThemeManager::getInstance().addListener(this);

    // 1. Título Minimalista y Botón de Configuración
    titleLabel_.setText("N8 EFFECT", juce::dontSendNotification);
    titleLabel_.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel_);

    configBtn_.setButtonText("CONFIG");
    configBtn_.onClick = [this]() {
        isConfigModalVisible_ = !isConfigModalVisible_;
        configModal_.setVisible(isConfigModalVisible_);
        if (isConfigModalVisible_) {
            configModal_.toFront(true);
            const auto metrics = processorRef.getPerformanceMetrics();
            configModal_.setMetrics(metrics,
                                    static_cast<int>(processorRef.getGraph().getNodes().size()),
                                    processorRef.getSampleRate(),
                                    processorRef.getBlockSize());
        }
    };
    addAndMakeVisible(configBtn_);

    // 2. Controles Maestros Dry / Wet
    drySlider_.setSliderStyle(juce::Slider::LinearBar);
    drySlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 18);
    dryLabel_.setText("DRY", juce::dontSendNotification);
    dryLabel_.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    dryLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(drySlider_);
    addAndMakeVisible(dryLabel_);

    dryAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "dry_level", drySlider_);

    wetSlider_.setSliderStyle(juce::Slider::LinearBar);
    wetSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 18);
    wetLabel_.setText("WET", juce::dontSendNotification);
    wetLabel_.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    wetLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(wetSlider_);
    addAndMakeVisible(wetLabel_);

    wetAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "wet_level", wetSlider_);

    // 3. Preset Bar & Escenas
    std::vector<PresetBarComponent::PresetItem> presetItems;
    const auto& factoryPresets = processorRef.getPresetManager().getFactoryPresets();
    presetItems.reserve(factoryPresets.size());
    for (const auto& entry : factoryPresets) {
        presetItems.push_back({ entry.name, entry.category });
    }
    presetBar_.setPresetList(presetItems);

    presetBar_.setOnPresetSelected([this](int index) {
        if (processorRef.loadFactoryPreset(static_cast<size_t>(index))) {
            canvas_.rebuildFromGraph();
            macroDashboard_.updateKnobValues();
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
    });

    presetBar_.setOnUndoRequested([this]() {
        PresetMetadata meta;
        std::array<float, 8> macros;
        if (processorRef.undo(meta, macros)) {
            canvas_.rebuildFromGraph();
        }
    });

    presetBar_.setOnRedoRequested([this]() {
        PresetMetadata meta;
        std::array<float, 8> macros;
        if (processorRef.redo(meta, macros)) {
            canvas_.rebuildFromGraph();
        }
    });

    presetBar_.setOnTogglePianoRequested([this](bool vis) {
        isPianoVisible_ = vis;
        piano_.setVisible(vis);
        resized();
    });

    presetBar_.setOnBrowseRequested([this]() {
        isPresetDrawerVisible_ = !isPresetDrawerVisible_;
        presetDrawer_.setVisible(isPresetDrawerVisible_);
        if (isPresetDrawerVisible_) {
            presetDrawer_.reloadPresets();
            presetDrawer_.toFront(true);
        }
        resized();
    });

    presetDrawer_.setOnPresetLoaded([this]() {
        canvas_.rebuildFromGraph();
        auto name = presetDrawer_.getSelectedPresetName();
        auto cat = presetDrawer_.getSelectedPresetCategory();
        presetBar_.setCurrentPreset(name, cat);
    });

    presetDrawer_.setOnCloseRequested([this]() {
        isPresetDrawerVisible_ = false;
        resized();
    });

    addChildComponent(presetDrawer_);

    presetBar_.setOnRandomizeRequested([this](int modeIdx) {
        auto mode = static_cast<SmartRandomizer::RandomMode>(std::clamp(modeIdx, 0, 2));
        if (processorRef.randomizeGraph(mode)) {
            canvas_.rebuildFromGraph();
        }
    });

    presetBar_.setOnExportPresetFileRequested([this]() {
        fileChooser_ = std::make_unique<juce::FileChooser>(
            "Export N8 Preset File",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.n8preset");

        const auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting;
        fileChooser_->launchAsync(flags, [this](const juce::FileChooser& fc) {
            const auto file = fc.getResult();
            if (file != juce::File{}) {
                PresetMetadata meta{
                    .schemaVersion = GraphSerializer::CurrentSchemaVersion,
                    .name = file.getFileNameWithoutExtension().toStdString(),
                    .author = "User",
                    .category = "User",
                    .description = "Exported N8Effect Preset",
                    .dryLevel = processorRef.getDualWorldEngine().getDryLevel(),
                    .wetLevel = processorRef.getDualWorldEngine().getWetLevel()
                };
                std::array<float, 8> macros{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
                std::string json = GraphSerializer::serialize(processorRef.getGraph(), meta, macros);
                file.replaceWithText(json);
            }
        });
    });

    presetBar_.setOnImportPresetFileRequested([this]() {
        fileChooser_ = std::make_unique<juce::FileChooser>(
            "Import N8 Preset File",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.n8preset");

        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser_->launchAsync(flags, [this](const juce::FileChooser& fc) {
            const auto file = fc.getResult();
            if (file.existsAsFile()) {
                std::string json = file.loadFileAsString().toStdString();
                if (processorRef.loadPresetFromJson(json)) {
                    canvas_.rebuildFromGraph();
                }
            }
        });
    });

    addAndMakeVisible(presetBar_);

    // 4. Conexión de la paleta con el canvas
    palette_.setOnAddNodeRequested([this](NodeType type) {
        static int spawnCount = 0;
        const float x = 80.0f + static_cast<float>((spawnCount % 5) * 40);
        const float y = 60.0f + static_cast<float>((spawnCount % 5) * 30);
        spawnCount++;
        canvas_.addNodeAtPosition(type, x, y);
    });

    addAndMakeVisible(palette_);
    addAndMakeVisible(canvas_);

    // Conectar selección de nodo en el canvas con el secuenciador contextual, telemetría probe y dashboard de macros
    canvas_.setOnNodeSelected([this](NodeId id) {
        sequencerLane_.setTargetNode(id, processorRef.getGraph().getNode(id));
        processorRef.setProbeNodeId(id);
        macroDashboard_.setSelectedNodeId(id);
    });

    canvas_.setOnSampleLoaded([this]() {
        piano_.setTimbreMode(TestTimbreMode::LoadedSample);
        if (!isPianoVisible_) {
            isPianoVisible_ = true;
            presetBar_.setPianoToggleState(true);
            piano_.setVisible(true);
            resized();
        }
    });

    // 5. Teclado de Piano Visual de Prueba para Efectos
    piano_.setOnNoteOn([this](int note, float vel) {
        processorRef.getTestSynthesizer().noteOn(note, vel);
    });
    piano_.setOnNoteOff([this](int note) {
        processorRef.getTestSynthesizer().noteOff(note);
    });
    piano_.setOnTimbreChanged([this](TestTimbreMode mode) {
        processorRef.getTestSynthesizer().setTimbreMode(mode);
    });
    piano_.setOnCloseRequested([this]() {
        isPianoVisible_ = false;
        presetBar_.setPianoToggleState(false);
        piano_.setVisible(false);
        resized();
    });
    addAndMakeVisible(piano_);

    // 6. Carril de Secuenciación de Automatización por Efecto
    sequencerLane_.setOnCloseRequested([this]() {
        isSeqVisible_ = false;
        presetBar_.setSeqToggleState(false);
        sequencerLane_.setVisible(false);
        resized();
    });
    presetBar_.setOnToggleSeqRequested([this](bool vis) {
        isSeqVisible_ = vis;
        sequencerLane_.setVisible(vis);
        resized();
    });
    addAndMakeVisible(sequencerLane_);

    // 7. Visualizador de Audio y Espectrograma en Tiempo Real
    presetBar_.setOnToggleVisualizerRequested([this](bool vis) {
        isVisVisible_ = vis;
        visualizer_.setVisible(vis);
        resized();
    });
    addAndMakeVisible(visualizer_);

    // 8. Dashboard de 8 Performance Macros Globales
    presetBar_.setOnToggleMacrosRequested([this](bool vis) {
        isMacrosVisible_ = vis;
        macroDashboard_.setVisible(vis);
        resized();
    });
    addAndMakeVisible(macroDashboard_);

    hud_.setOnResetOverload([this]() {
        processorRef.resetCpuOverload();
    });
    addAndMakeVisible(hud_);

    configModal_.setOnCloseRequested([this]() {
        isConfigModalVisible_ = false;
        configModal_.setVisible(false);
    });
    configModal_.setOnToggleHud([this](bool vis) {
        isHudVisible_ = vis;
        hud_.setVisible(vis);
        resized();
    });
    configModal_.setOnAudioSettingsRequested([this]() {
        // En Standalone, disparar el diálogo nativo de configuración de audio de JUCE
        if (auto* top = getTopLevelComponent()) {
            std::function<bool(juce::Component*)> findAndClick = [&](juce::Component* comp) -> bool {
                if (comp == nullptr) return false;
                if (auto* btn = dynamic_cast<juce::Button*>(comp)) {
                    if (btn != &configBtn_ && btn->findParentComponentOfClass<juce::AudioProcessorEditor>() == nullptr) {
                        btn->triggerClick();
                        return true;
                    }
                }
                for (int i = 0; i < comp->getNumChildComponents(); ++i) {
                    if (findAndClick(comp->getChildComponent(i))) return true;
                }
                return false;
            };
            findAndClick(top);
        }
    });
    configModal_.setVisible(false);
    addChildComponent(configModal_);

    // Inicializar Aceleración por Hardware GPU con OpenGL 3.3+ (Reglas 13, 21, 23)
    openGLContext_ = std::make_unique<juce::OpenGLContext>();
    openGLContext_->setSwapInterval(1); // Sincronizado a VSync (60 FPS estables)
    openGLContext_->attachTo(*this);

    startTimerHz(30); // 30 fps para actualizaciones de telemetría sin locks
}

N8AudioProcessorEditor::~N8AudioProcessorEditor() {
    if (openGLContext_ != nullptr) {
        openGLContext_->detach();
        openGLContext_.reset();
    }
    ThemeManager::getInstance().removeListener(this);
    stopTimer();
    setLookAndFeel(nullptr);
}

void N8AudioProcessorEditor::themeChanged(const ThemeColors& /*newTheme*/, ThemePreset /*preset*/) {
    ThemeManager::getInstance().applyToLookAndFeel(lookAndFeel_);
    repaint();
    canvas_.repaint();
    palette_.repaint();
    presetBar_.repaint();
    visualizer_.repaint();
    macroDashboard_.repaint();
}

void N8AudioProcessorEditor::parentHierarchyChanged() {
    juce::AudioProcessorEditor::parentHierarchyChanged();

    // En Standalone, ocultar y colapsar cualquier barra de notificación del contenedor padre
    if (auto* parent = getParentComponent()) {
        for (int i = 0; i < parent->getNumChildComponents(); ++i) {
            auto* child = parent->getChildComponent(i);
            if (child != this) {
                child->setVisible(false);
                child->setBounds(0, 0, 0, 0);
            }
        }
    }
}

void N8AudioProcessorEditor::timerCallback() {
    // Si algún banner o notificación del wrapper intentara aparecer, asegurar que se mantenga invisible
    if (auto* parent = getParentComponent()) {
        for (int i = 0; i < parent->getNumChildComponents(); ++i) {
            auto* child = parent->getChildComponent(i);
            if (child != this && child->isVisible()) {
                child->setVisible(false);
                child->setBounds(0, 0, 0, 0);
            }
        }
    }

    const auto metrics = processorRef.getPerformanceMetrics();
    hud_.updateMetrics(metrics);
    sequencerLane_.updatePlayhead();

    if (isVisVisible_) {
        visualizer_.updateTelemetry();
    }
    if (isMacrosVisible_) {
        macroDashboard_.updateKnobValues();
    }

    if (isConfigModalVisible_) {
        configModal_.setMetrics(metrics,
                                static_cast<int>(processorRef.getGraph().getNodes().size()),
                                processorRef.getSampleRate(),
                                processorRef.getBlockSize());
    }

    presetBar_.setUndoRedoEnabled(processorRef.getUndoManager().canUndo(),
                                  processorRef.getUndoManager().canRedo());
}

void N8AudioProcessorEditor::paint(juce::Graphics& g) {
    const auto& theme = ThemeManager::getInstance().getColors();
    // Fondo de la barra superior según tema activo
    g.setColour(theme.headerDark);
    g.fillRect(0, 0, getWidth(), 88);

    // Divisor de barra superior
    g.setColour(theme.borderMuted);
    g.drawHorizontalLine(87, 0.0f, static_cast<float>(getWidth()));
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
    if (isHudVisible_) {
        auto hudArea = header.removeFromRight(230);
        hud_.setBounds(hudArea);
        header.removeFromRight(12);
    }

    configBtn_.setBounds(header.removeFromRight(70).withSizeKeepingCentre(66, 24));
    header.removeFromRight(10);

    titleLabel_.setBounds(header.removeFromTop(32));

    // 2. Barra de Presets y Escenas (estilo Arturia / FLEX)
    presetBar_.setBounds(area.removeFromTop(32));

    // 3. Piano visual en la parte inferior si está activo
    if (isPianoVisible_) {
        piano_.setBounds(area.removeFromBottom(84));
    }

    // 4. Carril de Secuenciación por Efecto si está activo
    if (isSeqVisible_) {
        sequencerLane_.setBounds(area.removeFromBottom(115));
    }

    // 4.1 Visualizador de Audio y Espectrograma si está activo
    if (isVisVisible_) {
        visualizer_.setBounds(area.removeFromBottom(125));
    }

    // 4.2 Dashboard de 8 Macros de Rendimiento reacomodado en el dock inferior (minimalista 42px)
    if (isMacrosVisible_) {
        macroDashboard_.setBounds(area.removeFromBottom(42));
    }

    // 5. Barra lateral de paleta de módulos (espaciosa 195px para tarjetas categorizadas y buscador)
    palette_.setBounds(area.removeFromLeft(195));

    // 6. Canvas del grafo DAG
    canvas_.setBounds(area);

    // 7. Modal de configuración cubre toda la ventana cuando está activo
    configModal_.setBounds(getLocalBounds());

    // 8. Drawer de Presets de Usuario y Fábrica
    if (isPresetDrawerVisible_) {
        presetDrawer_.setBounds(getLocalBounds().reduced(48, 32));
        presetDrawer_.toFront(true);
    }
}

} // namespace audio_graph
