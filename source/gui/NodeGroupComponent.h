#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <unordered_map>
#include <array>
#include <string>
#include <algorithm>
#include <cmath>
#include "../core/Types.h"
#include "../graph/Graph.h"

namespace audio_graph {

class GraphCanvasComponent;
class N8AudioProcessor;
class NodeComponent;

/**
 * @brief Paleta de 8 colores de acento neón curados para cajas de grupo (Regla R2).
 */
static constexpr std::array<uint32_t, 8> kGroupColorPalette = {
    0x00d4ffff, // Neon Cyan
    0x00e676ff, // Emerald Green
    0xa855f7ff, // Electric Violet
    0xffffa020, // Warm Amber
    0xffff3355, // Coral Red
    0xffffdd00, // Cyber Gold
    0x38bdf8ff, // Sky Blue
    0xec4899ff  // Hot Pink
};

/**
 * @brief Convierte entero uint32 RGBA a juce::Colour.
 */
inline juce::Colour rgbaToJuceColour(uint32_t rgba) noexcept {
    uint8_t a = static_cast<uint8_t>(rgba & 0xff);
    if (a == 0) a = 255;
    const uint8_t r = static_cast<uint8_t>((rgba >> 24) & 0xff);
    const uint8_t g = static_cast<uint8_t>((rgba >> 16) & 0xff);
    const uint8_t b = static_cast<uint8_t>((rgba >> 8) & 0xff);
    return juce::Colour(r, g, b, a);
}

/**
 * @brief Convierte juce::Colour a entero uint32 RGBA.
 */
inline uint32_t juceColourToRgba(juce::Colour c) noexcept {
    return (static_cast<uint32_t>(c.getRed()) << 24) |
           (static_cast<uint32_t>(c.getGreen()) << 16) |
           (static_cast<uint32_t>(c.getBlue()) << 8) |
           static_cast<uint32_t>(c.getAlpha());
}

/**
 * @brief Botón con cumplimiento estricto de Regla 48 (Disambiguación Drag & Drop vs Clic).
 */
class Rule48Button : public juce::Button {
public:
    explicit Rule48Button(const juce::String& name = {}) : juce::Button(name) {}

