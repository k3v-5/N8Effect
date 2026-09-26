#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <span>
#include "../core/Types.h"
#include "../dsp/processors/GranularNode.h"

namespace audio_graph {

/**
 * @brief Editor Visual Granular Interactivo con Scrubbing y Nube de Granos (Reglas 23, 24, 25, 48, 49).
 * Permite visualizar la cinta de audio grabada en tiempo real, interactuar mediante scrubbing horizontal
 * para fijar el foco de emisión de granos, congelar el buffer de audio (Freeze) y renderizar
 * las partículas de la nube de granos con paneo estéreo y pitch vertical.
 */
class GranularEditorComponent : public juce::Component, public juce::Timer {
public:
    explicit GranularEditorComponent(GranularNode* granularNode = nullptr)
        : node_(granularNode)
    {
        setOpaque(true);

        freezeBtn_.setButtonText("FREEZE");
        freezeBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181818));
        freezeBtn_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00d4ff));
        freezeBtn_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        freezeBtn_.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        freezeBtn_.setClickingTogglesState(true);
        freezeBtn_.onClick = [this]() {
            if (node_) {
                node_->setParameter(GranularNode::Freeze, freezeBtn_.getToggleState() ? 1.0f : 0.0f);
            }
        };
        addAndMakeVisible(freezeBtn_);

        auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text, float min, float max, float def) {
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
            s.setRange(min, max);
            s.setValue(def);
            s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::white.withAlpha(0.85f));
            s.setColour(juce::Slider::thumbColourId, juce::Colours::white);
            addAndMakeVisible(s);

            l.setText(text, juce::dontSendNotification);
            l.setFont(juce::Font(9.0f, juce::Font::bold));
            l.setJustificationType(juce::Justification::centred);
            l.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));
            addAndMakeVisible(l);
        };

        setupSlider(sizeSlider_, sizeLabel_, "SIZE MS", 10.0f, 500.0f, 80.0f);
        setupSlider(densitySlider_, densityLabel_, "DENSITY", 1.0f, 60.0f, 15.0f);
        setupSlider(posSpraySlider_, posSprayLabel_, "SPRAY MS", 0.0f, 200.0f, 20.0f);
        setupSlider(pitchSpraySlider_, pitchSprayLabel_, "PITCH SPRAY", 0.0f, 12.0f, 0.0f);
        setupSlider(mixSlider_, mixLabel_, "MIX", 0.0f, 1.0f, 0.5f);

        sizeSlider_.onValueChange = [this]() { if (node_) node_->setParameter(GranularNode::GrainSizeMs, static_cast<float>(sizeSlider_.getValue())); };
        densitySlider_.onValueChange = [this]() { if (node_) node_->setParameter(GranularNode::Density, static_cast<float>(densitySlider_.getValue())); };
        posSpraySlider_.onValueChange = [this]() { if (node_) node_->setParameter(GranularNode::PositionSprayMs, static_cast<float>(posSpraySlider_.getValue())); };
        pitchSpraySlider_.onValueChange = [this]() { if (node_) node_->setParameter(GranularNode::PitchSpray, static_cast<float>(pitchSpraySlider_.getValue())); };
        mixSlider_.onValueChange = [this]() { if (node_) node_->setParameter(GranularNode::DryWet, static_cast<float>(mixSlider_.getValue())); };

        waveformOverview_.fill(0.0f);
        startTimerHz(30); // 30 FPS para telemetría visual fluida
    }

    ~GranularEditorComponent() override {
        stopTimer();
    }

    void setGranularNode(GranularNode* node) {
        node_ = node;
        if (node_) {
            sizeSlider_.setValue(node_->getParameter(GranularNode::GrainSizeMs), juce::dontSendNotification);
            densitySlider_.setValue(node_->getParameter(GranularNode::Density), juce::dontSendNotification);
            posSpraySlider_.setValue(node_->getParameter(GranularNode::PositionSprayMs), juce::dontSendNotification);
            pitchSpraySlider_.setValue(node_->getParameter(GranularNode::PitchSpray), juce::dontSendNotification);
            mixSlider_.setValue(node_->getParameter(GranularNode::DryWet), juce::dontSendNotification);
            freezeBtn_.setToggleState(node_->getParameter(GranularNode::Freeze) > 0.5f, juce::dontSendNotification);
        }
    }

    void timerCallback() override {
        if (node_) {
            node_->copyWaveformOverview(waveformOverview_.data(), WaveformPoints);
            std::span<GranularNode::GrainCloudPoint> spanPts(activeGrains_.data(), activeGrains_.size());
            activeGrainCount_ = node_->copyActiveGrainsSnapshot(spanPts);
            writeHeadNorm_ = node_->getWritePositionNormalized();
        }
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override {
        hasDragged_ = false;
        if (tapeBounds_.contains(e.position)) {
            updateScrubFromMouse(e.position.x);
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (!hasDragged_ && e.getDistanceFromDragStart() > 4) {
            hasDragged_ = true;
        }
        if (tapeBounds_.contains(e.position)) {
            updateScrubFromMouse(e.position.x);
        }
    }

    void mouseUp(const juce::MouseEvent& /*e*/) override {
        hasDragged_ = false;
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // Fondo principal
        g.setColour(juce::Colour(0xff0a0d14));
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

        // Header
        auto header = bounds.removeFromTop(28.0f).reduced(8.0f, 4.0f);
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.drawText("GRANULAR CLOUD SYNTHESIZER & TAPE SCRUBBER", header, juce::Justification::centredLeft);

        // Cinta de audio / Nube de granos
        tapeBounds_ = bounds.removeFromTop(bounds.getHeight() - 70.0f).reduced(8.0f, 4.0f);
        drawTape(g, tapeBounds_);
    }

    void resized() override {
        auto bounds = getLocalBounds();
        auto header = bounds.removeFromTop(28);
        freezeBtn_.setBounds(header.removeFromRight(74).reduced(4, 2));

        auto controlsArea = bounds.removeFromBottom(68).reduced(6, 4);
        const int sliderW = controlsArea.getWidth() / 5;

        auto placeSlider = [&controlsArea, sliderW](juce::Slider& s, juce::Label& l) {
            auto col = controlsArea.removeFromLeft(sliderW);
            l.setBounds(col.removeFromTop(14));
            s.setBounds(col);
        };

        placeSlider(sizeSlider_, sizeLabel_);
        placeSlider(densitySlider_, densityLabel_);
        placeSlider(posSpraySlider_, posSprayLabel_);
        placeSlider(pitchSpraySlider_, pitchSprayLabel_);
        placeSlider(mixSlider_, mixLabel_);
    }

private:
    void updateScrubFromMouse(float mouseX) {
        if (tapeBounds_.getWidth() <= 0.0f) return;
        scrubPosNorm_ = std::clamp((mouseX - tapeBounds_.getX()) / tapeBounds_.getWidth(), 0.0f, 1.0f);
        if (node_) {
            node_->setParameter(GranularNode::ScrubPosition, scrubPosNorm_);
        }
        repaint();
    }

    void drawTape(juce::Graphics& g, juce::Rectangle<float> bounds) {
        // Marco de la cinta de audio
        g.setColour(juce::Colour(0xff05070a));
        g.fillRoundedRectangle(bounds, 3.0f);

        g.setColour(juce::Colour(0xff182230));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

        const float midY = bounds.getCentreY();
        const float halfH = bounds.getHeight() * 0.45f;
        const float w = bounds.getWidth();
        const float x = bounds.getX();

        // Línea central
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawHorizontalLine(static_cast<int>(midY), x, x + w);

        // 1. Dibujar forma de onda del buffer
        juce::Path wavePath;
        const float ptW = w / static_cast<float>(WaveformPoints);

        for (size_t p = 0; p < WaveformPoints; ++p) {
            float minVal = waveformOverview_[p * 2];
            float maxVal = waveformOverview_[p * 2 + 1];
            float px = x + p * ptW;
            float topY = midY - maxVal * halfH;
            float btmY = midY - minVal * halfH;

            g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.35f));
            g.drawVerticalLine(static_cast<int>(px), topY, btmY);
        }

        // 2. Dibujar zona de spray alrededor de scrubPos
        if (scrubPosNorm_ > 0.0001f) {
            const float sprayMs = (node_ != nullptr) ? node_->getParameter(GranularNode::PositionSprayMs) : 20.0f;
            const float sprayNorm = sprayMs / 2000.0f; // Asumiendo buffer de 2s
            const float sprayW = sprayNorm * w;
            const float scrubX = x + scrubPosNorm_ * w;

            g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.12f));
            g.fillRect(scrubX - sprayW, bounds.getY(), sprayW * 2.0f, bounds.getHeight());

            // Línea de Scrubbing Playhead
            g.setColour(juce::Colour(0xff00d4ff));
            g.drawVerticalLine(static_cast<int>(scrubX), bounds.getY(), bounds.getBottom());
        }

        // 3. Dibujar cabezal de escritura en vivo
        const float writeX = x + writeHeadNorm_ * w;
        g.setColour(juce::Colour(0xffff3366).withAlpha(0.75f));
        g.drawVerticalLine(static_cast<int>(writeX), bounds.getY(), bounds.getBottom());

        // 4. Renderizar Nube de Granos Activos (Grain Cloud Particles)
        for (size_t i = 0; i < activeGrainCount_; ++i) {
            const auto& pt = activeGrains_[i];
            if (!pt.active || pt.envelope <= 0.001f) continue;

            const float gx = x + pt.normPosition * w;
            // Desplazamiento vertical por pitch (-1 octava a +1 octava)
            const float pitchOct = std::log2(std::clamp(pt.pitchRatio, 0.25f, 4.0f));
            const float gy = midY - pitchOct * halfH * 0.7f;

            // Color según paneo: Izquierda (Cian), Centro (Esmeralda), Derecha (Ámbar)
            juce::Colour grainCol;
            if (pt.pan < 0.5f) {
                float t = pt.pan / 0.5f;
                grainCol = juce::Colour(0xff00d4ff).interpolatedWith(juce::Colour(0xff00ff88), t);
            } else {
                float t = (pt.pan - 0.5f) / 0.5f;
                grainCol = juce::Colour(0xff00ff88).interpolatedWith(juce::Colour(0xffff9900), t);
            }

            const float rad = 2.5f + pt.envelope * 3.5f;
            g.setColour(grainCol.withAlpha(pt.envelope * 0.85f));
            g.fillEllipse(gx - rad, gy - rad, rad * 2.0f, rad * 2.0f);
            g.setColour(juce::Colours::white.withAlpha(pt.envelope * 0.9f));
            g.fillEllipse(gx - rad * 0.4f, gy - rad * 0.4f, rad * 0.8f, rad * 0.8f);
        }

        // HUD overlay
        g.setFont(juce::Font(8.5f, juce::Font::bold));
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        juce::String status = "ACTIVE GRAINS: " + juce::String(static_cast<int>(activeGrainCount_)) + " | CLICK & DRAG TO SCRUB";
        g.drawText(status, bounds.reduced(6.0f).removeFromBottom(14.0f), juce::Justification::bottomRight, false);
    }

    GranularNode* node_{ nullptr };
    static constexpr size_t WaveformPoints = 256;
    std::array<float, WaveformPoints * 2> waveformOverview_{};
    std::array<GranularNode::GrainCloudPoint, 128> activeGrains_{};
    size_t activeGrainCount_{ 0 };

    float scrubPosNorm_{ 0.0f };
    float writeHeadNorm_{ 0.0f };
    bool hasDragged_{ false };
    juce::Rectangle<float> tapeBounds_{};

    juce::TextButton freezeBtn_;
    juce::Slider sizeSlider_;
    juce::Label sizeLabel_;
    juce::Slider densitySlider_;
    juce::Label densityLabel_;
    juce::Slider posSpraySlider_;
    juce::Label posSprayLabel_;
    juce::Slider pitchSpraySlider_;
    juce::Label pitchSprayLabel_;
    juce::Slider mixSlider_;
    juce::Label mixLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularEditorComponent)
};

} // namespace audio_graph
