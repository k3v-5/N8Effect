#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <memory>
#include <functional>
#include <cmath>
#include "../plugin/PluginProcessor.h"
#include "../modulation/MacroManager.h"
#include "../modulation/ModulationTypes.h"
#include "ModulationDragPayload.h"

namespace audio_graph {

/**
 * @brief Micro-conector de parcheo de modulacion minimalista estilo jack modular (Reglas 7, 23, 25, 48).
 * Elimina los botones toscos rectangulares '+ DRAG' en favor de un puerto concentrico aeroespacial.
 */
class MacroDragPin : public juce::Component, public juce::SettableTooltipClient {
public:
    MacroDragPin(ModSourceType srcType, const juce::String& srcName, juce::Colour col)
        : srcType_(srcType), srcName_(srcName), col_(col)
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

    void mouseDown(const juce::MouseEvent& /*e*/) override {
        hasDragged_ = false;
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        auto* ddc = juce::DragAndDropContainer::findParentDragContainerFor(this);
        const bool isDnd = (ddc != nullptr && ddc->isDragAndDropActive());
        if (!hasDragged_ && (e.getDistanceFromDragStart() > 4 || isDnd)) {
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
            return; // Invariante Regla 48: supresion de clic tras arrastre
        }
        if (e.getDistanceFromDragStart() <= 4 && onClick_) {
            onClick_();
        }
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float r = std::min(bounds.getWidth(), bounds.getHeight()) * 0.44f;

        // Anillo exterior mecanizado
        g.setColour(juce::Colour(0xff181d26));
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour(isMouseOver() ? col_ : juce::Colour(0xff2d3646));
        g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);

        // Agujero central
        const float innerR = r * 0.55f;
        g.setColour(juce::Colour(0xff080a0e));
        g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);

        // LED central luminoso
        g.setColour(col_.withAlpha(isMouseOver() ? 1.0f : 0.8f));
        g.fillEllipse(cx - innerR * 0.55f, cy - innerR * 0.55f, innerR * 1.1f, innerR * 1.1f);
    }

private:
    ModSourceType srcType_{ ModSourceType::None };
    juce::String srcName_;
    juce::Colour col_{ juce::Colours::white };
    bool hasDragged_{ false };
    std::function<void()> onClick_;
};

/**
 * @brief Dial rotatorio ultra-minimalista para Macro individual (Estilo Arturia / FLEX / Pigments).
 */
class MacroKnob : public juce::Component {
public:
    MacroKnob(const juce::String& name, juce::RangedAudioParameter* param, ModSourceType srcType, juce::Colour col)
        : name_(name), param_(param), sourceType_(srcType), color_(col),
          dragPin_(srcType, name, col)
    {
        slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider_.setRange(0.0, 1.0, 0.001);
        slider_.setValue(param_ ? param_->getValue() : 0.5);

        slider_.onValueChange = [this]() {
            if (param_ != nullptr) {
                param_->setValueNotifyingHost(static_cast<float>(slider_.getValue()));
            }
            if (onValueChanged_) {
                onValueChanged_(static_cast<float>(slider_.getValue()));
            }
            repaint();
        };

        addAndMakeVisible(slider_);

        dragPin_.setOnClick([this]() {
            if (onMapClicked_) {
                onMapClicked_();
            }
        });
        addAndMakeVisible(dragPin_);
    }

    void setValue(float v) {
        slider_.setValue(v, juce::dontSendNotification);
        repaint();
    }

    float getValue() const noexcept {
        return static_cast<float>(slider_.getValue());
    }

    void setOnValueChanged(std::function<void(float)> cb) { onValueChanged_ = std::move(cb); }
    void setOnMapClicked(std::function<void()> cb) { onMapClicked_ = std::move(cb); }

    void paint(juce::Graphics& g) override {
        // Area del dial rotatorio (izquierda)
        const float dialSize = 26.0f;
        const float dialX = 4.0f;
        const float dialY = (getHeight() - dialSize) * 0.5f;
        const float cx = dialX + dialSize * 0.5f;
        const float cy = dialY + dialSize * 0.5f;
        const float radius = dialSize * 0.44f;

        // Anillo de fondo
        g.setColour(juce::Colour(0xff141820));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 2.0f);