    void mouseDown(const juce::MouseEvent& e) override {
        hasDragged_ = false;
        juce::Button::mouseDown(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (!hasDragged_ && e.getDistanceFromDragStart() > 4) {
            hasDragged_ = true;
        }
        if (!hasDragged_) {
            juce::Button::mouseDrag(e);
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (hasDragged_) {
            hasDragged_ = false;
            setState(buttonNormal);
            return; // Regla 48: Supresión absoluta de clic tras arrastre
        }
        juce::Button::mouseUp(e);
    }

    void clicked() override {
        if (!hasDragged_ && onCleanClick_) {
            onCleanClick_();
        }
    }

    void setOnCleanClick(std::function<void()> cb) { onCleanClick_ = std::move(cb); }

    void paintButton(juce::Graphics& /*g*/, bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/) override {}

private:
    bool hasDragged_{ false };
    std::function<void()> onCleanClick_;
};

/**
 * @brief Mini dial rotatorio de control macro con cumplimiento estricto de Regla 48 (Drag vs Click Guard).
 */
class GroupMacroKnob : public juce::Component {
public:
    GroupMacroKnob(GroupId groupId, size_t macroIndex, GraphCanvasComponent& canvas, N8AudioProcessor& processor)
        : groupId_(groupId), macroIndex_(macroIndex), canvas_(canvas), processor_(processor)
    {
        setInterceptsMouseClicks(true, false);
        setRepaintsOnMouseActivity(true);
    }

    void setValue(float val) {
        val = std::clamp(val, 0.0f, 1.0f);
        if (std::abs(value_ - val) > 1e-4f) {
            value_ = val;
            applyValueToEngine();
            repaint();
        }
    }

    float getValue() const noexcept { return value_; }

    void refreshFromEngine();

    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e) override {
        hasDragged_ = false;
        dragStartVal_ = value_;
        dragStartPos_ = e.position;
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        const float dist = std::hypot(e.position.x - dragStartPos_.x, e.position.y - dragStartPos_.y);
        if (!hasDragged_ && dist > 4.0f) {
            hasDragged_ = true; // Regla 48: Superado umbral de 4px
        }

        if (hasDragged_) {
            const float delta = ((dragStartPos_.y - e.position.y) + (e.position.x - dragStartPos_.x)) * 0.005f;
            setValue(std::clamp(dragStartVal_ + delta, 0.0f, 1.0f));
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (hasDragged_) {
            hasDragged_ = false;
            return; // Regla 48: Supresión absoluta de clic tras arrastre
        }

        // Clic limpio (distancia <= 4px)
        if (e.mods.isPopupMenu() || e.mods.isRightButtonDown()) {
            showMappingMenu();
        } else if (e.getNumberOfClicks() >= 2) {
            setValue(0.5f); // Doble clic restaura al centro neutral
        }
    }

private:
    void applyValueToEngine();
    void showMappingMenu();

    GroupId groupId_;
    size_t macroIndex_{ 0 };
    GraphCanvasComponent& canvas_;
    N8AudioProcessor& processor_;

    float value_{ 0.5f };
    bool hasDragged_{ false };
    float dragStartVal_{ 0.5f };
    juce::Point<float> dragStartPos_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GroupMacroKnob)
};

/**
 * @brief Componente visual de Caja de Agrupación de Nodos en el Grafo (Regla R2).
 * 
 * Encerramiento translúcido de nodos miembros con chasis coloreado, barra de encabezado,
 * conmutador de colapso, selector de color, bypass colectivo con LED, y 3 knobs macro dedicados.
 * Implementa movimiento síncrono de nodos miembros y protección de hit testing.
 */
class NodeGroupComponent : public juce::Component {
public:
    static constexpr int kHeaderHeight = 44;

    NodeGroupComponent(GroupId groupId, GraphCanvasComponent& canvas, N8AudioProcessor& processor);
    ~NodeGroupComponent() override = default;

    GroupId getGroupId() const noexcept { return groupId_; }

    void updateBoundsFromMembers();
    void syncWithGraph();

    bool hitTest(int x, int y) override {
        if (isCollapsed_) {
            return true; // En estado colapsado, toda la barra es interactiva
        }

        // Encabezado interactivo
        if (y >= 0 && y <= kHeaderHeight) {
            return true;
        }

        // Marco perimetral de 6px interactivo para mover o redimensionar
        constexpr int borderMargin = 6;
        if (x <= borderMargin || x >= getWidth() - borderMargin ||
            y >= getHeight() - borderMargin) {
            return true;
        }

        // El área interior cae a través hacia los nodos miembros y el canvas
        return false;
    }

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Movimiento síncrono del encabezado con Regla 48
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void toggleCollapse();
    void toggleBypass();
    void cycleColor();

private:
    NodeGroup* getGroup();
    const NodeGroup* getGroup() const;
    juce::Colour getGroupColour() const;

    GroupId groupId_{ InvalidGroupId };
    GraphCanvasComponent& canvas_;
    N8AudioProcessor& processor_;

    bool isCollapsed_{ false };
    bool isBypassed_{ false };
    uint32_t colorRgba_{ 0x00d4ffff };
    size_t paletteIndex_{ 0 };

    // Controles de encabezado
    juce::Label titleLabel_;
    std::unique_ptr<Rule48Button> bypassButton_;
    std::unique_ptr<Rule48Button> collapseButton_;
    std::unique_ptr<Rule48Button> colorButton_;
    std::array<std::unique_ptr<GroupMacroKnob>, 3> macroKnobs_;

    // Estado para arrastre síncrono de nodos (Regla 48)
    bool hasDragged_{ false };
    juce::Point<float> dragStartCanvasPos_;
    juce::Point<int> dragStartGroupPos_;
    std::unordered_map<NodeId, juce::Point<int>> initialNodePositions_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NodeGroupComponent)
};

} // namespace audio_graph
