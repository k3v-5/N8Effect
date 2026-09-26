#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <memory>
#include <functional>
#include <cmath>
#include <algorithm>
#include "../plugin/PluginProcessor.h"
#include "../modulation/MacroManager.h"
#include "../modulation/ModulationTypes.h"
#include "ModulationDragPayload.h"
#include "ThemeManager.h"

namespace audio_graph {

/**
 * @brief Micro-conector de parcheo de modulacion minimalista estilo jack coaxial aeroespacial (Reglas 7, 23, 25, 48).
 * Elimina botones toscos rectangulares en favor de un conector jack aeroespacial integrado con corona
 * metalica pulida, bisel conico, 4 muescas moleteadas y micro-LED interior de estado.
 */
class MacroDragPin : public juce::Component, public juce::SettableTooltipClient {
public:
    MacroDragPin(ModSourceType srcType, const juce::String& srcName, juce::Colour col)
        : srcType_(srcType), srcName_(srcName), col_(col),
          theme_(ThemeManager::getInstance().getColors()),
          preset_(ThemeManager::getInstance().getCurrentPreset())
    {
        setRepaintsOnMouseActivity(true);
        setTooltip("Arrastra al dial de un nodo para modular, o haz clic para mapeo rapido");
    }

    void setSourceInfo(ModSourceType type, const juce::String& name, juce::Colour col) {
        srcType_ = type;
        srcName_ = name;
        col_ = col;
        repaint();
    }

    void setOnClick(std::function<void()> cb) { onClick_ = std::move(cb); }

    void applyTheme(const ThemeColors& theme, ThemePreset preset) {
        theme_ = theme;
        preset_ = preset;
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override {
        hasDragged_ = false;
        dragStartPos_ = e.position;
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        auto* ddc = juce::DragAndDropContainer::findParentDragContainerFor(this);
        const bool isDnd = (ddc != nullptr && ddc->isDragAndDropActive());
        const float dist = e.position.getDistanceFrom(dragStartPos_);
        if (!hasDragged_ && (dist > 4.0f || isDnd)) {
            hasDragged_ = true;
            if (ddc != nullptr) {
                auto encoded = ModulationDragPayload::encode(srcType_, srcName_, col_);
                auto snapshot = createComponentSnapshot(getLocalBounds());
                ddc->startDragging(encoded, this, juce::ScaledImage(snapshot), true);
            }
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (hasDragged_) {
            hasDragged_ = false;
            repaint();
            return; // Invariante Regla 48: supresion absoluta de clic tras arrastre
        }
        const float dist = e.position.getDistanceFrom(dragStartPos_);
        if (dist <= 4.0f && onClick_) {
            onClick_();
        }
        repaint();
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float r = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f - 0.5f;

        // Anillo exterior mecanizado estilo jack aeroespacial
        juce::Colour topMetal = theme_.cardSurface.brighter(0.18f);
        juce::Colour btmMetal = theme_.headerDark.darker(0.12f);
        juce::ColourGradient collarGrad(topMetal, cx - r, cy - r, btmMetal, cx + r, cy + r, false);
        g.setGradientFill(collarGrad);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);

        // Bisel perimetral CNC
        const bool isHot = isMouseOver() || hasDragged_;
        const auto rimColor = isHot
            ? (preset_ == ThemePreset::PhosphorCRT ? theme_.accentPrimary : theme_.borderFocused)
            : theme_.borderMuted.withAlpha(0.65f);
        g.setColour(rimColor);
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 0.75f);

        // 4 muescas moleteadas perimetrales a 45°, 135°, 225°, 315°
        static constexpr float kNotchAngles[] = { 0.7853982f, 2.3561945f, 3.9269908f, 5.4977871f };
        g.setColour(theme_.borderMuted.withAlpha(0.65f));
        for (float ang : kNotchAngles) {
            const float cosA = std::cos(ang);
            const float sinA = std::sin(ang);
            g.drawLine(cx + cosA * (r * 0.72f), cy + sinA * (r * 0.72f),
                       cx + cosA * (r * 0.98f), cy + sinA * (r * 0.98f), 0.8f);
        }

        // Cono interior del socket simulando profundidad en el chasis
        const float innerR = r * 0.58f;
        juce::ColourGradient boreGrad(theme_.headerDark, cx, cy - innerR,
                                      theme_.backgroundDark.darker(0.4f), cx, cy + innerR, false);
        g.setGradientFill(boreGrad);
        g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);
        g.setColour(theme_.backgroundDark.darker(0.5f));
        g.drawEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f, 0.5f);

