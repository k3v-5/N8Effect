#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>
#include <string>

namespace audio_graph {

/**
 * @brief Barra superior de control de Presets, Captura de Escenas, Morphing y Undo/Redo (Reglas 21, 22, 25).
 * Soporta navegación por 20 categorías temáticas con encabezados de sección ('addSectionHeading').
 */
class PresetBarComponent : public juce::Component {
public:
    struct PresetItem {
        std::string name;
        std::string category;
    };

    PresetBarComponent() {
        // 1. Selector de Presets con categorías y encabezados de sección
        presetCombo_.setTextWhenNothingSelected("Select Preset...");
        presetCombo_.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff181824));
        presetCombo_.setColour(juce::ComboBox::textColourId, juce::Colours::white);
        presetCombo_.onChange = [this]() {
            if (onPresetSelected_) {
                const int id = presetCombo_.getSelectedId();
                if (id > 0) {
                    onPresetSelected_(id - 1);
                }
            }
        };
        addAndMakeVisible(presetCombo_);

        prevBtn_.setButtonText("<");
        prevBtn_.onClick = [this]() {
            const int id = presetCombo_.getSelectedId();
            if (id > 1) {
                presetCombo_.setSelectedId(id - 1, juce::sendNotificationSync);
            }
        };
        addAndMakeVisible(prevBtn_);

        nextBtn_.setButtonText(">");
        nextBtn_.onClick = [this]() {
            const int id = presetCombo_.getSelectedId();
            if (id >= 1 && id < totalPresets_) {
                presetCombo_.setSelectedId(id + 1, juce::sendNotificationSync);
            }
        };
        addAndMakeVisible(nextBtn_);

        saveBtn_.setButtonText("Save");
        saveBtn_.onClick = [this]() {
            if (onSavePresetRequested_) {
                onSavePresetRequested_();
            }
        };
        addAndMakeVisible(saveBtn_);

        randomBtn_.setButtonText(juce::String::fromUTF8("🎲 Random"));
        randomBtn_.onClick = [this]() {
            if (onRandomizeRequested_) {
                onRandomizeRequested_(1);
            }
        };
        addAndMakeVisible(randomBtn_);

        // 2. Botones de Escena y Slider de Morphing
        captureABtn_.setButtonText("Cap A");
        captureABtn_.onClick = [this]() {
            if (onCaptureSceneARequested_) onCaptureSceneARequested_();
        };
        addAndMakeVisible(captureABtn_);

        captureBBtn_.setButtonText("Cap B");
        captureBBtn_.onClick = [this]() {
            if (onCaptureSceneBRequested_) onCaptureSceneBRequested_();
        };
        addAndMakeVisible(captureBBtn_);

        morphSlider_.setSliderStyle(juce::Slider::LinearBar);
        morphSlider_.setRange(0.0, 1.0, 0.01);
        morphSlider_.setValue(0.0);
        morphSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
        morphSlider_.onValueChange = [this]() {
            if (onMorphChanged_) {
                onMorphChanged_(static_cast<float>(morphSlider_.getValue()));
            }
        };
        addAndMakeVisible(morphSlider_);

        morphLabel_.setText("MORPH A-B", juce::dontSendNotification);
        morphLabel_.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        morphLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(morphLabel_);

        // 3. Botones Undo / Redo
        undoBtn_.setButtonText("Undo");
        undoBtn_.onClick = [this]() { if (onUndoRequested_) onUndoRequested_(); };
        addAndMakeVisible(undoBtn_);

        redoBtn_.setButtonText("Redo");
        redoBtn_.onClick = [this]() { if (onRedoRequested_) onRedoRequested_(); };
        addAndMakeVisible(redoBtn_);

        // 4. Importar / Exportar presets en disco (.n8preset)
        importBtn_.setButtonText("Import");
        importBtn_.onClick = [this]() { if (onImportPresetFileRequested_) onImportPresetFileRequested_(); };
        addAndMakeVisible(importBtn_);

        exportBtn_.setButtonText("Export");
        exportBtn_.onClick = [this]() { if (onExportPresetFileRequested_) onExportPresetFileRequested_(); };
        addAndMakeVisible(exportBtn_);

        // 5. Botón de Conmutación de Piano Visual
        pianoToggleBtn_.setButtonText("Piano: ON");
        pianoToggleBtn_.onClick = [this]() {
            isPianoVisible_ = !isPianoVisible_;
            pianoToggleBtn_.setButtonText(isPianoVisible_ ? "Piano: ON" : "Piano: OFF");
            if (onTogglePianoRequested_) onTogglePianoRequested_(isPianoVisible_);
        };
        addAndMakeVisible(pianoToggleBtn_);

