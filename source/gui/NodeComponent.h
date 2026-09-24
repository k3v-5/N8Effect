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
            // Generar mini-sliders para los parámetros
            const auto params = processor_->getParameters();
            const size_t numSliders = std::min(size_t{ 4 }, params.size()); // Hasta 4 parámetros destacados
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

    void setOnNodeMoved(std::function<void(NodeId, float, float)> cb) { onNodeMoved_ = std::move(cb); }
    void setOnNodeDeleted(std::function<void(NodeId)> cb) { onNodeDeleted_ = std::move(cb); }
    void setOnPinDragStarted(std::function<void(NodeId, PinId, PinDataType, juce::Point<float>)> cb) { onPinDragStarted_ = std::move(cb); }
    void setOnPinConnected(std::function<void(NodeId, PinId, PinDataType)> cb) { onPinConnected_ = std::move(cb); }
    void setOnParameterChanged(std::function<void(NodeId, ParameterId, float)> cb) { onParameterChanged_ = std::move(cb); }

    static juce::Colour getCategoryColor(NodeType t) noexcept {
        switch (t) {
            case NodeType::Filter: return juce::Colour(0xfff39c12);     // Ámbar
            case NodeType::Delay: return juce::Colour(0xff00d2ff);      // Cian
            case NodeType::Reverb: return juce::Colour(0xff9b59b6);     // Púrpura
            case NodeType::Distortion: return juce::Colour(0xffe74c3c); // Rojo
            case NodeType::Compressor:
            case NodeType::Multiband: return juce::Colour(0xffe67e22);  // Naranja
            case NodeType::Granular: return juce::Colour(0xff2ecc71);   // Verde
            case NodeType::Spectral: return juce::Colour(0xff1abc9c);   // Turquesa
            case NodeType::Glitch: return juce::Colour(0xffe84393);     // Rosa chicle
            case NodeType::PitchShifter:
            case NodeType::Resonator: return juce::Colour(0xff6c5ce7);  // Azul índigo
            default: return juce::Colour(0xff34495e);
        }
    }

    juce::Point<float> getPinCenterInCanvas(PinId pinId) const {
        if (processor_ == nullptr) return getBounds().getCentre().toFloat();

        const auto pins = processor_->getPins();
        for (size_t i = 0; i < pins.size(); ++i) {
            if (pins[i].id == pinId) {
                const auto r = getPinLocalRect(static_cast<int>(i), pins[i].type == PinType::AudioInput || pins[i].type == PinType::EventInput);
                const auto localPt = r.getCentre();
                return getPosition().toFloat() + localPt;
            }
        }
        return getBounds().getCentre().toFloat();
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // 1. Fondo de la tarjeta del nodo
        g.setColour(juce::Colour(0xff12121a));
        g.fillRoundedRectangle(bounds, 8.0f);

        // 2. Cabecera del nodo con color según categoría
        auto headerRect = bounds.removeFromTop(28.0f);
        const juce::Colour catColor = getCategoryColor(type_);

        juce::ColourGradient grad(catColor.withAlpha(0.6f), headerRect.getX(), headerRect.getY(),
                                  catColor.withAlpha(0.15f), headerRect.getX(), headerRect.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(headerRect, 8.0f);

        // Rectángulo inferior de cabecera recto para ensamblar con el cuerpo
        g.fillRect(headerRect.removeFromBottom(6.0f));

        // Título del nodo
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(name_, headerRect.reduced(8.0f, 0.0f), juce::Justification::centredLeft, true);

        // Botón de eliminar (X)
        auto closeBtnRect = headerRect.removeFromRight(20.0f).reduced(2.0f);
        g.setColour(juce::Colour(0xff888899));
        g.drawText("×", closeBtnRect, juce::Justification::centred, false);

        // 3. Renderizado de Pines
        if (processor_ != nullptr) {
            const auto pins = processor_->getPins();
            for (size_t i = 0; i < pins.size(); ++i) {
                const bool isInput = (pins[i].type == PinType::AudioInput || pins[i].type == PinType::EventInput);
                auto pinRect = getPinLocalRect(static_cast<int>(i), isInput);

                const juce::Colour pinColor = WireRenderer::getPinColour(pins[i].dataType);
                g.setColour(pinColor);
                g.fillEllipse(pinRect);

                g.setColour(juce::Colours::white);
                g.drawEllipse(pinRect, 1.2f);

                // Etiqueta del pin
                g.setFont(10.0f);
                g.setColour(juce::Colour(0xffb0b8d0));
                if (isInput) {
                    g.drawText(pins[i].name, static_cast<int>(pinRect.getRight() + 4.0f), static_cast<int>(pinRect.getY() - 2.0f), 60, 16, juce::Justification::centredLeft, true);
                } else {
                    g.drawText(pins[i].name, static_cast<int>(pinRect.getX() - 64.0f), static_cast<int>(pinRect.getY() - 2.0f), 60, 16, juce::Justification::centredRight, true);
                }
            }
        }

        // 4. Borde del nodo (resaltado si está seleccionado)
        g.setColour(isSelected_ ? juce::Colour(0xff00d2ff) : juce::Colour(0xff252538));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f, isSelected_ ? 2.0f : 1.0f);
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

        // Comprobar clic en pin
        if (processor_ != nullptr) {
            const auto pins = processor_->getPins();
            for (size_t i = 0; i < pins.size(); ++i) {
                const bool isInput = (pins[i].type == PinType::AudioInput || pins[i].type == PinType::EventInput);
                auto pinRect = getPinLocalRect(static_cast<int>(i), isInput);
                if (pinRect.expanded(4.0f).contains(e.position)) {
                    if (!isInput && onPinDragStarted_) {
                        onPinDragStarted_(id_, pins[i].id, pins[i].dataType, getPosition().toFloat() + pinRect.getCentre());
                        return;
                    }
                }
            }
        }

        dragger_.startDraggingComponent(this, e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        dragger_.dragComponent(this, e, nullptr);
        if (onNodeMoved_) {
            onNodeMoved_(id_, static_cast<float>(getX()), static_cast<float>(getY()));
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (processor_ != nullptr) {
            const auto pins = processor_->getPins();
            for (size_t i = 0; i < pins.size(); ++i) {
                const bool isInput = (pins[i].type == PinType::AudioInput || pins[i].type == PinType::EventInput);
                if (isInput) {
                    auto pinRect = getPinLocalRect(static_cast<int>(i), isInput);
                    if (pinRect.expanded(6.0f).contains(e.position)) {
                        if (onPinConnected_) {
                            onPinConnected_(id_, pins[i].id, pins[i].dataType);
                            return;
                        }
                    }
                }
            }
        }
    }

private:
    juce::Rectangle<float> getPinLocalRect(int pinIndex, bool isInput) const {
        const float pinY = 40.0f + static_cast<float>(pinIndex) * 22.0f;
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
    juce::ComponentDragger dragger_;
    std::vector<std::unique_ptr<ModulationSlider>> sliders_;

    std::function<void(NodeId, float, float)> onNodeMoved_;
    std::function<void(NodeId)> onNodeDeleted_;
    std::function<void(NodeId, PinId, PinDataType, juce::Point<float>)> onPinDragStarted_;
    std::function<void(NodeId, PinId, PinDataType)> onPinConnected_;
    std::function<void(NodeId, ParameterId, float)> onParameterChanged_;
};

} // namespace audio_graph
