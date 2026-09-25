#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <string>

namespace audio_graph {

/**
 * @brief Slider minimalista con visualización en tiempo real de Modulación y contornos blancos nítidos (Regla 25).
 * Muestra simultáneamente: Base Value, Modulation Depth y Current Modulated Value animado.
 */
class ModulationSlider : public juce::Component {
public:
    ModulationSlider(const juce::String& paramName, float minVal, float maxVal, float defaultVal)
        : name_(paramName), minVal_(minVal), maxVal_(maxVal), baseValue_(defaultVal), currentModulatedValue_(defaultVal)
    {
        setRepaintsOnMouseActivity(true);
    }

    void setBaseValue(float v) {
        baseValue_ = std::clamp(v, minVal_, maxVal_);
        currentModulatedValue_ = baseValue_;
        repaint();
    }

    float getBaseValue() const noexcept { return baseValue_; }

    void setModulationDepth(float depth) {
        modDepth_ = std::clamp(depth, -1.0f, 1.0f);
        repaint();
    }

    void setLiveModulatedValue(float modVal) {
        currentModulatedValue_ = std::clamp(modVal, minVal_, maxVal_);
        repaint();
    }

    void setOnValueChanged(std::function<void(float)> callback) {
        onValueChanged_ = std::move(callback);
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(1.5f);

        // 1. Fondo oscuro absoluto del slider
        g.setColour(juce::Colour(0xff000000));
        g.fillRoundedRectangle(bounds, 3.0f);

        // 2. Contorno blanco nítido minimalista
        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

        const float normBase = (maxVal_ > minVal_) ? (baseValue_ - minVal_) / (maxVal_ - minVal_) : 0.0f;
        const float normLive = (maxVal_ > minVal_) ? (currentModulatedValue_ - minVal_) / (maxVal_ - minVal_) : 0.0f;

        // 3. Barra de progreso de valor base en blanco sutil
        const float barWidth = bounds.getWidth() * normBase;
        auto baseRect = bounds.withWidth(barWidth);
        g.setColour(juce::Colours::white.withAlpha(0.20f));
        g.fillRoundedRectangle(baseRect, 2.0f);

        // Indicador de posición de valor base (línea vertical blanca nítida)
        if (barWidth > 1.0f && barWidth < bounds.getWidth() - 1.0f) {
            g.setColour(juce::Colours::white);
            g.drawVerticalLine(static_cast<int>(baseRect.getRight()), bounds.getY() + 1.0f, bounds.getBottom() - 1.0f);
        }

        // 4. Rango de modulación visual (sombreado blanco traslúcido)
        if (std::abs(modDepth_) > 0.001f) {
            const float modEnd = std::clamp(normBase + modDepth_, 0.0f, 1.0f);
            const float startX = bounds.getX() + std::min(normBase, modEnd) * bounds.getWidth();
            const float modW = std::abs(modEnd - normBase) * bounds.getWidth();
            g.setColour(juce::Colours::white.withAlpha(0.12f));
            g.fillRect(startX, bounds.getY() + 1.0f, modW, bounds.getHeight() - 2.0f);
            g.setColour(juce::Colours::white.withAlpha(0.40f));
            g.drawRect(startX, bounds.getY() + 1.0f, modW, bounds.getHeight() - 2.0f, 1.0f);
        }

        // 5. Indicador animado de modulación en vivo (círculo nítido con núcleo blanco)
        const float liveX = bounds.getX() + normLive * bounds.getWidth();
        g.setColour(juce::Colour(0xff000000));
        g.fillEllipse(liveX - 3.5f, bounds.getCentreY() - 3.5f, 7.0f, 7.0f);
        g.setColour(juce::Colours::white);
        g.drawEllipse(liveX - 3.5f, bounds.getCentreY() - 3.5f, 7.0f, 7.0f, 1.0f);
        g.fillEllipse(liveX - 1.5f, bounds.getCentreY() - 1.5f, 3.0f, 3.0f);

        // 6. Texto de etiqueta y valor numérico
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(10.5f, juce::Font::plain));
        g.drawText(name_, bounds.reduced(6.0f, 0.0f), juce::Justification::centredLeft, true);

        juce::String valStr = juce::String(baseValue_, (maxVal_ - minVal_ > 10.0f) ? 1 : 2);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(valStr, bounds.reduced(6.0f, 0.0f), juce::Justification::centredRight, true);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        updateFromMouse(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        updateFromMouse(e);
    }

private:
    void updateFromMouse(const juce::MouseEvent& e) {
        auto bounds = getLocalBounds().toFloat().reduced(1.5f);
        const float norm = std::clamp((e.position.x - bounds.getX()) / bounds.getWidth(), 0.0f, 1.0f);
        baseValue_ = minVal_ + norm * (maxVal_ - minVal_);
        currentModulatedValue_ = baseValue_;
        if (onValueChanged_) {
            onValueChanged_(baseValue_);
        }
        repaint();
    }

    juce::String name_;
    float minVal_{ 0.0f };
    float maxVal_{ 1.0f };
    float baseValue_{ 0.5f };
    float modDepth_{ 0.0f };
    float currentModulatedValue_{ 0.5f };
    std::function<void(float)> onValueChanged_;
};

} // namespace audio_graph
