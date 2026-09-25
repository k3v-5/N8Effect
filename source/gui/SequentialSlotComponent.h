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

        // 3. Botón Bypass / Power (Rocker switch táctil estilo consola)
        bypassBtn_.setButtonText("ON");
        bypassBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff122420));
        bypassBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00ff88));
        bypassBtn_.onClick = [this]() {
            isBypassed_ = !isBypassed_;
            updateBypassVisuals();
            if (onBypassToggled_) onBypassToggled_(id_, isBypassed_);
        };
        addAndMakeVisible(bypassBtn_);

        // 4. Botón Eliminar Slot ✕
        deleteBtn_.setButtonText(juce::CharPointer_UTF8("\xE2\x9C\x95")); // ✕
        deleteBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1c1216));
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

        setSize(196, 280);
    }

    NodeId getNodeId() const noexcept { return id_; }
    int getSlotIndex() const noexcept { return slotIndex_; }
    NodeType getNodeType() const noexcept { return type_; }
    const juce::String& getNodeName() const noexcept { return name_; }
    bool isBypassed() const noexcept { return isBypassed_; }
    void setBypassed(bool b) {
        if (isBypassed_ != b) {
            isBypassed_ = b;
            updateBypassVisuals();
        }
    }

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
            bypassBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff221418));
            bypassBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff886677));
            setAlpha(0.68f);
        } else {
            bypassBtn_.setButtonText("ON");
            bypassBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff122420));
            bypassBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00ff88));
            setAlpha(1.0f);
        }
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        const juce::Colour catColor = NodeComponent::getCategoryColor(type_);

        // 1. Sombra exterior
        g.setColour(juce::Colour(0x60000000));
        g.fillRoundedRectangle(bounds.translated(0.0f, 4.0f), 8.0f);

        // 2. Chasis de titanio oscuro cepillado (Arturia / FLEX Hardware Card)
        juce::ColourGradient chassisGrad(juce::Colour(0xff161924), bounds.getX(), bounds.getY(),
                                         juce::Colour(0xff0a0c10), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(chassisGrad);
        g.fillRoundedRectangle(bounds, 8.0f);

        // Bisel metálico superior 3D
        g.setColour(juce::Colour(0xff2d3748).withAlpha(0.5f));
        g.drawHorizontalLine(static_cast<int>(bounds.getY() + 1.0f), bounds.getX() + 6.0f, bounds.getRight() - 6.0f);

        // 3. Tira de Luz LED de Categoría con resplandor en el borde superior
        g.setColour(catColor.withAlpha(0.35f));
        g.fillRect(bounds.getX() + 4.0f, bounds.getY() + 1.0f, bounds.getWidth() - 8.0f, 4.0f);
        g.setColour(catColor);
        g.fillRect(bounds.getX() + 8.0f, bounds.getY() + 1.0f, bounds.getWidth() - 16.0f, 2.0f);

        // 4. Cabecera (Badge de índice, Nombre y Subtítulo)
        auto header = bounds.removeFromTop(34.0f);

        // Badge circular de índice de slot ("01", "02")
        const juce::Rectangle<float> badgeRect(bounds.getX() + 8.0f, 8.0f, 20.0f, 20.0f);
        g.setColour(juce::Colour(0xff06070a));
        g.fillEllipse(badgeRect);
        g.setColour(catColor);
        g.drawEllipse(badgeRect, 1.5f);

        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        juce::String idxStr = (slotIndex_ < 9 ? "0" : "") + juce::String(slotIndex_ + 1);
        g.drawText(idxStr, badgeRect, juce::Justification::centred, false);

        // Nombre del Procesador
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.setColour(juce::Colours::white);
        juce::Rectangle<float> titleRect(34.0f, 8.0f, getWidth() - 130.0f, 18.0f);
        g.drawText(name_, titleRect, juce::Justification::centredLeft, true);

        // 5. Barra / Pastilla de Subtítulo de Categoría
        juce::Rectangle<float> subRect(8.0f, 36.0f, static_cast<float>(getWidth() - 16), 14.0f);
        g.setColour(catColor.withAlpha(0.18f));
        g.fillRoundedRectangle(subRect, 3.0f);
        g.setColour(catColor);
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText(NodeComponent::getCategorySubtitle(type_), subRect.reduced(6.0f, 0.0f), juce::Justification::centredLeft, false);

        // 6. Mini Pantalla Gráfica LCD Interactiva
        juce::Rectangle<float> lcdBounds(8.0f, 54.0f, static_cast<float>(getWidth() - 16), 34.0f);
        NodeComponent::drawMiniVisualCurve(g, lcdBounds, type_, catColor, isBypassed_);

        // 7. Borde de la tarjeta
        if (isDragging_) {
            g.setColour(juce::Colour(0xff00f0ff));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f, 2.0f);
        } else {
            g.setColour(juce::Colour(0xff202534));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 8.0f, 1.0f);
        }
    }

    void resized() override {
        // Cabecera: Botones de control táctiles a la derecha
        const int btnY = 8;
        const int btnH = 18;

        deleteBtn_.setBounds(getWidth() - 24, btnY, 18, btnH);
        bypassBtn_.setBounds(getWidth() - 58, btnY, 30, btnH);
        moveRightBtn_.setBounds(getWidth() - 76, btnY, 16, btnH);
        moveLeftBtn_.setBounds(getWidth() - 94, btnY, 16, btnH);

        // Área de Sliders de Parámetros (debajo de cabecera + categoría + mini LCD)
        auto area = getLocalBounds();
        area.removeFromTop(94); // Cabecera (34) + Categoría (18) + Mini LCD (36) + Margen (6)
        area.removeFromBottom(8);
        area.reduce(10, 0);

        for (auto& slider : sliders_) {
            slider->setBounds(area.removeFromTop(25));
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
