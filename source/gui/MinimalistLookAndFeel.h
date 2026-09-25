#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace audio_graph {

/**
 * @brief LookAndFeel minimalista con fondos negros profundos y contornos blancos nítidos (Reglas 23, 24).
 */
class MinimalistLookAndFeel : public juce::LookAndFeel_V4 {
public:
    MinimalistLookAndFeel() {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff000000));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff000000));
        setColour(juce::ComboBox::textColourId, juce::Colours::white);
        setColour(juce::ComboBox::outlineColourId, juce::Colours::white);
        setColour(juce::ComboBox::arrowColourId, juce::Colours::white);

        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff000000));
        setColour(juce::PopupMenu::textColourId, juce::Colours::white);
        setColour(juce::PopupMenu::headerTextColourId, juce::Colours::white);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0x33ffffff));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff000000));
        setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);

        setColour(juce::Slider::backgroundColourId, juce::Colour(0xff000000));
        setColour(juce::Slider::trackColourId, juce::Colours::white.withAlpha(0.25f));
        setColour(juce::Slider::thumbColourId, juce::Colours::white);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff000000));
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::white);

        setColour(juce::Label::textColourId, juce::Colours::white);
        setColour(juce::ScrollBar::thumbColourId, juce::Colours::white.withAlpha(0.4f));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& /*backgroundColour*/,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        const float alpha = button.isEnabled() ? 1.0f : 0.35f;

        // Fondo negro o sutil resaltado al interactuar
        if (shouldDrawButtonAsDown) {
            g.setColour(juce::Colours::white.withAlpha(0.30f));
        } else if (shouldDrawButtonAsHighlighted) {
            g.setColour(juce::Colours::white.withAlpha(0.15f));
        } else {
            g.setColour(juce::Colour(0xff000000));
        }
        g.fillRoundedRectangle(bounds, 3.0f);

        // Contorno blanco nítido
        g.setColour(juce::Colours::white.withAlpha(alpha));
        g.drawRoundedRectangle(bounds, 3.0f, (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown) ? 1.5f : 1.0f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override {
        g.setFont(juce::FontOptions(button.getHeight() > 24 ? 11.5f : 10.0f, juce::Font::bold));
        const float alpha = button.isEnabled() ? 1.0f : 0.35f;
        g.setColour(juce::Colours::white.withAlpha(alpha));
        g.drawText(button.getButtonText(), button.getLocalBounds().reduced(2), juce::Justification::centred, true);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                      int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& /*box*/) override {
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);
        
        g.setColour(juce::Colour(0xff000000));
        g.fillRoundedRectangle(bounds, 3.0f);

        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

        // Flecha triangular en blanco nítido
        juce::Path arrow;
        const float arrowX = static_cast<float>(buttonX + (buttonW / 2));
        const float arrowY = static_cast<float>(buttonY + (buttonH / 2));
        arrow.addTriangle(arrowX - 4.0f, arrowY - 2.0f,
                          arrowX + 4.0f, arrowY - 2.0f,
                          arrowX, arrowY + 3.0f);
        g.setColour(juce::Colours::white);
        g.fillPath(arrow);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(0.5f);

        if (style == juce::Slider::LinearBar || style == juce::Slider::LinearHorizontal) {
            // Fondo de la pista en negro absoluto
            g.setColour(juce::Colour(0xff000000));
            g.fillRoundedRectangle(bounds, 2.0f);

            // Barra de progreso en blanco sutil
            auto fillRect = bounds.withWidth(std::max(0.0f, sliderPos - bounds.getX()));
            g.setColour(juce::Colours::white.withAlpha(0.25f));
            g.fillRoundedRectangle(fillRect, 2.0f);

            // Indicador de posición de valor en línea blanca nítida
            if (sliderPos > bounds.getX() + 1.0f && sliderPos < bounds.getRight() - 1.0f) {
                g.setColour(juce::Colours::white);
                g.drawVerticalLine(static_cast<int>(sliderPos), bounds.getY() + 1.0f, bounds.getBottom() - 1.0f);
            }

            // Contorno blanco nítido
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
        } else {
            juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, 0, 0, style, slider);
        }
    }
};

} // namespace audio_graph