        // Micro-LED joya central
        const float ledR = innerR * 0.52f;
        const juce::Colour ledCol = (preset_ == ThemePreset::PhosphorCRT)
            ? theme_.accentPrimary
            : (col_ != juce::Colours::white ? col_ : theme_.ledActive);

        if (isHot) {
            // Halo de brillo activo
            g.setColour(ledCol.withAlpha(0.35f));
            g.fillEllipse(cx - ledR * 1.85f, cy - ledR * 1.85f, ledR * 3.7f, ledR * 3.7f);
            g.setColour(ledCol);
            g.fillEllipse(cx - ledR, cy - ledR, ledR * 2.0f, ledR * 2.0f);
            // Destello especular
            g.setColour(juce::Colours::white.withAlpha(0.75f));
            g.fillEllipse(cx - ledR * 0.45f, cy - ledR * 0.45f, ledR * 0.65f, ledR * 0.65f);
        } else {
            // Halo sutil en reposo
            g.setColour(ledCol.withAlpha(0.18f));
            g.fillEllipse(cx - ledR * 1.4f, cy - ledR * 1.4f, ledR * 2.8f, ledR * 2.8f);
            g.setColour(ledCol.withAlpha(0.90f));
            g.fillEllipse(cx - ledR, cy - ledR, ledR * 2.0f, ledR * 2.0f);
        }
    }

private:
    ModSourceType srcType_{ ModSourceType::None };
    juce::String srcName_;
    juce::Colour col_{ juce::Colours::white };
    bool hasDragged_{ false };
    juce::Point<float> dragStartPos_;
    std::function<void()> onClick_;
    ThemeColors theme_{};
    ThemePreset preset_{ ThemePreset::CleanStudio };
};

/**
 * @brief Dial rotatorio de alta precision de inspiracion Minimalista Suizo / Teenage Engineering
 * (OP-1 / TX-6 / Arturia Pigments) para Macros de rendimiento (Reglas 7, 8, 23, 25, 48).
 * Incorpora rotor satinado aeroespacial, graduacion perimetral de 11 micro-ticks, carril rebajado,
 * arco de valor fino con halo difuso, puntero afilado, tipografia tecnica monotipo y guardia estricta Regla 48.
 */
class MacroKnob : public juce::Component {
public:
    MacroKnob(const juce::String& name, juce::RangedAudioParameter* param, ModSourceType srcType, juce::Colour col)
        : name_(name), param_(param), sourceType_(srcType), color_(col),
          dragPin_(srcType, name, col),
          theme_(ThemeManager::getInstance().getColors()),
          preset_(ThemeManager::getInstance().getCurrentPreset())
    {
        if (param_ != nullptr) {
            value_ = param_->getValue();
        }

        dragPin_.setOnClick([this]() {
            if (onMapClicked_) {
                onMapClicked_();
            }
        });
        addAndMakeVisible(dragPin_);
    }

    ~MacroKnob() override {
        endGesture();
    }

    void setValue(float v) {
        const float clamped = std::clamp(v, 0.0f, 1.0f);
        if (std::abs(value_ - clamped) > 0.0001f) {
            value_ = clamped;
            repaint();
        }
    }

    float getValue() const noexcept { return value_; }
    bool isDragging() const noexcept { return isDragging_; }

    void setOnValueChanged(std::function<void(float)> cb) { onValueChanged_ = std::move(cb); }
    void setOnMapClicked(std::function<void()> cb) { onMapClicked_ = std::move(cb); }

    void applyTheme(const ThemeColors& theme, ThemePreset preset) {
        theme_ = theme;
        preset_ = preset;
        dragPin_.applyTheme(theme, preset);
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            showContextMenu();
            return;
        }

        hasDragged_ = false;
        isDragging_ = false;
        dragStartPos_ = e.position;
        dragStartValue_ = value_;

