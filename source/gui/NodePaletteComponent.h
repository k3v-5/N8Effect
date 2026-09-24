#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>
#include "../core/Types.h"
#include "../graph/NodeFactory.h"

namespace audio_graph {

/**
 * @brief Barra lateral o menú de catálogo de procesadores disponibles (Reglas 19, 20, 24).
 */
class NodePaletteComponent : public juce::Component {
public:
    NodePaletteComponent() {
        titleLabel_.setText("DSP MODULES", juce::dontSendNotification);
        titleLabel_.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        titleLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff00d2ff));
        titleLabel_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(titleLabel_);

        // Registrar botones para los módulos principales
        struct ModuleButtonInfo {
            NodeType type;
            const char* label;
            juce::Colour colour;
        };

        const std::vector<ModuleButtonInfo> modules = {
            { NodeType::Compressor, "Compressor", juce::Colour(0xffe67e22) },
            { NodeType::Multiband, "Multiband OTT", juce::Colour(0xffe67e22) },
            { NodeType::Filter, "Parametric EQ", juce::Colour(0xfff39c12) },
            { NodeType::Distortion, "Distortion", juce::Colour(0xffe74c3c) },
            { NodeType::Delay, "Stereo Delay", juce::Colour(0xff00d2ff) },
            { NodeType::Reverb, "FDN Reverb", juce::Colour(0xff9b59b6) },
            { NodeType::PitchShifter, "Pitch Shifter", juce::Colour(0xff6c5ce7) },
            { NodeType::Spectral, "Spectral Freeze", juce::Colour(0xff1abc9c) },
            { NodeType::Granular, "Granular Engine", juce::Colour(0xff2ecc71) },
            { NodeType::Resonator, "Resonator Bank", juce::Colour(0xff6c5ce7) },
            { NodeType::Glitch, "Glitch Slicer", juce::Colour(0xffe84393) },
            { NodeType::Phaser, "Phaser", juce::Colour(0xff9b59b6) },
            { NodeType::Chorus, "Chorus", juce::Colour(0xff00d2ff) },
            { NodeType::Flanger, "Flanger", juce::Colour(0xff0984e3) },
            { NodeType::RingModulator, "Ring Mod", juce::Colour(0xffd63031) },
            { NodeType::FrequencyShifter, "Freq Shift", juce::Colour(0xff6c5ce7) },
            { NodeType::Tape, "Tape Saturation", juce::Colour(0xffe17055) },
            { NodeType::Container, "Container", juce::Colour(0xfffdcb6e) },
            { NodeType::Feedback, "Feedback Loop", juce::Colour(0xfffab1a0) },
            { NodeType::EventContainer, "Event Rack", juce::Colour(0xff00cec9) },
            { NodeType::MidSideEncoder, "M/S Encoder", juce::Colour(0xffa29bfe) },
            { NodeType::MidSideDecoder, "M/S Decoder", juce::Colour(0xff6c5ce7) },
            { NodeType::SpatialPanner, "3D Panner", juce::Colour(0xff55efc4) }
        };

        for (const auto& mod : modules) {
            auto btn = std::make_unique<juce::TextButton>(mod.label);
            btn->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181824));
            btn->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            const NodeType t = mod.type;
            btn->onClick = [this, t]() {
                if (onAddNodeRequested_) {
                    onAddNodeRequested_(t);
                }
            };
            addAndMakeVisible(*btn);
            buttons_.push_back(std::move(btn));
        }
    }

    void setOnAddNodeRequested(std::function<void(NodeType)> cb) {
        onAddNodeRequested_ = std::move(cb);
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xff0e0e16));
        g.fillRect(bounds);

        // Borde derecho divisor
        g.setColour(juce::Colour(0xff1f1f2e));
        g.drawVerticalLine(getWidth() - 1, 0.0f, static_cast<float>(getHeight()));
    }

    void resized() override {
        auto area = getLocalBounds().reduced(6, 4);
        titleLabel_.setBounds(area.removeFromTop(20));
        area.removeFromTop(4);

        for (auto& btn : buttons_) {
            btn->setBounds(area.removeFromTop(22));
            area.removeFromTop(2);
        }
    }

private:
    juce::Label titleLabel_;
    std::vector<std::unique_ptr<juce::TextButton>> buttons_;
    std::function<void(NodeType)> onAddNodeRequested_;
};

} // namespace audio_graph
