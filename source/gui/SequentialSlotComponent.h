#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>
#include <functional>
#include "../core/Types.h"
#include "../graph/AudioProcessorNode.h"
#include "ModulationSlider.h"
#include "NodeComponent.h"

namespace audio_graph {

/**
 * @brief Tarjeta interactiva para un efecto dentro de la Tira Secuencial / Cola de Efectos (Estilo Arturia Efx MOTIONS).
 * Ofrece controles esenciales, bypass, reordenamiento ágil y visualización de modulación.
 */
class SequentialSlotComponent : public juce::Component {
public:
    SequentialSlotComponent(NodeId id, int slotIndex, const juce::String& name, NodeType type, AudioProcessorNode* processor)
        : id_(id), slotIndex_(slotIndex), name_(name), type_(type), processor_(processor)
    {
        setRepaintsOnMouseActivity(true);

        // 1. Botón Mover a la Izquierda ◀
        moveLeftBtn_.setButtonText(juce::CharPointer_UTF8("\xE2\x97\x80")); // ◀
        moveLeftBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181826));
        moveLeftBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        moveLeftBtn_.onClick = [this]() {
            if (onMoveLeftRequested_) onMoveLeftRequested_(slotIndex_);
        };
        addAndMakeVisible(moveLeftBtn_);

