#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <memory>
#include <functional>
#include "../plugin/PluginProcessor.h"
#include "../modulation/MacroManager.h"
#include "../modulation/ModulationTypes.h"

namespace audio_graph {

/**
 * @brief Control rotatorio estilizado para un Macro individual con anillo luminoso (Reglas 8, 23, 25).
 */
class MacroKnob : public juce::Component {
public:
    MacroKnob(const juce::String& name, juce::RangedAudioParameter* param)
        : name_(name), param_(param)
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

        mapBtn_.setButtonText("+ MAP");
        mapBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff141414));
        mapBtn_.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.75f));
        mapBtn_.onClick = [this]() {
            if (onMapClicked_) {
                onMapClicked_();
            }
        };
        addAndMakeVisible(mapBtn_);
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
        auto bounds = getLocalBounds().toFloat();
        const auto knobArea = bounds.removeFromTop(bounds.getHeight() - 32.0f).reduced(4.0f);

        const float centreX = knobArea.getCentreX();
        const float centreY = knobArea.getCentreY();
        const float radius = std::min(knobArea.getWidth(), knobArea.getHeight()) * 0.42f;

        // Anillo de fondo
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 3.0f);

        // Anillo de valor activo (Arco blanco luminoso de 7 a 5 en reloj)
        constexpr float startAngle = 2.4f;
        constexpr float endAngle = 7.0f;
        const float currentAngle = startAngle + static_cast<float>(slider_.getValue()) * (endAngle - startAngle);

        juce::Path arcPath;
        arcPath.addCentredArc(centreX, centreY, radius, radius, 0.0f, startAngle, currentAngle, true);
        g.setColour(juce::Colours::white);
        g.strokePath(arcPath, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Núcleo central
        g.setColour(juce::Colour(0xff0d0d0d));
        g.fillEllipse(centreX - radius * 0.7f, centreY - radius * 0.7f, radius * 1.4f, radius * 1.4f);
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawEllipse(centreX - radius * 0.7f, centreY - radius * 0.7f, radius * 1.4f, radius * 1.4f, 1.0f);

        // Indicador de aguja
        const float needleLen = radius * 0.65f;
        const float nx = centreX + std::sin(currentAngle) * needleLen;
        const float ny = centreY - std::cos(currentAngle) * needleLen;
        g.setColour(juce::Colours::white);
        g.drawLine(centreX, centreY, nx, ny, 1.8f);

        // Título del Macro
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(juce::Colours::white);
        g.drawText(name_, 0, static_cast<int>(bounds.getY() - 4), getWidth(), 14, juce::Justification::centred);
    }

    void resized() override {
        auto area = getLocalBounds();
        mapBtn_.setBounds(area.removeFromBottom(16).reduced(6, 0));
        area.removeFromBottom(16); // Espacio para el texto de nombre
        slider_.setBounds(area);
    }

private:
    juce::String name_;
    juce::RangedAudioParameter* param_{ nullptr };
    juce::Slider slider_;
    juce::TextButton mapBtn_;

    std::function<void(float)> onValueChanged_;
    std::function<void()> onMapClicked_;
};

/**
 * @brief Dashboard visual con 8 Performance Macros globales (Reglas 7, 8, 23, 25).
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
            auto knob = std::make_unique<MacroKnob>(macroNames[i], param);

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

        // Fondo oscuro
        g.setColour(juce::Colour(0xff080808));
        g.fillRoundedRectangle(bounds, 4.0f);

        // Borde fino
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
    }

    void resized() override {
        auto area = getLocalBounds().reduced(6, 4);
        const int knobWidth = area.getWidth() / 8;

        for (size_t i = 0; i < 8; ++i) {
            knobs_[i]->setBounds(area.removeFromLeft(knobWidth).reduced(3, 0));
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

        // Mapear automáticamente al primer parámetro libre o abrir menú emergente
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
