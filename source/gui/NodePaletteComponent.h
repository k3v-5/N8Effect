#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>
#include <memory>
#include "../core/Types.h"
#include "../graph/NodeFactory.h"

namespace audio_graph {

class PaletteItemButton : public juce::TextButton {
public:
    PaletteItemButton(NodeType type, const juce::String& label)
        : juce::TextButton(label), type_(type) {}

    void mouseDown(const juce::MouseEvent& e) override {
        hasDragged_ = false;
        isDragging_ = false;
        juce::TextButton::mouseDown(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (!isDragging_ && e.getDistanceFromDragStart() > 4) {
            isDragging_ = true;
            hasDragged_ = true;
            if (auto* ddc = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
                const juce::String desc = "N8_MODULE_TYPE:" + juce::String(static_cast<int>(type_)) +
                                          "|N8_MODULE_NAME:" + getButtonText();
                ddc->startDragging(desc, this, juce::ScaledImage(), false, nullptr, &(e.source));
            }
        }
        if (!isDragging_) {
            juce::TextButton::mouseDrag(e);
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (hasDragged_ || isDragging_) {
            hasDragged_ = false;
            isDragging_ = false;
            setState(juce::Button::buttonNormal);
            return;
        }
        juce::TextButton::mouseUp(e);
    }

    void clicked() override {
        if (hasDragged_ || isDragging_) return;
        juce::TextButton::clicked();
    }

    void clicked(const juce::ModifierKeys& mods) override {
        if (hasDragged_ || isDragging_) return;
        juce::TextButton::clicked(mods);
    }

private:
    NodeType type_;
    bool isDragging_{ false };
    bool hasDragged_{ false };
};

/**
 * @brief Barra lateral de catálogo de procesadores con soporte Drag & Drop y estética minimalista oscura (Reglas 19, 20, 24).
 */
class NodePaletteComponent : public juce::Component {
public:
    NodePaletteComponent() {
        titleLabel_.setText("DSP MODULES", juce::dontSendNotification);
        titleLabel_.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
        titleLabel_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(titleLabel_);

        contentComponent_ = std::make_unique<juce::Component>();

        struct ModuleButtonInfo {
            NodeType type;
            const char* label;
        };

        const std::vector<ModuleButtonInfo> modules = {
            { NodeType::Compressor, "Compressor" },
            { NodeType::Multiband, "Multiband OTT" },
            { NodeType::ParametricEQ, "Parametric EQ" },
            { NodeType::Filter, "SVF Filter" },
            { NodeType::Distortion, "Distortion" },
            { NodeType::AdvancedDelay, "Stereo Delay" },
            { NodeType::Delay, "Simple Delay" },
            { NodeType::Reverb, "FDN Reverb" },
            { NodeType::PitchShifter, "Pitch Shifter" },
            { NodeType::Spectral, "Spectral Freeze" },
            { NodeType::SpectralProcessor, "Spectral Gate/Tilt" },
            { NodeType::Granular, "Granular Engine" },
            { NodeType::Resonator, "Resonator Bank" },
            { NodeType::Glitch, "Glitch Slicer" },
            { NodeType::Phaser, "Phaser" },
            { NodeType::Chorus, "Chorus" },
            { NodeType::Flanger, "Flanger" },
            { NodeType::RingModulator, "Ring Mod" },
            { NodeType::FrequencyShifter, "Freq Shift" },
            { NodeType::Tape, "Tape Saturation" },
            { NodeType::Container, "Container" },
            { NodeType::Feedback, "Feedback Loop" },
            { NodeType::EventContainer, "Event Rack" },
            { NodeType::MidSideEncoder, "M/S Encoder" },
            { NodeType::MidSideDecoder, "M/S Decoder" },
            { NodeType::SpatialPanner, "3D Panner" },
            { NodeType::TapeStop, "Tape Stop" },
            { NodeType::FormantFilter, "Formant Filter" },
            { NodeType::NoiseTexture, "Noise / Texture" },
            { NodeType::TransientShaper, "Transient Shaper" },
            { NodeType::RotarySpeaker, "Rotary Speaker" },
            { NodeType::HarmonicExciter, "Harmonic Exciter" },
            { NodeType::Vocoder, "Vocoder" },
            { NodeType::KarplusStrong, "Karplus-Strong" },
            { NodeType::ReverseReverb, "Reverse Reverb" },
            { NodeType::BrickwallLimiter, "Brickwall Limiter" },
            { NodeType::Bitcrusher, "Bitcrusher Lo-Fi" },
            { NodeType::NoiseGate, "Noise Gate" },
            { NodeType::DeEsser, "De-Esser" },
            { NodeType::MidiArpeggiator, "Arpeggiator" },
            { NodeType::MidiChordEngine, "Chord Engine" },
            { NodeType::MidiScaleQuantizer, "Scale Quantizer" },
            { NodeType::ExternalSidechain, "Ext Sidechain" },
            { NodeType::Passthrough, "Gain / Utility" }
        };

        for (const auto& mod : modules) {
            auto btn = std::make_unique<PaletteItemButton>(mod.type, mod.label);
            const NodeType t = mod.type;
            btn->onClick = [this, t]() {
                if (onAddNodeRequested_) {
                    onAddNodeRequested_(t);
                }
            };
            contentComponent_->addAndMakeVisible(*btn);
            buttons_.push_back(std::move(btn));
        }

        viewport_.setViewedComponent(contentComponent_.get(), false);
        viewport_.setScrollBarsShown(true, false, false, false);
        addAndMakeVisible(viewport_);
    }

    void setOnAddNodeRequested(std::function<void(NodeType)> cb) {
        onAddNodeRequested_ = std::move(cb);
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        // Fondo negro absoluto
        g.setColour(juce::Colour(0xff000000));
        g.fillRect(bounds);

        // Borde derecho divisor blanco nítido
        g.setColour(juce::Colours::white);
        g.drawVerticalLine(getWidth() - 1, 0.0f, static_cast<float>(getHeight()));
    }

    void resized() override {
        auto area = getLocalBounds();
        titleLabel_.setBounds(area.removeFromTop(24));
        area.removeFromTop(2);

        viewport_.setBounds(area.reduced(4, 2));

        const int btnH = 22;
        const int spacing = 2;
        const int totalH = static_cast<int>(buttons_.size()) * (btnH + spacing) + 4;
        const int contentW = std::max(10, viewport_.getWidth() - 4);
        contentComponent_->setBounds(0, 0, contentW, totalH);

        int y = 2;
        for (auto& btn : buttons_) {
            btn->setBounds(1, y, contentW - 2, btnH);
            y += btnH + spacing;
        }
    }

private:
    juce::Label titleLabel_;
    juce::Viewport viewport_;
    std::unique_ptr<juce::Component> contentComponent_;
    std::vector<std::unique_ptr<PaletteItemButton>> buttons_;
    std::function<void(NodeType)> onAddNodeRequested_;
};

} // namespace audio_graph