        // 6. Botón de Conmutación del Carril de Secuenciador por Efecto
        seqToggleBtn_.setButtonText("Seq: ON");
        seqToggleBtn_.onClick = [this]() {
            isSeqVisible_ = !isSeqVisible_;
            seqToggleBtn_.setButtonText(isSeqVisible_ ? "Seq: ON" : "Seq: OFF");
            if (onToggleSeqRequested_) onToggleSeqRequested_(isSeqVisible_);
        };
        addAndMakeVisible(seqToggleBtn_);

        // 7. Botones de Conmutación de Visualizador y Macros
        visToggleBtn_.setButtonText("Vis: ON");
        visToggleBtn_.onClick = [this]() {
            isVisVisible_ = !isVisVisible_;
            visToggleBtn_.setButtonText(isVisVisible_ ? "Vis: ON" : "Vis: OFF");
            if (onToggleVisRequested_) onToggleVisRequested_(isVisVisible_);
        };
        addAndMakeVisible(visToggleBtn_);

        macrosToggleBtn_.setButtonText("Macros: ON");
        macrosToggleBtn_.onClick = [this]() {
            isMacrosVisible_ = !isMacrosVisible_;
            macrosToggleBtn_.setButtonText(isMacrosVisible_ ? "Macros: ON" : "Macros: OFF");
            if (onToggleMacrosRequested_) onToggleMacrosRequested_(isMacrosVisible_);
        };
        addAndMakeVisible(macrosToggleBtn_);
    }

    void setPresetList(const std::vector<PresetItem>& presets) {
        presetCombo_.clear(juce::dontSendNotification);
        totalPresets_ = static_cast<int>(presets.size());
        std::string currentCategory = "";

        for (int i = 0; i < totalPresets_; ++i) {
            if (!presets[i].category.empty() && presets[i].category != currentCategory) {
                currentCategory = presets[i].category;
                presetCombo_.addSectionHeading(currentCategory);
            }
            presetCombo_.addItem(presets[i].name, i + 1);
        }

        if (totalPresets_ > 0) {
            presetCombo_.setSelectedId(1, juce::dontSendNotification);
        }
    }

    void setPresetList(const std::vector<std::string>& presetNames) {
        presetCombo_.clear(juce::dontSendNotification);
        totalPresets_ = static_cast<int>(presetNames.size());
        for (int i = 0; i < totalPresets_; ++i) {
            presetCombo_.addItem(presetNames[i], i + 1);
        }
        if (totalPresets_ > 0) {
            presetCombo_.setSelectedId(1, juce::dontSendNotification);
        }
    }

    void setUndoRedoEnabled(bool canUndo, bool canRedo) {
        undoBtn_.setEnabled(canUndo);
        redoBtn_.setEnabled(canRedo);
    }

    void setPianoToggleState(bool isVisible) {
        isPianoVisible_ = isVisible;
        pianoToggleBtn_.setButtonText(isPianoVisible_ ? "Piano: ON" : "Piano: OFF");
    }

    void setSeqToggleState(bool isVisible) {
        isSeqVisible_ = isVisible;
        seqToggleBtn_.setButtonText(isSeqVisible_ ? "Seq: ON" : "Seq: OFF");
    }

    void setOnPresetSelected(std::function<void(int)> cb) { onPresetSelected_ = std::move(cb); }
    void setOnSavePresetRequested(std::function<void()> cb) { onSavePresetRequested_ = std::move(cb); }
    void setOnCaptureSceneA(std::function<void()> cb) { onCaptureSceneARequested_ = std::move(cb); }
    void setOnCaptureSceneB(std::function<void()> cb) { onCaptureSceneBRequested_ = std::move(cb); }
    void setOnMorphChanged(std::function<void(float)> cb) { onMorphChanged_ = std::move(cb); }
    void setOnUndoRequested(std::function<void()> cb) { onUndoRequested_ = std::move(cb); }
    void setOnRedoRequested(std::function<void()> cb) { onRedoRequested_ = std::move(cb); }
    void setOnImportPresetFileRequested(std::function<void()> cb) { onImportPresetFileRequested_ = std::move(cb); }
    void setOnExportPresetFileRequested(std::function<void()> cb) { onExportPresetFileRequested_ = std::move(cb); }
    void setOnTogglePianoRequested(std::function<void(bool)> cb) { onTogglePianoRequested_ = std::move(cb); }
    void setOnToggleSeqRequested(std::function<void(bool)> cb) { onToggleSeqRequested_ = std::move(cb); }
    void setOnToggleVisualizerRequested(std::function<void(bool)> cb) { onToggleVisRequested_ = std::move(cb); }
    void setOnToggleMacrosRequested(std::function<void(bool)> cb) { onToggleMacrosRequested_ = std::move(cb); }
    void setOnRandomizeRequested(std::function<void(int)> cb) { onRandomizeRequested_ = std::move(cb); }

    void paint(juce::Graphics& g) override {
        g.setColour(juce::Colour(0xff000000));
        g.fillRect(getLocalBounds());

        // Línea divisoria inferior blanca nítida
        g.setColour(juce::Colours::white);
        g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));
    }

    void resized() override {
        auto area = getLocalBounds().reduced(4, 2);

        // Botón derecho: Macros, Vis, Seq, Piano y Export/Import
        macrosToggleBtn_.setBounds(area.removeFromRight(76));
        area.removeFromRight(4);
        visToggleBtn_.setBounds(area.removeFromRight(66));
        area.removeFromRight(4);
        seqToggleBtn_.setBounds(area.removeFromRight(66));
        area.removeFromRight(4);
        pianoToggleBtn_.setBounds(area.removeFromRight(76));
        area.removeFromRight(6);
        exportBtn_.setBounds(area.removeFromRight(46));
        area.removeFromRight(2);
        importBtn_.setBounds(area.removeFromRight(46));
        area.removeFromRight(8);

        // Undo / Redo
        undoBtn_.setBounds(area.removeFromLeft(44));
        area.removeFromLeft(2);
        redoBtn_.setBounds(area.removeFromLeft(44));
        area.removeFromLeft(8);

        // Selector y botones de preset (ancho ampliado para categorías y títulos creativos)
        prevBtn_.setBounds(area.removeFromLeft(22));
        area.removeFromLeft(2);
        presetCombo_.setBounds(area.removeFromLeft(180));
        area.removeFromLeft(2);
        nextBtn_.setBounds(area.removeFromLeft(22));
        area.removeFromLeft(4);
        saveBtn_.setBounds(area.removeFromLeft(42));
        area.removeFromLeft(4);
        randomBtn_.setBounds(area.removeFromLeft(70));
        area.removeFromLeft(10);

        // Escenas y Morph
        captureABtn_.setBounds(area.removeFromLeft(46));
        area.removeFromLeft(4);
        morphLabel_.setBounds(area.removeFromLeft(64));
        morphSlider_.setBounds(area.removeFromLeft(95));
        area.removeFromLeft(4);
        captureBBtn_.setBounds(area.removeFromLeft(46));
    }

