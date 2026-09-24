#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>
#include <string>

namespace audio_graph {

/**
 * @brief Barra superior de control de Presets, Captura de Escenas, Morphing y Undo/Redo (Reglas 21, 22, 25).
 */
class PresetBarComponent : public juce::Component {
public:
    PresetBarComponent() {
        // 1. Selector de Presets
        presetCombo_.setTextWhenNothingSelected("Select Preset...");
        presetCombo_.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff181824));
        presetCombo_.setColour(juce::ComboBox::textColourId, juce::Colours::white);
        presetCombo_.onChange = [this]() {
            if (onPresetSelected_) {
                onPresetSelected_(presetCombo_.getSelectedItemIndex());
            }
        };
        addAndMakeVisible(presetCombo_);

        prevBtn_.setButtonText("<");
        prevBtn_.onClick = [this]() {
            int idx = presetCombo_.getSelectedItemIndex();
            if (idx > 0) presetCombo_.setSelectedItemIndex(idx - 1);
        };
        addAndMakeVisible(prevBtn_);

        nextBtn_.setButtonText(">");
        nextBtn_.onClick = [this]() {
            int idx = presetCombo_.getSelectedItemIndex();
            if (idx + 1 < presetCombo_.getNumItems()) presetCombo_.setSelectedItemIndex(idx + 1);
        };
        addAndMakeVisible(nextBtn_);

        saveBtn_.setButtonText("Save");
        saveBtn_.onClick = [this]() {
            if (onSavePresetRequested_) {
                onSavePresetRequested_();
            }
        };
        addAndMakeVisible(saveBtn_);

        // 2. Botones de Escena y Slider de Morphing
        captureABtn_.setButtonText("Cap A");
        captureABtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2980b9));
        captureABtn_.onClick = [this]() {
            if (onCaptureSceneARequested_) onCaptureSceneARequested_();
        };
        addAndMakeVisible(captureABtn_);

        captureBBtn_.setButtonText("Cap B");
        captureBBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff8e44ad));
        captureBBtn_.onClick = [this]() {
            if (onCaptureSceneBRequested_) onCaptureSceneBRequested_();
        };
        addAndMakeVisible(captureBBtn_);

        morphSlider_.setSliderStyle(juce::Slider::LinearBar);
        morphSlider_.setRange(0.0, 1.0, 0.01);
        morphSlider_.setValue(0.0);
        morphSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
        morphSlider_.setColour(juce::Slider::trackColourId, juce::Colour(0xff00ff88));
        morphSlider_.onValueChange = [this]() {
            if (onMorphChanged_) {
                onMorphChanged_(static_cast<float>(morphSlider_.getValue()));
            }
        };
        addAndMakeVisible(morphSlider_);

        morphLabel_.setText("MORPH A-B", juce::dontSendNotification);
        morphLabel_.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        morphLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff00ff88));
        addAndMakeVisible(morphLabel_);

        // 3. Botones Undo / Redo
        undoBtn_.setButtonText("Undo");
        undoBtn_.onClick = [this]() { if (onUndoRequested_) onUndoRequested_(); };
        addAndMakeVisible(undoBtn_);

        redoBtn_.setButtonText("Redo");
        redoBtn_.onClick = [this]() { if (onRedoRequested_) onRedoRequested_(); };
        addAndMakeVisible(redoBtn_);
    }

    void setPresetList(const std::vector<std::string>& presetNames) {
        presetCombo_.clear(juce::dontSendNotification);
        for (int i = 0; i < static_cast<int>(presetNames.size()); ++i) {
            presetCombo_.addItem(presetNames[i], i + 1);
        }
        if (!presetNames.empty()) {
            presetCombo_.setSelectedItemIndex(0, juce::dontSendNotification);
        }
    }

    void setUndoRedoEnabled(bool canUndo, bool canRedo) {
        undoBtn_.setEnabled(canUndo);
        redoBtn_.setEnabled(canRedo);
    }

    void setOnPresetSelected(std::function<void(int)> cb) { onPresetSelected_ = std::move(cb); }
    void setOnSavePresetRequested(std::function<void()> cb) { onSavePresetRequested_ = std::move(cb); }
    void setOnCaptureSceneA(std::function<void()> cb) { onCaptureSceneARequested_ = std::move(cb); }
    void setOnCaptureSceneB(std::function<void()> cb) { onCaptureSceneBRequested_ = std::move(cb); }
    void setOnMorphChanged(std::function<void(float)> cb) { onMorphChanged_ = std::move(cb); }
    void setOnUndoRequested(std::function<void()> cb) { onUndoRequested_ = std::move(cb); }
    void setOnRedoRequested(std::function<void()> cb) { onRedoRequested_ = std::move(cb); }

    void paint(juce::Graphics& g) override {
        g.setColour(juce::Colour(0xff12121c));
        g.fillRect(getLocalBounds());
    }

    void resized() override {
        auto area = getLocalBounds().reduced(4, 2);

        // Undo / Redo
        undoBtn_.setBounds(area.removeFromLeft(46));
        area.removeFromLeft(2);
        redoBtn_.setBounds(area.removeFromLeft(46));
        area.removeFromLeft(8);

        // Selector y botones
        prevBtn_.setBounds(area.removeFromLeft(24));
        area.removeFromLeft(2);
        presetCombo_.setBounds(area.removeFromLeft(180));
        area.removeFromLeft(2);
        nextBtn_.setBounds(area.removeFromLeft(24));
        area.removeFromLeft(4);
        saveBtn_.setBounds(area.removeFromLeft(44));
        area.removeFromLeft(12);

        // Escenas y Morph
        captureABtn_.setBounds(area.removeFromLeft(48));
        area.removeFromLeft(4);
        morphLabel_.setBounds(area.removeFromLeft(66));
        morphSlider_.setBounds(area.removeFromLeft(110));
        area.removeFromLeft(4);
        captureBBtn_.setBounds(area.removeFromLeft(48));
    }

private:
    juce::ComboBox presetCombo_;
    juce::TextButton prevBtn_;
    juce::TextButton nextBtn_;
    juce::TextButton saveBtn_;

    juce::TextButton captureABtn_;
    juce::TextButton captureBBtn_;
    juce::Slider morphSlider_;
    juce::Label morphLabel_;

    juce::TextButton undoBtn_;
    juce::TextButton redoBtn_;

    std::function<void(int)> onPresetSelected_;
    std::function<void()> onSavePresetRequested_;
    std::function<void()> onCaptureSceneARequested_;
    std::function<void()> onCaptureSceneBRequested_;
    std::function<void(float)> onMorphChanged_;
    std::function<void()> onUndoRequested_;
    std::function<void()> onRedoRequested_;
};

} // namespace audio_graph
