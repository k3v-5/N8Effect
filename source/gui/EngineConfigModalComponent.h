#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../core/CpuProfiler.h"
#include "../core/Types.h"
#include "ThemeManager.h"
#include "../plugin/PluginProcessor.h"
#include <functional>
#include <string>

namespace audio_graph {

/**
 * @brief Modal de Configuración y Diagnóstico del Motor de Audio (Reglas 23, 26, 34, 46, 47).
 * Centraliza la leyenda del audio, estado de pools, métricas de seguridad en tiempo real,
 * parámetros del host, selector de modo de concurrencia (Milestone 1) y selector de tema visual del chasis.
 */
class EngineConfigModalComponent : public juce::Component {
public:
    /**
     * @brief Botón conmutador estilizado con ThemeManager para el selector de concurrencia y temas (Regla 48).
     */
    class SegmentedModeButton : public juce::Button {
    public:
        explicit SegmentedModeButton(const juce::String& name) : juce::Button(name) {}

        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
            const auto& theme = ThemeManager::getInstance().getColors();
            auto bounds = getLocalBounds().toFloat().reduced(0.5f);
            const bool active = getToggleState();

            if (active) {
                g.setColour(theme.accentPrimary.withAlpha(0.25f));
                g.fillRoundedRectangle(bounds, 3.0f);
                g.setColour(theme.accentPrimary);
                g.drawRoundedRectangle(bounds, 3.0f, 1.5f);
            } else {
                if (shouldDrawButtonAsDown) {
                    g.setColour(theme.borderMuted.withAlpha(0.30f));
                } else if (shouldDrawButtonAsHighlighted) {
                    g.setColour(theme.borderMuted.withAlpha(0.15f));
                } else {
                    g.setColour(theme.panelSurface);
                }
                g.fillRoundedRectangle(bounds, 3.0f);
                g.setColour(theme.borderMuted);
                g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
            }

            g.setFont(juce::FontOptions(10.0f, active ? juce::Font::bold : juce::Font::plain));
            g.setColour(active ? theme.accentPrimary : theme.textSecondary);
            g.drawText(getButtonText(), getLocalBounds(), juce::Justification::centred, true);
        }
    };

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

        // 1. Botones de Modo de Concurrencia (Milestone 1, Reglas 4, 9, 26, 47)
        auto setupModeBtn = [this](SegmentedModeButton& btn, ConcurrencyMode mode) {
            btn.onClick = [this, mode]() {
                setConcurrencyMode(mode);
            };
            addAndMakeVisible(btn);
        };

        setupModeBtn(modeSingleBtn_, ConcurrencyMode::SingleThreaded);
        setupModeBtn(modeSmartBtn_, ConcurrencyMode::SmartMultithreaded);
        setupModeBtn(modeMultiBtn_, ConcurrencyMode::AlwaysMultithreaded);
        updateModeButtons();

