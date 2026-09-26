#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

namespace audio_graph {

/**
 * @brief Paletas de colores del chasis para N8Effect (Regla 23, 25).
 */
enum class ThemePreset {
    Cyberpunk = 0,
    VintageConsole,
    CleanStudio,
    PhosphorCRT,
    HighContrastDark,
    HighContrastLight
};

/**
 * @brief Tokens de diseño y colores unificados para el motor gráfico y GUI.
 */
struct ThemeColors {
    juce::Colour backgroundDark;
    juce::Colour headerDark;
    juce::Colour panelSurface;
    juce::Colour cardSurface;
    juce::Colour accentPrimary;
    juce::Colour accentSecondary;
    juce::Colour borderMuted;
    juce::Colour borderFocused;
    juce::Colour textPrimary;
    juce::Colour textSecondary;
    juce::Colour ledActive;
    juce::Colour ledBypassed;
    juce::Colour wireGlowColor;
    juce::Colour waveformLcdColor;
};

/**
 * @brief Gestor centralizado del tema visual del chasis (Theme Customizer).
 */
class ThemeManager {
public:
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void themeChanged(const ThemeColors& newTheme, ThemePreset preset) = 0;
    };

    static ThemeManager& getInstance() {
        static ThemeManager instance;
        return instance;
    }

    ThemePreset getCurrentPreset() const noexcept { return currentPreset_; }
    const ThemeColors& getColors() const noexcept { return currentColors_; }

    static juce::String getPresetName(ThemePreset preset) noexcept {
        switch (preset) {
            case ThemePreset::Cyberpunk:         return "CYBERPUNK";
            case ThemePreset::VintageConsole:    return "VINTAGE CONSOLE";
            case ThemePreset::CleanStudio:       return "CLEAN STUDIO";
            case ThemePreset::PhosphorCRT:       return "PHOSPHOR CRT";
            case ThemePreset::HighContrastDark:  return "HIGH CONTRAST DARK (WCAG AAA)";
            case ThemePreset::HighContrastLight: return "HIGH CONTRAST LIGHT (WCAG AAA)";
        }
        return "CYBERPUNK";
    }

