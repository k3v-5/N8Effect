#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>
#include <functional>
#include "../core/Types.h"
#include "../graph/AudioProcessorNode.h"
#include "ModulationSlider.h"
#include "WireRenderer.h"

namespace audio_graph {

/**
 * @brief Componente visual interactivo para un nodo en el canvas del grafo (Reglas 4, 8, 23, 24).
 */
class NodeComponent : public juce::Component {
public:
    NodeComponent(NodeId id, const juce::String& name, NodeType type, AudioProcessorNode* processor)
        : id_(id), name_(name), type_(type), processor_(processor)
    {
        setRepaintsOnMouseActivity(true);

        if (processor_ != nullptr) {
            // Generar mini-sliders para todos los parámetros del procesador
            const auto params = processor_->getParameters();
            const size_t numSliders = params.size();
            for (size_t i = 0; i < numSliders; ++i) {
                const auto& p = params[i];
                auto slider = std::make_unique<ModulationSlider>(p.name, p.minValue, p.maxValue, p.defaultValue);
                const ParameterId pid = p.id;
                slider->setBaseValue(processor_->getParameter(pid));
                slider->setOnValueChanged([this, pid](float val) {
                    if (processor_ != nullptr) {
                        processor_->setParameter(pid, val);
                    }
                    if (onParameterChanged_) {
                        onParameterChanged_(id_, pid, val);
                    }
                });
                addAndMakeVisible(*slider);
                sliders_.push_back(std::move(slider));
            }
        }

        updateDimensions();
    }

    NodeId getNodeId() const noexcept { return id_; }
    NodeType getNodeType() const noexcept { return type_; }
    const juce::String& getNodeName() const noexcept { return name_; }

    void setSelected(bool sel) {
        isSelected_ = sel;
        repaint();
    }

    bool isSelected() const noexcept { return isSelected_; }

    void setOnNodeSelected(std::function<void(NodeId)> cb) { onNodeSelected_ = std::move(cb); }
    void setOnNodeMoved(std::function<void(NodeId, float, float)> cb) { onNodeMoved_ = std::move(cb); }
    void setOnNodeDeleted(std::function<void(NodeId)> cb) { onNodeDeleted_ = std::move(cb); }
    void setOnPinDragStarted(std::function<void(NodeId, PinId, PinDataType, juce::Point<float>)> cb) { onPinDragStarted_ = std::move(cb); }
    void setOnPinDragging(std::function<void(juce::Point<float>)> cb) { onPinDragging_ = std::move(cb); }
    void setOnPinDragEnded(std::function<void(NodeId, PinId, PinType, PinDataType, juce::Point<float>)> cb) { onPinDragEnded_ = std::move(cb); }
    void setOnPinRightClicked(std::function<void(NodeId, PinId)> cb) { onPinRightClicked_ = std::move(cb); }
    void setOnNodeDraggingOverCanvas(std::function<void(NodeId, juce::Rectangle<int>)> cb) { onNodeDraggingOverCanvas_ = std::move(cb); }
    void setOnNodeDropped(std::function<void(NodeId, juce::Rectangle<int>)> cb) { onNodeDropped_ = std::move(cb); }
    void setOnParameterChanged(std::function<void(NodeId, ParameterId, float)> cb) { onParameterChanged_ = std::move(cb); }

    void setDropCandidate(bool cand) {
        if (isDropCandidate_ != cand) {
            isDropCandidate_ = cand;
            repaint();
        }
    }

    void setHighlightedPin(PinId pinId) {
        if (highlightedPinId_ != pinId) {
            highlightedPinId_ = pinId;
            repaint();
        }
    }

    bool hitTestPin(juce::Point<float> canvasPos, PinId& outPinId, PinType& outPinType, PinDataType& outDataType, juce::Point<float>& outCenter, float tolerance = 16.0f) const {
        if (processor_ == nullptr) return false;
        const auto localPos = canvasPos - getPosition().toFloat();
        const auto pins = processor_->getPins();
        for (size_t i = 0; i < pins.size(); ++i) {
            auto pinRect = getPinLocalRect(i);
            if (pinRect.expanded(tolerance).contains(localPos)) {
                outPinId = pins[i].id;
                outPinType = pins[i].type;
                outDataType = pins[i].dataType;
                outCenter = getPosition().toFloat() + pinRect.getCentre();
                return true;
            }
        }
        return false;
    }