        beginGesture();
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        const float dist = e.position.getDistanceFrom(dragStartPos_);
        if (!hasDragged_ && dist > 4.0f) {
            hasDragged_ = true;
            isDragging_ = true;
        }

        if (hasDragged_) {
            const float deltaY = dragStartPos_.y - e.position.y;
            const bool isShift = e.mods.isShiftDown();
            const float sensitivity = isShift ? 0.001f : 0.005f; // Modo precision 0.2x con Shift
            const float newVal = std::clamp(dragStartValue_ + deltaY * sensitivity, 0.0f, 1.0f);
            if (std::abs(newVal - value_) > 0.0001f) {
                setValueInternal(newVal, true);
            }
        }
    }

    void mouseUp(const juce::MouseEvent& /*e*/) override {
        endGesture();

        if (hasDragged_) {
            hasDragged_ = false;
            isDragging_ = false;
            repaint();
            return; // Invariante Regla 48: supresion total de clic tras arrastre
        }

        isDragging_ = false;
        repaint();
    }

    void mouseDoubleClick(const juce::MouseEvent& /*e*/) override {
        // Doble clic: reset a valor por defecto 50% (0.50f)
        beginGesture();
        setValueInternal(0.50f, true);
        endGesture();
    }

    void mouseWheelMove(const juce::MouseEvent& /*e*/, const juce::MouseWheelDetails& wheel) override {
        float delta = (wheel.deltaY > 0.0f ? 0.02f : (wheel.deltaY < 0.0f ? -0.02f : 0.0f));
        if (wheel.isReversed) delta = -delta;
        if (delta != 0.0f) {
            beginGesture();
            setValueInternal(std::clamp(value_ + delta, 0.0f, 1.0f), true);
            endGesture();
        }
    }

    void paint(juce::Graphics& g) override {
        // Dial de precision de 32px centrado verticalmente en el slot de 42px ((42 - 32) * 0.5 = 5px)
        constexpr float dialSize = 32.0f;
        constexpr float dialX = 6.0f;
        const float dialY = (static_cast<float>(getHeight()) - dialSize) * 0.5f;
        constexpr float cx = dialX + dialSize * 0.5f;
        const float cy = dialY + dialSize * 0.5f;

        // Geometria angular (barrido de 270° = 1.5*pi)
        constexpr float startAngle = 3.9269908f; // 225° (7:30 o'clock, bottom-left)
        constexpr float endAngle   = 8.6393798f; // 495° = 135° (4:30 o'clock, bottom-right)
        constexpr float sweepAngle = endAngle - startAngle; // 270° (1.5 * pi = 4.7123890f)
        const float currentAngle = startAngle + value_ * sweepAngle;

        // 1. Marcas discretas perimetrales (11 micro-ticks calibrados a intervalos de 10% y puntos 25%/75%)
        struct TickDef { float frac; bool major; };
        static constexpr TickDef kTicks[] = {
            { 0.00f, true  },
            { 0.10f, false },
            { 0.20f, false },
            { 0.25f, true  },
            { 0.30f, false },
            { 0.40f, false },
            { 0.50f, true  },
            { 0.60f, false },
            { 0.70f, false },
            { 0.75f, true  },
            { 0.80f, false },
            { 0.90f, false },
            { 1.00f, true  }
        };

        constexpr float tickBaseR = 13.8f;
        for (const auto& t : kTicks) {
            const float tickLen = t.major ? 2.2f : 1.2f;
            const float tickStroke = t.major ? 1.0f : 0.75f;
            const float ang = startAngle + t.frac * sweepAngle;
            const float cosA = std::cos(ang);
            const float sinA = std::sin(ang);

            const bool isActive = (t.frac <= value_ + 0.005f);
            if (isActive) {
                const auto activeTickCol = (preset_ == ThemePreset::PhosphorCRT)
                    ? theme_.accentPrimary
                    : color_;
                g.setColour(activeTickCol.withAlpha(0.95f));
            } else {
                g.setColour(theme_.borderMuted.withAlpha(0.40f));
            }

            g.drawLine(cx + sinA * tickBaseR, cy - cosA * tickBaseR,
                       cx + sinA * (tickBaseR + tickLen), cy - cosA * (tickBaseR + tickLen),
                       tickStroke);
        }

        // 2. Carril rebajado (Recessed groove track)
        constexpr float trackRadius = 12.5f;
        juce::Path trackPath;
        trackPath.addCentredArc(cx, cy, trackRadius, trackRadius, 0.0f, startAngle, endAngle, true);
        g.setColour(theme_.backgroundDark.darker(0.3f));
        g.strokePath(trackPath, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // 3. Arco de valor activo fino (1.8px) con terminaciones redondeadas y resplandor sutil (glow halo)
        if (value_ > 0.001f) {
            juce::Path arcPath;
            arcPath.addCentredArc(cx, cy, trackRadius, trackRadius, 0.0f, startAngle, currentAngle, true);

            const auto arcCol = (preset_ == ThemePreset::PhosphorCRT)
                ? theme_.accentPrimary
                : color_;

            // Resplandor difuso exterior (glow halo)
            const auto glowCol = (preset_ == ThemePreset::PhosphorCRT)
                ? theme_.wireGlowColor.withAlpha(0.25f)
                : arcCol.withAlpha(0.20f);
            g.setColour(glowCol);
            g.strokePath(arcPath, juce::PathStrokeType(3.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Linea de arco principal de alta definicion (1.8px)
            g.setColour(arcCol);
            g.strokePath(arcPath, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // 4. Rotor circular plano con acabado mate oscuro aeroespacial (Rotor disc)
        constexpr float rotorR = 9.8f;
        juce::ColourGradient rotorGrad(theme_.cardSurface.brighter(0.08f), cx, cy - rotorR,
                                       theme_.cardSurface.darker(0.12f), cx, cy + rotorR, false);
        g.setGradientFill(rotorGrad);
        g.fillEllipse(cx - rotorR, cy - rotorR, rotorR * 2.0f, rotorR * 2.0f);

        // Bisel sutil mecanizado CNC
        g.setColour(theme_.borderMuted.withAlpha(0.50f));
        g.drawEllipse(cx - rotorR, cy - rotorR, rotorR * 2.0f, rotorR * 2.0f, 0.75f);

        // 5. Aguja puntero de precision afilada
        const float sinNeedle = std::sin(currentAngle);
        const float cosNeedle = std::cos(currentAngle);
        const float nx1 = cx + sinNeedle * 2.2f;
        const float ny1 = cy - cosNeedle * 2.2f;
        const float nx2 = cx + sinNeedle * 9.0f;
        const float ny2 = cy - cosNeedle * 9.0f;

        const auto needleCol = (preset_ == ThemePreset::PhosphorCRT)
            ? theme_.accentPrimary
            : theme_.textPrimary;
        g.setColour(needleCol);
        g.drawLine(nx1, ny1, nx2, ny2, 1.4f);

        // Micro-pivote central
        g.setColour(theme_.backgroundDark);
        g.fillEllipse(cx - 1.2f, cy - 1.2f, 2.4f, 2.4f);

        // 6. Tipografia tecnica monotipo centrada
        constexpr int textLeft = 44;
        const int textRight = getWidth() - 24;
        const int textWidth = std::max(10, textRight - textLeft);

        // Fila 1: Titulo del Macro en negrita tecnica monotipo
        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 8.5f, juce::Font::bold));
        const auto titleCol = (preset_ == ThemePreset::PhosphorCRT)
            ? theme_.accentPrimary
            : theme_.textPrimary;
        g.setColour(titleCol);
        g.drawText(name_, textLeft, 7, textWidth, 13, juce::Justification::centredLeft, true);

        // Fila 2: Lectura numerica porcentual ordenada
        const int pct = static_cast<int>(std::round(value_ * 100.0f));
        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 8.0f, juce::Font::plain));
        g.setColour(theme_.textSecondary);
        g.drawText(juce::String(pct) + "%", textLeft, 21, textWidth, 13, juce::Justification::centredLeft, true);
    }

    void resized() override {
        constexpr int pinSize = 16;
        dragPin_.setBounds(getWidth() - pinSize - 6,
                           static_cast<int>((getHeight() - pinSize) * 0.5f),
                           pinSize, pinSize);
    }

private:
    void beginGesture() {
        if (param_ != nullptr && !isGestureActive_) {
            isGestureActive_ = true;
            param_->beginChangeGesture();
        }
    }

    void endGesture() {
        if (param_ != nullptr && isGestureActive_) {
            isGestureActive_ = false;
            param_->endChangeGesture();
        }
    }

    void setValueInternal(float newVal, bool notifyHost) {
        value_ = newVal;
        if (notifyHost && param_ != nullptr) {
            param_->setValueNotifyingHost(value_);
        }
        if (onValueChanged_) {
            onValueChanged_(value_);
        }
        repaint();
    }

    void showContextMenu() {
        juce::PopupMenu menu;
        menu.addSectionHeader(name_ + " (" + juce::String(static_cast<int>(std::round(value_ * 100.0f))) + "%)");
        menu.addItem(1, "Reset to Default (50%)", true, false);
        menu.addItem(2, "Set to 0%", true, false);
        menu.addItem(3, "Set to 100%", true, false);
        menu.addSeparator();
        menu.addItem(4, "Quick Map to Selected Node...", onMapClicked_ != nullptr, false);

        juce::Component::SafePointer<MacroKnob> safeThis(this);
        menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(this),
            [safeThis](int result) {
                if (safeThis == nullptr) return;
                if (result == 1) { // Reset
                    safeThis->beginGesture();
                    safeThis->setValueInternal(0.50f, true);
                    safeThis->endGesture();
                } else if (result == 2) { // 0%
                    safeThis->beginGesture();
                    safeThis->setValueInternal(0.0f, true);
                    safeThis->endGesture();
                } else if (result == 3) { // 100%
                    safeThis->beginGesture();
                    safeThis->setValueInternal(1.0f, true);
                    safeThis->endGesture();
                } else if (result == 4) { // Quick Map
                    if (safeThis->onMapClicked_) {
                        safeThis->onMapClicked_();
                    }
                }
            });
    }

    juce::String name_;
    juce::RangedAudioParameter* param_{ nullptr };
    ModSourceType sourceType_{ ModSourceType::None };
    juce::Colour color_{ juce::Colours::white };
    float value_{ 0.5f };

    bool isGestureActive_{ false };
    bool hasDragged_{ false };
    bool isDragging_{ false };
    float dragStartValue_{ 0.5f };
    juce::Point<float> dragStartPos_;

    MacroDragPin dragPin_;
    ThemeColors theme_{};
    ThemePreset preset_{ ThemePreset::CleanStudio };

    std::function<void(float)> onValueChanged_;
    std::function<void()> onMapClicked_;
};

