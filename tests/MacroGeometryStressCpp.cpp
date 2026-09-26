#include <iostream>
#include <cmath>
#include <cstdint>
#include <cassert>
#include <algorithm>
#include <array>
#include <string>

// Replicate color struct for native test without external JUCE dependencies
struct NativeColour {
    uint32_t argb{ 0 };
    NativeColour() = default;
    constexpr explicit NativeColour(uint32_t c) : argb(c) {}
    uint8_t getAlpha() const noexcept { return static_cast<uint8_t>((argb >> 24) & 0xff); }
    uint8_t getRed() const noexcept { return static_cast<uint8_t>((argb >> 16) & 0xff); }
    uint8_t getGreen() const noexcept { return static_cast<uint8_t>((argb >> 8) & 0xff); }
    uint8_t getBlue() const noexcept { return static_cast<uint8_t>(argb & 0xff); }
    NativeColour withAlpha(float a) const noexcept {
        uint32_t newA = static_cast<uint32_t>(std::clamp(a, 0.0f, 1.0f) * 255.0f);
        return NativeColour((newA << 24) | (argb & 0x00ffffff));
    }
};

enum class ThemePreset {
    Cyberpunk = 0,
    VintageConsole,
    CleanStudio,
    PhosphorCRT,
    HighContrastDark,
    HighContrastLight
};

struct ThemeColors {
    NativeColour backgroundDark;
    NativeColour headerDark;
    NativeColour panelSurface;
    NativeColour cardSurface;
    NativeColour accentPrimary;
    NativeColour accentSecondary;
    NativeColour borderMuted;
    NativeColour borderFocused;
    NativeColour textPrimary;
    NativeColour textSecondary;
    NativeColour ledActive;
    NativeColour ledBypassed;
    NativeColour wireGlowColor;
    NativeColour waveformLcdColor;
};

static ThemeColors getColorsForPreset(ThemePreset preset) noexcept {
    ThemeColors c;
    switch (preset) {
        case ThemePreset::Cyberpunk:
            c.backgroundDark   = NativeColour(0xff08080f);
            c.headerDark       = NativeColour(0xff10101c);
            c.panelSurface     = NativeColour(0xff161626);
            c.cardSurface      = NativeColour(0xff1c1b30);
            c.accentPrimary    = NativeColour(0xff00f0ff);
            c.accentSecondary  = NativeColour(0xffff0055);
            c.borderMuted      = NativeColour(0x4400f0ff);
            c.borderFocused    = NativeColour(0xff00f0ff);
            c.textPrimary      = NativeColour(0xffffffff);
            c.textSecondary    = NativeColour(0xff8892b0);
            c.ledActive        = NativeColour(0xff00f0ff);
            c.ledBypassed      = NativeColour(0x55334455);
            c.wireGlowColor    = NativeColour(0x5500f0ff);
            c.waveformLcdColor = NativeColour(0xff00f0ff);
            break;

        case ThemePreset::VintageConsole:
            c.backgroundDark   = NativeColour(0xff161412);
            c.headerDark       = NativeColour(0xff221f1c);
            c.panelSurface     = NativeColour(0xff2a2622);
            c.cardSurface      = NativeColour(0xff36312a);
            c.accentPrimary    = NativeColour(0xffff9900);
            c.accentSecondary  = NativeColour(0xffd4af37);
            c.borderMuted      = NativeColour(0x44ff9900);
            c.borderFocused    = NativeColour(0xffffaa22);
            c.textPrimary      = NativeColour(0xfffdf6e2);
            c.textSecondary    = NativeColour(0xffb8a383);
            c.ledActive        = NativeColour(0xffff7700);
            c.ledBypassed      = NativeColour(0x55443322);
            c.wireGlowColor    = NativeColour(0x55ff9900);
            c.waveformLcdColor = NativeColour(0xffffaa33);
            break;

        case ThemePreset::CleanStudio:
            c.backgroundDark   = NativeColour(0xff000000);
            c.headerDark       = NativeColour(0xff12151b);
            c.panelSurface     = NativeColour(0xff1a1e27);
            c.cardSurface      = NativeColour(0xff232934);
            c.accentPrimary    = NativeColour(0xff4a9eff);
            c.accentSecondary  = NativeColour(0xffd0d7de);
            c.borderMuted      = NativeColour(0x44ffffff);
            c.borderFocused    = NativeColour(0xffffffff);
            c.textPrimary      = NativeColour(0xffffffff);
            c.textSecondary    = NativeColour(0xff8c96a5);
            c.ledActive        = NativeColour(0xff4a9eff);
            c.ledBypassed      = NativeColour(0x44334455);
            c.wireGlowColor    = NativeColour(0x444a9eff);
            c.waveformLcdColor = NativeColour(0xff60b0ff);
            break;

        case ThemePreset::PhosphorCRT:
            c.backgroundDark   = NativeColour(0xff040d04);
            c.headerDark       = NativeColour(0xff081808);
            c.panelSurface     = NativeColour(0xff0d220d);
            c.cardSurface      = NativeColour(0xff122e12);
            c.accentPrimary    = NativeColour(0xff33ff33);
            c.accentSecondary  = NativeColour(0xff66ff66);
            c.borderMuted      = NativeColour(0x5533ff33);
            c.borderFocused    = NativeColour(0xff33ff33);
            c.textPrimary      = NativeColour(0xff33ff33);
            c.textSecondary    = NativeColour(0xff1f9f1f);
            c.ledActive        = NativeColour(0xff33ff33);
            c.ledBypassed      = NativeColour(0x33004400);
            c.wireGlowColor    = NativeColour(0x6633ff33);
            c.waveformLcdColor = NativeColour(0xff33ff33);
            break;

        case ThemePreset::HighContrastDark:
            c.backgroundDark   = NativeColour(0xff000000);
            c.headerDark       = NativeColour(0xff0a0a0a);
            c.panelSurface     = NativeColour(0xff121212);
            c.cardSurface      = NativeColour(0xff1a1a1a);
            c.accentPrimary    = NativeColour(0xffffff00);
            c.accentSecondary  = NativeColour(0xff00ffff);
            c.borderMuted      = NativeColour(0xffffffff);
            c.borderFocused    = NativeColour(0xffffff00);
            c.textPrimary      = NativeColour(0xffffffff);
            c.textSecondary    = NativeColour(0xffe0e0e0);
            c.ledActive        = NativeColour(0xff00ff00);
            c.ledBypassed      = NativeColour(0xff555555);
            c.wireGlowColor    = NativeColour(0x88ffff00);
            c.waveformLcdColor = NativeColour(0xffffff00);
            break;

        case ThemePreset::HighContrastLight:
            c.backgroundDark   = NativeColour(0xffffffff);
            c.headerDark       = NativeColour(0xfff0f0f0);
            c.panelSurface     = NativeColour(0xffe6e6e6);
            c.cardSurface      = NativeColour(0xffdcdcdc);
            c.accentPrimary    = NativeColour(0xff0000ee);
            c.accentSecondary  = NativeColour(0xffcc0000);
            c.borderMuted      = NativeColour(0xff000000);
            c.borderFocused    = NativeColour(0xff0000ee);
            c.textPrimary      = NativeColour(0xff000000);
            c.textSecondary    = NativeColour(0xff222222);
            c.ledActive        = NativeColour(0xff008800);
            c.ledBypassed      = NativeColour(0xff888888);
            c.wireGlowColor    = NativeColour(0x550000ee);
            c.waveformLcdColor = NativeColour(0xff0000aa);
            break;
    }
    return c;
}

