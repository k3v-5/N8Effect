#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <string>
#include <cmath>
#include "ModulationDragPayload.h"

namespace audio_graph {

/**
 * @brief Slider minimalista con visualización en tiempo real de Modulación, halo animado
 * interactivo y soporte de Drag & Drop Target (Reglas 23, 24, 25, 48).
 */
class ModulationSlider : public juce::Component, public juce::DragAndDropTarget {
public:
    ModulationSlider(const juce::String& paramName, float minVal, float maxVal, float defaultVal)
        : name_(paramName), minVal_(minVal), maxVal_(maxVal), baseValue_(defaultVal), currentModulatedValue_(defaultVal)
    {
        setRepaintsOnMouseActivity(true);
    }

    void setBaseValue(float v) {
        baseValue_ = std::clamp(v, minVal_, maxVal_);
        currentModulatedValue_ = baseValue_;
        repaint();
    }

    float getBaseValue() const noexcept { return baseValue_; }

    void setModulationDepth(float depth, juce::Colour col = juce::Colours::white) {
        modDepth_ = std::clamp(depth, -1.0f, 1.0f);
        if (col != juce::Colours::white) {
            modColor_ = col;
        }
        repaint();
    }

    float getModulationDepth() const noexcept { return modDepth_; }

    void setModulationColor(juce::Colour col) {
        modColor_ = col;
        repaint();
    }

    juce::Colour getModulationColor() const noexcept { return modColor_; }

    void setLiveModulatedValue(float modVal) {
        currentModulatedValue_ = std::clamp(modVal, minVal_, maxVal_);
        repaint();
    }

    void setOnValueChanged(std::function<void(float)> callback) {
        onValueChanged_ = std::move(callback);
    }

    void setOnModulationDropped(std::function<void(ModSourceType, float, juce::Colour)> cb) {
        onModulationDropped_ = std::move(cb);
    }

    void setOnModulationDepthChanged(std::function<void(float)> cb) {
        onModulationDepthChanged_ = std::move(cb);
    }

    void setOnModulationRemoved(std::function<void()> cb) {
        onModulationRemoved_ = std::move(cb);
    }

    // --- DragAndDropTarget Interface (Regla 48) ---
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override {
        ModulationDragPayload payload;
        return ModulationDragPayload::decode(dragSourceDetails.description.toString(), payload);
    }

    void itemDragEnter(const SourceDetails&) override {
        isDropCandidate_ = true;
        repaint();
    }

    void itemDragExit(const SourceDetails&) override {
        isDropCandidate_ = false;
        repaint();
    }

    void itemDropped(const SourceDetails& dragSourceDetails) override {
        isDropCandidate_ = false;
        ModulationDragPayload payload;
        if (ModulationDragPayload::decode(dragSourceDetails.description.toString(), payload)) {
            modColor_ = payload.sourceColor;
            modDepth_ = 0.5f; // Profundidad inicial intuitiva
            if (onModulationDropped_) {
                onModulationDropped_(payload.sourceType, modDepth_, modColor_);
            }
            repaint();
        }
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        // 1. Chasis / Canal ranurado anodizado profundo (Groove metálico)
        g.setColour(juce::Colour(0xff080a10));
        g.fillRoundedRectangle(bounds, 3.0f);

        // Bisel interior superior (sombra profunda de ranura)
        g.setColour(juce::Colour(0xff030406));
        g.drawHorizontalLine(static_cast<int>(bounds.getY()), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);

        const float normBase = (maxVal_ > minVal_) ? (baseValue_ - minVal_) / (maxVal_ - minVal_) : 0.0f;
        const float normLive = (maxVal_ > minVal_) ? (currentModulatedValue_ - minVal_) / (maxVal_ - minVal_) : 0.0f;

        // 2. Barra de valor base con sutil gradiente metálico
        const float barWidth = bounds.getWidth() * normBase;
        if (barWidth > 1.0f) {
            auto baseRect = bounds.withWidth(barWidth);
            juce::ColourGradient fillGrad(juce::Colour(0x3500d4ff), baseRect.getX(), baseRect.getY(),
                                          juce::Colour(0x18ffffff), baseRect.getRight(), baseRect.getY(), false);
            g.setGradientFill(fillGrad);
            g.fillRoundedRectangle(baseRect, 2.5f);
        }

        // 3. Rango de modulación visual (Halo reactivo)
        if (std::abs(modDepth_) > 0.001f) {
            const float modEnd = std::clamp(normBase + modDepth_, 0.0f, 1.0f);
            const float startX = bounds.getX() + std::min(normBase, modEnd) * bounds.getWidth();
            const float modW = std::abs(modEnd - normBase) * bounds.getWidth();

            g.setColour(modColor_.withAlpha(0.24f));
            g.fillRect(startX, bounds.getY() + 1.0f, modW, bounds.getHeight() - 2.0f);

            g.setColour(modColor_.withAlpha(0.85f));
            g.drawRect(startX, bounds.getY() + 1.0f, modW, bounds.getHeight() - 2.0f, 1.0f);
        }

        // 4. Aguja indicadora de posición base (Needle vertical nítida con cabezal LED)
        const float needleX = bounds.getX() + normBase * bounds.getWidth();
        if (barWidth > 1.0f && barWidth < bounds.getWidth() - 1.0f) {
            g.setColour(juce::Colour(0xffffffff));
            g.drawVerticalLine(static_cast<int>(needleX), bounds.getY() + 1.0f, bounds.getBottom() - 1.0f);
            // Cabezal luminoso superior
            g.setColour(std::abs(modDepth_) > 0.001f ? modColor_ : juce::Colour(0xff00f0ff));
            g.fillRect(needleX - 1.0f, bounds.getY() + 1.0f, 2.0f, 3.0f);
        }

        // 5. Indicador animado de modulación en vivo (círculo con núcleo coloreado)
        const float liveX = bounds.getX() + normLive * bounds.getWidth();
        g.setColour(juce::Colour(0xff06070a));
        g.fillEllipse(liveX - 4.5f, bounds.getCentreY() - 4.5f, 9.0f, 9.0f);

        g.setColour(std::abs(modDepth_) > 0.001f ? modColor_ : juce::Colours::white);
        g.drawEllipse(liveX - 4.5f, bounds.getCentreY() - 4.5f, 9.0f, 9.0f, 1.2f);
        g.fillEllipse(liveX - 2.0f, bounds.getCentreY() - 2.0f, 4.0f, 4.0f);

        // 6. Contorno exterior con micro-bisel
        if (isDropCandidate_) {
            g.setColour(juce::Colour(0xff00d4ff)); // Resplandor cian de snapping
            g.drawRoundedRectangle(bounds, 3.0f, 2.0f);
        } else if (std::abs(modDepth_) > 0.001f) {
            g.setColour(modColor_.withAlpha(0.70f));
            g.drawRoundedRectangle(bounds, 3.0f, 1.2f);
        } else {
            g.setColour(juce::Colour(0xff222a36));
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
        }

        // 7. Texto de etiqueta y valor numérico
        g.setColour(juce::Colour(0xffd1d5db));
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawText(name_, bounds.reduced(7.0f, 0.0f), juce::Justification::centredLeft, true);

        juce::String valStr = juce::String(baseValue_, (maxVal_ - minVal_ > 10.0f) ? 1 : 2);
        if (std::abs(modDepth_) > 0.001f) {
            valStr += " (" + juce::String(static_cast<int>(modDepth_ * 100.0f)) + "%)";
        }
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(std::abs(modDepth_) > 0.001f ? modColor_ : juce::Colour(0xff00f0ff));
        g.drawText(valStr, bounds.reduced(7.0f, 0.0f), juce::Justification::centredRight, true);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            showContextMenu();
            return;
        }

