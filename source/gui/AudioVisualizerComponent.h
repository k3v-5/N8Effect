#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <numbers>
#include "../core/Types.h"
#include "../core/RealtimePools.h"
#include "../event/EventTypes.h"
#include "../event/EventTelemetryBuffer.h"

namespace audio_graph {

class N8AudioProcessor;

/**
 * @brief Visualizador de Audio y Espectrograma en Tiempo Real (Reglas 23, 24, 25, 26).
 * Modos: Analizador de Espectro FFT, Osciloscopio con Zero-Crossing, Goniometro Lissajous, Split y Radar 3D de Eventos Acústicos.
 * Soporta Master Output y Node Probing interactivo.
 */
class AudioVisualizerComponent : public juce::Component {
public:
    enum class DisplayMode : uint8_t {
        Spectrum = 0,
        Oscilloscope = 1,
        Goniometer = 2,
        Split = 3,
        Radar = 4,
        Waterfall = 5
    };

    enum class SourceMode : uint8_t {
        Master = 0,
        NodeProbe = 1
    };

    struct VisualEventParticle {
        float pan{ 0.0f };
        float pitchRatio{ 1.0f };
        float energy{ 0.0f };
        float distance{ 1.0f };
        float azimuth{ 0.0f };
        float radius{ 4.0f };
        float alpha{ 0.0f };
        EventType type{ EventType::Fragment };
        uint8_t generation{ 0 };
    };

