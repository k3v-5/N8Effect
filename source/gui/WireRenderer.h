#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../core/Types.h"

namespace audio_graph {

/**
 * @brief Renderizador de curvas de Bézier cúbicas de alta fidelidad para conexiones del grafo (Reglas 4, 24, 25).
 */
class WireRenderer {
public:
    static juce::Colour getPinColour(PinDataType dataType) noexcept {
        switch (dataType) {
            case PinDataType::AudioStereo:
            case PinDataType::AudioMono:
                return juce::Colour(0xff00d2ff); // Cian brillante para Audio
            case PinDataType::EventMessage:
                return juce::Colour(0xffff2d88); // Magenta eléctrico para Eventos
            case PinDataType::ModulationScalar:
                return juce::Colour(0xff00ff88); // Verde esmeralda para Modulación
            default:
                return juce::Colours::white;
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

        const juce::Colour baseColour = getPinColour(dataType);

        // Halo / resplandor exterior suave
        g.setColour(baseColour.withAlpha(isHovered ? 0.35f : 0.15f));
        g.strokePath(path, juce::PathStrokeType(isHovered ? 6.0f : 4.0f));

        // Línea central sólida o discontinua
        g.setColour(isHovered ? baseColour.brighter(0.2f) : baseColour);
        if (isDashed) {
            juce::PathStrokeType stroke(2.5f);
            float dashes[] = { 6.0f, 4.0f };
            stroke.createDashedStroke(path, path, dashes, 2);
            g.strokePath(path, stroke);
        } else {
            g.strokePath(path, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Puntos terminales en conectores
        g.setColour(baseColour.brighter(0.5f));
        g.fillEllipse(start.x - 3.5f, start.y - 3.5f, 7.0f, 7.0f);
        g.fillEllipse(end.x - 3.5f, end.y - 3.5f, 7.0f, 7.0f);
    }
};

} // namespace audio_graph