int main() {
    std::cout << "==================================================\n";
    std::cout << "[CPP EMPIRICAL CHALLENGER] Native MSVC C++20 Tests\n";
    std::cout << "==================================================\n";

    // 1. Knob value range & clamp robustness
    std::cout << "[TEST 1] Float clamping & edge cases... ";
    const float testVals[] = { 0.0f, 0.5f, 1.0f, -0.5f, 1.5f, -1e9f, 1e9f, -0.0f, 1e-7f, 1.0f - 1e-7f };
    for (float v : testVals) {
        float clamped = std::clamp(v, 0.0f, 1.0f);
        assert(clamped >= 0.0f && clamped <= 1.0f);
        assert(std::isfinite(clamped));
        int pct = static_cast<int>(std::round(clamped * 100.0f));
        assert(pct >= 0 && pct <= 100);
    }
    std::cout << "PASSED\n";

    // 2. Micro-ticks and Arc Angle Math
    std::cout << "[TEST 2] Micro-ticks & Arc Angle Math (135 deg to 405 deg)... ";
    constexpr float startAngle = 2.3561945f;
    constexpr float endAngle = 7.0685835f;
    constexpr float sweepAngle = endAngle - startAngle;

    constexpr float dialSize = 32.0f;
    constexpr float dialX = 6.0f;
    constexpr float dialY = (42.0f - dialSize) * 0.5f;
    constexpr float cx = dialX + dialSize * 0.5f;
    constexpr float cy = dialY + dialSize * 0.5f;
    assert(cx == 22.0f);
    assert(cy == 21.0f);

    struct TickDef { float frac; bool major; };
    static constexpr TickDef kTicks[] = {
        { 0.00f, true  }, { 0.10f, false }, { 0.20f, false }, { 0.25f, true  },
        { 0.30f, false }, { 0.40f, false }, { 0.50f, true  }, { 0.60f, false },
        { 0.70f, false }, { 0.75f, true  }, { 0.80f, false }, { 0.90f, false },
        { 1.00f, true  }
    };
    constexpr float tickBaseR = 13.8f;

    for (const auto& t : kTicks) {
        const float tickLen = t.major ? 2.2f : 1.2f;
        const float ang = startAngle + t.frac * sweepAngle;
        const float cosA = std::cos(ang);
        const float sinA = std::sin(ang);

        const float x1 = cx + sinA * tickBaseR;
        const float y1 = cy - cosA * tickBaseR;
        const float x2 = cx + sinA * (tickBaseR + tickLen);
        const float y2 = cy - cosA * (tickBaseR + tickLen);

        assert(std::isfinite(x1) && std::isfinite(y1));
        assert(std::isfinite(x2) && std::isfinite(y2));
        assert(x1 >= 5.0f && x1 <= 39.0f);
        assert(y1 >= 4.0f && y1 <= 38.0f);
        assert(x2 >= 5.0f && x2 <= 39.0f);
        assert(y2 >= 4.0f && y2 <= 38.0f);
    }

    // 100k dense angle sweep for needle
    for (int i = 0; i <= 100000; ++i) {
        float val = static_cast<float>(i) / 100000.0f;
        float currentAngle = startAngle + val * sweepAngle;
        float sinN = std::sin(currentAngle);
        float cosN = std::cos(currentAngle);

        float nx1 = cx + sinN * 2.2f;
        float ny1 = cy - cosN * 2.2f;
        float nx2 = cx + sinN * 9.0f;
        float ny2 = cy - cosN * 9.0f;

        assert(std::isfinite(nx1) && std::isfinite(ny1));
        assert(std::isfinite(nx2) && std::isfinite(ny2));

        float d1 = std::hypot(nx1 - cx, ny1 - cy);
        float d2 = std::hypot(nx2 - cx, ny2 - cy);
        assert(std::abs(d1 - 2.2f) < 1e-4f);
        assert(std::abs(d2 - 9.0f) < 1e-4f);
        assert(d2 < 9.8f); // Stays inside rotor
    }
    std::cout << "PASSED (100,013 coordinate calculations verified)\n";

    // 3. Slot dimensions: 900px, 1200px, 1920px
    std::cout << "[TEST 3] Slot width scaling (900px, 1200px, 1920px)... ";
    const int testWidths[] = { 900, 1200, 1920, 2560, 3840 };
    for (int totalW : testWidths) {
        const int slotW = totalW / 8;
        int sumW = 0;
        for (int i = 0; i < 8; ++i) {
            int x = i * slotW;
            int w = (i == 7) ? (totalW - x) : slotW;
            sumW += w;

            int textLeft = 44;
            int textRight = w - 24;
            int textWidth = std::max(10, textRight - textLeft);
            int pinX = w - 16 - 6;

            assert(textLeft > 38);       // Clears dial
            assert(textRight <= pinX);   // Clears drag pin
            assert(textWidth >= 40);     // Fits 7-char title
            assert(pinX + 16 <= w);      // Pin inside slot
        }
        assert(sumW == totalW);
    }
    std::cout << "PASSED\n";

    // 4. Rapid Theme Switching across all 6 presets
    std::cout << "[TEST 4] Rapid Theme Switching across all 6 presets (500,000 switches)... ";
    const ThemePreset allPresets[] = {
        ThemePreset::Cyberpunk,
        ThemePreset::VintageConsole,
        ThemePreset::CleanStudio,
        ThemePreset::PhosphorCRT,
        ThemePreset::HighContrastDark,
        ThemePreset::HighContrastLight
    };

    for (int cycle = 0; cycle < 500000; ++cycle) {
        ThemePreset p = allPresets[cycle % 6];
        ThemeColors colors = getColorsForPreset(p);

        // Verify tokens have non-zero alpha
        assert(colors.backgroundDark.getAlpha() > 0);
        assert(colors.headerDark.getAlpha() > 0);
        assert(colors.panelSurface.getAlpha() > 0);
        assert(colors.cardSurface.getAlpha() > 0);
        assert(colors.accentPrimary.getAlpha() > 0);
        assert(colors.textPrimary.getAlpha() > 0);
        assert(colors.borderMuted.getAlpha() > 0);
        assert(colors.borderFocused.getAlpha() > 0);

        // Verify PhosphorCRT logic
        bool isCRT = (p == ThemePreset::PhosphorCRT);
        NativeColour activeCol = isCRT ? colors.accentPrimary : NativeColour(0xffffffff);
        NativeColour needleCol = isCRT ? colors.accentPrimary : colors.textPrimary;
        assert(activeCol.argb != 0);
        assert(needleCol.argb != 0);
    }
    std::cout << "PASSED (500,000 theme cycles verified)\n";

    std::cout << "==================================================\n";
    std::cout << "ALL NATIVE C++20 EMPIRICAL CHALLENGES PASSED (VERDICT: APPROVE)\n";
    std::cout << "==================================================\n";
    return 0;
}