    explicit AudioVisualizerComponent(N8AudioProcessor& processor)
        : processor_(processor)
    {
        setOpaque(true);

        auto configureButton = [](juce::TextButton& btn, const juce::String& text) {
            btn.setButtonText(text);
            btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff121212));
            btn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::white);
            btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
            btn.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
            btn.setClickingTogglesState(false);
        };

        configureButton(specBtn_, "SPECTRUM");
        configureButton(scopeBtn_, "SCOPE");
        configureButton(gonioBtn_, "LISSAJOUS");
        configureButton(splitBtn_, "SPLIT");
        configureButton(radarBtn_, "RADAR 3D");
        configureButton(waterfallBtn_, "WATERFALL 3D");
        configureButton(sourceToggleBtn_, "SRC: MASTER");

        specBtn_.onClick = [this]() { setMode(DisplayMode::Spectrum); };
        scopeBtn_.onClick = [this]() { setMode(DisplayMode::Oscilloscope); };
        gonioBtn_.onClick = [this]() { setMode(DisplayMode::Goniometer); };
        splitBtn_.onClick = [this]() { setMode(DisplayMode::Split); };
        radarBtn_.onClick = [this]() { setMode(DisplayMode::Radar); };
        waterfallBtn_.onClick = [this]() { setMode(DisplayMode::Waterfall); };

        sourceToggleBtn_.onClick = [this]() {
            if (sourceMode_ == SourceMode::Master) {
                sourceMode_ = SourceMode::NodeProbe;
                sourceToggleBtn_.setButtonText("SRC: PROBE");
            } else {
                sourceMode_ = SourceMode::Master;
                sourceToggleBtn_.setButtonText("SRC: MASTER");
            }
            repaint();
        };

        addAndMakeVisible(specBtn_);
        addAndMakeVisible(scopeBtn_);
        addAndMakeVisible(gonioBtn_);
        addAndMakeVisible(splitBtn_);
        addAndMakeVisible(radarBtn_);
        addAndMakeVisible(waterfallBtn_);
        addAndMakeVisible(sourceToggleBtn_);

        updateButtonStates();

        displayBufferL_.fill(0.0f);
        displayBufferR_.fill(0.0f);
        fftData_.fill(0.0f);
        smoothedSpectrum_.fill(0.0f);
        for (auto& s : waterfallSlices_) {
            s.magnitudes.fill(0.0f);
            s.peak = 0.0f;
        }
    }

    void setMode(DisplayMode mode) {
        mode_ = mode;
        updateButtonStates();
        repaint();
    }

    void setSourceMode(SourceMode src) {
        sourceMode_ = src;
        sourceToggleBtn_.setButtonText(src == SourceMode::Master ? "SRC: MASTER" : "SRC: PROBE");
        repaint();
    }

    void updateTelemetry();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void updateButtonStates() {
        specBtn_.setColour(juce::TextButton::buttonColourId, mode_ == DisplayMode::Spectrum ? juce::Colours::white : juce::Colour(0xff141414));
        specBtn_.setColour(juce::TextButton::textColourOffId, mode_ == DisplayMode::Spectrum ? juce::Colours::black : juce::Colours::white);

        scopeBtn_.setColour(juce::TextButton::buttonColourId, mode_ == DisplayMode::Oscilloscope ? juce::Colours::white : juce::Colour(0xff141414));
        scopeBtn_.setColour(juce::TextButton::textColourOffId, mode_ == DisplayMode::Oscilloscope ? juce::Colours::black : juce::Colours::white);

        gonioBtn_.setColour(juce::TextButton::buttonColourId, mode_ == DisplayMode::Goniometer ? juce::Colours::white : juce::Colour(0xff141414));
        gonioBtn_.setColour(juce::TextButton::textColourOffId, mode_ == DisplayMode::Goniometer ? juce::Colours::black : juce::Colours::white);

        splitBtn_.setColour(juce::TextButton::buttonColourId, mode_ == DisplayMode::Split ? juce::Colours::white : juce::Colour(0xff141414));
        splitBtn_.setColour(juce::TextButton::textColourOffId, mode_ == DisplayMode::Split ? juce::Colours::black : juce::Colours::white);

        radarBtn_.setColour(juce::TextButton::buttonColourId, mode_ == DisplayMode::Radar ? juce::Colours::white : juce::Colour(0xff141414));
        radarBtn_.setColour(juce::TextButton::textColourOffId, mode_ == DisplayMode::Radar ? juce::Colours::black : juce::Colours::white);

        waterfallBtn_.setColour(juce::TextButton::buttonColourId, mode_ == DisplayMode::Waterfall ? juce::Colours::white : juce::Colour(0xff141414));
        waterfallBtn_.setColour(juce::TextButton::textColourOffId, mode_ == DisplayMode::Waterfall ? juce::Colours::black : juce::Colours::white);
    }

    void drawSpectrum(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawOscilloscope(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawGoniometer(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawRadar(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawWaterfall(juce::Graphics& g, juce::Rectangle<float> bounds);

    N8AudioProcessor& processor_;
    DisplayMode mode_{ DisplayMode::Spectrum };
    SourceMode sourceMode_{ SourceMode::Master };

    juce::TextButton specBtn_;
    juce::TextButton scopeBtn_;
    juce::TextButton gonioBtn_;
    juce::TextButton splitBtn_;
    juce::TextButton radarBtn_;
    juce::TextButton waterfallBtn_;
    juce::TextButton sourceToggleBtn_;

    static constexpr size_t MaxRadarParticles = 64;
    std::array<VisualEventParticle, MaxRadarParticles> radarParticles_{};
    float radarAngle_{ 0.0f };

    static constexpr size_t FftOrder = 10; // 1024 puntos
    static constexpr size_t FftSize = 1 << FftOrder;
    static constexpr size_t ScopeSize = 1024;

    static constexpr size_t WaterfallSlices = 36;
    static constexpr size_t WaterfallBins = 128;
    struct WaterfallSlice {
        std::array<float, WaterfallBins> magnitudes{};
        float peak{ 0.0f };
    };
    std::array<WaterfallSlice, WaterfallSlices> waterfallSlices_{};
    float waterfallPhase_{ 0.0f };

    std::array<float, ScopeSize> displayBufferL_{};
    std::array<float, ScopeSize> displayBufferR_{};
    std::array<float, FftSize * 2> fftData_{};
    std::array<float, FftSize / 2> smoothedSpectrum_{};

    float phaseCorrelation_{ 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioVisualizerComponent)
};

} // namespace audio_graph
