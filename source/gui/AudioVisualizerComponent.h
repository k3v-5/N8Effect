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
#include "../dsp/processors/GranularNode.h"
#include "ThemeManager.h"

namespace audio_graph {

class N8AudioProcessor;

/**
 * @brief Visualizador de Audio y Espectrograma en Tiempo Real (Reglas 23, 24, 25, 26).
 * Modos: Analizador de Espectro FFT, Osciloscopio con Zero-Crossing, Goniometro Lissajous, Split, Radar 3D y Nube Granular.
 * Soporta Master Output y Node Probing interactivo.
 */
class AudioVisualizerComponent : public juce::Component, public ThemeManager::Listener {
public:
    enum class DisplayMode : uint8_t {
        Spectrum = 0,
        Oscilloscope = 1,
        Goniometer = 2,
        Split = 3,
        Radar = 4,
        Waterfall = 5,
        GrainCloud = 6
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
        ThemeManager::getInstance().addListener(this);

        auto configureButton = [](juce::TextButton& btn, const juce::String& text) {
            btn.setButtonText(text);
            btn.setClickingTogglesState(false);
        };

        configureButton(specBtn_, "SPECTRUM");
        configureButton(scopeBtn_, "SCOPE");
        configureButton(gonioBtn_, "LISSAJOUS");
        configureButton(splitBtn_, "SPLIT");
        configureButton(radarBtn_, "RADAR 3D");
        configureButton(waterfallBtn_, "WATERFALL 3D");
        configureButton(cloudBtn_, "GRAIN CLOUD");
        configureButton(sourceToggleBtn_, "SRC: MASTER");

        specBtn_.onClick = [this]() { setMode(DisplayMode::Spectrum); };
        scopeBtn_.onClick = [this]() { setMode(DisplayMode::Oscilloscope); };
        gonioBtn_.onClick = [this]() { setMode(DisplayMode::Goniometer); };
        splitBtn_.onClick = [this]() { setMode(DisplayMode::Split); };
        radarBtn_.onClick = [this]() { setMode(DisplayMode::Radar); };
        waterfallBtn_.onClick = [this]() { setMode(DisplayMode::Waterfall); };
        cloudBtn_.onClick = [this]() { setMode(DisplayMode::GrainCloud); };

        sourceToggleBtn_.onClick = [this]() {
            if (sourceMode_ == SourceMode::Master) {
                sourceMode_ = SourceMode::NodeProbe;
            } else {
                sourceMode_ = SourceMode::Master;
            }
            updateButtonStates();
            repaint();
        };

        addAndMakeVisible(specBtn_);
        addAndMakeVisible(scopeBtn_);
        addAndMakeVisible(gonioBtn_);
        addAndMakeVisible(splitBtn_);
        addAndMakeVisible(radarBtn_);
        addAndMakeVisible(waterfallBtn_);
        addAndMakeVisible(cloudBtn_);
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

    ~AudioVisualizerComponent() override {
        ThemeManager::getInstance().removeListener(this);
    }

    void themeChanged(const ThemeColors& /*newTheme*/, ThemePreset /*preset*/) override {
        updateButtonStates();
        repaint();
    }

    void setMode(DisplayMode mode) {
        mode_ = mode;
        updateButtonStates();
        repaint();
    }

    void setSourceMode(SourceMode src) {
        sourceMode_ = src;
        updateButtonStates();
        repaint();
    }

    void updateTelemetry();

    std::function<void(float azimuth, float distance)> onRadarSpatialNodeMoved;
    float getInteractiveAzimuth() const noexcept { return interactiveAzimuth_; }
    float getInteractiveDistance() const noexcept { return interactiveDistance_; }
    void setInteractiveSpatialCoordinates(float azimuth, float distance) noexcept {
        interactiveAzimuth_ = std::clamp(azimuth, -1.0f, 1.0f);
        interactiveDistance_ = std::clamp(distance, 0.1f, 10.0f);
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void updateButtonStates() {
        const auto& theme = ThemeManager::getInstance().getColors();
        auto setBtnStyle = [&](juce::TextButton& btn, bool active) {
            if (active) {
                btn.setColour(juce::TextButton::buttonColourId, theme.accentPrimary);
                btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff000000));
                btn.setColour(juce::TextButton::buttonOnColourId, theme.accentPrimary.brighter(0.2f));
                btn.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff000000));
            } else {
                btn.setColour(juce::TextButton::buttonColourId, theme.cardSurface);
                btn.setColour(juce::TextButton::textColourOffId, theme.textSecondary);
                btn.setColour(juce::TextButton::buttonOnColourId, theme.cardSurface);
                btn.setColour(juce::TextButton::textColourOnId, theme.textPrimary);
            }
        };

        setBtnStyle(specBtn_, mode_ == DisplayMode::Spectrum);
        setBtnStyle(scopeBtn_, mode_ == DisplayMode::Oscilloscope);
        setBtnStyle(gonioBtn_, mode_ == DisplayMode::Goniometer);
        setBtnStyle(splitBtn_, mode_ == DisplayMode::Split);
        setBtnStyle(radarBtn_, mode_ == DisplayMode::Radar);
        setBtnStyle(waterfallBtn_, mode_ == DisplayMode::Waterfall);
        setBtnStyle(cloudBtn_, mode_ == DisplayMode::GrainCloud);

        if (sourceMode_ == SourceMode::Master) {
            sourceToggleBtn_.setButtonText("SRC: MASTER");
            sourceToggleBtn_.setColour(juce::TextButton::buttonColourId, theme.cardSurface);
            sourceToggleBtn_.setColour(juce::TextButton::textColourOffId, theme.textPrimary);
        } else {
            sourceToggleBtn_.setButtonText("SRC: PROBE");
            sourceToggleBtn_.setColour(juce::TextButton::buttonColourId, theme.accentSecondary);
            sourceToggleBtn_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        }
    }

    void drawSpectrum(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawOscilloscope(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawGoniometer(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawRadar(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawWaterfall(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawGrainCloud(juce::Graphics& g, juce::Rectangle<float> bounds);

    N8AudioProcessor& processor_;
    DisplayMode mode_{ DisplayMode::Spectrum };
    SourceMode sourceMode_{ SourceMode::Master };

    juce::TextButton specBtn_;
    juce::TextButton scopeBtn_;
    juce::TextButton gonioBtn_;
    juce::TextButton splitBtn_;
    juce::TextButton radarBtn_;
    juce::TextButton waterfallBtn_;
    juce::TextButton cloudBtn_;
    juce::TextButton sourceToggleBtn_;

    static constexpr size_t MaxGrainCloudParticles = 128;
    std::array<GranularNode::GrainCloudPoint, MaxGrainCloudParticles> grainCloudParticles_{};
    size_t grainCloudCount_{ 0 };
    float grainCloudDecay_{ 1.0f };
    float grainScanAngle_{ 0.0f };

    static constexpr size_t MaxRadarParticles = 64;
    std::array<VisualEventParticle, MaxRadarParticles> radarParticles_{};
    float radarAngle_{ 0.0f };

    bool isDraggingRadar_{ false };
    bool hasDragged_{ false };
    float interactiveAzimuth_{ 0.0f };
    float interactiveDistance_{ 2.5f };

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
    uint64_t lastSamplesWritten_{ 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioVisualizerComponent)
};

} // namespace audio_graph
