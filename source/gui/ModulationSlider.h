#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <string>
#include <cmath>
#include <numbers>
#include <algorithm>
#include "ModulationDragPayload.h"

namespace audio_graph {

/**
 * @brief Estilo de renderizado para el control de modulación (Reglas 23, 24, 25).
 */
enum class SliderStyle {
    RotaryKnob,
    LinearBar
};

/**
 * @brief Control interactivo con visualización en tiempo real de Modulación,
 * anillos animados bipolares en perillas y barras, halo reactivo y soporte de Drag & Drop (Reglas 23, 24, 25, 48).
 */
class ModulationSlider : public juce::Component, public juce::DragAndDropTarget {
public:
    ModulationSlider(const juce::String& paramName, float minVal, float maxVal, float defaultVal)
        : name_(paramName), minVal_(minVal), maxVal_(maxVal), baseValue_(defaultVal), currentModulatedValue_(defaultVal)
    {
        setRepaintsOnMouseActivity(true);
    }

    void setSliderStyle(SliderStyle style) noexcept {
        if (style_ != style) {
            style_ = style;
            repaint();
        }
    }

    SliderStyle getSliderStyle() const noexcept { return style_; }

    void setBaseValue(float v) {
        baseValue_ = std::clamp(v, minVal_, maxVal_);
        currentModulatedValue_ = baseValue_;
        repaint();
    }

    float getBaseValue() const noexcept { return baseValue_; }
    float getMinValue() const noexcept { return minVal_; }
    float getMaxValue() const noexcept { return maxVal_; }
    const juce::String& getParamName() const noexcept { return name_; }

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

    float getLiveModulatedValue() const noexcept { return currentModulatedValue_; }

    void setDropCandidate(bool cand) noexcept {
        if (isDropCandidate_ != cand) {
            isDropCandidate_ = cand;
            repaint();
        }
    }

    bool isDropCandidate() const noexcept { return isDropCandidate_; }

    juce::Point<float> getKnobCenterInLocal() const noexcept {
        auto bounds = getLocalBounds().toFloat();
        if (style_ == SliderStyle::RotaryKnob) {
            return { bounds.getX() + bounds.getHeight() * 0.5f, bounds.getCentreY() };
        }
        return bounds.getCentre();
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
            modDepth_ = 0.5f; // Profundidad inicial intuitiva (+50%)
            if (onModulationDropped_) {
                onModulationDropped_(payload.sourceType, modDepth_, modColor_);
            }
            repaint();
        }
    }

