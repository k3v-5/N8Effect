#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>
#include <string>
#include <algorithm>
#include "../core/Types.h"
#include "../graph/Graph.h"
#include "../modulation/NodeAutomationSequencer.h"

namespace audio_graph {

/**
 * @brief Componente Visual de Secuenciación de Automatización por Nodo (Reglas 4, 8, 23, 24, 25).
 * Permite editar hasta 4 pistas simultáneas de automatización por pasos para cualquier parámetro
 * del efecto seleccionado en el canvas del grafo DAG, con cursor de playhead en tiempo real.
 */
class NodeAutomationSequencerComponent : public juce::Component {
public:
    NodeAutomationSequencerComponent() {
        setRepaintsOnMouseActivity(true);

        // 1. Botones de Pestañas Multi-Lane (Lane 1 a 4)
        for (size_t i = 0; i < NodeAutomationBank::MaxLanes; ++i) {
            auto& btn = laneTabs_[i];
            btn.setButtonText("Lane " + juce::String(static_cast<int>(i + 1)));
            btn.onClick = [this, i]() {
                selectLane(i);
            };
            addAndMakeVisible(btn);
        }

        // 2. Selector de Parámetro Destino
        paramCombo_.setTextWhenNothingSelected("Select Parameter...");
        paramCombo_.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff121218));
        paramCombo_.setColour(juce::ComboBox::textColourId, juce::Colours::white);
        paramCombo_.onChange = [this]() {
            if (activeBank_ != nullptr) {
                auto& lane = activeBank_->getLane(selectedLaneIndex_);
                const int selectedId = paramCombo_.getSelectedId();
                if (selectedId > 0) {
                    lane.targetParamId = static_cast<ParameterId>(selectedId);
                    lane.hasCapturedBase = false;
                } else {
                    lane.targetParamId = 0;
                }
                updateLaneTabLabels();
                repaint();
            }
        };
        addAndMakeVisible(paramCombo_);

        // 3. Botón de Activación de la Pista
        activeToggleBtn_.setButtonText("Active: OFF");
        activeToggleBtn_.onClick = [this]() {
            if (activeBank_ != nullptr) {
                auto& lane = activeBank_->getLane(selectedLaneIndex_);
                lane.active = !lane.active;
                activeToggleBtn_.setButtonText(lane.active ? "Active: ON" : "Active: OFF");
                updateLaneTabLabels();
                repaint();
            }
        };
        addAndMakeVisible(activeToggleBtn_);

        // 4. Selector de Métrica / Sync Rate
        rateCombo_.addItem("1/4 Beat", 1);
        rateCombo_.addItem("1/8 Beat", 2);
        rateCombo_.addItem("1/16 Beat", 3);
        rateCombo_.addItem("1/32 Beat", 4);
        rateCombo_.addItem("1/8T Triplet", 5);
        rateCombo_.addItem("1/16T Triplet", 6);
        rateCombo_.addItem("1/8D Dotted", 7);
        rateCombo_.setSelectedId(3, juce::dontSendNotification); // 1/16 por defecto
        rateCombo_.onChange = [this]() {
            if (activeBank_ != nullptr) {
                auto& lane = activeBank_->getLane(selectedLaneIndex_);
                switch (rateCombo_.getSelectedId()) {
                    case 1: lane.rate = SyncDivision::Quarter; break;
                    case 2: lane.rate = SyncDivision::Eighth; break;
                    case 3: lane.rate = SyncDivision::Sixteenth; break;
                    case 4: lane.rate = SyncDivision::ThirtySecond; break;
                    case 5: lane.rate = SyncDivision::Triplet_Eighth; break;
                    case 6: lane.rate = SyncDivision::Triplet_Quarter; break;
                    case 7: lane.rate = SyncDivision::Dotted_Eighth; break;
                    default: lane.rate = SyncDivision::Sixteenth; break;
                }
            }
        };
        addAndMakeVisible(rateCombo_);