    static ThemeColors getColorsForPreset(ThemePreset preset) noexcept {
        ThemeColors c;
        switch (preset) {
            case ThemePreset::Cyberpunk:
                // OLED Black + Neon Cyan + Electric Magenta
                c.backgroundDark   = juce::Colour(0xff08080f);
                c.headerDark       = juce::Colour(0xff10101c);
                c.panelSurface     = juce::Colour(0xff161626);
                c.cardSurface      = juce::Colour(0xff1c1b30);
                c.accentPrimary    = juce::Colour(0xff00f0ff); // Neon Cyan
                c.accentSecondary  = juce::Colour(0xffff0055); // Electric Magenta
                c.borderMuted      = juce::Colour(0x4400f0ff);
                c.borderFocused    = juce::Colour(0xff00f0ff);
                c.textPrimary      = juce::Colour(0xffffffff);
                c.textSecondary    = juce::Colour(0xff8892b0);
                c.ledActive        = juce::Colour(0xff00f0ff);
                c.ledBypassed      = juce::Colour(0x55334455);
                c.wireGlowColor    = juce::Colour(0x5500f0ff);
                c.waveformLcdColor = juce::Colour(0xff00f0ff);
                break;

            case ThemePreset::VintageConsole:
                // Warm Mahogany + Brushed Bronze + Amber Glow + Parchment Cream
                c.backgroundDark   = juce::Colour(0xff161412);
                c.headerDark       = juce::Colour(0xff221f1c);
                c.panelSurface     = juce::Colour(0xff2a2622);
                c.cardSurface      = juce::Colour(0xff36312a);
                c.accentPrimary    = juce::Colour(0xffff9900); // Warm Amber
                c.accentSecondary  = juce::Colour(0xffd4af37); // Brass Gold
                c.borderMuted      = juce::Colour(0x44ff9900);
                c.borderFocused    = juce::Colour(0xffffaa22);
                c.textPrimary      = juce::Colour(0xfffdf6e2); // Parchment cream
                c.textSecondary    = juce::Colour(0xffb8a383);
                c.ledActive        = juce::Colour(0xffff7700); // Amber LED
                c.ledBypassed      = juce::Colour(0x55443322);
                c.wireGlowColor    = juce::Colour(0x55ff9900);
                c.waveformLcdColor = juce::Colour(0xffffaa33);
                break;

            case ThemePreset::CleanStudio:
                // Slate Graphite + Stark White + Studio Ice Blue + Silver
                c.backgroundDark   = juce::Colour(0xff000000);
                c.headerDark       = juce::Colour(0xff12151b);
                c.panelSurface     = juce::Colour(0xff1a1e27);
                c.cardSurface      = juce::Colour(0xff232934);
                c.accentPrimary    = juce::Colour(0xff4a9eff); // Ice Blue
                c.accentSecondary  = juce::Colour(0xffd0d7de); // Studio Silver
                c.borderMuted      = juce::Colour(0x44ffffff);
                c.borderFocused    = juce::Colour(0xffffffff);
                c.textPrimary      = juce::Colour(0xffffffff);
                c.textSecondary    = juce::Colour(0xff8c96a5);
                c.ledActive        = juce::Colour(0xff4a9eff);
                c.ledBypassed      = juce::Colour(0x44334455);
                c.wireGlowColor    = juce::Colour(0x444a9eff);
                c.waveformLcdColor = juce::Colour(0xff60b0ff);
                break;

            case ThemePreset::PhosphorCRT:
                // Monochrome P1 CRT Green + Scanline Dark + Intense Phosphor
                c.backgroundDark   = juce::Colour(0xff040d04);
                c.headerDark       = juce::Colour(0xff081808);
                c.panelSurface     = juce::Colour(0xff0d220d);
                c.cardSurface      = juce::Colour(0xff122e12);
                c.accentPrimary    = juce::Colour(0xff33ff33); // Vivid P1 Green
                c.accentSecondary  = juce::Colour(0xff66ff66);
                c.borderMuted      = juce::Colour(0x5533ff33);
                c.borderFocused    = juce::Colour(0xff33ff33);
                c.textPrimary      = juce::Colour(0xff33ff33);
                c.textSecondary    = juce::Colour(0xff1f9f1f);
                c.ledActive        = juce::Colour(0xff33ff33);
                c.ledBypassed      = juce::Colour(0x33004400);
                c.wireGlowColor    = juce::Colour(0x6633ff33);
                c.waveformLcdColor = juce::Colour(0xff33ff33);
                break;

            case ThemePreset::HighContrastDark:
                // OLED Black + Pure White text (21:1 WCAG AAA) + Electric Yellow and Cyan Wires
                c.backgroundDark   = juce::Colour(0xff000000); // 100% OLED Black
                c.headerDark       = juce::Colour(0xff0a0a0a);
                c.panelSurface     = juce::Colour(0xff121212);
                c.cardSurface      = juce::Colour(0xff1a1a1a);
                c.accentPrimary    = juce::Colour(0xffffff00); // Electric Yellow
                c.accentSecondary  = juce::Colour(0xff00ffff); // Electric Cyan
                c.borderMuted      = juce::Colour(0xffffffff); // Solid 100% White border
                c.borderFocused    = juce::Colour(0xffffff00);
                c.textPrimary      = juce::Colour(0xffffffff); // 100% White (21:1 contrast)
                c.textSecondary    = juce::Colour(0xffe0e0e0);
                c.ledActive        = juce::Colour(0xff00ff00); // Pure Green
                c.ledBypassed      = juce::Colour(0xff555555);
                c.wireGlowColor    = juce::Colour(0x88ffff00);
                c.waveformLcdColor = juce::Colour(0xffffff00);
                break;

            case ThemePreset::HighContrastLight:
                // Stark White + Solid Black text (21:1 WCAG AAA) + Deep Blue and Red
                c.backgroundDark   = juce::Colour(0xffffffff); // Pure White
                c.headerDark       = juce::Colour(0xfff0f0f0);
                c.panelSurface     = juce::Colour(0xffe6e6e6);
                c.cardSurface      = juce::Colour(0xffdcdcdc);
                c.accentPrimary    = juce::Colour(0xff0000ee); // Vivid Deep Blue
                c.accentSecondary  = juce::Colour(0xffcc0000); // Deep Crimson
                c.borderMuted      = juce::Colour(0xff000000); // Solid Black border
                c.borderFocused    = juce::Colour(0xff0000ee);
                c.textPrimary      = juce::Colour(0xff000000); // Pure Black (21:1 contrast)
                c.textSecondary    = juce::Colour(0xff222222);
                c.ledActive        = juce::Colour(0xff008800);
                c.ledBypassed      = juce::Colour(0xff888888);
                c.wireGlowColor    = juce::Colour(0x550000ee);
                c.waveformLcdColor = juce::Colour(0xff0000aa);
                break;
        }
        return c;
    }