        // 2. Botones de selección de tema de chasis
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
        if (auto* proc = getProcessor()) {
            currentMode_ = proc->getConcurrencyMode();
        }
        updateThemeButtons();
        updateModeButtons();
        repaint();
    }

    void setHudVisibleState(bool visible) {
        isHudVisible_ = visible;
        hudToggleBtn_.setButtonText(isHudVisible_ ? "CPU HUD: VISIBLE" : "CPU HUD: HIDDEN");
    }

    void setConcurrencyMode(ConcurrencyMode mode) {
        currentMode_ = mode;
        updateModeButtons();
        if (auto* proc = getProcessor()) {
            proc->setConcurrencyMode(mode);
        }
        if (onConcurrencyModeChanged_) {
            onConcurrencyModeChanged_(mode);
        }
        repaint();
    }

    ConcurrencyMode getConcurrencyMode() const noexcept { return currentMode_; }

    void setProcessor(N8AudioProcessor* proc) noexcept {
        processor_ = proc;
        if (proc != nullptr) {
            currentMode_ = proc->getConcurrencyMode();
            updateModeButtons();
        }
    }

    void setOnCloseRequested(std::function<void()> cb) { onCloseRequested_ = std::move(cb); }
    void setOnToggleHud(std::function<void(bool)> cb) { onToggleHud_ = std::move(cb); }
    void setOnAudioSettingsRequested(std::function<void()> cb) { onAudioSettingsRequested_ = std::move(cb); }
    void setOnConcurrencyModeChanged(std::function<void(ConcurrencyMode)> cb) { onConcurrencyModeChanged_ = std::move(cb); }

    juce::Rectangle<int> getDialogBounds() const noexcept {
        return getLocalBounds().withSizeKeepingCentre(std::min(getWidth() - 40, 460),
                                                     std::min(getHeight() - 40, 520));
    }

    void mouseDown(const juce::MouseEvent& e) override {
        // Cerrar si se hace clic fuera del cuadro de diálogo central
        auto dialogArea = getDialogBounds();
        if (!dialogArea.contains(e.getPosition())) {
            if (onCloseRequested_) onCloseRequested_();
        }
    }

    void paint(juce::Graphics& g) override {
        const auto& theme = ThemeManager::getInstance().getColors();

        // Fondo atenuado oscuro detrás del modal
        g.fillAll(juce::Colours::black.withAlpha(0.70f));

        // Cuadro de diálogo central
        auto dialogArea = getDialogBounds();

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
        content.removeFromTop(6);
        g.setColour(theme.borderMuted);
        g.drawHorizontalLine(content.getY(), static_cast<float>(content.getX()), static_cast<float>(content.getRight()));
        content.removeFromTop(8);

        // 2. Sección: Leyenda e Identidad del Motor
        drawSectionHeader(g, content.removeFromTop(16), "SISTEMA & ARQUITECTURA", theme);
        drawInfoRow(g, content.removeFromTop(15), "Motor:", "Audio Event Graph Engine v1.0", theme);
        drawInfoRow(g, content.removeFromTop(15), "Estado:", "Active & Compiled (Double-Buffered)", theme);
        drawInfoRow(g, content.removeFromTop(15), "Real-Time Safety:", "Lock-Free Audio Thread (0 mallocs)", theme);
        drawInfoRow(g, content.removeFromTop(15), "Anti-Denormals:", "Hardware FTZ & DAZ Active (RAII)", theme);

        content.removeFromTop(8);

        // 3. Sección: Nodos y Pools en Tiempo Real
        drawSectionHeader(g, content.removeFromTop(16), "GRAFO & EVENT POOLS", theme);
        drawInfoRow(g, content.removeFromTop(15), "Nodos en Grafo:", juce::String(numNodes_) + " procesadores activos", theme);
        drawInfoRow(g, content.removeFromTop(15), "Event Pool:", juce::String(static_cast<int>(metrics_.activeEvents)) + " / 1024 slots asignados", theme);
        drawInfoRow(g, content.removeFromTop(15), "Grain Pool:", "128 micro-granos (Zero-Leak Recycling)", theme);
        drawInfoRow(g, content.removeFromTop(15), "Feedback Loops:", "Runaway Protection Active (fastTanh)", theme);

        content.removeFromTop(8);

        // 4. Sección: Hardware del Host DAW
        drawSectionHeader(g, content.removeFromTop(16), "DISPOSITIVO DE AUDIO", theme);
        drawInfoRow(g, content.removeFromTop(15), "Sample Rate:", juce::String(sampleRate_, 1) + " Hz", theme);
        drawInfoRow(g, content.removeFromTop(15), "Buffer Size:", juce::String(blockSize_) + " samples", theme);
        drawInfoRow(g, content.removeFromTop(15), "Ruteo de Canales:", "Stereo Main + Stereo Sidechain", theme);

        content.removeFromTop(8);

        // 5. Sección: Modo de Concurrencia (Milestone 1, Reglas 4, 9, 26, 47)
        drawSectionHeader(g, content.removeFromTop(16), "MODO DE CONCURRENCIA (CPU)", theme);
        drawInfoRow(g, content.removeFromTop(15), "Modo Activo:", getConcurrencyModeName(currentMode_), theme);
        drawInfoRow(g, content.removeFromTop(15), "Hilos Auxiliares:", currentMode_ == ConcurrencyMode::SingleThreaded ? "0 (Suspendidos)" : "2 (0% en reposo)", theme);

        content.removeFromTop(36); // Espacio reservado para los botones de concurrencia colocados en resized()

        // 6. Sección: Visualización & Temas del Chasis
        drawSectionHeader(g, content.removeFromTop(16), "CHASSIS THEME PALETTE", theme);
        drawInfoRow(g, content.removeFromTop(15), "Tema Activo:", ThemeManager::getPresetName(ThemeManager::getInstance().getCurrentPreset()), theme);
    }

    void resized() override {
        auto dialogArea = getDialogBounds();

        // Botón de cierre en esquina superior derecha
        closeBtn_.setBounds(dialogArea.getRight() - 28, dialogArea.getY() + 8, 20, 20);

        // Fila de 3 botones de concurrencia (Milestone 1)
        const int modeRowY = dialogArea.getY() + 355;
        const int modeBtnWidth = (dialogArea.getWidth() - 32 - 12) / 3;
        const int modeBtnHeight = 22;
        int modeX = dialogArea.getX() + 16;

        modeSingleBtn_.setBounds(modeX, modeRowY, modeBtnWidth, modeBtnHeight);
        modeX += modeBtnWidth + 6;
        modeSmartBtn_.setBounds(modeX, modeRowY, modeBtnWidth, modeBtnHeight);
        modeX += modeBtnWidth + 6;
        modeMultiBtn_.setBounds(modeX, modeRowY, modeBtnWidth, modeBtnHeight);

        // Fila de 4 temas visuales
        const int themeRowY = dialogArea.getY() + 422;
        const int themeBtnWidth = (dialogArea.getWidth() - 32 - 18) / 4;
        const int themeBtnHeight = 22;
        int themeX = dialogArea.getX() + 16;

        themeCyberpunkBtn_.setBounds(themeX, themeRowY, themeBtnWidth, themeBtnHeight);
        themeX += themeBtnWidth + 6;
        themeVintageBtn_.setBounds(themeX, themeRowY, themeBtnWidth, themeBtnHeight);
        themeX += themeBtnWidth + 6;
        themeCleanBtn_.setBounds(themeX, themeRowY, themeBtnWidth, themeBtnHeight);
        themeX += themeBtnWidth + 6;
        themePhosphorBtn_.setBounds(themeX, themeRowY, themeBtnWidth, themeBtnHeight);

        // Botón toggle de HUD en la parte inferior
        hudToggleBtn_.setBounds(dialogArea.getX() + 16, dialogArea.getBottom() - 34, 140, 22);

        // Botón de configuración de hardware de audio
        audioSettingsBtn_.setBounds(dialogArea.getX() + 165, dialogArea.getBottom() - 34, 170, 22);
    }