private:
    juce::ComboBox presetCombo_;
    juce::TextButton prevBtn_;
    juce::TextButton nextBtn_;
    juce::TextButton saveBtn_;
    juce::TextButton randomBtn_;

    juce::TextButton captureABtn_;
    juce::TextButton captureBBtn_;
    juce::Slider morphSlider_;
    juce::Label morphLabel_;

    juce::TextButton undoBtn_;
    juce::TextButton redoBtn_;

    juce::TextButton importBtn_;
    juce::TextButton exportBtn_;
    juce::TextButton pianoToggleBtn_;
    juce::TextButton seqToggleBtn_;
    juce::TextButton visToggleBtn_;
    juce::TextButton macrosToggleBtn_;

    bool isPianoVisible_{ true };
    bool isSeqVisible_{ true };
    bool isVisVisible_{ true };
    bool isMacrosVisible_{ true };
    int totalPresets_{ 0 };

    std::function<void(int)> onPresetSelected_;
    std::function<void()> onSavePresetRequested_;
    std::function<void()> onCaptureSceneARequested_;
    std::function<void()> onCaptureSceneBRequested_;
    std::function<void(float)> onMorphChanged_;
    std::function<void()> onUndoRequested_;
    std::function<void()> onRedoRequested_;
    std::function<void()> onImportPresetFileRequested_;
    std::function<void()> onExportPresetFileRequested_;
    std::function<void(bool)> onTogglePianoRequested_;
    std::function<void(bool)> onToggleSeqRequested_;
    std::function<void(bool)> onToggleVisRequested_;
    std::function<void(bool)> onToggleMacrosRequested_;
    std::function<void(int)> onRandomizeRequested_;
};

} // namespace audio_graph
