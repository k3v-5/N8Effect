#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>
#include <vector>
#include <string>
#include "../dsp/core/TestInputSynthesizer.h"

namespace audio_graph {

/**
 * @brief Teclado de Piano Visual Interactivo con Estética Minimalista Oscura (Reglas 1, 9, 23, 24).
 * Permite generar señal de audio de prueba en tiempo real para verificar la respuesta
 * del grafo modular de efectos y el cese de colas acústicas al liberar teclas (modo CUT).
 */
class VirtualPianoComponent : public juce::Component {
public:
    VirtualPianoComponent() {
        setWantsKeyboardFocus(true);

        // Configuración de botones de octava
        octaveDownBtn_.setButtonText("-");
        octaveDownBtn_.onClick = [this]() {
            if (baseOctave_ > 1) {
                baseOctave_--;
                updateOctaveLabel();
                repaint();
            }
        };
        addAndMakeVisible(octaveDownBtn_);

        octaveUpBtn_.setButtonText("+");
        octaveUpBtn_.onClick = [this]() {
            if (baseOctave_ < 6) {
                baseOctave_++;
                updateOctaveLabel();
                repaint();
            }
        };
        addAndMakeVisible(octaveUpBtn_);

        octaveLabel_.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        octaveLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
        octaveLabel_.setJustificationType(juce::Justification::centred);
        updateOctaveLabel();
        addAndMakeVisible(octaveLabel_);

        // Selector de timbre de prueba
        timbreBox_.addItem("E-Piano", 1);
        timbreBox_.addItem("Sine Tone", 2);
        timbreBox_.addItem("Triangle", 3);
        timbreBox_.addItem("Warm Saw", 4);
        timbreBox_.setSelectedId(1, juce::dontSendNotification);
        timbreBox_.onChange = [this]() {
            const int id = timbreBox_.getSelectedId();
            TestTimbreMode mode = TestTimbreMode::ElectricPiano;
            if (id == 2) mode = TestTimbreMode::Sine;
            else if (id == 3) mode = TestTimbreMode::Triangle;
            else if (id == 4) mode = TestTimbreMode::WarmSaw;
            if (onTimbreChanged_) {
                onTimbreChanged_(mode);
            }
        };
        addAndMakeVisible(timbreBox_);

        // Etiqueta de encabezado
        titleLabel_.setText("TEST INPUT PIANO [EFFECT PREVIEW]", juce::dontSendNotification);
        titleLabel_.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.85f));
        addAndMakeVisible(titleLabel_);

