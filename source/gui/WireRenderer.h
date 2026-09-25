#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../core/Types.h"

#include "ThemeManager.h"

namespace audio_graph {

/**
 * @brief Renderizador de curvas de Bézier monocromáticas y adaptativas al tema del chasis (Reglas 4, 24, 25).
 */
class WireRenderer {
public:
    static juce::Colour getPinColour(PinDataType dataType) noexcept {
        const auto& theme = ThemeManager::getInstance().getColors();
        switch (dataType) {
            case PinDataType::AudioStereo:
            case PinDataType::AudioMono:
                return theme.textPrimary;
            case PinDataType::EventMessage:
                return theme.accentSecondary;
            case PinDataType::ModulationScalar:
                return theme.accentPrimary;
            default:
                return theme.textPrimary;
        }
    }

    static void drawWire(juce::Graphics& g,
                         juce::Point<float> start,
                         juce::Point<float> end,
                         PinDataType dataType,
                         bool isHovered = false,
                         bool isDashed = false) {
        juce::Path path;
        path.startNewSubPath(start);

        const float dx = std::max(40.0f, std::abs(end.x - start.x) * 0.5f);
        const juce::Point<float> c1(start.x + dx, start.y);
        const juce::Point<float> c2(end.x - dx, end.y);

        path.cubicTo(c1, c2, end);

        // Halo / resplandor exterior según tema cuando se pasa el ratón por encima
        if (isHovered) {
            g.setColour(ThemeManager::getInstance().getColors().wireGlowColor);
            g.strokePath(path, juce::PathStrokeType(5.0f));
        }

        // Trazado de línea según tema
        g.setColour(isHovered ? ThemeManager::getInstance().getColors().accentPrimary : getPinColour(dataType));

        const bool shouldDash = isDashed || (dataType == PinDataType::EventMessage);
        const bool shouldDot = (dataType == PinDataType::ModulationScalar);

        if (shouldDash) {
            juce::PathStrokeType stroke(isHovered ? 2.0f : 1.5f);
            float dashes[] = { 6.0f, 4.0f };
            stroke.createDashedStroke(path, path, dashes, 2);
            g.strokePath(path, stroke);
        } else if (shouldDot) {
            juce::PathStrokeType stroke(isHovered ? 2.0f : 1.5f);
            float dashes[] = { 2.5f, 3.5f };
            stroke.createDashedStroke(path, path, dashes, 2);
            g.strokePath(path, stroke);
        } else {
            g.strokePath(path, juce::PathStrokeType(isHovered ? 2.2f : 1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Puntos terminales en conectores: círculo negro con contorno blanco nítido y punto central
        auto drawTerminal = [&](float x, float y) {
            g.setColour(juce::Colour(0xff000000));
            g.fillEllipse(x - 4.0f, y - 4.0f, 8.0f, 8.0f);
            g.setColour(juce::Colours::white);
            g.drawEllipse(x - 4.0f, y - 4.0f, 8.0f, 8.0f, 1.2f);
            g.fillEllipse(x - 1.5f, y - 1.5f, 3.0f, 3.0f);
        };

        drawTerminal(start.x, start.y);
        drawTerminal(end.x, end.y);
    }
};

} // namespace audio_graph