        // 5. Selector de Pasos (16 o 32)
        stepsCombo_.addItem("16 Steps", 1);
        stepsCombo_.addItem("32 Steps", 2);
        stepsCombo_.setSelectedId(1, juce::dontSendNotification);
        stepsCombo_.onChange = [this]() {
            if (activeBank_ != nullptr) {
                auto& lane = activeBank_->getLane(selectedLaneIndex_);
                lane.numSteps = (stepsCombo_.getSelectedId() == 2) ? 32 : 16;
                repaint();
            }
        };
        addAndMakeVisible(stepsCombo_);

        // 6. Sliders de Glide y Profundidad (Amount)
        glideSlider_.setSliderStyle(juce::Slider::LinearBar);
        glideSlider_.setRange(0.0, 1.0, 0.01);
        glideSlider_.setValue(0.1);
        glideSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 36, 16);
        glideSlider_.onValueChange = [this]() {
            if (activeBank_ != nullptr) {
                activeBank_->getLane(selectedLaneIndex_).glide = static_cast<float>(glideSlider_.getValue());
            }
        };
        addAndMakeVisible(glideSlider_);

        glideLabel_.setText("GLIDE", juce::dontSendNotification);
        glideLabel_.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        glideLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(glideLabel_);

        amountSlider_.setSliderStyle(juce::Slider::LinearBar);
        amountSlider_.setRange(0.0, 1.0, 0.01);
        amountSlider_.setValue(1.0);
        amountSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 36, 16);
        amountSlider_.onValueChange = [this]() {
            if (activeBank_ != nullptr) {
                activeBank_->getLane(selectedLaneIndex_).amount = static_cast<float>(amountSlider_.getValue());
            }
        };
        addAndMakeVisible(amountSlider_);

        amountLabel_.setText("DEPTH", juce::dontSendNotification);
        amountLabel_.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        amountLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(amountLabel_);

        // 7. Botones de Formas Rápidas (Quick Shapes)
        setupShapeButton(pumpBtn_, "Pump", NodeAutomationBank::SidechainPump);
        setupShapeButton(sawBtn_, "Saw", NodeAutomationBank::RampUp);
        setupShapeButton(staccatoBtn_, "Staccato", NodeAutomationBank::Staccato);
        setupShapeButton(triBtn_, "Tri", NodeAutomationBank::Triangle);
        setupShapeButton(randBtn_, "Rand", NodeAutomationBank::Random);
        setupShapeButton(flatBtn_, "Flat", NodeAutomationBank::Flat);

        // 8. Botón Cerrar / Ocultar Carril
        closeBtn_.setButtonText(juce::String::fromUTF8("×"));
        closeBtn_.onClick = [this]() {
            if (onCloseRequested_) onCloseRequested_();
        };
        addAndMakeVisible(closeBtn_);
    }

    void setTargetNode(NodeId id, NodeInstance* inst) {
        currentNodeId_ = id;
        currentNodeInst_ = inst;

        if (inst != nullptr && inst->processor != nullptr) {
            activeBank_ = &inst->sequencer;
            nodeTitle_ = "[ " + juce::String(inst->name).toUpperCase() + " ] AUTOMATION SEQUENCER";

            // Poblar dropdown con los parámetros del nodo
            paramCombo_.clear(juce::dontSendNotification);
            const auto params = inst->processor->getParameters();
            for (const auto& p : params) {
                paramCombo_.addItem(p.name, static_cast<int>(p.id));
            }

            selectLane(selectedLaneIndex_);
            updateLaneTabLabels();
            setEnabled(true);
        } else {
            activeBank_ = nullptr;
            nodeTitle_ = "NO EFFECT SELECTED — CLICK A NODE ON THE CANVAS";
            paramCombo_.clear(juce::dontSendNotification);
            setEnabled(false);
        }
        repaint();
    }

    void selectLane(size_t laneIdx) {
        selectedLaneIndex_ = laneIdx < NodeAutomationBank::MaxLanes ? laneIdx : 0;
        if (activeBank_ != nullptr) {
            activeBank_->setSelectedLaneIndex(selectedLaneIndex_);
            const auto& lane = activeBank_->getLane(selectedLaneIndex_);

            activeToggleBtn_.setButtonText(lane.active ? "Active: ON" : "Active: OFF");
            if (lane.targetParamId > 0) {
                paramCombo_.setSelectedId(static_cast<int>(lane.targetParamId), juce::dontSendNotification);
            } else {
                paramCombo_.setTextWhenNothingSelected("Select Parameter...");
                paramCombo_.setSelectedId(0, juce::dontSendNotification);
            }

            stepsCombo_.setSelectedId(lane.numSteps == 32 ? 2 : 1, juce::dontSendNotification);
            glideSlider_.setValue(lane.glide, juce::dontSendNotification);
            amountSlider_.setValue(lane.amount, juce::dontSendNotification);

            // Sincronizar rate
            switch (lane.rate) {
                case SyncDivision::Quarter: rateCombo_.setSelectedId(1, juce::dontSendNotification); break;
                case SyncDivision::Eighth: rateCombo_.setSelectedId(2, juce::dontSendNotification); break;
                case SyncDivision::Sixteenth: rateCombo_.setSelectedId(3, juce::dontSendNotification); break;
                case SyncDivision::ThirtySecond: rateCombo_.setSelectedId(4, juce::dontSendNotification); break;
                case SyncDivision::Triplet_Eighth: rateCombo_.setSelectedId(5, juce::dontSendNotification); break;
                case SyncDivision::Triplet_Quarter: rateCombo_.setSelectedId(6, juce::dontSendNotification); break;
                case SyncDivision::Dotted_Eighth: rateCombo_.setSelectedId(7, juce::dontSendNotification); break;
                default: rateCombo_.setSelectedId(3, juce::dontSendNotification); break;
            }
        }
        updateLaneTabLabels();
        repaint();
    }

    void updatePlayhead() {
        if (activeBank_ != nullptr && isVisible()) {
            repaint();
        }
    }

    void setOnCloseRequested(std::function<void()> cb) {
        onCloseRequested_ = std::move(cb);
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // 1. Fondo negro absoluto
        g.setColour(juce::Colour(0xff000000));
        g.fillRect(bounds);

        // 2. Borde superior blanco nítido
        g.setColour(juce::Colours::white);
        g.drawHorizontalLine(0, 0.0f, bounds.getWidth());

        // 3. Título del Nodo y Estado
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.setColour(juce::Colours::white);
        g.drawText(nodeTitle_, 10, 6, 380, 16, juce::Justification::centredLeft, true);

        // 4. Área de Cuadrícula de Pasos (Step Grid)
        auto gridArea = getStepGridArea();
        g.setColour(juce::Colour(0xff0a0a0f));
        g.fillRect(gridArea);
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawRect(gridArea, 1.0f);

        if (activeBank_ == nullptr) return;

        const auto& lane = activeBank_->getLane(selectedLaneIndex_);
        const uint32_t numSteps = std::clamp<uint32_t>(lane.numSteps, 1, 32);
        const float stepW = gridArea.getWidth() / static_cast<float>(numSteps);

        // Renderizado de las barras de pasos
        for (uint32_t i = 0; i < numSteps; ++i) {
            const float x = gridArea.getX() + static_cast<float>(i) * stepW;
            const float val = std::clamp(lane.steps[i], 0.0f, 1.0f);
            const float barH = val * (gridArea.getHeight() - 4.0f);
            const float y = gridArea.getBottom() - barH - 2.0f;

            // División de compás cada 4 pasos
            if (i > 0 && i % 4 == 0) {
                g.setColour(juce::Colours::white.withAlpha(0.25f));
                g.drawVerticalLine(static_cast<int>(x), gridArea.getY(), gridArea.getBottom());
            }

            // Barra rellena
            auto barRect = juce::Rectangle<float>(x + 1.5f, y, stepW - 3.0f, barH);
            const bool isPlayhead = (i == lane.currentStep);

            if (isPlayhead) {
                // Paso activo (Playhead brillante)
                g.setColour(juce::Colours::white);
                g.fillRect(barRect);
            } else {
                // Paso en espera
                g.setColour(juce::Colours::white.withAlpha(lane.active ? 0.35f : 0.15f));
                g.fillRect(barRect);
                g.setColour(juce::Colours::white.withAlpha(lane.active ? 0.85f : 0.40f));
                g.drawRect(barRect, 1.0f);
            }

            // Número del paso
            g.setFont(juce::FontOptions(8.5f, juce::Font::plain));
            g.setColour(isPlayhead ? juce::Colours::white : juce::Colours::white.withAlpha(0.5f));
            g.drawText(juce::String(i + 1), static_cast<int>(x), static_cast<int>(gridArea.getBottom() - 12.0f), static_cast<int>(stepW), 12, juce::Justification::centred, false);
        }

        // Línea vertical del Playhead en tiempo real
        if (lane.active && lane.currentStep < numSteps) {
            const float playheadX = gridArea.getX() + (static_cast<float>(lane.currentStep) + 0.5f) * stepW;
            g.setColour(juce::Colours::white);
            g.drawVerticalLine(static_cast<int>(playheadX), gridArea.getY(), gridArea.getBottom());
        }
    }

    void resized() override {
        auto area = getLocalBounds().reduced(8, 4);

        // Fila 1: Cabecera con Título, Tabs y Botón Cerrar
        auto topRow = area.removeFromTop(24);
        closeBtn_.setBounds(topRow.removeFromRight(22).reduced(2));
        topRow.removeFromRight(10);

        // Pestañas Multi-Lane alineadas a la derecha de la cabecera
        auto tabsArea = topRow.removeFromRight(280);
        const int tabW = 68;
        for (int i = static_cast<int>(NodeAutomationBank::MaxLanes) - 1; i >= 0; --i) {
            laneTabs_[static_cast<size_t>(i)].setBounds(tabsArea.removeFromRight(tabW).reduced(2, 0));
        }

        area.removeFromTop(4);

        // Fila 2: Controles de Parámetro, Sincronización y Formas Rápidas
        auto controlsRow = area.removeFromTop(22);

        paramCombo_.setBounds(controlsRow.removeFromLeft(160));
        controlsRow.removeFromLeft(6);
        activeToggleBtn_.setBounds(controlsRow.removeFromLeft(80));
        controlsRow.removeFromLeft(8);

        rateCombo_.setBounds(controlsRow.removeFromLeft(100));
        controlsRow.removeFromLeft(6);
        stepsCombo_.setBounds(controlsRow.removeFromLeft(80));
        controlsRow.removeFromLeft(10);

        glideLabel_.setBounds(controlsRow.removeFromLeft(40));
        glideSlider_.setBounds(controlsRow.removeFromLeft(65));
        controlsRow.removeFromLeft(10);

        amountLabel_.setBounds(controlsRow.removeFromLeft(42));
        amountSlider_.setBounds(controlsRow.removeFromLeft(65));
        controlsRow.removeFromLeft(12);

        // Botones de formas rápidas
        flatBtn_.setBounds(controlsRow.removeFromRight(36).reduced(1, 0));
        randBtn_.setBounds(controlsRow.removeFromRight(38).reduced(1, 0));
        triBtn_.setBounds(controlsRow.removeFromRight(32).reduced(1, 0));
        staccatoBtn_.setBounds(controlsRow.removeFromRight(55).reduced(1, 0));
        sawBtn_.setBounds(controlsRow.removeFromRight(36).reduced(1, 0));
        pumpBtn_.setBounds(controlsRow.removeFromRight(44).reduced(1, 0));
    }

    void mouseDown(const juce::MouseEvent& e) override {
        updateStepFromMouse(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        updateStepFromMouse(e);
    }

private:
    juce::Rectangle<float> getStepGridArea() const noexcept {
        auto area = getLocalBounds().toFloat().reduced(8.0f, 4.0f);
        area.removeFromTop(56.0f); // Espacio para cabecera y controles
        area.removeFromBottom(2.0f);
        return area;
    }

    void updateStepFromMouse(const juce::MouseEvent& e) {
        if (activeBank_ == nullptr) return;

        auto gridArea = getStepGridArea();
        if (!gridArea.contains(e.position)) return;

        auto& lane = activeBank_->getLane(selectedLaneIndex_);
        const uint32_t numSteps = std::clamp<uint32_t>(lane.numSteps, 1, 32);
        const float stepW = gridArea.getWidth() / static_cast<float>(numSteps);

        const int stepIdx = static_cast<int>((e.position.x - gridArea.getX()) / stepW);
        if (stepIdx >= 0 && stepIdx < static_cast<int>(numSteps)) {
            const float normVal = std::clamp(1.0f - (e.position.y - gridArea.getY()) / gridArea.getHeight(), 0.0f, 1.0f);
            lane.steps[static_cast<size_t>(stepIdx)] = normVal;
            repaint();
        }
    }

    void setupShapeButton(juce::TextButton& btn, const juce::String& text, NodeAutomationBank::ShapeType shape) {
        btn.setButtonText(text);
        btn.onClick = [this, shape]() {
            if (activeBank_ != nullptr) {
                activeBank_->applyShape(selectedLaneIndex_, shape);
                repaint();
            }
        };
        addAndMakeVisible(btn);
    }

    void updateLaneTabLabels() {
        if (activeBank_ == nullptr) {
            for (size_t i = 0; i < NodeAutomationBank::MaxLanes; ++i) {
                laneTabs_[i].setButtonText("Lane " + juce::String(static_cast<int>(i + 1)));
            }
            return;
        }

        for (size_t i = 0; i < NodeAutomationBank::MaxLanes; ++i) {
            const auto& lane = activeBank_->getLane(i);
            juce::String dot = lane.active ? juce::String::fromUTF8("● ") : juce::String::fromUTF8("○ ");
            juce::String label = dot + "L" + juce::String(static_cast<int>(i + 1));
            if (lane.targetParamId > 0 && currentNodeInst_ != nullptr && currentNodeInst_->processor != nullptr) {
                for (const auto& p : currentNodeInst_->processor->getParameters()) {
                    if (p.id == lane.targetParamId) {
                        label += ": " + juce::String(p.name).substring(0, 5);
                        break;
                    }
                }
            }
            laneTabs_[i].setButtonText(label);
        }
    }

    NodeId currentNodeId_{ InvalidNodeId };
    NodeInstance* currentNodeInst_{ nullptr };
    NodeAutomationBank* activeBank_{ nullptr };
    size_t selectedLaneIndex_{ 0 };

    juce::String nodeTitle_{ "AUTOMATION SEQUENCER" };

    std::array<juce::TextButton, NodeAutomationBank::MaxLanes> laneTabs_;
    juce::ComboBox paramCombo_;
    juce::TextButton activeToggleBtn_;
    juce::ComboBox rateCombo_;
    juce::ComboBox stepsCombo_;

    juce::Slider glideSlider_;
    juce::Label glideLabel_;
    juce::Slider amountSlider_;
    juce::Label amountLabel_;

    juce::TextButton pumpBtn_;
    juce::TextButton sawBtn_;
    juce::TextButton staccatoBtn_;
    juce::TextButton triBtn_;
    juce::TextButton randBtn_;
    juce::TextButton flatBtn_;
    juce::TextButton closeBtn_;

    std::function<void()> onCloseRequested_;
};

} // namespace audio_graph
