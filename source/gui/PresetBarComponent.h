#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>
#include <string>
#include <algorithm>

namespace audio_graph {

/**
 * @brief Capsula de visualizacion central de presets inspirada en Arturia (Pigments/V Collection) y Image-Line FLEX.
 * Muestra la categoria tematica, titulo de preset en negrita de alta definicion, tira LED y boton de apertura directa de browser.
 */
class ArturiaPresetCapsule : public juce::Component, public juce::SettableTooltipClient {
public:
    ArturiaPresetCapsule() {
        setRepaintsOnMouseActivity(true);
        setTooltip("Haz clic para abrir el Navegador de Presets Profesional");
    }

    void setPreset(const juce::String& name, const juce::String& category) {
        name_ = name.isEmpty() ? "Select Preset..." : name;
        category_ = category.isEmpty() ? "FACTORY" : category;
        repaint();
    }

    void setOnClick(std::function<void()> cb) { onClick_ = std::move(cb); }

    void mouseDown(const juce::MouseEvent& /*e*/) override {
        isDown_ = true;
        repaint();
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (isDown_ && e.getDistanceFromDragStart() <= 4 && onClick_) {
            onClick_();
        }
        isDown_ = false;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        const bool hovered = isMouseOver();

        // 1. Chasis estilo capsula de aluminio titanio
        juce::ColourGradient grad(
            isDown_ ? juce::Colour(0xff090c12) : (hovered ? juce::Colour(0xff182230) : juce::Colour(0xff0e131c)),
            bounds.getTopLeft(),
            juce::Colour(0xff06080e), bounds.getBottomLeft(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, 4.0f);

        // Borde reactivo
        g.setColour(hovered ? juce::Colour(0xff00d4ff).withAlpha(0.65f) : juce::Colour(0xff1e2636));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Tira LED lateral izquierda con color de categoria
        const auto catCol = getCategoryColor(category_);
        auto led = bounds.removeFromLeft(3.0f).reduced(0.0f, 2.0f);
        g.setColour(catCol);
        g.fillRoundedRectangle(led, 1.5f);

        bounds.removeFromLeft(6.0f);
        auto browseIconArea = bounds.removeFromRight(60.0f);

        // 2. Fila superior: Categoria
        auto catArea = bounds.removeFromTop(bounds.getHeight() * 0.44f);
        g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
        g.setColour(catCol.withAlpha(0.9f));
        g.drawText(category_.toUpperCase(), catArea, juce::Justification::centredLeft, true);

        // 3. Fila inferior: Nombre del Preset
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.setColour(hovered ? juce::Colours::white : juce::Colour(0xffe2e8f0));
        g.drawText(name_, bounds, juce::Justification::centredLeft, true);

        // 4. Boton / Distintivo BROWSE a la derecha
        g.setColour(hovered ? juce::Colour(0xff00d4ff) : juce::Colour(0xff556477));
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.drawText(juce::String::fromUTF8("\xE2\x97\x88 BROWSE"), browseIconArea, juce::Justification::centredRight, true);
    }

private:
    static juce::Colour getCategoryColor(const juce::String& cat) {
        auto c = cat.toLowerCase();
        if (c.contains("space") || c.contains("reverb") || c.contains("ambient")) return juce::Colour(0xff00e5ff);
        if (c.contains("glitch") || c.contains("grain") || c.contains("spectral")) return juce::Colour(0xffffcc00);
        if (c.contains("drive") || c.contains("distort") || c.contains("fuzz")) return juce::Colour(0xffff3355);
        if (c.contains("mod") || c.contains("chorus") || c.contains("phase")) return juce::Colour(0xffbf55ec);
        if (c.contains("dyn") || c.contains("comp")) return juce::Colour(0xff00ff88);
        if (c.contains("synth") || c.contains("bass")) return juce::Colour(0xffa0e000);
        return juce::Colour(0xff00d4ff);
    }

    juce::String name_{ "Select Preset..." };
    juce::String category_{ "FACTORY" };
    bool isDown_{ false };
    std::function<void()> onClick_;
};

/**
 * @brief Barra superior de control de Presets, Captura de Escenas, Morphing y Undo/Redo (Reglas 21, 22, 25).
 * Estilo de consola profesional de produccion (Arturia / FLEX).
 */
class PresetBarComponent : public juce::Component {
public:
    struct PresetItem {
        std::string name;
        std::string category;
    };