        // Shift+Click o Alt+Click ajusta profundidad de modulación directamente en el halo
        if (e.mods.isShiftDown() || e.mods.isAltDown()) {
            isAdjustingModDepth_ = true;
            updateModDepthFromMouse(e);
            return;
        }

        isAdjustingModDepth_ = false;
        updateFromMouse(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (isAdjustingModDepth_) {
            updateModDepthFromMouse(e);
            return;
        }
        updateFromMouse(e);
    }

    void mouseUp(const juce::MouseEvent&) override {
        isAdjustingModDepth_ = false;
    }

private:
    void updateFromMouse(const juce::MouseEvent& e) {
        auto bounds = getLocalBounds().toFloat().reduced(1.5f);
        const float norm = std::clamp((e.position.x - bounds.getX()) / bounds.getWidth(), 0.0f, 1.0f);
        baseValue_ = minVal_ + norm * (maxVal_ - minVal_);
        currentModulatedValue_ = baseValue_;
        if (onValueChanged_) {
            onValueChanged_(baseValue_);
        }
        repaint();
    }

    void updateModDepthFromMouse(const juce::MouseEvent& e) {
        auto bounds = getLocalBounds().toFloat().reduced(1.5f);
        const float normBase = (maxVal_ > minVal_) ? (baseValue_ - minVal_) / (maxVal_ - minVal_) : 0.0f;
        const float currentNorm = std::clamp((e.position.x - bounds.getX()) / bounds.getWidth(), 0.0f, 1.0f);
        modDepth_ = std::clamp(currentNorm - normBase, -1.0f, 1.0f);
        if (onModulationDepthChanged_) {
            onModulationDepthChanged_(modDepth_);
        }
        repaint();
    }

    void showContextMenu() {
        juce::PopupMenu menu;
        menu.addSectionHeader(name_ + " Modulation");

        if (std::abs(modDepth_) > 0.001f) {
            menu.addItem("Invert Modulation Depth", [this]() {
                modDepth_ = -modDepth_;
                if (onModulationDepthChanged_) onModulationDepthChanged_(modDepth_);
                repaint();
            });
            menu.addItem("Set Depth: +100%", [this]() {
                modDepth_ = 1.0f;
                if (onModulationDepthChanged_) onModulationDepthChanged_(modDepth_);
                repaint();
            });
            menu.addItem("Set Depth: +50%", [this]() {
                modDepth_ = 0.5f;
                if (onModulationDepthChanged_) onModulationDepthChanged_(modDepth_);
                repaint();
            });
            menu.addItem("Set Depth: -50%", [this]() {
                modDepth_ = -0.5f;
                if (onModulationDepthChanged_) onModulationDepthChanged_(modDepth_);
                repaint();
            });
            menu.addSeparator();
            menu.addItem("Remove Modulation", [this]() {
                modDepth_ = 0.0f;
                modColor_ = juce::Colours::white;
                if (onModulationRemoved_) onModulationRemoved_();
                repaint();
            });
        } else {
            menu.addItem("Drag any Macro or LFO onto this slider to modulate", false, false, nullptr);
        }

        menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(this));
    }

    juce::String name_;
    float minVal_{ 0.0f };
    float maxVal_{ 1.0f };
    float baseValue_{ 0.5f };
    float modDepth_{ 0.0f };
    float currentModulatedValue_{ 0.5f };
    juce::Colour modColor_{ juce::Colours::white };

    bool isDropCandidate_{ false };
    bool isAdjustingModDepth_{ false };

    std::function<void(float)> onValueChanged_;
    std::function<void(ModSourceType, float, juce::Colour)> onModulationDropped_;
    std::function<void(float)> onModulationDepthChanged_;
    std::function<void()> onModulationRemoved_;
};

} // namespace audio_graph
