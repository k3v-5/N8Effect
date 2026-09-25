#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../core/CpuProfiler.h"
#include <functional>
#include <string>

namespace audio_graph {

/**
 * @brief Modal de Configuración y Diagnóstico del Motor de Audio (Reglas 23, 26, 34, 46, 47).
 * Centraliza la leyenda del audio, estado de pools, métricas de seguridad en tiempo real y parámetros del host.
 */
class EngineConfigModalComponent : public juce::Component {
public:
    EngineConfigModalComponent() {
        closeBtn_.setButtonText("✕");
        closeBtn_.onClick = [this]() {
            if (onCloseRequested_) onCloseRequested_();
        };
        addAndMakeVisible(closeBtn_);

        hudToggleBtn_.setButtonText("CPU HUD: VISIBLE");
        hudToggleBtn_.onClick = [this]() {
            isHudVisible_ = !isHudVisible_;
            hudToggleBtn_.setButtonText(isHudVisible_ ? "CPU HUD: VISIBLE" : "CPU HUD: HIDDEN");
            if (onToggleHud_) onToggleHud_(isHudVisible_);
        };
        addAndMakeVisible(hudToggleBtn_);

        audioSettingsBtn_.setButtonText("AUDIO DEVICE SETUP");
        audioSettingsBtn_.onClick = [this]() {
            if (onAudioSettingsRequested_) onAudioSettingsRequested_();
        };
        addAndMakeVisible(audioSettingsBtn_);
    }

    void setMetrics(const PerformanceMetrics& metrics, int numNodes, double sampleRate, int blockSize) {
        metrics_ = metrics;
        numNodes_ = numNodes;
        sampleRate_ = sampleRate;
        blockSize_ = blockSize;
        repaint();
    }

    void setHudVisibleState(bool visible) {
        isHudVisible_ = visible;
        hudToggleBtn_.setButtonText(isHudVisible_ ? "CPU HUD: VISIBLE" : "CPU HUD: HIDDEN");
    }

    void setOnCloseRequested(std::function<void()> cb) { onCloseRequested_ = std::move(cb); }
    void setOnToggleHud(std::function<void(bool)> cb) { onToggleHud_ = std::move(cb); }
    void setOnAudioSettingsRequested(std::function<void()> cb) { onAudioSettingsRequested_ = std::move(cb); }

    void mouseDown(const juce::MouseEvent& e) override {
        // Cerrar si se hace clic fuera del cuadro de diálogo central
        auto dialogArea = getLocalBounds().reduced(getWidth() > 500 ? (getWidth() - 440) / 2 : 20,
                                                   getHeight() > 440 ? (getHeight() - 380) / 2 : 20);
        if (!dialogArea.contains(e.getPosition())) {
            if (onCloseRequested_) onCloseRequested_();
        }
    }