    juce::Point<float> getPinCenterInCanvas(PinId pinId) const {
        if (processor_ == nullptr) return getBounds().getCentre().toFloat();

        const auto pins = processor_->getPins();
        for (size_t i = 0; i < pins.size(); ++i) {
            if (pins[i].id == pinId) {
                const auto r = getPinLocalRect(i);
                const auto localPt = r.getCentre();
                return getPosition().toFloat() + localPt;
            }
        }
        return getBounds().getCentre().toFloat();
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // 1. Fondo de la tarjeta del nodo (Negro absoluto)
        g.setColour(juce::Colour(0xff000000));
        g.fillRoundedRectangle(bounds, 4.0f);

        // 2. Cabecera del nodo (Línea divisoria horizontal blanca nítida)
        auto headerRect = bounds.removeFromTop(28.0f);
        g.setColour(juce::Colours::white);
        g.drawHorizontalLine(28, 0.0f, bounds.getWidth());

        // Título del nodo (Blanco nítido en mayúsculas elegante)
        g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
        g.drawText(name_.toUpperCase(), headerRect.reduced(10.0f, 0.0f), juce::Justification::centredLeft, true);

        // Botón de eliminar (X) en contorno minimalista
        auto closeBtnRect = headerRect.removeFromRight(22.0f).reduced(4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.drawText("×", closeBtnRect, juce::Justification::centred, false);

        // 3. Renderizado de Pines minimalistas (anillo blanco con núcleo negro y centro blanco)
        if (processor_ != nullptr) {
            const auto pins = processor_->getPins();
            for (size_t i = 0; i < pins.size(); ++i) {
                const bool isInput = (pins[i].type == PinType::AudioInput || pins[i].type == PinType::EventInput);
                auto pinRect = getPinLocalRect(i);
                const bool isHighlighted = (pins[i].id == highlightedPinId_);

                if (isHighlighted) {
                    // Resplandor blanco concéntrico para pin objetivo en snapping
                    g.setColour(juce::Colours::white.withAlpha(0.35f));
                    g.fillEllipse(pinRect.expanded(4.0f));
                }

                g.setColour(juce::Colour(0xff000000));
                g.fillEllipse(pinRect);

                g.setColour(juce::Colours::white);
                g.drawEllipse(pinRect, isHighlighted ? 2.0f : 1.2f);
                g.fillEllipse(pinRect.getCentreX() - 1.5f, pinRect.getCentreY() - 1.5f, 3.0f, 3.0f);

                // Etiqueta del pin (blanco limpio)
                g.setFont(juce::FontOptions(9.5f, juce::Font::plain));
                g.setColour(juce::Colours::white.withAlpha(0.85f));
                if (isInput) {
                    g.drawText(pins[i].name, static_cast<int>(pinRect.getRight() + 4.0f), static_cast<int>(pinRect.getY() - 2.0f), 60, 16, juce::Justification::centredLeft, true);
                } else {
                    g.drawText(pins[i].name, static_cast<int>(pinRect.getX() - 64.0f), static_cast<int>(pinRect.getY() - 2.0f), 60, 16, juce::Justification::centredRight, true);
                }
            }
        }

        // 4. Borde del nodo: Contorno blanco nítido
        if (isDropCandidate_) {
            // Halo de encadenamiento automático drag-and-drop
            g.setColour(juce::Colours::white.withAlpha(0.3f));
            g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 4.0f);
            g.setColour(juce::Colours::white);
            const float dashPattern[] = { 4.0f, 3.0f };
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, 2.0f);
        } else if (isSelected_) {
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, 4.0f);
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, 2.0f);
        } else {
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.2f);
        }
    }

    void resized() override {
        auto area = getLocalBounds();
        area.removeFromTop(32); // Cabecera
        area.removeFromBottom(8);
        area.reduce(14, 0);

        for (auto& slider : sliders_) {
            slider->setBounds(area.removeFromTop(24));
            area.removeFromTop(6); // Espaciado
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        // Comprobar clic en botón cerrar
        if (e.position.y < 28.0f && e.position.x > getWidth() - 28.0f) {
            if (onNodeDeleted_) {
                onNodeDeleted_(id_);
            }
            return;
        }

        // Comprobar clic en pin (Izquierdo para cablear, Derecho para desconectar)
        if (processor_ != nullptr) {
            const auto pins = processor_->getPins();
            for (size_t i = 0; i < pins.size(); ++i) {
                auto pinRect = getPinLocalRect(i);
                if (pinRect.expanded(6.0f).contains(e.position)) {
                    if (e.mods.isRightButtonDown()) {
                        if (onPinRightClicked_) {
                            onPinRightClicked_(id_, pins[i].id);
                        }
                        return;
                    }
                    isDraggingPin_ = true;
                    draggedPinId_ = pins[i].id;
                    draggedPinType_ = pins[i].type;
                    draggedPinDataType_ = pins[i].dataType;
                    if (onPinDragStarted_) {
                        const auto canvasPt = getPosition().toFloat() + pinRect.getCentre();
                        onPinDragStarted_(id_, pins[i].id, pins[i].dataType, canvasPt);
                    }
                    return;
                }
            }
        }

        isDraggingPin_ = false;
        if (onNodeSelected_) {
            onNodeSelected_(id_);
        }
        dragger_.startDraggingComponent(this, e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (isDraggingPin_) {
            auto canvasPos = (getParentComponent() != nullptr)
                ? getParentComponent()->getLocalPoint(this, e.position).toFloat()
                : (getPosition().toFloat() + e.position);
            if (onPinDragging_) {
                onPinDragging_(canvasPos);
            }
            return;
        }

        dragger_.dragComponent(this, e, nullptr);
        if (onNodeMoved_) {
            onNodeMoved_(id_, static_cast<float>(getX()), static_cast<float>(getY()));
        }
        if (onNodeDraggingOverCanvas_) {
            onNodeDraggingOverCanvas_(id_, getBounds());
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (isDraggingPin_) {
            isDraggingPin_ = false;
            auto canvasPos = (getParentComponent() != nullptr)
                ? getParentComponent()->getLocalPoint(this, e.position).toFloat()
                : (getPosition().toFloat() + e.position);
            if (onPinDragEnded_) {
                onPinDragEnded_(id_, draggedPinId_, draggedPinType_, draggedPinDataType_, canvasPos);
            }
            return;
        }

        if (onNodeDropped_) {
            onNodeDropped_(id_, getBounds());
        }
    }

private:
    juce::Rectangle<float> getPinLocalRect(size_t pinIndex) const {
        if (processor_ == nullptr) return { 4.0f, 40.0f, 10.0f, 10.0f };
        const auto pins = processor_->getPins();
        if (pinIndex >= pins.size()) return { 4.0f, 40.0f, 10.0f, 10.0f };

        const bool isInput = (pins[pinIndex].type == PinType::AudioInput || pins[pinIndex].type == PinType::EventInput);
        int slot = 0;
        for (size_t i = 0; i < pinIndex; ++i) {
            const bool otherIsInput = (pins[i].type == PinType::AudioInput || pins[i].type == PinType::EventInput);
            if (otherIsInput == isInput) ++slot;
        }

        const float pinY = 40.0f + static_cast<float>(slot) * 24.0f;
        const float pinX = isInput ? 4.0f : static_cast<float>(getWidth() - 14.0f);
        return { pinX, pinY, 10.0f, 10.0f };
    }

    void updateDimensions() {
        const size_t numParams = sliders_.size();
        const size_t numPins = (processor_ != nullptr) ? processor_->getPins().size() : 2;
        const int heightFromPins = 45 + static_cast<int>(numPins) * 22;
        const int heightFromParams = 42 + static_cast<int>(numParams) * 30;
        const int totalH = std::max({ 100, heightFromPins, heightFromParams });
        setSize(180, totalH);
    }

    NodeId id_{ InvalidNodeId };
    juce::String name_;
    NodeType type_{ NodeType::Unknown };
    AudioProcessorNode* processor_{ nullptr };

    bool isSelected_{ false };
    bool isDropCandidate_{ false };
    PinId highlightedPinId_{ InvalidPinId };

    bool isDraggingPin_{ false };
    PinId draggedPinId_{ InvalidPinId };
    PinType draggedPinType_{ PinType::AudioOutput };
    PinDataType draggedPinDataType_{ PinDataType::AudioStereo };

    juce::ComponentDragger dragger_;
    std::vector<std::unique_ptr<ModulationSlider>> sliders_;

    std::function<void(NodeId)> onNodeSelected_;
    std::function<void(NodeId, float, float)> onNodeMoved_;
    std::function<void(NodeId)> onNodeDeleted_;
    std::function<void(NodeId, PinId, PinDataType, juce::Point<float>)> onPinDragStarted_;
    std::function<void(juce::Point<float>)> onPinDragging_;
    std::function<void(NodeId, PinId, PinType, PinDataType, juce::Point<float>)> onPinDragEnded_;
    std::function<void(NodeId, PinId)> onPinRightClicked_;
    std::function<void(NodeId, juce::Rectangle<int>)> onNodeDraggingOverCanvas_;
    std::function<void(NodeId, juce::Rectangle<int>)> onNodeDropped_;
    std::function<void(NodeId, ParameterId, float)> onParameterChanged_;
};

} // namespace audio_graph