/**
 * @brief Dashboard visual con 8 Performance Macros globales (Reglas 7, 8, 23, 25).
 * Estilo dock minimalista horizontal para anclaje inferior con reactividad dinamica a ThemeManager.
 */
class MacroDashboardComponent : public juce::Component, public ThemeManager::Listener {
public:
    explicit MacroDashboardComponent(N8AudioProcessor& processor)
        : processor_(processor)
    {
        setOpaque(true);

        const char* macroIds[8] = {
            "macro_texture", "macro_motion", "macro_space", "macro_color",
            "macro_chaos", "macro_density", "macro_energy", "macro_morph"
        };
        const char* macroNames[8] = {
            "TEXTURE", "MOTION", "SPACE", "COLOR",
            "CHAOS", "DENSITY", "ENERGY", "MORPH"
        };

        for (size_t i = 0; i < 8; ++i) {
            auto* param = processor_.getAPVTS().getParameter(macroIds[i]);
            const auto srcType = static_cast<ModSourceType>(static_cast<size_t>(ModSourceType::MacroTexture) + i);
            const auto col = ModulationDragPayload::getDefaultColor(srcType);
            auto knob = std::make_unique<MacroKnob>(macroNames[i], param, srcType, col);

            const size_t macroIdx = i;
            knob->setOnMapClicked([this, macroIdx]() {
                handleQuickMap(macroIdx);
            });

            addAndMakeVisible(*knob);
            knobs_[i] = std::move(knob);
        }

        // Suscripcion dinamica y reactiva a ThemeManager (Regla 23, 25)
        ThemeManager::getInstance().addListener(this);
        themeChanged(ThemeManager::getInstance().getColors(), ThemeManager::getInstance().getCurrentPreset());
    }

