#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../plugin/PluginProcessor.h"
#include "../dsp/core/TestSynthEngine.h"

namespace audio_graph {

/**
 * @brief Componente de Teclado Virtual Interactivo estilo Arturia Efx (Reglas 1, 9, 23, 24).
 * Permite audicionar y probar la cadena secuencial y el grafo modular en tiempo real sin requerir DAW externo.
 */
class VirtualKeyboardComponent : public juce::Component {
public:
    VirtualKeyboardComponent(juce::MidiKeyboardState& keyboardState, N8AudioProcessor& processor)
        : processorRef_(processor),
          keyboard_(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
        // 1. Configurar teclado JUCE con paleta Arturia Efx
        keyboard_.setKeyWidth(19.0f);
        keyboard_.setBlackNoteLengthProportion(0.64f);
        keyboard_.setBlackNoteWidthProportion(0.68f);
        keyboard_.setAvailableRange(24, 108); // C1 a C8
        keyboard_.setLowestVisibleKey(48);    // C3 centrado por defecto
        keyboard_.setScrollButtonsVisible(false); // Usamos nuestros propios botones elegantes

        // Colores temáticos oscuros con acento Cyan (#00d2ff)
        keyboard_.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffdcdfe5));
        keyboard_.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff181924));
        keyboard_.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff0e0e16));
        keyboard_.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0x3300d2ff));
        keyboard_.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0xcc00d2ff));
        keyboard_.setColour(juce::MidiKeyboardComponent::textLabelColourId, juce::Colour(0xff757d8a));
        keyboard_.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colour(0x35000000));
        addAndMakeVisible(keyboard_);

        // 2. Título / Etiqueta de sección
        titleLabel_.setText(juce::CharPointer_UTF8("\xF0\x9F\x8E\xB9 TECLADO AUDICI\xC3\x93N"), juce::dontSendNotification); // 🎹 TECLADO AUDICIÓN
        titleLabel_.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        titleLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff00d2ff));
        addAndMakeVisible(titleLabel_);

        // 3. Controles de Octava
        octaveDownBtn_.setButtonText(juce::CharPointer_UTF8("\xE2\x97\x80 OCT")); // ◀ OCT
        octaveDownBtn_.onClick = [this]() { shiftOctave(-1); };
        styleMiniButton(octaveDownBtn_);
        addAndMakeVisible(octaveDownBtn_);

        octaveLabel_.setText("Oct: C3", juce::dontSendNotification);
        octaveLabel_.setFont(juce::FontOptions(11.0f, juce::Font::plain));
        octaveLabel_.setJustificationType(juce::Justification::centred);
        octaveLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffdcdfe5));
        addAndMakeVisible(octaveLabel_);

        octaveUpBtn_.setButtonText(juce::CharPointer_UTF8("OCT \xE2\x96\xB6")); // OCT ▶
        octaveUpBtn_.onClick = [this]() { shiftOctave(1); };
        styleMiniButton(octaveUpBtn_);
        addAndMakeVisible(octaveUpBtn_);

        // 4. Selector de Timbre / Forma de Onda (Saw, Sine, Square, Pluck)
        soundLabel_.setText("TIMBRE:", juce::dontSendNotification);
        soundLabel_.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        soundLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff8c96a5));
        addAndMakeVisible(soundLabel_);

        setupSoundButton(sawBtn_, "SAW", SynthSoundType::SynthSaw);
        setupSoundButton(sineBtn_, "SINE", SynthSoundType::WarmSine);
        setupSoundButton(squareBtn_, "SQR", SynthSoundType::SquareLead);
        setupSoundButton(pluckBtn_, "PLUCK", SynthSoundType::Pluck);
        updateSoundButtonsSelection();

        // 5. Interruptor de Sonido de Prueba (Audition Mute/Unmute)
        soundPowerBtn_.setButtonText(juce::CharPointer_UTF8("\xF0\x9F\x94\x8A SONIDO ON")); // 🔊 SONIDO ON
        soundPowerBtn_.setClickingTogglesState(true);
        soundPowerBtn_.setToggleState(processorRef_.getTestSynth().isEnabled(), juce::dontSendNotification);
        soundPowerBtn_.onClick = [this]() {
            const bool enabled = soundPowerBtn_.getToggleState();
            processorRef_.getTestSynth().setEnabled(enabled);
            soundPowerBtn_.setButtonText(enabled ? juce::CharPointer_UTF8("\xF0\x9F\x94\x8A SONIDO ON")
                                                 : juce::CharPointer_UTF8("\xF0\x9F\x94\x87 MUDO"));
            updatePowerButtonAppearance();
        };
        updatePowerButtonAppearance();
        addAndMakeVisible(soundPowerBtn_);

        // 6. Control de Ganancia / Volumen
        volLabel_.setText("VOL", juce::dontSendNotification);
        volLabel_.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        volLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff8c96a5));
        addAndMakeVisible(volLabel_);

        volSlider_.setSliderStyle(juce::Slider::LinearBar);
        volSlider_.setRange(0.0, 1.5, 0.01);
        volSlider_.setValue(processorRef_.getTestSynth().getGain(), juce::dontSendNotification);
        volSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 36, 16);
        volSlider_.setColour(juce::Slider::trackColourId, juce::Colour(0xff00d2ff));
        volSlider_.onValueChange = [this]() {
            processorRef_.getTestSynth().setGain(static_cast<float>(volSlider_.getValue()));
        };
        addAndMakeVisible(volSlider_);

        // 7. Botón de Cerrar / Minimizar
        closeBtn_.setButtonText(juce::CharPointer_UTF8("\xE2\x9C\x95")); // ✕
        closeBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181924));
        closeBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        closeBtn_.onClick = [this]() {
            if (onCloseRequested) onCloseRequested();
        };
        addAndMakeVisible(closeBtn_);
    }

    ~VirtualKeyboardComponent() override = default;

    std::function<void()> onCloseRequested;

    void paint(juce::Graphics& g) override {
        // Fondo oscuro estilo hardware Arturia
        g.setColour(juce::Colour(0xff12121a));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);

        // Borde exterior sutil
        g.setColour(juce::Colour(0xff222332));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 6.0f, 1.0f);

        // Barra divisoria entre controles y teclas
        g.setColour(juce::Colour(0xff1f202e));
        g.drawHorizontalLine(28, 4.0f, static_cast<float>(getWidth() - 4));
    }

    void resized() override {
        auto area = getLocalBounds().reduced(4, 2);

        // 1. Barra de Herramientas Superior (alto: 24px)
        auto topBar = area.removeFromTop(24);

        titleLabel_.setBounds(topBar.removeFromLeft(130));

        topBar.removeFromLeft(6);
        octaveDownBtn_.setBounds(topBar.removeFromLeft(48).reduced(1, 2));
        octaveLabel_.setBounds(topBar.removeFromLeft(52));
        octaveUpBtn_.setBounds(topBar.removeFromLeft(48).reduced(1, 2));

        topBar.removeFromLeft(12);
        soundLabel_.setBounds(topBar.removeFromLeft(52));
        sawBtn_.setBounds(topBar.removeFromLeft(42).reduced(1, 2));
        sineBtn_.setBounds(topBar.removeFromLeft(42).reduced(1, 2));
        squareBtn_.setBounds(topBar.removeFromLeft(42).reduced(1, 2));
        pluckBtn_.setBounds(topBar.removeFromLeft(50).reduced(1, 2));

        // Lado derecho
        closeBtn_.setBounds(topBar.removeFromRight(24).reduced(2, 2));
        topBar.removeFromRight(8);

        volSlider_.setBounds(topBar.removeFromRight(76).reduced(0, 3));
        volLabel_.setBounds(topBar.removeFromRight(28));
        topBar.removeFromRight(8);

        soundPowerBtn_.setBounds(topBar.removeFromRight(100).reduced(2, 2));

        // 2. Área del Teclado Piano (restante del componente)
        area.removeFromTop(3);
        keyboard_.setBounds(area);
    }

