#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../core/CpuProfiler.h"
#include <functional>
#include <string>

namespace audio_graph {

/**
 * @brief Widget HUD de telemetría de rendimiento y CPU en tiempo real (Reglas 23, 26, 39, 47)
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

        // Fondo tipo panel de telemetría HUD
        g.setColour(juce::Colour(0xcc141420));
        g.fillRoundedRectangle(bounds, 5.0f);
        g.setColour(juce::Colour(0xff2a2a3e));
        g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

        auto inner = bounds.reduced(6.0f, 4.0f);

        // Fila 1: Lectura de CPU % y Tiempo en microsegundos
        auto topRow = inner.removeFromTop(14.0f);

        juce::String cpuText = "CPU: " + juce::String(metrics_.currentCpuPercent, 1) + "%";
        juce::String peakText = "[Pk " + juce::String(metrics_.peakCpuPercent, 1) + "%]";
        juce::String dspTimeText = juce::String(static_cast<int>(metrics_.totalDspUs)) + " µs";

        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.setColour(getCpuColour(metrics_.currentCpuPercent));
        g.drawText(cpuText, topRow.removeFromLeft(64.0f), juce::Justification::centredLeft, true);

        g.setFont(juce::FontOptions(9.5f, juce::Font::plain));
        g.setColour(juce::Colour(0xff8c96a5));
        g.drawText(peakText, topRow.removeFromLeft(54.0f), juce::Justification::centredLeft, true);

        // Badge de Overload a la derecha
        overloadBadgeBounds_ = topRow.removeFromRight(36.0f).toNearestInt();
        if (metrics_.overloadDetected) {
            g.setColour(juce::Colour(0xffff1744));
            g.fillRoundedRectangle(overloadBadgeBounds_.toFloat(), 3.0f);
            g.setColour(juce::Colours::white);
            g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
            g.drawText("OVR!", overloadBadgeBounds_, juce::Justification::centred, true);
        } else {
            g.setColour(juce::Colour(0x3300e676));
            g.fillRoundedRectangle(overloadBadgeBounds_.toFloat(), 3.0f);
            g.setColour(juce::Colour(0xff00e676));
            g.setFont(juce::FontOptions(8.5f, juce::Font::plain));
            g.drawText("OK", overloadBadgeBounds_, juce::Justification::centred, true);
        }

        g.setFont(juce::FontOptions(9.5f, juce::Font::plain));
        g.setColour(juce::Colour(0xff00d2ff));
        g.drawText(dspTimeText, topRow, juce::Justification::centredRight, true);

        inner.removeFromTop(3.0f);

        // Fila 2: Barra de Medición de CPU con indicador de Pico
        auto barArea = inner.removeFromTop(6.0f);
        g.setColour(juce::Colour(0xff0b0b12));
        g.fillRoundedRectangle(barArea, 3.0f);

        const float clampedCpu = std::clamp(metrics_.currentCpuPercent / 100.0f, 0.0f, 1.0f);
        if (clampedCpu > 0.001f) {
            auto fillArea = barArea;
            fillArea.setWidth(barArea.getWidth() * clampedCpu);
            g.setColour(getCpuColour(metrics_.currentCpuPercent));
            g.fillRoundedRectangle(fillArea, 3.0f);
        }

        // Ticked Peak Marker
        const float clampedPeak = std::clamp(metrics_.peakCpuPercent / 100.0f, 0.0f, 1.0f);
        if (clampedPeak > 0.01f) {
            const float peakX = barArea.getX() + (barArea.getWidth() * clampedPeak);
            g.setColour(juce::Colours::white.withAlpha(0.85f));
            g.drawVerticalLine(static_cast<int>(peakX), barArea.getY(), barArea.getBottom());
        }

        inner.removeFromTop(3.0f);

        // Fila 3: Telemetría de Pools y Recursos
        auto bottomRow = inner.removeFromTop(12.0f);
        g.setFont(juce::FontOptions(8.5f, juce::Font::plain));
        g.setColour(juce::Colour(0xff758296));

        juce::String telemetry = "EVT: " + juce::String(static_cast<int>(metrics_.activeEvents)) + "/1024" +
                                 " | G: " + juce::String(static_cast<int>(metrics_.graphExecutionUs)) + "µs" +
                                 " | A: " + juce::String(static_cast<int>(metrics_.analysisUs)) + "µs";
        g.drawText(telemetry, bottomRow, juce::Justification::centredLeft, true);
    }

private:
    static juce::Colour getCpuColour(float cpuPercent) noexcept {
        if (cpuPercent < 50.0f) return juce::Colour(0xff00e676); // Verde brillante
        if (cpuPercent < 80.0f) return juce::Colour(0xffffd600); // Amarillo advertencia
        if (cpuPercent < 95.0f) return juce::Colour(0xffff9100); // Naranja precaución
        return juce::Colour(0xffff1744);                        // Rojo crítico
    }

    PerformanceMetrics metrics_{};
    juce::Rectangle<int> overloadBadgeBounds_{};
    std::function<void()> onResetOverload_{};
};

} // namespace audio_graph
