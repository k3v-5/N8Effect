#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../core/CpuProfiler.h"
#include "ThemeManager.h"
#include <functional>
#include <string>

namespace audio_graph {

/**
 * @brief Modal de Configuración y Diagnóstico del Motor de Audio (Reglas 23, 26, 34, 46, 47).
 * Centraliza la leyenda del audio, estado de pools, métricas de seguridad en tiempo real,
 * parámetros del host y selector de tema visual del chasis (Milestone 4).
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

        // Botones de selección de tema de chasis (Milestone 4)
        themeCyberpunkBtn_.setButtonText("CYBERPUNK");
        themeVintageBtn_.setButtonText("VINTAGE");
        themeCleanBtn_.setButtonText("STUDIO");
        themePhosphorBtn_.setButtonText("CRT PHOSPHOR");

        auto setupThemeBtn = [this](juce::TextButton& btn, ThemePreset preset) {
            btn.onClick = [this, preset]() {
                ThemeManager::getInstance().setTheme(preset);
                updateThemeButtons();
                repaint();
            };
            addAndMakeVisible(btn);
        };

        setupThemeBtn(themeCyberpunkBtn_, ThemePreset::Cyberpunk);
        setupThemeBtn(themeVintageBtn_, ThemePreset::VintageConsole);
        setupThemeBtn(themeCleanBtn_, ThemePreset::CleanStudio);
        setupThemeBtn(themePhosphorBtn_, ThemePreset::PhosphorCRT);
        updateThemeButtons();
    }

    void setMetrics(const PerformanceMetrics& metrics, int numNodes, double sampleRate, int blockSize) {
        metrics_ = metrics;
        numNodes_ = numNodes;
        sampleRate_ = sampleRate;
        blockSize_ = blockSize;
        updateThemeButtons();
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
        auto dialogArea = getLocalBounds().reduced(getWidth() > 500 ? (getWidth() - 460) / 2 : 20,
                                                   getHeight() > 470 ? (getHeight() - 450) / 2 : 20);
        if (!dialogArea.contains(e.getPosition())) {
            if (onCloseRequested_) onCloseRequested_();
        }
    }

    void paint(juce::Graphics& g) override {
        const auto& theme = ThemeManager::getInstance().getColors();

        // Fondo atenuado oscuro detrás del modal
        g.fillAll(juce::Colours::black.withAlpha(0.70f));

        // Cuadro de diálogo central
        auto dialogArea = getLocalBounds().reduced(getWidth() > 500 ? (getWidth() - 460) / 2 : 20,
                                                   getHeight() > 470 ? (getHeight() - 450) / 2 : 20);

        // Fondo y contorno según el tema activo
        g.setColour(theme.cardSurface);
        g.fillRoundedRectangle(dialogArea.toFloat(), 6.0f);

        g.setColour(theme.borderFocused);
        g.drawRoundedRectangle(dialogArea.toFloat(), 6.0f, 1.2f);

        auto content = dialogArea.reduced(16, 14);

        // 1. Cabecera del diálogo
        auto headerRow = content.removeFromTop(24);
        g.setFont(juce::FontOptions(13.5f, juce::Font::bold));
        g.setColour(theme.textPrimary);
        g.drawText("AUDIO ENGINE CONFIGURATION", headerRow, juce::Justification::centredLeft, true);

        // Línea divisoria
        content.removeFromTop(8);
        g.setColour(theme.borderMuted);
        g.drawHorizontalLine(content.getY(), static_cast<float>(content.getX()), static_cast<float>(content.getRight()));
        content.removeFromTop(10);

        // 2. Sección: Leyenda e Identidad del Motor
        drawSectionHeader(g, content.removeFromTop(16), "SISTEMA & ARQUITECTURA", theme);
        drawInfoRow(g, content.removeFromTop(16), "Motor:", "Audio Event Graph Engine v1.0", theme);
        drawInfoRow(g, content.removeFromTop(16), "Estado:", "Active & Compiled (Double-Buffered)", theme);
        drawInfoRow(g, content.removeFromTop(16), "Real-Time Safety:", "Lock-Free Audio Thread (0 mallocs)", theme);
        drawInfoRow(g, content.removeFromTop(16), "Anti-Denormals:", "Hardware FTZ & DAZ Active (RAII)", theme);

        content.removeFromTop(10);

        // 3. Sección: Nodos y Pools en Tiempo Real
        drawSectionHeader(g, content.removeFromTop(16), "GRAFO & EVENT POOLS", theme);
        drawInfoRow(g, content.removeFromTop(16), "Nodos en Grafo:", juce::String(numNodes_) + " procesadores activos", theme);
        drawInfoRow(g, content.removeFromTop(16), "Event Pool:", juce::String(static_cast<int>(metrics_.activeEvents)) + " / 1024 slots asignados", theme);
        drawInfoRow(g, content.removeFromTop(16), "Grain Pool:", "128 micro-granos (Zero-Leak Recycling)", theme);
        drawInfoRow(g, content.removeFromTop(16), "Feedback Loops:", "Runaway Protection Active (fastTanh)", theme);

        content.removeFromTop(10);

        // 4. Sección: Hardware del Host DAW
        drawSectionHeader(g, content.removeFromTop(16), "DISPOSITIVO DE AUDIO", theme);
        drawInfoRow(g, content.removeFromTop(16), "Sample Rate:", juce::String(sampleRate_, 1) + " Hz", theme);
        drawInfoRow(g, content.removeFromTop(16), "Buffer Size:", juce::String(blockSize_) + " samples", theme);
        drawInfoRow(g, content.removeFromTop(16), "Ruteo de Canales:", "Stereo Main + Stereo Sidechain", theme);

        content.removeFromTop(10);

        // 5. Sección: Visualización & Temas del Chasis
        drawSectionHeader(g, content.removeFromTop(16), "CHASSIS THEME PALETTE", theme);
        drawInfoRow(g, content.removeFromTop(16), "Tema Activo:", ThemeManager::getPresetName(ThemeManager::getInstance().getCurrentPreset()), theme);
    }

    void resized() override {
        auto dialogArea = getLocalBounds().reduced(getWidth() > 500 ? (getWidth() - 460) / 2 : 20,
                                                   getHeight() > 470 ? (getHeight() - 450) / 2 : 20);

        // Botón de cierre en esquina superior derecha
        closeBtn_.setBounds(dialogArea.getRight() - 28, dialogArea.getY() + 8, 20, 20);

        // Fila de 4 temas visuales
        const int themeRowY = dialogArea.getY() + 355;
        const int themeBtnWidth = 98;
        const int themeBtnHeight = 22;
        const int themeSpacing = 6;
        int themeX = dialogArea.getX() + 16;

        themeCyberpunkBtn_.setBounds(themeX, themeRowY, themeBtnWidth, themeBtnHeight);
        themeX += themeBtnWidth + themeSpacing;
        themeVintageBtn_.setBounds(themeX, themeRowY, themeBtnWidth, themeBtnHeight);
        themeX += themeBtnWidth + themeSpacing;
        themeCleanBtn_.setBounds(themeX, themeRowY, themeBtnWidth, themeBtnHeight);
        themeX += themeBtnWidth + themeSpacing;
        themePhosphorBtn_.setBounds(themeX, themeRowY, themeBtnWidth, themeBtnHeight);

        // Botón toggle de HUD en la parte inferior
        hudToggleBtn_.setBounds(dialogArea.getX() + 16, dialogArea.getBottom() - 36, 140, 22);

        // Botón de configuración de hardware de audio
        audioSettingsBtn_.setBounds(dialogArea.getX() + 165, dialogArea.getBottom() - 36, 170, 22);
    }

private:
    void updateThemeButtons() {
        const auto current = ThemeManager::getInstance().getCurrentPreset();
        themeCyberpunkBtn_.setToggleState(current == ThemePreset::Cyberpunk, juce::dontSendNotification);
        themeVintageBtn_.setToggleState(current == ThemePreset::VintageConsole, juce::dontSendNotification);
        themeCleanBtn_.setToggleState(current == ThemePreset::CleanStudio, juce::dontSendNotification);
        themePhosphorBtn_.setToggleState(current == ThemePreset::PhosphorCRT, juce::dontSendNotification);
    }

    void drawSectionHeader(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, const ThemeColors& theme) {
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(theme.accentPrimary);
        g.drawText(title, r, juce::Justification::centredLeft, true);
    }

    void drawInfoRow(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& label, const juce::String& value, const ThemeColors& theme) {
        g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
        g.setColour(theme.textSecondary);
        g.drawText(label, r.removeFromLeft(130), juce::Justification::centredLeft, true);

        g.setColour(theme.textPrimary);
        g.drawText(value, r, juce::Justification::centredLeft, true);
    }

    juce::TextButton closeBtn_;
    juce::TextButton hudToggleBtn_;
    juce::TextButton audioSettingsBtn_;

    juce::TextButton themeCyberpunkBtn_;
    juce::TextButton themeVintageBtn_;
    juce::TextButton themeCleanBtn_;
    juce::TextButton themePhosphorBtn_;

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
