#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <string>
#include "../preset/GraphUndoManager.h"

namespace audio_graph {

/**
 * @brief Componente Visual de Línea de Tiempo de Undo/Redo y Time-Travel Histórico (Reglas 21, 22, 48).
 * Renderiza la secuencia cronológica de checkpoints del grafo, badges categóricos e interactividad
 * para viajar instantáneamente a cualquier versión histórica.
 */
class UndoTimelineComponent : public juce::Component {
public:
    using JumpCallback = std::function<void(size_t)>;
    using ActionCallback = std::function<void()>;

    explicit UndoTimelineComponent(GraphUndoManager& undoManager)
        : undoManager_(undoManager)
    {
        setOpaque(true);

        auto setupButton = [](juce::TextButton& btn, const juce::String& text) {
            btn.setButtonText(text);
            btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181818));
            btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
            btn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::white);
        };

        setupButton(undoBtn_, "< UNDO");
        setupButton(redoBtn_, "REDO >");

        undoBtn_.onClick = [this]() {
            if (onUndoRequested_) onUndoRequested_();
            updateState();
        };

        redoBtn_.onClick = [this]() {
            if (onRedoRequested_) onRedoRequested_();
            updateState();
        };

        addAndMakeVisible(undoBtn_);
        addAndMakeVisible(redoBtn_);

        updateState();
    }

    void setOnJumpRequested(JumpCallback cb) { onJumpRequested_ = std::move(cb); }
    void setOnUndoRequested(ActionCallback cb) { onUndoRequested_ = std::move(cb); }
    void setOnRedoRequested(ActionCallback cb) { onRedoRequested_ = std::move(cb); }

    void updateState() {
        undoBtn_.setEnabled(undoManager_.canUndo());
        redoBtn_.setEnabled(undoManager_.canRedo());

        if (undoManager_.canUndo()) {
            undoBtn_.setTooltip("Undo: " + undoManager_.getNextUndoDescription());
        } else {
            undoBtn_.setTooltip("No actions to undo");
        }

        if (undoManager_.canRedo()) {
            redoBtn_.setTooltip("Redo: " + undoManager_.getNextRedoDescription());
        } else {
            redoBtn_.setTooltip("No actions to redo");
        }

        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override {
        hasDragged_ = false;
        clickStartPos_ = e.getPosition();
        handleTimelineClick(e.position);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (!hasDragged_ && e.getDistanceFromDragStart() > 4) {
            hasDragged_ = true;
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (hasDragged_) {
            hasDragged_ = false;
            return;
        }
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // Fondo
        g.setColour(juce::Colour(0xff0b0e14));
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

        // Header info
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.setColour(juce::Colours::white.withAlpha(0.75f));
        g.drawText("ACTION TIMELINE & TIME-TRAVEL", bounds.reduced(10.0f).removeFromTop(18.0f), juce::Justification::centredLeft);

        // Timeline de checkpoints
        drawTimeline(g, bounds);
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(8, 6);
        auto header = bounds.removeFromTop(22);

        undoBtn_.setBounds(header.removeFromRight(64));
        header.removeFromRight(6);
        redoBtn_.setBounds(header.removeFromRight(64));
    }

private:
    void handleTimelineClick(juce::Point<int> pos) {
        if (itemBoundsList_.empty()) return;
        for (size_t i = 0; i < itemBoundsList_.size(); ++i) {
            if (itemBoundsList_[i].contains(pos.toFloat())) {
                if (onJumpRequested_) {
                    onJumpRequested_(i);
                }
                updateState();
                break;
            }
        }
    }

    void drawTimeline(juce::Graphics& g, juce::Rectangle<float> bounds) {
        itemBoundsList_.clear();
        const auto& timeline = undoManager_.getTimeline();
        if (timeline.empty()) {
            g.setFont(juce::Font(10.0f, juce::Font::italic));
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.drawText("No history recorded yet", bounds.reduced(12.0f), juce::Justification::centred);
            return;
        }

        const size_t currentIdx = undoManager_.getCurrentIndex();
        auto area = bounds;
        area.removeFromTop(26.0f);
        area.reduce(6.0f, 4.0f);

        const float itemH = 22.0f;
        const float gap = 3.0f;
        const size_t count = timeline.size();

        for (size_t i = 0; i < count; ++i) {
            const auto& entry = timeline[i];
            auto itemBox = area.removeFromTop(itemH);
            area.removeFromTop(gap);
            itemBoundsList_.push_back(itemBox);

            const bool isCurrent = (i == currentIdx);
            const bool isPast = (i < currentIdx);

            // Fondo del ítem
            juce::Colour bgCol = isCurrent ? juce::Colour(0xff142232) :
                                 isPast ? juce::Colour(0xff121418) :
                                          juce::Colour(0xff0d0e12);

            g.setColour(bgCol);
            g.fillRoundedRectangle(itemBox, 3.0f);

            // Borde si es el paso activo
            if (isCurrent) {
                g.setColour(juce::Colour(0xff00d4ff));
                g.drawRoundedRectangle(itemBox, 3.0f, 1.2f);
            } else {
                g.setColour(juce::Colours::white.withAlpha(isPast ? 0.1f : 0.04f));
                g.drawRoundedRectangle(itemBox, 3.0f, 0.8f);
            }

            auto contentBox = itemBox.reduced(6.0f, 2.0f);

            // Badge numérico / Categoría
            auto badgeBox = contentBox.removeFromLeft(44.0f);
            g.setFont(juce::Font(8.0f, juce::Font::bold));
            juce::Colour badgeCol = getCategoryColour(entry.category);
            g.setColour(badgeCol.withAlpha(isCurrent ? 1.0f : (isPast ? 0.7f : 0.4f)));
            g.drawText(getCategoryBadge(entry.category), badgeBox, juce::Justification::centredLeft);

            // Descripción de la acción
            g.setFont(juce::Font(9.0f, isCurrent ? juce::Font::bold : juce::Font::plain));
            g.setColour(isCurrent ? juce::Colours::white : juce::Colours::white.withAlpha(isPast ? 0.75f : 0.35f));
            g.drawText(entry.description, contentBox, juce::Justification::centredLeft, true);

            // Indicador de estado actual
            if (isCurrent) {
                g.setColour(juce::Colour(0xff00d4ff));
                g.fillEllipse(itemBox.getRight() - 14.0f, itemBox.getCentreY() - 3.0f, 6.0f, 6.0f);
            }
        }
    }

    [[nodiscard]] static const char* getCategoryBadge(ActionCategory cat) noexcept {
        switch (cat) {
            case ActionCategory::NodeAdd: return "[+ NODE]";
            case ActionCategory::NodeRemove: return "[- NODE]";
            case ActionCategory::Connection: return "[LINK]";
            case ActionCategory::Disconnection: return "[UNLINK]";
            case ActionCategory::ParameterChange: return "[PARAM]";
            case ActionCategory::PresetLoad: return "[PRESET]";
            case ActionCategory::Randomize: return "[RND]";
            case ActionCategory::General: default: return "[STATE]";
        }
    }

    [[nodiscard]] static juce::Colour getCategoryColour(ActionCategory cat) noexcept {
        switch (cat) {
            case ActionCategory::NodeAdd: return juce::Colour(0xff00ff88);
            case ActionCategory::NodeRemove: return juce::Colour(0xffff4466);
            case ActionCategory::Connection: return juce::Colour(0xff00d4ff);
            case ActionCategory::Disconnection: return juce::Colour(0xffff9900);
            case ActionCategory::ParameterChange: return juce::Colour(0xffddaa00);
            case ActionCategory::PresetLoad: return juce::Colour(0xffaa66ff);
            case ActionCategory::Randomize: return juce::Colour(0xffff00bb);
            case ActionCategory::General: default: return juce::Colours::white;
        }
    }

    GraphUndoManager& undoManager_;
    juce::TextButton undoBtn_;
    juce::TextButton redoBtn_;

    JumpCallback onJumpRequested_;
    ActionCallback onUndoRequested_;
    ActionCallback onRedoRequested_;

    std::vector<juce::Rectangle<float>> itemBoundsList_;
    bool hasDragged_{ false };
    juce::Point<int> clickStartPos_{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndoTimelineComponent)
};

} // namespace audio_graph