    ~MacroDashboardComponent() override {
        ThemeManager::getInstance().removeListener(this);
    }

    void themeChanged(const ThemeColors& newTheme, ThemePreset preset) override {
        theme_ = newTheme;
        preset_ = preset;
        for (auto& knob : knobs_) {
            if (knob != nullptr) {
                knob->applyTheme(newTheme, preset);
            }
        }
        repaint();
    }

    void updateKnobValues() {
        const char* macroIds[8] = {
            "macro_texture", "macro_motion", "macro_space", "macro_color",
            "macro_chaos", "macro_density", "macro_energy", "macro_morph"
        };
        for (size_t i = 0; i < 8; ++i) {
            if (knobs_[i] != nullptr && !knobs_[i]->isDragging()) {
                if (auto* p = processor_.getAPVTS().getParameter(macroIds[i])) {
                    knobs_[i]->setValue(p->getValue());
                }
            }
        }
    }

    void setSelectedNodeId(NodeId id) noexcept {
        selectedNodeId_ = id;
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // Chasis titanio oscuro minimalista de consola adaptado dinamicamente al tema activo
        juce::ColourGradient bgGrad(theme_.panelSurface, bounds.getTopLeft(),
                                    theme_.backgroundDark, bounds.getBottomLeft(), false);
        g.setGradientFill(bgGrad);
        g.fillRect(bounds);

        // Borde superior hairline rim en borderMuted
        g.setColour(theme_.borderMuted);
        g.drawHorizontalLine(0, bounds.getX(), bounds.getRight());

        // Divisores sutiles entre los 8 slots
        const float slotW = bounds.getWidth() / 8.0f;
        g.setColour(theme_.borderMuted.withAlpha(0.25f));
        for (int i = 1; i < 8; ++i) {
            const float x = std::round(slotW * static_cast<float>(i));
            g.drawVerticalLine(static_cast<int>(x), 3.0f, bounds.getBottom() - 3.0f);
        }
    }