        // Anillo de valor activo (Arco luminoso de 2.4 rad a 7.0 rad)
        constexpr float startAngle = 2.4f;
        constexpr float endAngle = 7.0f;
        const float currentAngle = startAngle + static_cast<float>(slider_.getValue()) * (endAngle - startAngle);

        juce::Path arcPath;
        arcPath.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, currentAngle, true);
        g.setColour(color_);
        g.strokePath(arcPath, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Tapa central del dial
        const float capR = radius * 0.68f;
        g.setColour(juce::Colour(0xff090c10));
        g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
        g.setColour(juce::Colour(0xff1e2530));
        g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.0f);

        // Aguja / Notch indicador
        const float needleLen = capR * 0.85f;
        const float nx = cx + std::sin(currentAngle) * needleLen;
        const float ny = cy - std::cos(currentAngle) * needleLen;
        g.setColour(juce::Colours::white);
        g.drawLine(cx, cy, nx, ny, 1.4f);

        // Textos del Macro (derecha del dial)
        const int textLeft = static_cast<int>(dialX + dialSize + 6.0f);
        const int textWidth = getWidth() - textLeft - 20;

        // Fila 1: Nombre en negrita
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.setColour(color_.withAlpha(0.95f));
        g.drawText(name_, textLeft, 6, textWidth, 12, juce::Justification::centredLeft, true);

        // Fila 2: Valor porcentual sutil
        const int pct = static_cast<int>(std::round(slider_.getValue() * 100.0));
        g.setFont(juce::FontOptions(8.0f, juce::Font::plain));
        g.setColour(juce::Colour(0xff8c96a5));
        g.drawText(juce::String(pct) + "%", textLeft, 18, textWidth, 12, juce::Justification::centredLeft, true);
    }

    void resized() override {
        // El slider cubre el area del dial rotatorio en el lado izquierdo
        slider_.setBounds(2, static_cast<int>((getHeight() - 28) * 0.5f), 28, 28);

        // El pin de modulacion se ubica en el extremo derecho
        const int pinSize = 14;
        dragPin_.setBounds(getWidth() - pinSize - 4, static_cast<int>((getHeight() - pinSize) * 0.5f), pinSize, pinSize);
    }

private:
    juce::String name_;
    juce::RangedAudioParameter* param_{ nullptr };
    ModSourceType sourceType_{ ModSourceType::None };
    juce::Colour color_{ juce::Colours::white };
    juce::Slider slider_;
    MacroDragPin dragPin_;

    std::function<void(float)> onValueChanged_;
    std::function<void()> onMapClicked_;
};

/**
 * @brief Dashboard visual con 8 Performance Macros globales (Reglas 7, 8, 23, 25).
 * Estilo dock minimalista horizontal para anclaje inferior.
 */
class MacroDashboardComponent : public juce::Component {
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
    }

    void updateKnobValues() {
        const char* macroIds[8] = {
            "macro_texture", "macro_motion", "macro_space", "macro_color",
            "macro_chaos", "macro_density", "macro_energy", "macro_morph"
        };
        for (size_t i = 0; i < 8; ++i) {
            if (auto* p = processor_.getAPVTS().getParameter(macroIds[i])) {
                knobs_[i]->setValue(p->getValue());
            }
        }
    }

    void setSelectedNodeId(NodeId id) noexcept {
        selectedNodeId_ = id;
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // Chasis titanio oscuro minimalista de consola
        juce::ColourGradient bgGrad(juce::Colour(0xff0c0f16), bounds.getTopLeft(),
                                    juce::Colour(0xff06070a), bounds.getBottomLeft(), false);
        g.setGradientFill(bgGrad);
        g.fillRect(bounds);

        // Borde superior luminoso sutil
        g.setColour(juce::Colour(0xff1e2634));
        g.drawHorizontalLine(0, bounds.getX(), bounds.getRight());

        // Divisores sutiles entre los 8 slots
        const float slotW = bounds.getWidth() / 8.0f;
        g.setColour(juce::Colour(0xff121620));
        for (int i = 1; i < 8; ++i) {
            const float x = std::round(slotW * static_cast<float>(i));
            g.drawVerticalLine(static_cast<int>(x), 4.0f, bounds.getBottom() - 4.0f);
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroDashboardComponent)
};

} // namespace audio_graph