private:
    void shiftOctave(int delta) {
        int currentLowest = keyboard_.getLowestVisibleKey();
        int newLowest = std::clamp(currentLowest + (delta * 12), 24, 96);
        keyboard_.setLowestVisibleKey(newLowest);

        // Actualizar etiqueta (ej: C2, C3, C4)
        int octaveNum = (newLowest / 12) - 1;
        octaveLabel_.setText("Oct: C" + juce::String(octaveNum), juce::dontSendNotification);
    }

    void styleMiniButton(juce::TextButton& btn) {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e202d));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc5cad4));
    }

    void setupSoundButton(juce::TextButton& btn, const juce::String& text, SynthSoundType type) {
        btn.setButtonText(text);
        btn.onClick = [this, type]() {
            processorRef_.getTestSynth().setSoundType(type);
            updateSoundButtonsSelection();
        };
        addAndMakeVisible(btn);
    }

    void updateSoundButtonsSelection() {
        const auto activeType = processorRef_.getTestSynth().getSoundType();
        auto updateBtn = [activeType](juce::TextButton& btn, SynthSoundType t) {
            const bool isSelected = (activeType == t);
            btn.setColour(juce::TextButton::buttonColourId, isSelected ? juce::Colour(0xff00d2ff) : juce::Colour(0xff181924));
            btn.setColour(juce::TextButton::textColourOffId, isSelected ? juce::Colour(0xff090910) : juce::Colour(0xff8c96a5));
        };

        updateBtn(sawBtn_, SynthSoundType::SynthSaw);
        updateBtn(sineBtn_, SynthSoundType::WarmSine);
        updateBtn(squareBtn_, SynthSoundType::SquareLead);
        updateBtn(pluckBtn_, SynthSoundType::Pluck);
    }

    void updatePowerButtonAppearance() {
        const bool on = soundPowerBtn_.getToggleState();
        soundPowerBtn_.setColour(juce::TextButton::buttonColourId, on ? juce::Colour(0xff0088aa) : juce::Colour(0xff222330));
        soundPowerBtn_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00aacc));
        soundPowerBtn_.setColour(juce::TextButton::textColourOffId, on ? juce::Colours::white : juce::Colour(0xff757d8a));
        soundPowerBtn_.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    }

    N8AudioProcessor& processorRef_;
    juce::MidiKeyboardComponent keyboard_;

    juce::Label titleLabel_;
    juce::TextButton octaveDownBtn_;
    juce::Label octaveLabel_;
    juce::TextButton octaveUpBtn_;

    juce::Label soundLabel_;
    juce::TextButton sawBtn_;
    juce::TextButton sineBtn_;
    juce::TextButton squareBtn_;
    juce::TextButton pluckBtn_;

    juce::TextButton soundPowerBtn_;
    juce::Label volLabel_;
    juce::Slider volSlider_;
    juce::TextButton closeBtn_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VirtualKeyboardComponent)
};

} // namespace audio_graph