    void resized() override {
        auto area = getLocalBounds();
        const int totalW = area.getWidth();
        const int slotW = totalW / 8;

        for (size_t i = 0; i < 8; ++i) {
            const int x = static_cast<int>(i) * slotW;
            const int w = (i == 7) ? (totalW - x) : slotW;
            knobs_[i]->setBounds(x, 0, w, area.getHeight());
        }
    }

private:
    void handleQuickMap(size_t macroIdx) {
        if (selectedNodeId_ == InvalidNodeId) {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Mapeo de Macro",
                "Por favor selecciona primero un nodo en el canvas del grafo para mapearle este macro.");
            return;
        }

        auto* node = processor_.getGraph().getNode(selectedNodeId_);
        if (node == nullptr || node->processor == nullptr) return;

        auto params = node->processor->getParameters();
        if (params.empty()) return;

        juce::PopupMenu menu;
        menu.addSectionHeader("Mapear a: " + juce::String(node->name));

        const auto srcType = static_cast<ModSourceType>(static_cast<size_t>(ModSourceType::MacroTexture) + macroIdx);

        for (size_t p = 0; p < params.size(); ++p) {
            const auto pid = params[p].id;
            const auto pname = params[p].name;
            menu.addItem(juce::PopupMenu::Item("Param: " + juce::String(pname)).setAction([this, srcType, pid]() {
                auto& mat = processor_.getDualWorldEngine().getModulationEngine().getMatrix();
                mat.addRoute(srcType, selectedNodeId_, pid, 0.75f, true);
            }));
        }

        menu.addSeparator();
        menu.addItem(juce::PopupMenu::Item("Limpiar mapeos de este nodo").setAction([this]() {
            auto& mat = processor_.getDualWorldEngine().getModulationEngine().getMatrix();
            for (size_t r = 0; r < mat.getMaxRoutes(); ++r) {
                if (mat.getRoute(r).targetNodeId == selectedNodeId_) {
                    mat.removeRoute(r);
                }
            }
        }));

        menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(knobs_[macroIdx].get()));
    }

    N8AudioProcessor& processor_;
    NodeId selectedNodeId_{ InvalidNodeId };
    std::array<std::unique_ptr<MacroKnob>, 8> knobs_;
    ThemeColors theme_{};
    ThemePreset preset_{ ThemePreset::CleanStudio };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroDashboardComponent)
};

} // namespace audio_graph