    void paint(juce::Graphics& g) override {
        if (style_ == SliderStyle::RotaryKnob) {
            paintRotaryKnob(g);
        } else {
            paintLinearBar(g);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            showContextMenu();
            return;
        }

        // Shift+Click o Alt+Click ajusta profundidad de modulación directamente en el dial/anillo
        if (e.mods.isShiftDown() || e.mods.isAltDown()) {
            isAdjustingModDepth_ = true;
            dragStartPos_ = e.position;
            dragStartDepth_ = modDepth_;
            return;
        }

        isAdjustingModDepth_ = false;
        dragStartPos_ = e.position;
        dragStartValue_ = baseValue_;

        if (style_ == SliderStyle::LinearBar) {
            updateFromMouse(e);
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (isAdjustingModDepth_) {
            const float delta = (e.position.x - dragStartPos_.x) - (e.position.y - dragStartPos_.y);
            modDepth_ = std::clamp(dragStartDepth_ + delta * 0.005f, -1.0f, 1.0f);
            if (onModulationDepthChanged_) {
                onModulationDepthChanged_(modDepth_);
            }
            repaint();
            return;
        }

        if (style_ == SliderStyle::RotaryKnob) {
            const float deltaY = dragStartPos_.y - e.position.y;
            const float deltaX = e.position.x - dragStartPos_.x;
            const float sensitivity = (maxVal_ - minVal_) * (e.mods.isShiftDown() ? 0.001f : 0.005f);
            baseValue_ = std::clamp(dragStartValue_ + (deltaY + deltaX * 0.4f) * sensitivity, minVal_, maxVal_);
            currentModulatedValue_ = baseValue_;
            if (onValueChanged_) {
                onValueChanged_(baseValue_);
            }
            repaint();
        } else {
            updateFromMouse(e);
        }
    }

    void mouseUp(const juce::MouseEvent&) override {
        isAdjustingModDepth_ = false;
    }

    void mouseDoubleClick(const juce::MouseEvent&) override {
        baseValue_ = (maxVal_ + minVal_) * 0.5f;
        currentModulatedValue_ = baseValue_;
        if (onValueChanged_) {
            onValueChanged_(baseValue_);
        }
        repaint();
    }

private:
    void paintRotaryKnob(juce::Graphics& g) {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);

        // Chasis de fondo ranurado elegante
        g.setColour(juce::Colour(0xff080b12));
        g.fillRoundedRectangle(bounds, 3.5f);

        // Bisel superior sutil
        g.setColour(juce::Colour(0x15ffffff));
        g.drawHorizontalLine(static_cast<int>(bounds.getY()), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);

        // Resaltado de snapping si es candidato de drop
        if (isDropCandidate_) {
            g.setColour(juce::Colour(0xff00d4ff));
            g.drawRoundedRectangle(bounds, 3.5f, 1.8f);
        } else if (std::abs(modDepth_) > 0.001f) {
            g.setColour(modColor_.withAlpha(0.35f));
            g.drawRoundedRectangle(bounds, 3.5f, 1.0f);
        } else {
            g.setColour(juce::Colour(0xff18202c));
            g.drawRoundedRectangle(bounds, 3.5f, 0.8f);
        }

        // Geometría del dial rotatorio
        const float knobH = bounds.getHeight();
        const float cx = bounds.getX() + knobH * 0.5f;
        const float cy = bounds.getCentreY();
        const float trackR = std::min(knobH * 0.5f - 2.5f, 9.5f);

        // Ángulos de 270 grados (-135° a +135° con 0° a las 12 en punto)
        constexpr float kStartAngle = -0.75f * std::numbers::pi_v<float>;
        constexpr float kEndAngle = 0.75f * std::numbers::pi_v<float>;
        constexpr float kTotalAngle = 1.5f * std::numbers::pi_v<float>;

        const float normBase = (maxVal_ > minVal_) ? std::clamp((baseValue_ - minVal_) / (maxVal_ - minVal_), 0.0f, 1.0f) : 0.0f;
        const float normLive = (maxVal_ > minVal_) ? std::clamp((currentModulatedValue_ - minVal_) / (maxVal_ - minVal_), 0.0f, 1.0f) : 0.0f;

        const float baseAngle = kStartAngle + normBase * kTotalAngle;
        const float liveAngle = kStartAngle + normLive * kTotalAngle;

        // 1. Carril base oscuro (Groove perimetral)
        juce::Path trackPath;
        trackPath.addCentredArc(cx, cy, trackR, trackR, 0.0f, kStartAngle, kEndAngle, true);
        g.setColour(juce::Colour(0xff141a24));
        g.strokePath(trackPath, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // 2. Arco de valor base (aguja / trazo principal)
        if (normBase > 0.005f) {
            juce::Path baseArc;
            baseArc.addCentredArc(cx, cy, trackR, trackR, 0.0f, kStartAngle, baseAngle, true);
            g.setColour(juce::Colour(0xffd1d5db));
            g.strokePath(baseArc, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // 3. Arco Bipolar de Modulación (Regla 25: Base Value + Modulation Depth Arc)
        if (std::abs(modDepth_) > 0.001f) {
            const float modEndNorm = std::clamp(normBase + modDepth_, 0.0f, 1.0f);
            const float modEndAngle = kStartAngle + modEndNorm * kTotalAngle;
            const float arcStart = std::min(baseAngle, modEndAngle);
            const float arcEnd = std::max(baseAngle, modEndAngle);

            juce::Path modArc;
            modArc.addCentredArc(cx, cy, trackR, trackR, 0.0f, arcStart, arcEnd, true);

            // Halo difuso exterior reactivo
            g.setColour(modColor_.withAlpha(0.35f));
            g.strokePath(modArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Trazo continuo del arco con color de la fuente
            g.setColour(modColor_.withAlpha(0.95f));
            g.strokePath(modArc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // 4. Rotor Central de Titanio mecanizado CNC
        const float rotorR = trackR - 2.8f;
        if (rotorR > 2.0f) {
            juce::ColourGradient rotorGrad(juce::Colour(0xff242b38), cx, cy - rotorR,
                                           juce::Colour(0xff10151f), cx, cy + rotorR, false);
            g.setGradientFill(rotorGrad);
            g.fillEllipse(cx - rotorR, cy - rotorR, rotorR * 2.0f, rotorR * 2.0f);

            g.setColour(juce::Colour(0xff333e50));
            g.drawEllipse(cx - rotorR, cy - rotorR, rotorR * 2.0f, rotorR * 2.0f, 0.7f);

            // Aguja / Muesca de posición base en el rotor
            const float sinBase = std::sin(baseAngle);
            const float cosBase = std::cos(baseAngle);
            g.setColour(juce::Colour(0xffffffff));
            g.drawLine(cx + sinBase * (rotorR * 0.25f), cy - cosBase * (rotorR * 0.25f),
                       cx + sinBase * (rotorR * 0.95f), cy - cosBase * (rotorR * 0.95f), 1.2f);
        }

        // 5. Indicador animado de Modulación en Vivo a 60 FPS (Puck / LED dot) (Regla 25)
        const float puckX = cx + std::sin(liveAngle) * trackR;
        const float puckY = cy - std::cos(liveAngle) * trackR;

        // Halo del LED en vivo
        const auto liveCol = (std::abs(modDepth_) > 0.001f) ? modColor_ : juce::Colour(0xff00f0ff);
        g.setColour(liveCol.withAlpha(0.40f));
        g.fillEllipse(puckX - 3.5f, puckY - 3.5f, 7.0f, 7.0f);

        // Núcleo blanco brillante del LED
        g.setColour(juce::Colours::white);
        g.fillEllipse(puckX - 1.8f, puckY - 1.8f, 3.6f, 3.6f);

        // 6. Tipografía y Valores a la derecha del dial
        auto textArea = bounds.withTrimmedLeft(knobH + 5.0f).reduced(2.0f, 1.5f);

        // Fila 1: Nombre del parámetro
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xffd1d5db));
        g.drawText(name_, textArea.removeFromTop(11.0f), juce::Justification::centredLeft, true);

        // Fila 2: Valor numérico y profundidad de modulación porcentual
        juce::String valStr = juce::String(baseValue_, (maxVal_ - minVal_ > 10.0f) ? 1 : 2);
        if (std::abs(modDepth_) > 0.001f) {
            valStr += " (" + juce::String((modDepth_ > 0 ? "+" : "")) + juce::String(static_cast<int>(modDepth_ * 100.0f)) + "%)";
        }
        g.setFont(juce::FontOptions(8.5f, juce::Font::plain));
        g.setColour(std::abs(modDepth_) > 0.001f ? modColor_ : juce::Colour(0xff00f0ff));
        g.drawText(valStr, textArea, juce::Justification::centredLeft, true);
    }

    void paintLinearBar(juce::Graphics& g) {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        // 1. Chasis / Canal ranurado anodizado profundo
        g.setColour(juce::Colour(0xff080a10));
        g.fillRoundedRectangle(bounds, 3.0f);

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

        // 4. Aguja indicadora de posición base
        const float needleX = bounds.getX() + normBase * bounds.getWidth();
        if (barWidth > 1.0f && barWidth < bounds.getWidth() - 1.0f) {
            g.setColour(juce::Colour(0xffffffff));
            g.drawVerticalLine(static_cast<int>(needleX), bounds.getY() + 1.0f, bounds.getBottom() - 1.0f);
            g.setColour(std::abs(modDepth_) > 0.001f ? modColor_ : juce::Colour(0xff00f0ff));
            g.fillRect(needleX - 1.0f, bounds.getY() + 1.0f, 2.0f, 3.0f);
        }

        // 5. Indicador animado de modulación en vivo
        const float liveX = bounds.getX() + normLive * bounds.getWidth();
        g.setColour(juce::Colour(0xff06070a));
        g.fillEllipse(liveX - 4.5f, bounds.getCentreY() - 4.5f, 9.0f, 9.0f);

        g.setColour(std::abs(modDepth_) > 0.001f ? modColor_ : juce::Colours::white);
        g.drawEllipse(liveX - 4.5f, bounds.getCentreY() - 4.5f, 9.0f, 9.0f, 1.2f);
        g.fillEllipse(liveX - 2.0f, bounds.getCentreY() - 2.0f, 4.0f, 4.0f);

        // 6. Contorno exterior con micro-bisel
        if (isDropCandidate_) {
            g.setColour(juce::Colour(0xff00d4ff));
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
            valStr += " (" + juce::String(modDepth_ > 0 ? "+" : "") + juce::String(static_cast<int>(modDepth_ * 100.0f)) + "%)";
        }
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(std::abs(modDepth_) > 0.001f ? modColor_ : juce::Colour(0xff00f0ff));
        g.drawText(valStr, bounds.reduced(7.0f, 0.0f), juce::Justification::centredRight, true);
    }

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
            menu.addItem("Set Depth: -100%", [this]() {
                modDepth_ = -1.0f;
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
            menu.addItem("Drag any Macro or LFO onto this dial to modulate", false, false, nullptr);
        }

        menu.addSeparator();
        const bool isRotary = (style_ == SliderStyle::RotaryKnob);
        menu.addItem(isRotary ? "Switch to Linear Bar Style" : "Switch to Rotary Knob Style", [this, isRotary]() {
            setSliderStyle(isRotary ? SliderStyle::LinearBar : SliderStyle::RotaryKnob);
        });

        menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(this));
    }

    juce::String name_;
    float minVal_{ 0.0f };
    float maxVal_{ 1.0f };
    float baseValue_{ 0.5f };
    float modDepth_{ 0.0f };
    float currentModulatedValue_{ 0.5f };
    juce::Colour modColor_{ juce::Colours::white };

    SliderStyle style_{ SliderStyle::RotaryKnob };
    bool isDropCandidate_{ false };
    bool isAdjustingModDepth_{ false };
    juce::Point<float> dragStartPos_;
    float dragStartValue_{ 0.5f };
    float dragStartDepth_{ 0.0f };

    std::function<void(float)> onValueChanged_;
    std::function<void(ModSourceType, float, juce::Colour)> onModulationDropped_;
    std::function<void(float)> onModulationDepthChanged_;
    std::function<void()> onModulationRemoved_;
};

} // namespace audio_graph