        // Botón de cierre
        closeBtn_.setButtonText("×");
        closeBtn_.onClick = [this]() {
            if (onCloseRequested_) {
                onCloseRequested_();
            }
        };
        addAndMakeVisible(closeBtn_);
    }

    ~VirtualPianoComponent() override {
        allNotesOff();
    }

    void setOnNoteOn(std::function<void(int, float)> cb) { onNoteOn_ = std::move(cb); }
    void setOnNoteOff(std::function<void(int)> cb) { onNoteOff_ = std::move(cb); }
    void setOnTimbreChanged(std::function<void(TestTimbreMode)> cb) { onTimbreChanged_ = std::move(cb); }
    void setOnCloseRequested(std::function<void()> cb) { onCloseRequested_ = std::move(cb); }

    void allNotesOff() {
        for (int note = 0; note < 128; ++note) {
            if (activeKeys_[note]) {
                activeKeys_[note] = false;
                if (onNoteOff_) onNoteOff_(note);
            }
        }
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // 1. Fondo negro absoluto
        g.setColour(juce::Colour(0xff000000));
        g.fillRect(bounds);

        // 2. Línea divisoria horizontal superior
        g.setColour(juce::Colours::white);
        g.drawHorizontalLine(0, 0.0f, bounds.getWidth());

        // 3. Renderizar teclas de piano (3 octavas completas = 36 teclas, 21 blancas)
        const auto keyArea = getKeyArea();
        constexpr int numWhiteKeys = 21;
        const float whiteKeyW = keyArea.getWidth() / static_cast<float>(numWhiteKeys);
        const float whiteKeyH = keyArea.getHeight();
        const float blackKeyW = whiteKeyW * 0.62f;
        const float blackKeyH = whiteKeyH * 0.60f;

        const int startMidiNote = (baseOctave_ + 1) * 12; // C_base

        // A. Dibujar teclas blancas primero
        int whiteIndex = 0;
        for (int note = startMidiNote; note < startMidiNote + 36; ++note) {
            if (!isBlackKey(note % 12)) {
                juce::Rectangle<float> r(keyArea.getX() + static_cast<float>(whiteIndex) * whiteKeyW,
                                         keyArea.getY(), whiteKeyW, whiteKeyH);

                const bool isDown = activeKeys_[note];

                if (isDown) {
                    g.setColour(juce::Colours::white.withAlpha(0.35f));
                    g.fillRect(r);
                } else {
                    g.setColour(juce::Colour(0xff050505));
                    g.fillRect(r);
                }

                // Contorno blanco nítido
                g.setColour(juce::Colours::white);
                g.drawRect(r, 1.0f);

                // Etiqueta en notas C
                if ((note % 12) == 0) {
                    const int oct = (note / 12) - 1;
                    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
                    g.setColour(juce::Colours::white.withAlpha(0.7f));
                    g.drawText("C" + juce::String(oct), r.removeFromBottom(14.0f), juce::Justification::centred, false);
                }

                whiteIndex++;
            }
        }

        // B. Dibujar teclas negras por encima
        whiteIndex = 0;
        for (int note = startMidiNote; note < startMidiNote + 36; ++note) {
            const int noteInOct = note % 12;
            if (isBlackKey(noteInOct)) {
                // Posición relativa entre la tecla blanca actual y la previa
                const float centerX = keyArea.getX() + static_cast<float>(whiteIndex) * whiteKeyW;
                juce::Rectangle<float> r(centerX - (blackKeyW * 0.5f), keyArea.getY(), blackKeyW, blackKeyH);

                const bool isDown = activeKeys_[note];

                if (isDown) {
                    g.setColour(juce::Colours::white);
                    g.fillRect(r);
                    g.setColour(juce::Colours::black);
                    g.drawRect(r, 1.0f);
                } else {
                    g.setColour(juce::Colour(0xff000000));
                    g.fillRect(r);
                    g.setColour(juce::Colours::white);
                    g.drawRect(r, 1.2f);
                }
            } else {
                whiteIndex++;
            }
        }
    }

    void resized() override {
        auto area = getLocalBounds();
        auto header = area.removeFromTop(24).reduced(6, 2);

        closeBtn_.setBounds(header.removeFromRight(20));
        header.removeFromRight(8);

        timbreBox_.setBounds(header.removeFromRight(95));
        header.removeFromRight(8);

        octaveUpBtn_.setBounds(header.removeFromRight(20));
        octaveLabel_.setBounds(header.removeFromRight(46));
        octaveDownBtn_.setBounds(header.removeFromRight(20));

        titleLabel_.setBounds(header);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        const int note = getNoteAtPosition(e.position);
        if (note >= 0 && note < 128) {
            triggerNoteOn(note, 0.85f);
            currentDraggingNote_ = note;
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        const int note = getNoteAtPosition(e.position);
        if (note != currentDraggingNote_) {
            if (currentDraggingNote_ >= 0) {
                triggerNoteOff(currentDraggingNote_);
            }
            if (note >= 0 && note < 128) {
                triggerNoteOn(note, 0.85f);
            }
            currentDraggingNote_ = note;
        }
    }

    void mouseUp(const juce::MouseEvent& /*e*/) override {
        if (currentDraggingNote_ >= 0) {
            triggerNoteOff(currentDraggingNote_);
            currentDraggingNote_ = -1;
        }
    }

    bool keyPressed(const juce::KeyPress& key) override {
        const int note = mapKeyToMidiNote(key.getKeyCode());
        if (note >= 0 && !activeKeys_[note]) {
            triggerNoteOn(note, 0.85f);
            return true;
        }
        return false;
    }

    bool keyStateChanged(bool /*isKeyDown*/) override {
        // Apagar notas de teclado que ya no estén presionadas
        for (int key = 'A'; key <= 'Z'; ++key) {
            if (!juce::KeyPress::isKeyCurrentlyDown(key)) {
                const int note = mapKeyToMidiNote(key);
                if (note >= 0 && activeKeys_[note]) {
                    triggerNoteOff(note);
                }
            }
        }
        return true;
    }

private:
    juce::Rectangle<float> getKeyArea() const noexcept {
        return getLocalBounds().toFloat().withTrimmedTop(24.0f).reduced(4.0f, 2.0f);
    }

    static bool isBlackKey(int noteInOctave) noexcept {
        return noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10;
    }

    int getNoteAtPosition(juce::Point<float> pt) const noexcept {
        const auto keyArea = getKeyArea();
        if (!keyArea.contains(pt)) return -1;

        constexpr int numWhiteKeys = 21;
        const float whiteKeyW = keyArea.getWidth() / static_cast<float>(numWhiteKeys);
        const float whiteKeyH = keyArea.getHeight();
        const float blackKeyW = whiteKeyW * 0.62f;
        const float blackKeyH = whiteKeyH * 0.60f;

        const int startMidiNote = (baseOctave_ + 1) * 12;

        // 1. Prioridad: Verificar primero teclas negras (están encima)
        if (pt.y <= keyArea.getY() + blackKeyH) {
            int whiteIndex = 0;
            for (int note = startMidiNote; note < startMidiNote + 36; ++note) {
                const int noteInOct = note % 12;
                if (isBlackKey(noteInOct)) {
                    const float centerX = keyArea.getX() + static_cast<float>(whiteIndex) * whiteKeyW;
                    juce::Rectangle<float> r(centerX - (blackKeyW * 0.5f), keyArea.getY(), blackKeyW, blackKeyH);
                    if (r.contains(pt)) {
                        return note;
                    }
                } else {
                    whiteIndex++;
                }
            }
        }

        // 2. Si no es tecla negra, verificar teclas blancas
        int whiteIndex = static_cast<int>((pt.x - keyArea.getX()) / whiteKeyW);
        whiteIndex = std::clamp(whiteIndex, 0, numWhiteKeys - 1);

        int currentWhite = 0;
        for (int note = startMidiNote; note < startMidiNote + 36; ++note) {
            if (!isBlackKey(note % 12)) {
                if (currentWhite == whiteIndex) {
                    return note;
                }
                currentWhite++;
            }
        }

        return -1;
    }

    int mapKeyToMidiNote(int keyCode) const noexcept {
        const int baseC = (baseOctave_ + 1) * 12;
        switch (std::toupper(keyCode)) {
            case 'A': return baseC;      // C
            case 'W': return baseC + 1;  // C#
            case 'S': return baseC + 2;  // D
            case 'E': return baseC + 3;  // D#
            case 'D': return baseC + 4;  // E
            case 'F': return baseC + 5;  // F
            case 'T': return baseC + 6;  // F#
            case 'G': return baseC + 7;  // G
            case 'Y': return baseC + 8;  // G#
            case 'H': return baseC + 9;  // A
            case 'U': return baseC + 10; // A#
            case 'J': return baseC + 11; // B
            case 'K': return baseC + 12; // C (+1)
            case 'O': return baseC + 13; // C# (+1)
            case 'L': return baseC + 14; // D (+1)
            default: return -1;
        }
    }

    void triggerNoteOn(int note, float vel) {
        if (note >= 0 && note < 128) {
            activeKeys_[note] = true;
            if (onNoteOn_) onNoteOn_(note, vel);
            repaint();
        }
    }

    void triggerNoteOff(int note) {
        if (note >= 0 && note < 128) {
            activeKeys_[note] = false;
            if (onNoteOff_) onNoteOff_(note);
            repaint();
        }
    }

    void updateOctaveLabel() {
        octaveLabel_.setText("OCT " + juce::String(baseOctave_), juce::dontSendNotification);
    }

    int baseOctave_{ 3 }; // Octava por defecto C3 - B5
    int currentDraggingNote_{ -1 };
    std::array<bool, 128> activeKeys_{ false };

    juce::Label titleLabel_;
    juce::TextButton octaveDownBtn_;
    juce::TextButton octaveUpBtn_;
    juce::Label octaveLabel_;
    juce::ComboBox timbreBox_;
    juce::TextButton closeBtn_;

    std::function<void(int, float)> onNoteOn_;
    std::function<void(int)> onNoteOff_;
    std::function<void(TestTimbreMode)> onTimbreChanged_;
    std::function<void()> onCloseRequested_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VirtualPianoComponent)
};

} // namespace audio_graph