    PresetBarComponent() {
        // 1. Selector de Presets Arturia / FLEX
        capsule_.setOnClick([this]() {
            if (onBrowseRequested_) {
                onBrowseRequested_();
            }
        });
        addAndMakeVisible(capsule_);

        // Boton Stepper Izquierdo ◀
        prevBtn_.setButtonText(juce::CharPointer_UTF8("\xE2\x97\x80"));
        prevBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff121622));
        prevBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        prevBtn_.onClick = [this]() {
            if (totalPresets_ <= 0) return;
            currentPresetIndex_ = (currentPresetIndex_ - 1 + totalPresets_) % totalPresets_;
            updateCapsuleDisplay();
            if (onPresetSelected_) onPresetSelected_(currentPresetIndex_);
        };
        addAndMakeVisible(prevBtn_);

        // Boton Stepper Derecho ▶
        nextBtn_.setButtonText(juce::CharPointer_UTF8("\xE2\x96\xB6"));
        nextBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff121622));
        nextBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        nextBtn_.onClick = [this]() {
            if (totalPresets_ <= 0) return;
            currentPresetIndex_ = (currentPresetIndex_ + 1) % totalPresets_;
            updateCapsuleDisplay();
            if (onPresetSelected_) onPresetSelected_(currentPresetIndex_);
        };
        addAndMakeVisible(nextBtn_);

        saveBtn_.setButtonText("Save");
        saveBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff121622));
        saveBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        saveBtn_.onClick = [this]() {
            if (onSavePresetRequested_) {
                onSavePresetRequested_();
            }
        };
        addAndMakeVisible(saveBtn_);

        randomBtn_.setButtonText(juce::String::fromUTF8("🎲 Rand"));
        randomBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161a26));
        randomBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffaa00));
        randomBtn_.onClick = [this]() {
            if (onRandomizeRequested_) {
                onRandomizeRequested_(1);
            }
        };
        addAndMakeVisible(randomBtn_);

        // 2. Botones de Escena y Slider de Morphing
        captureABtn_.setButtonText("Cap A");
        captureABtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff121622));
        captureABtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        captureABtn_.onClick = [this]() {
            if (onCaptureSceneARequested_) onCaptureSceneARequested_();
        };
        addAndMakeVisible(captureABtn_);

        captureBBtn_.setButtonText("Cap B");
        captureBBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff121622));
        captureBBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        captureBBtn_.onClick = [this]() {
            if (onCaptureSceneBRequested_) onCaptureSceneBRequested_();
        };
        addAndMakeVisible(captureBBtn_);

        morphSlider_.setSliderStyle(juce::Slider::LinearBar);
        morphSlider_.setRange(0.0, 1.0, 0.01);
        morphSlider_.setValue(0.0);
        morphSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 36, 16);
        morphSlider_.onValueChange = [this]() {
            if (onMorphChanged_) {
                onMorphChanged_(static_cast<float>(morphSlider_.getValue()));
            }
        };
        addAndMakeVisible(morphSlider_);

        morphLabel_.setText("MORPH", juce::dontSendNotification);
        morphLabel_.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        morphLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff8c96a5));
        addAndMakeVisible(morphLabel_);

        // 3. Botones Undo / Redo
        undoBtn_.setButtonText("Undo");
        undoBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff121622));
        undoBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        undoBtn_.onClick = [this]() { if (onUndoRequested_) onUndoRequested_(); };
        addAndMakeVisible(undoBtn_);

        redoBtn_.setButtonText("Redo");
        redoBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff121622));
        redoBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        redoBtn_.onClick = [this]() { if (onRedoRequested_) onRedoRequested_(); };
        addAndMakeVisible(redoBtn_);

        // 4. Importar / Exportar presets en disco (.n8preset)
        importBtn_.setButtonText("Imp");
        importBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff121622));
        importBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        importBtn_.onClick = [this]() { if (onImportPresetFileRequested_) onImportPresetFileRequested_(); };
        addAndMakeVisible(importBtn_);

        exportBtn_.setButtonText("Exp");
        exportBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff121622));
        exportBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        exportBtn_.onClick = [this]() { if (onExportPresetFileRequested_) onExportPresetFileRequested_(); };
        addAndMakeVisible(exportBtn_);

        // 5. Boton de Conmutacion de Piano Visual
        pianoToggleBtn_.setButtonText("Piano");
        pianoToggleBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161a26));
        pianoToggleBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00d4ff));
        pianoToggleBtn_.onClick = [this]() {
            isPianoVisible_ = !isPianoVisible_;
            pianoToggleBtn_.setColour(juce::TextButton::textColourOffId, isPianoVisible_ ? juce::Colour(0xff00d4ff) : juce::Colour(0xff606c80));
            if (onTogglePianoRequested_) onTogglePianoRequested_(isPianoVisible_);
        };
        addAndMakeVisible(pianoToggleBtn_);

        // 6. Boton de Conmutacion del Carril de Secuenciador por Efecto
        seqToggleBtn_.setButtonText("Seq");
        seqToggleBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161a26));
        seqToggleBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00d4ff));
        seqToggleBtn_.onClick = [this]() {
            isSeqVisible_ = !isSeqVisible_;
            seqToggleBtn_.setColour(juce::TextButton::textColourOffId, isSeqVisible_ ? juce::Colour(0xff00d4ff) : juce::Colour(0xff606c80));
            if (onToggleSeqRequested_) onToggleSeqRequested_(isSeqVisible_);
        };
        addAndMakeVisible(seqToggleBtn_);

        // 7. Botones de Conmutacion de Visualizador y Macros
        visToggleBtn_.setButtonText("Vis");
        visToggleBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161a26));
        visToggleBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00d4ff));
        visToggleBtn_.onClick = [this]() {
            isVisVisible_ = !isVisVisible_;
            visToggleBtn_.setColour(juce::TextButton::textColourOffId, isVisVisible_ ? juce::Colour(0xff00d4ff) : juce::Colour(0xff606c80));
            if (onToggleVisRequested_) onToggleVisRequested_(isVisVisible_);
        };
        addAndMakeVisible(visToggleBtn_);

        macrosToggleBtn_.setButtonText("Macros");
        macrosToggleBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161a26));
        macrosToggleBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00d4ff));
        macrosToggleBtn_.onClick = [this]() {
            isMacrosVisible_ = !isMacrosVisible_;
            macrosToggleBtn_.setColour(juce::TextButton::textColourOffId, isMacrosVisible_ ? juce::Colour(0xff00d4ff) : juce::Colour(0xff606c80));
            if (onToggleMacrosRequested_) onToggleMacrosRequested_(isMacrosVisible_);
        };
        addAndMakeVisible(macrosToggleBtn_);
    }

    void setPresetList(const std::vector<PresetItem>& presets) {
        presetItems_ = presets;
        totalPresets_ = static_cast<int>(presetItems_.size());
        currentPresetIndex_ = 0;
        updateCapsuleDisplay();
    }

    void setPresetList(const std::vector<std::string>& presetNames) {
        presetItems_.clear();
        presetItems_.reserve(presetNames.size());
        for (const auto& name : presetNames) {
            presetItems_.push_back({ name, "General" });
        }
        totalPresets_ = static_cast<int>(presetItems_.size());
        currentPresetIndex_ = 0;
        updateCapsuleDisplay();
    }

    void setCurrentPreset(int index) {
        if (index >= 0 && index < totalPresets_) {
            currentPresetIndex_ = index;
            updateCapsuleDisplay();
        }
    }

    void setCurrentPreset(const juce::String& name, const juce::String& category) {
        capsule_.setPreset(name, category);
        for (size_t i = 0; i < presetItems_.size(); ++i) {
            if (presetItems_[i].name == name.toStdString()) {
                currentPresetIndex_ = static_cast<int>(i);
                break;
            }
        }
    }

    int getCurrentPresetIndex() const noexcept {
        return currentPresetIndex_;
    }

    void setUndoRedoEnabled(bool canUndo, bool canRedo) {
        undoBtn_.setEnabled(canUndo);
        redoBtn_.setEnabled(canRedo);
    }

    void setPianoToggleState(bool isVisible) {
        isPianoVisible_ = isVisible;
        pianoToggleBtn_.setColour(juce::TextButton::textColourOffId, isPianoVisible_ ? juce::Colour(0xff00d4ff) : juce::Colour(0xff606c80));
    }

    void setSeqToggleState(bool isVisible) {
        isSeqVisible_ = isVisible;
        seqToggleBtn_.setColour(juce::TextButton::textColourOffId, isSeqVisible_ ? juce::Colour(0xff00d4ff) : juce::Colour(0xff606c80));
    }

    void setOnPresetSelected(std::function<void(int)> cb) { onPresetSelected_ = std::move(cb); }
    void setOnSavePresetRequested(std::function<void()> cb) { onSavePresetRequested_ = std::move(cb); }
    void setOnBrowseRequested(std::function<void()> cb) { onBrowseRequested_ = std::move(cb); }
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

        // Linea divisoria inferior
        g.setColour(juce::Colour(0xff1c2432));
        g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));
    }

    void resized() override {
        auto area = getLocalBounds().reduced(4, 2);

        // Grupo derecho: Toggles de vistas (Macros, Vis, Seq, Piano) + Imp/Exp
        macrosToggleBtn_.setBounds(area.removeFromRight(56));
        area.removeFromRight(3);
        visToggleBtn_.setBounds(area.removeFromRight(36));
        area.removeFromRight(3);
        seqToggleBtn_.setBounds(area.removeFromRight(36));
        area.removeFromRight(3);
        pianoToggleBtn_.setBounds(area.removeFromRight(46));
        area.removeFromRight(6);
        exportBtn_.setBounds(area.removeFromRight(34));
        area.removeFromRight(2);
        importBtn_.setBounds(area.removeFromRight(34));
        area.removeFromRight(8);

        // Grupo izquierdo: Undo / Redo
        undoBtn_.setBounds(area.removeFromLeft(38));
        area.removeFromLeft(2);
        redoBtn_.setBounds(area.removeFromLeft(38));
        area.removeFromLeft(8);

        // Centro: Capsula Arturia / FLEX con Steppers
        prevBtn_.setBounds(area.removeFromLeft(20));
        area.removeFromLeft(2);
        capsule_.setBounds(area.removeFromLeft(240));
        area.removeFromLeft(2);
        nextBtn_.setBounds(area.removeFromLeft(20));
        area.removeFromLeft(6);

        saveBtn_.setBounds(area.removeFromLeft(38));
        area.removeFromLeft(3);
        randomBtn_.setBounds(area.removeFromLeft(54));
        area.removeFromLeft(10);

        // Escenas y Morph
        captureABtn_.setBounds(area.removeFromLeft(42));
        area.removeFromLeft(4);
        morphLabel_.setBounds(area.removeFromLeft(46));
        morphSlider_.setBounds(area.removeFromLeft(80));
        area.removeFromLeft(4);
        captureBBtn_.setBounds(area.removeFromLeft(42));
    }

private:
    void updateCapsuleDisplay() {
        if (currentPresetIndex_ >= 0 && currentPresetIndex_ < static_cast<int>(presetItems_.size())) {
            capsule_.setPreset(presetItems_[currentPresetIndex_].name, presetItems_[currentPresetIndex_].category);
        }
    }

    ArturiaPresetCapsule capsule_;
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
    int currentPresetIndex_{ 0 };
    std::vector<PresetItem> presetItems_;

    std::function<void(int)> onPresetSelected_;
    std::function<void()> onSavePresetRequested_;
    std::function<void()> onBrowseRequested_;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBarComponent)
};

} // namespace audio_graph
