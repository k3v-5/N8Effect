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
                         bool isDashed = false,
                         bool isInvalid = false) {
        juce::Path path;
        path.startNewSubPath(start);

        const float dx = std::max(40.0f, std::abs(end.x - start.x) * 0.5f);
        const juce::Point<float> c1(start.x + dx, start.y);
        const juce::Point<float> c2(end.x - dx, end.y);

        path.cubicTo(c1, c2, end);

        if (isInvalid) {
            // Rechazo visual en rojo brillante (#FF2040) con halo de advertencia (Reglas 4, 15, 24, 28)
            g.setColour(juce::Colour(0x66FF2040));
            g.strokePath(path, juce::PathStrokeType(6.0f));

            g.setColour(juce::Colour(0xFFFF2040));
            juce::PathStrokeType stroke(2.5f);
            float dashes[] = { 8.0f, 4.0f };
            stroke.createDashedStroke(path, path, dashes, 2);
            g.strokePath(path, stroke);
        } else {
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
        }

        // Puntos terminales en conectores
        auto drawTerminal = [&](float x, float y) {
            g.setColour(isInvalid ? juce::Colour(0xFFFF2040) : juce::Colour(0xff000000));
            g.fillEllipse(x - 4.0f, y - 4.0f, 8.0f, 8.0f);
            g.setColour(isInvalid ? juce::Colour(0xFFFF8090) : juce::Colours::white);
            g.drawEllipse(x - 4.0f, y - 4.0f, 8.0f, 8.0f, 1.2f);
            g.fillEllipse(x - 1.5f, y - 1.5f, 3.0f, 3.0f);
        };

        drawTerminal(start.x, start.y);
        drawTerminal(end.x, end.y);
    }

    static void drawModulationWire(juce::Graphics& g,
                                  juce::Point<float> start,
                                  juce::Point<float> end,
                                  juce::Colour col,
                                  float amount,
                                  float animPhase,
                                  bool isDragging = false,
                                  bool isSnapped = false) {
        juce::Path path;
        path.startNewSubPath(start);

        const float dx = std::max(45.0f, std::abs(end.x - start.x) * 0.45f);
        const float dy = (end.y - start.y) * 0.15f;
        const juce::Point<float> c1(start.x + dx, start.y + dy);
        const juce::Point<float> c2(end.x - dx, end.y - dy);

        path.cubicTo(c1, c2, end);

        // 1. Resplandor difuso exterior (Glow halo)
        g.setColour(col.withAlpha(isDragging ? 0.35f : 0.18f));
        g.strokePath(path, juce::PathStrokeType(isDragging ? 6.5f : 5.0f));

        // 2. Línea principal del cable de modulación (punteada fina estilo modular)
        g.setColour(col.withAlpha(0.95f));
        juce::PathStrokeType stroke(isDragging ? 2.2f : 1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        float dashes[] = { 4.0f, 3.0f };
        juce::Path dashedPath;
        stroke.createDashedStroke(dashedPath, path, dashes, 2);
        g.strokePath(dashedPath, stroke);

        // 3. Partículas de flujo de energía animada en tiempo real a 60 FPS (Energy Particle Flow)
        constexpr int numParticles = 4;
        const float flowDir = (amount >= 0.0f) ? 1.0f : -1.0f;
        for (int p = 0; p < numParticles; ++p) {
            float t = std::fmod(animPhase * flowDir + static_cast<float>(p) / static_cast<float>(numParticles) + 10.0f, 1.0f);

            // Evaluación de Bézier cúbica B(t)
            const float u = 1.0f - t;
            const float tt = t * t;
            const float uu = u * u;
            const float uuu = uu * u;
            const float ttt = tt * t;

            const float px = uuu * start.x + 3.0f * uu * t * c1.x + 3.0f * u * tt * c2.x + ttt * end.x;
            const float py = uuu * start.y + 3.0f * uu * t * c1.y + 3.0f * u * tt * c2.y + ttt * end.y;

            // Halo de la partícula
            g.setColour(col.withAlpha(0.45f));
            g.fillEllipse(px - 3.5f, py - 3.5f, 7.0f, 7.0f);

            // Núcleo brillante
            g.setColour(juce::Colours::white);
            g.fillEllipse(px - 1.5f, py - 1.5f, 3.0f, 3.0f);
        }

        // 4. Conectores terminales estilo jack de parcheo
        auto drawTerminal = [&](float x, float y, bool snapped) {
            g.setColour(juce::Colour(0xff090c12));
            g.fillEllipse(x - 5.0f, y - 5.0f, 10.0f, 10.0f);

            g.setColour(col);
            g.drawEllipse(x - 5.0f, y - 5.0f, 10.0f, 10.0f, snapped ? 2.0f : 1.2f);

            if (snapped) {
                g.setColour(col.withAlpha(0.35f));
                g.drawEllipse(x - 8.0f, y - 8.0f, 16.0f, 16.0f, 1.5f);
            }

            g.setColour(juce::Colours::white);
            g.fillEllipse(x - 2.0f, y - 2.0f, 4.0f, 4.0f);
        };

        drawTerminal(start.x, start.y, false);
        drawTerminal(end.x, end.y, isSnapped);
    }
};

} // namespace audio_graph