        // 2. Botón Mover a la Derecha ▶
        moveRightBtn_.setButtonText(juce::CharPointer_UTF8("\xE2\x96\xB6")); // ▶
        moveRightBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181826));
        moveRightBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        moveRightBtn_.onClick = [this]() {
            if (onMoveRightRequested_) onMoveRightRequested_(slotIndex_);
        };
        addAndMakeVisible(moveRightBtn_);

        // 3. Botón Bypass / Power
        bypassBtn_.setButtonText("ON");
        bypassBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff162828));
        bypassBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00f0ff));
        bypassBtn_.onClick = [this]() {
            isBypassed_ = !isBypassed_;
            updateBypassVisuals();
            if (onBypassToggled_) onBypassToggled_(id_, isBypassed_);
        };
        addAndMakeVisible(bypassBtn_);

        // 4. Botón Eliminar Slot ✕
        deleteBtn_.setButtonText(juce::CharPointer_UTF8("\xE2\x9C\x95")); // ✕
        deleteBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff221418));
        deleteBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff5566));
        deleteBtn_.onClick = [this]() {
            if (onDeleteRequested_) onDeleteRequested_(id_);
        };
        addAndMakeVisible(deleteBtn_);

        // 5. Parámetros Destacados (Sliders con halos de modulación)
        if (processor_ != nullptr) {
            const auto params = processor_->getParameters();
            const size_t numSliders = std::min(size_t{ 4 }, params.size());
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

        setSize(184, 260);
    }

    NodeId getNodeId() const noexcept { return id_; }
    int getSlotIndex() const noexcept { return slotIndex_; }
    NodeType getNodeType() const noexcept { return type_; }
    const juce::String& getNodeName() const noexcept { return name_; }
    bool isBypassed() const noexcept { return isBypassed_; }

    void setSlotIndex(int idx, bool isFirst, bool isLast) {
        slotIndex_ = idx;
        moveLeftBtn_.setEnabled(!isFirst);
        moveRightBtn_.setEnabled(!isLast);
        repaint();
    }

    void setOnMoveLeftRequested(std::function<void(int)> cb) { onMoveLeftRequested_ = std::move(cb); }
    void setOnMoveRightRequested(std::function<void(int)> cb) { onMoveRightRequested_ = std::move(cb); }
    void setOnDeleteRequested(std::function<void(NodeId)> cb) { onDeleteRequested_ = std::move(cb); }
    void setOnBypassToggled(std::function<void(NodeId, bool)> cb) { onBypassToggled_ = std::move(cb); }
    void setOnParameterChanged(std::function<void(NodeId, ParameterId, float)> cb) { onParameterChanged_ = std::move(cb); }
    void setOnSlotDragged(std::function<void(SequentialSlotComponent*, const juce::MouseEvent&)> cb) { onSlotDragged_ = std::move(cb); }
    void setOnSlotDragEnded(std::function<void(SequentialSlotComponent*, const juce::MouseEvent&)> cb) { onSlotDragEnded_ = std::move(cb); }

    void updateBypassVisuals() {
        if (isBypassed_) {
            bypassBtn_.setButtonText("BYP");
            bypassBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff22222a));
            bypassBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff777788));
            setAlpha(0.6f);
        } else {
            bypassBtn_.setButtonText("ON");
            bypassBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff162828));
            bypassBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00f0ff));
            setAlpha(1.0f);
        }
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        const juce::Colour catColor = NodeComponent::getCategoryColor(type_);

        // 1. Sombra exterior
        g.setColour(juce::Colour(0x60000000));
        g.fillRoundedRectangle(bounds.translated(0.0f, 3.0f), 10.0f);

        // 2. Fondo del Slot (Chasis Arturia oscuro)
        g.setColour(juce::Colour(0xff13141f));
        g.fillRoundedRectangle(bounds, 10.0f);

        // 3. Cabecera con degradado de categoría
        auto header = bounds.removeFromTop(36.0f);
        juce::ColourGradient grad(catColor.withAlpha(0.65f), header.getX(), header.getY(),
                                  catColor.withAlpha(0.12f), header.getX(), header.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(header, 10.0f);
        g.fillRect(header.removeFromBottom(8.0f)); // Ensamble recto con el cuerpo

        // 4. Badge circular con el índice del slot (ej. "01", "02")
        const juce::Rectangle<float> badgeRect(bounds.getX() + 8.0f, 8.0f, 20.0f, 20.0f);
        g.setColour(juce::Colour(0xff090910));
        g.fillEllipse(badgeRect);
        g.setColour(catColor);
        g.drawEllipse(badgeRect, 1.5f);

        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        juce::String idxStr = (slotIndex_ < 9 ? "0" : "") + juce::String(slotIndex_ + 1);
        g.drawText(idxStr, badgeRect, juce::Justification::centred, false);

        // 5. Nombre del Procesador
        g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
        juce::Rectangle<float> titleRect(34.0f, 8.0f, 70.0f, 20.0f);
        g.drawText(name_, titleRect, juce::Justification::centredLeft, true);

        // 6. Barra indicadora de categoría / tag
        g.setColour(catColor.withAlpha(0.25f));
        g.fillRect(8.0f, 40.0f, getWidth() - 16.0f, 15.0f);
        g.setColour(catColor);
        g.drawText(getCategoryName(type_), juce::Rectangle<float>(12.0f, 40.0f, static_cast<float>(getWidth() - 24), 15.0f), juce::Justification::centredLeft, false);

        // 7. Borde de la tarjeta
        g.setColour(isDragging_ ? juce::Colour(0xff00f0ff) : juce::Colour(0xff222436));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 10.0f, isDragging_ ? 2.0f : 1.0f);
    }

    void resized() override {
        // Cabecera: Botones de control a la derecha
        const int btnY = 7;
        const int btnW = 16;
        const int btnH = 20;

        deleteBtn_.setBounds(getWidth() - 22, btnY, btnW, btnH);
        bypassBtn_.setBounds(getWidth() - 48, btnY, 24, btnH);
        moveRightBtn_.setBounds(getWidth() - 66, btnY, btnW, btnH);
        moveLeftBtn_.setBounds(getWidth() - 84, btnY, btnW, btnH);

        // Área de Sliders de Parámetros
        auto area = getLocalBounds();
        area.removeFromTop(60); // Cabecera + barra de categoría
        area.removeFromBottom(8);
        area.reduce(10, 0);

        for (auto& slider : sliders_) {
            slider->setBounds(area.removeFromTop(26));
            area.removeFromTop(6); // Espacio entre sliders
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isLeftButtonDown() && e.position.y < 36.0f) {
            isDragging_ = true;
            repaint();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (isDragging_ && onSlotDragged_) {
            onSlotDragged_(this, e);
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (isDragging_) {
            isDragging_ = false;
            repaint();
            if (onSlotDragEnded_) {
                onSlotDragEnded_(this, e);
            }
        }
    }

    static juce::String getCategoryName(NodeType t) noexcept {
        switch (t) {
            case NodeType::Compressor:
            case NodeType::Multiband: return "DYNAMICS // VCA & OTT";
            case NodeType::Filter: return "EQ // 3-BAND PARAMETRIC";
            case NodeType::Distortion: return "DRIVE // MULTI-SHAPER";
            case NodeType::Delay: return "TIME // PING-PONG STEREO";
            case NodeType::Reverb: return "SPACE // FDN DIFFUSION";
            case NodeType::Granular: return "TEXTURE // GRAIN CLOUD";
            case NodeType::Glitch: return "RHYTHM // BEAT SLICER";
            case NodeType::Spectral: return "SPECTRAL // FREEZE & SMEAR";
            case NodeType::Resonator: return "MODAL // RESONATOR BANK";
            case NodeType::PitchShifter:
            case NodeType::FrequencyShifter: return "PITCH // SSB & CROSSFADE";
            case NodeType::Phaser:
            case NodeType::Chorus:
            case NodeType::Flanger:
            case NodeType::RingModulator: return "MODULATION // ANALOG CORE";
            case NodeType::Tape: return "COLOR // TAPE WARMTH";
            case NodeType::Container:
            case NodeType::Feedback:
            case NodeType::EventContainer: return "CONTAINER // SUBGRAPH";
            case NodeType::MidSideEncoder:
            case NodeType::MidSideDecoder: return "ROUTING // MID-SIDE MATRIX";
            case NodeType::SpatialPanner: return "SPATIAL // 3D BINAURAL";
            default: return "DSP MODULE";
        }
    }

private:
    NodeId id_{ InvalidNodeId };
    int slotIndex_{ 0 };
    juce::String name_;
    NodeType type_{ NodeType::Unknown };
    AudioProcessorNode* processor_{ nullptr };

    bool isBypassed_{ false };
    bool isDragging_{ false };

    juce::TextButton moveLeftBtn_;
    juce::TextButton moveRightBtn_;
    juce::TextButton bypassBtn_;
    juce::TextButton deleteBtn_;

    std::vector<std::unique_ptr<ModulationSlider>> sliders_;

    std::function<void(int)> onMoveLeftRequested_;
    std::function<void(int)> onMoveRightRequested_;
    std::function<void(NodeId)> onDeleteRequested_;
    std::function<void(NodeId, bool)> onBypassToggled_;
    std::function<void(NodeId, ParameterId, float)> onParameterChanged_;
    std::function<void(SequentialSlotComponent*, const juce::MouseEvent&)> onSlotDragged_;
    std::function<void(SequentialSlotComponent*, const juce::MouseEvent&)> onSlotDragEnded_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequentialSlotComponent)
};

} // namespace audio_graph
