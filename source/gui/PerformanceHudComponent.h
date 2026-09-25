#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../core/CpuProfiler.h"
#include <functional>
#include <string>

namespace audio_graph {

/**
 * @brief Widget HUD de telemetría de rendimiento y CPU en tiempo real con estética minimalista oscura y contornos blancos (Reglas 23, 26, 39, 47).
 */
class PerformanceHudComponent : public juce::Component {
public:
    PerformanceHudComponent() {
        setOpaque(false);
    }

    void updateMetrics(const PerformanceMetrics& metrics) {
        metrics_ = metrics;
        repaint();
    }

    void setOnResetOverload(std::function<void()> cb) {
        onResetOverload_ = std::move(cb);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (overloadBadgeBounds_.contains(e.getPosition()) && metrics_.overloadDetected) {
            if (onResetOverload_) {
                onResetOverload_();
            }
        }
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        // 1. Fondo negro absoluto del HUD
        g.setColour(juce::Colour(0xff000000));
        g.fillRoundedRectangle(bounds, 3.0f);

        // 2. Contorno blanco nítido
        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

        auto inner = bounds.reduced(6.0f, 4.0f);

        // Fila 1: Lectura de CPU % y Tiempo en microsegundos
        auto topRow = inner.removeFromTop(14.0f);

        juce::String cpuText = "CPU: " + juce::String(metrics_.currentCpuPercent, 1) + "%";
        juce::String peakText = "[Pk " + juce::String(metrics_.peakCpuPercent, 1) + "%]";
        juce::String dspTimeText = juce::String(static_cast<int>(metrics_.totalDspUs)) + " µs";

        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.setColour(juce::Colours::white);
        g.drawText(cpuText, topRow.removeFromLeft(64.0f), juce::Justification::centredLeft, true);

        g.setFont(juce::FontOptions(9.5f, juce::Font::plain));
        g.setColour(juce::Colours::white.withAlpha(0.65f));
        g.drawText(peakText, topRow.removeFromLeft(54.0f), juce::Justification::centredLeft, true);

        // Badge de Overload a la derecha (estilo minimalista con contorno blanco)
        overloadBadgeBounds_ = topRow.removeFromRight(36.0f).toNearestInt();
        if (metrics_.overloadDetected) {
            g.setColour(juce::Colours::white);
            g.fillRoundedRectangle(overloadBadgeBounds_.toFloat(), 2.0f);
            g.setColour(juce::Colour(0xff000000));
            g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
            g.drawText("OVR!", overloadBadgeBounds_, juce::Justification::centred, true);
        } else {
            g.setColour(juce::Colour(0xff000000));
            g.fillRoundedRectangle(overloadBadgeBounds_.toFloat(), 2.0f);
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(overloadBadgeBounds_.toFloat(), 2.0f, 1.0f);
            g.setFont(juce::FontOptions(8.5f, juce::Font::plain));
            g.drawText("OK", overloadBadgeBounds_, juce::Justification::centred, true);
        }

        g.setFont(juce::FontOptions(9.5f, juce::Font::plain));
        g.setColour(juce::Colours::white);
        g.drawText(dspTimeText, topRow, juce::Justification::centredRight, true);

        inner.removeFromTop(3.0f);

        // Fila 2: Barra de Medición de CPU con indicador de Pico
        auto barArea = inner.removeFromTop(6.0f);
        g.setColour(juce::Colour(0xff000000));
        g.fillRoundedRectangle(barArea, 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.drawRoundedRectangle(barArea, 2.0f, 1.0f);

        const float clampedCpu = std::clamp(metrics_.currentCpuPercent / 100.0f, 0.0f, 1.0f);
        if (clampedCpu > 0.001f) {
            auto fillArea = barArea.reduced(1.0f);
            fillArea.setWidth(fillArea.getWidth() * clampedCpu);
            g.setColour(juce::Colours::white.withAlpha(0.35f));
            g.fillRoundedRectangle(fillArea, 1.5f);
        }

        // Ticked Peak Marker en blanco sólido
        const float clampedPeak = std::clamp(metrics_.peakCpuPercent / 100.0f, 0.0f, 1.0f);
        if (clampedPeak > 0.01f) {
            const float peakX = barArea.getX() + (barArea.getWidth() * clampedPeak);
            g.setColour(juce::Colours::white);
            g.drawVerticalLine(static_cast<int>(peakX), barArea.getY(), barArea.getBottom());
        }

        inner.removeFromTop(3.0f);

        // Fila 3: Telemetría de Pools y Recursos
        auto bottomRow = inner.removeFromTop(12.0f);
        g.setFont(juce::FontOptions(8.5f, juce::Font::plain));
        g.setColour(juce::Colours::white.withAlpha(0.70f));

        juce::String telemetry = "EVT: " + juce::String(static_cast<int>(metrics_.activeEvents)) + "/1024" +
                                 " | G: " + juce::String(static_cast<int>(metrics_.graphExecutionUs)) + "µs" +
                                 " | A: " + juce::String(static_cast<int>(metrics_.analysisUs)) + "µs";
        g.drawText(telemetry, bottomRow, juce::Justification::centredLeft, true);
    }

private:
    PerformanceMetrics metrics_{};
    juce::Rectangle<int> overloadBadgeBounds_{};
    std::function<void()> onResetOverload_{};
};

} // namespace audio_graph