private:
    N8AudioProcessor* getProcessor() const {
        if (processor_ != nullptr) return processor_;
        if (auto* ed = findParentComponentOfClass<juce::AudioProcessorEditor>()) {
            return dynamic_cast<N8AudioProcessor*>(&ed->processor);
        }
        return nullptr;
    }

    void updateThemeButtons() {
        const auto current = ThemeManager::getInstance().getCurrentPreset();
        themeCyberpunkBtn_.setToggleState(current == ThemePreset::Cyberpunk, juce::dontSendNotification);
        themeVintageBtn_.setToggleState(current == ThemePreset::VintageConsole, juce::dontSendNotification);
        themeCleanBtn_.setToggleState(current == ThemePreset::CleanStudio, juce::dontSendNotification);
        themePhosphorBtn_.setToggleState(current == ThemePreset::PhosphorCRT, juce::dontSendNotification);
    }

    void updateModeButtons() {
        modeSingleBtn_.setToggleState(currentMode_ == ConcurrencyMode::SingleThreaded, juce::dontSendNotification);
        modeSmartBtn_.setToggleState(currentMode_ == ConcurrencyMode::SmartMultithreaded, juce::dontSendNotification);
        modeMultiBtn_.setToggleState(currentMode_ == ConcurrencyMode::AlwaysMultithreaded, juce::dontSendNotification);
    }

    static juce::String getConcurrencyModeName(ConcurrencyMode mode) noexcept {
        switch (mode) {
            case ConcurrencyMode::SingleThreaded: return "Monohilo (Bajo CPU)";
            case ConcurrencyMode::SmartMultithreaded: return "Multihilo Inteligente";
            case ConcurrencyMode::AlwaysMultithreaded: return "Multihilo Siempre Activo";
        }
        return "Desconocido";
    }

    void drawSectionHeader(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, const ThemeColors& theme) {
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(theme.accentPrimary);
        g.drawText(title, r, juce::Justification::centredLeft, true);
    }

    void drawInfoRow(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& label, const juce::String& value, const ThemeColors& theme) {
        g.setFont(juce::FontOptions(9.5f, juce::Font::plain));
        g.setColour(theme.textSecondary);
        g.drawText(label, r.removeFromLeft(130), juce::Justification::centredLeft, true);

        g.setColour(theme.textPrimary);
        g.drawText(value, r, juce::Justification::centredLeft, true);
    }

    juce::TextButton closeBtn_;
    juce::TextButton hudToggleBtn_;
    juce::TextButton audioSettingsBtn_;

    // 3 botones de concurrencia (Milestone 1)
    SegmentedModeButton modeSingleBtn_{ "Monohilo" };
    SegmentedModeButton modeSmartBtn_{ "Inteligente" };
    SegmentedModeButton modeMultiBtn_{ "Multihilo" };

    // 4 botones de tema
    juce::TextButton themeCyberpunkBtn_;
    juce::TextButton themeVintageBtn_;
    juce::TextButton themeCleanBtn_;
    juce::TextButton themePhosphorBtn_;

    bool isHudVisible_{ true };
    ConcurrencyMode currentMode_{ ConcurrencyMode::SmartMultithreaded };
    N8AudioProcessor* processor_{ nullptr };

    PerformanceMetrics metrics_{};
    int numNodes_{ 0 };
    double sampleRate_{ 48000.0 };
    int blockSize_{ 256 };

    std::function<void()> onCloseRequested_;
    std::function<void(bool)> onToggleHud_;
    std::function<void()> onAudioSettingsRequested_;
    std::function<void(ConcurrencyMode)> onConcurrencyModeChanged_;
};

} // namespace audio_graph