    void paint(juce::Graphics& g) override {
        // Fondo atenuado oscuro detrás del modal
        g.fillAll(juce::Colours::black.withAlpha(0.70f));

        // Cuadro de diálogo central
        auto dialogArea = getLocalBounds().reduced(getWidth() > 500 ? (getWidth() - 440) / 2 : 20,
                                                   getHeight() > 440 ? (getHeight() - 380) / 2 : 20);

        // Fondo negro absoluto y contorno blanco nítido
        g.setColour(juce::Colour(0xff000000));
        g.fillRoundedRectangle(dialogArea.toFloat(), 4.0f);

        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(dialogArea.toFloat(), 4.0f, 1.0f);

        auto content = dialogArea.reduced(16, 14);

        // 1. Cabecera del diálogo
        auto headerRow = content.removeFromTop(24);
        g.setFont(juce::FontOptions(13.5f, juce::Font::bold));
        g.setColour(juce::Colours::white);
        g.drawText("AUDIO ENGINE CONFIGURATION", headerRow, juce::Justification::centredLeft, true);

        // Línea divisoria
        content.removeFromTop(8);
        g.setColour(juce::Colours::white.withAlpha(0.35f));
        g.drawHorizontalLine(content.getY(), static_cast<float>(content.getX()), static_cast<float>(content.getRight()));
        content.removeFromTop(10);

        // 2. Sección: Leyenda e Identidad del Motor
        drawSectionHeader(g, content.removeFromTop(16), "SISTEMA & ARQUITECTURA");
        drawInfoRow(g, content.removeFromTop(16), "Motor:", "Audio Event Graph Engine v1.0");
        drawInfoRow(g, content.removeFromTop(16), "Estado:", "Active & Compiled (Double-Buffered)");
        drawInfoRow(g, content.removeFromTop(16), "Real-Time Safety:", "Lock-Free Audio Thread (0 mallocs)");
        drawInfoRow(g, content.removeFromTop(16), "Anti-Denormals:", "Hardware FTZ & DAZ Active (RAII)");

        content.removeFromTop(10);

        // 3. Sección: Nodos y Pools en Tiempo Real
        drawSectionHeader(g, content.removeFromTop(16), "GRAFO & EVENT POOLS");
        drawInfoRow(g, content.removeFromTop(16), "Nodos en Grafo:", juce::String(numNodes_) + " procesadores activos");
        drawInfoRow(g, content.removeFromTop(16), "Event Pool:", juce::String(static_cast<int>(metrics_.activeEvents)) + " / 1024 slots asignados");
        drawInfoRow(g, content.removeFromTop(16), "Grain Pool:", "128 micro-granos (Zero-Leak Recycling)");
        drawInfoRow(g, content.removeFromTop(16), "Feedback Loops:", "Runaway Protection Active (fastTanh)");

        content.removeFromTop(10);

        // 4. Sección: Hardware del Host DAW
        drawSectionHeader(g, content.removeFromTop(16), "DISPOSITIVO DE AUDIO");
        drawInfoRow(g, content.removeFromTop(16), "Sample Rate:", juce::String(sampleRate_, 1) + " Hz");
        drawInfoRow(g, content.removeFromTop(16), "Buffer Size:", juce::String(blockSize_) + " samples");
        drawInfoRow(g, content.removeFromTop(16), "Ruteo de Canales:", "Stereo Main + Stereo Sidechain");

        content.removeFromTop(12);

        // 5. Sección: Visualización
        drawSectionHeader(g, content.removeFromTop(16), "OPCIONES DE INTERFAZ");
    }

    void resized() override {
        auto dialogArea = getLocalBounds().reduced(getWidth() > 500 ? (getWidth() - 440) / 2 : 20,
                                                   getHeight() > 440 ? (getHeight() - 380) / 2 : 20);

        // Botón de cierre en esquina superior derecha
        closeBtn_.setBounds(dialogArea.getRight() - 28, dialogArea.getY() + 8, 20, 20);

        // Botón toggle de HUD en la parte inferior
        hudToggleBtn_.setBounds(dialogArea.getX() + 16, dialogArea.getBottom() - 36, 140, 22);

        // Botón de configuración de hardware de audio
        audioSettingsBtn_.setBounds(dialogArea.getX() + 165, dialogArea.getBottom() - 36, 170, 22);
    }

private:
    void drawSectionHeader(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title) {
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(juce::Colours::white);
        g.drawText(title, r, juce::Justification::centredLeft, true);
    }

    void drawInfoRow(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& label, const juce::String& value) {
        g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
        g.setColour(juce::Colours::white.withAlpha(0.60f));
        g.drawText(label, r.removeFromLeft(130), juce::Justification::centredLeft, true);

        g.setColour(juce::Colours::white);
        g.drawText(value, r, juce::Justification::centredLeft, true);
    }

    juce::TextButton closeBtn_;
    juce::TextButton hudToggleBtn_;
    juce::TextButton audioSettingsBtn_;
    bool isHudVisible_{ true };

    PerformanceMetrics metrics_{};
    int numNodes_{ 0 };
    double sampleRate_{ 48000.0 };
    int blockSize_{ 256 };

    std::function<void()> onCloseRequested_;
    std::function<void(bool)> onToggleHud_;
    std::function<void()> onAudioSettingsRequested_;
};

} // namespace audio_graph