    void setTheme(ThemePreset preset) {
        currentPreset_ = preset;
        currentColors_ = getColorsForPreset(preset);
        listeners_.call([this](Listener& l) {
            l.themeChanged(currentColors_, currentPreset_);
        });
    }

    void addListener(Listener* listener) {
        if (listener != nullptr) {
            listeners_.add(listener);
        }
    }

    void removeListener(Listener* listener) {
        if (listener != nullptr) {
            listeners_.remove(listener);
        }
    }

    void applyToLookAndFeel(juce::LookAndFeel_V4& laf) const {
        laf.setColour(juce::ResizableWindow::backgroundColourId, currentColors_.backgroundDark);
        laf.setColour(juce::ComboBox::backgroundColourId, currentColors_.backgroundDark);
        laf.setColour(juce::ComboBox::textColourId, currentColors_.textPrimary);
        laf.setColour(juce::ComboBox::outlineColourId, currentColors_.borderFocused);
        laf.setColour(juce::ComboBox::arrowColourId, currentColors_.accentPrimary);

        laf.setColour(juce::PopupMenu::backgroundColourId, currentColors_.panelSurface);
        laf.setColour(juce::PopupMenu::textColourId, currentColors_.textPrimary);
        laf.setColour(juce::PopupMenu::headerTextColourId, currentColors_.accentPrimary);
        laf.setColour(juce::PopupMenu::highlightedBackgroundColourId, currentColors_.accentPrimary.withAlpha(0.25f));
        laf.setColour(juce::PopupMenu::highlightedTextColourId, currentColors_.textPrimary);

        laf.setColour(juce::TextButton::buttonColourId, currentColors_.cardSurface);
        laf.setColour(juce::TextButton::textColourOffId, currentColors_.textPrimary);
        laf.setColour(juce::TextButton::textColourOnId, currentColors_.textPrimary);

        laf.setColour(juce::Slider::backgroundColourId, currentColors_.backgroundDark);
        laf.setColour(juce::Slider::trackColourId, currentColors_.accentPrimary.withAlpha(0.30f));
        laf.setColour(juce::Slider::thumbColourId, currentColors_.accentPrimary);
        laf.setColour(juce::Slider::textBoxBackgroundColourId, currentColors_.backgroundDark);
        laf.setColour(juce::Slider::textBoxTextColourId, currentColors_.textPrimary);
        laf.setColour(juce::Slider::textBoxOutlineColourId, currentColors_.borderMuted);

        laf.setColour(juce::Label::textColourId, currentColors_.textPrimary);
        laf.setColour(juce::ScrollBar::thumbColourId, currentColors_.accentPrimary.withAlpha(0.4f));
    }

private:
    ThemeManager()
        : currentPreset_(ThemePreset::CleanStudio),
          currentColors_(getColorsForPreset(ThemePreset::CleanStudio)) {}

    ThemePreset currentPreset_{ ThemePreset::CleanStudio };
    ThemeColors currentColors_{};
    juce::ListenerList<Listener> listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeManager)
};

} // namespace audio_graph
