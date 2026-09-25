#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>
#include <memory>
#include <string>
#include "../core/Types.h"
#include "../graph/NodeFactory.h"

namespace audio_graph {

/**
 * @brief Informacion descriptiva de un modulo para la paleta.
 */
struct PaletteModuleItem {
    NodeType type;
    juce::String name;
    juce::String subtitle;
    int categoryIndex; // 1: Dynamics, 2: Filters, 3: Space, 4: Drive, 5: Mod, 6: Spectral, 7: Synth, 8: Routing
    juce::Colour categoryColour;
};

/**
 * @brief Tarjeta interactiva para un modulo DSP en la paleta (Regla 19, 24, 48).
 * Soporta arrastre Drag & Drop y clic directo para agregar al grafo.
 */
class ModulePaletteCard : public juce::Component {
public:
    ModulePaletteCard(const PaletteModuleItem& item, std::function<void(NodeType)> onAdd)
        : item_(item), onAdd_(std::move(onAdd))
    {
        setRepaintsOnMouseActivity(true);
        addBtn_.setButtonText("+");
        addBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161b24));
        addBtn_.setColour(juce::TextButton::textColourOffId, item_.categoryColour);
        addBtn_.onClick = [this]() {
            if (onAdd_) onAdd_(item_.type);
        };
        addAndMakeVisible(addBtn_);
    }

    NodeType getType() const noexcept { return item_.type; }
    const PaletteModuleItem& getItem() const noexcept { return item_; }

    void mouseDown(const juce::MouseEvent& /*e*/) override {
        hasDragged_ = false;
        isDragging_ = false;
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (!isDragging_ && e.getDistanceFromDragStart() > 4) {
            isDragging_ = true;
            hasDragged_ = true;
            if (auto* ddc = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
                const juce::String desc = "N8_MODULE_TYPE:" + juce::String(static_cast<int>(item_.type)) +
                                          "|N8_MODULE_NAME:" + item_.name;
                ddc->startDragging(desc, this, juce::ScaledImage(), false, nullptr, &(e.source));
            }
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (hasDragged_ || isDragging_) {
            hasDragged_ = false;
            isDragging_ = false;
            repaint();
            return; // Invariante Regla 48: supresion de clic tras arrastre
        }
        // Clic directo en la tarjeta agrega el nodo
        if (e.getDistanceFromDragStart() <= 4 && onAdd_) {
            onAdd_(item_.type);
        }
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        const bool hovered = isMouseOver();

        // 1. Chasis de la tarjeta con fondo reactivo
        juce::Colour bgCol = hovered ? juce::Colour(0xff181f2c) : juce::Colour(0xff0d1118);
        g.setColour(bgCol);
        g.fillRoundedRectangle(bounds, 3.5f);

        // 2. Borde sutil
        g.setColour(hovered ? item_.categoryColour.withAlpha(0.6f) : juce::Colour(0xff1c2534));
        g.drawRoundedRectangle(bounds, 3.5f, 1.0f);

        // 3. Tira de acento de categoria en el borde izquierdo
        auto accentStrip = bounds.removeFromLeft(3.5f).reduced(0.0f, 2.0f);
        g.setColour(item_.categoryColour);
        g.fillRoundedRectangle(accentStrip, 1.5f);

        // 4. Textos informativos
        bounds.removeFromLeft(6.0f);
        auto textArea = bounds.removeFromLeft(bounds.getWidth() - 26.0f);

        // Titulo del modulo
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.setColour(hovered ? juce::Colours::white : juce::Colour(0xffe2e8f0));
        g.drawText(item_.name, textArea.removeFromTop(15.0f), juce::Justification::centredLeft, true);

        // Subtitulo algoritmico
        g.setFont(juce::FontOptions(7.5f, juce::Font::plain));
        g.setColour(item_.categoryColour.withAlpha(hovered ? 0.9f : 0.65f));
        g.drawText(item_.subtitle, textArea, juce::Justification::centredLeft, true);
    }

    void resized() override {
        addBtn_.setBounds(getWidth() - 24, (getHeight() - 18) / 2, 18, 18);
    }

private:
    PaletteModuleItem item_;
    std::function<void(NodeType)> onAdd_;
    juce::TextButton addBtn_;
    bool isDragging_{ false };
    bool hasDragged_{ false };
};

/**
 * @brief Encabezado de seccion de categoria para vista agrupada.
 */
class CategoryHeaderComponent : public juce::Component {
public:
    CategoryHeaderComponent(const juce::String& title, juce::Colour col)
        : title_(title), col_(col) {}

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.setColour(col_.withAlpha(0.85f));
        g.drawText(title_.toUpperCase(), bounds.removeFromLeft(120.0f), juce::Justification::centredLeft, true);

        // Linea divisoria horizontal sutil
        g.setColour(juce::Colour(0xff1c2432));
        g.drawLine(bounds.getX() + 6.0f, bounds.getCentreY(), bounds.getRight() - 6.0f, bounds.getCentreY(), 1.0f);
    }

private:
    juce::String title_;
    juce::Colour col_;
};

/**
 * @brief Barra lateral de catalogo de procesadores redisenada con pestanas de categorias,
 * buscador interactivo y tarjetas espaciosas organizadas (Reglas 19, 20, 24, 48).
 */
class NodePaletteComponent : public juce::Component {
public:
    NodePaletteComponent() {
        // 1. Titulo de la barra lateral
        titleLabel_.setText("DSP & FX MODULES", juce::dontSendNotification);
        titleLabel_.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
        titleLabel_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(titleLabel_);

        // 2. Buscador interactivo
        searchBox_.setTextToShowWhenEmpty("Buscar modulo...", juce::Colour(0xff556275));
        searchBox_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff090c12));
        searchBox_.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff1c2434));
        searchBox_.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff00d4ff));
        searchBox_.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        searchBox_.setFont(juce::FontOptions(10.0f, juce::Font::plain));
        searchBox_.onTextChange = [this]() { updateFilter(); };
        addAndMakeVisible(searchBox_);

        // 3. Pestanas / Selector de categorias
        categoryCombo_.addItem("TODAS LAS CATEGORIAS", 1);
        categoryCombo_.addItem("DYNAMICS (6)", 2);
        categoryCombo_.addItem("FILTERS & EQ (3)", 3);
        categoryCombo_.addItem("TIME & SPACE (6)", 4);
        categoryCombo_.addItem("DRIVE & LO-FI (3)", 5);
        categoryCombo_.addItem("MODULATION (7)", 6);
        categoryCombo_.addItem("SPECTRAL & GLITCH (6)", 7);
        categoryCombo_.addItem("SYNTH & MIDI (6)", 8);
        categoryCombo_.addItem("ROUTING & UTIL (8)", 9);
        categoryCombo_.setSelectedId(1, juce::dontSendNotification);
        categoryCombo_.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff0e131c));
        categoryCombo_.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff1e2838));
        categoryCombo_.setColour(juce::ComboBox::textColourId, juce::Colour(0xff00d4ff));
        categoryCombo_.onChange = [this]() { updateFilter(); };
        addAndMakeVisible(categoryCombo_);

        // 4. Inicializar catalogo completo de modulos
        initializeCatalog();

        // 5. Contenedor con scroll para las tarjetas
        contentComponent_ = std::make_unique<juce::Component>();
        viewport_.setViewedComponent(contentComponent_.get(), false);
        viewport_.setScrollBarsShown(true, false, false, false);
        viewport_.setScrollBarThickness(6);
        addAndMakeVisible(viewport_);

        updateFilter();
    }

    void setOnAddNodeRequested(std::function<void(NodeType)> cb) {
        onAddNodeRequested_ = std::move(cb);
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // Fondo oscuro de consola
        g.setColour(juce::Colour(0xff07090e));
        g.fillRect(bounds);

        // Borde derecho divisor
        g.setColour(juce::Colour(0xff1c2432));
        g.drawVerticalLine(getWidth() - 1, 0.0f, static_cast<float>(getHeight()));
    }

    void resized() override {
        auto area = getLocalBounds().reduced(4, 2);

        titleLabel_.setBounds(area.removeFromTop(20));
        area.removeFromTop(2);

        searchBox_.setBounds(area.removeFromTop(22).reduced(2, 0));
        area.removeFromTop(4);

        categoryCombo_.setBounds(area.removeFromTop(22).reduced(2, 0));
        area.removeFromTop(4);

        viewport_.setBounds(area);
        layoutContentCards();
    }

private:
    void initializeCatalog() {
        catalog_ = {
            // DYNAMICS (1)
            { NodeType::Compressor, "Compressor", "VCA Dual RMS", 1, juce::Colour(0xff00ff88) },
            { NodeType::Multiband, "Multiband OTT", "3-Way Down/Upward", 1, juce::Colour(0xff00ff88) },
            { NodeType::BrickwallLimiter, "Brickwall Limiter", "True Peak Ceiling", 1, juce::Colour(0xff00ff88) },
            { NodeType::NoiseGate, "Noise Gate", "Hysteresis Gate", 1, juce::Colour(0xff00ff88) },
            { NodeType::DeEsser, "De-Esser", "Surgical Dynamic Notch", 1, juce::Colour(0xff00ff88) },
            { NodeType::TransientShaper, "Transient Shaper", "Attack & Sustain", 1, juce::Colour(0xff00ff88) },

            // FILTERS & EQ (2)
            { NodeType::Filter, "SVF Filter", "12-24dB Biquad", 2, juce::Colour(0xffff8800) },
            { NodeType::ParametricEQ, "Parametric EQ", "4-Band Parametric", 2, juce::Colour(0xffff8800) },
            { NodeType::FormantFilter, "Formant Filter", "Vowel Vocal Shaper", 2, juce::Colour(0xffff8800) },

            // TIME & SPACE (3)
            { NodeType::AdvancedDelay, "Stereo Delay", "Ping-Pong Tape", 3, juce::Colour(0xff00e5ff) },
            { NodeType::Delay, "Simple Delay", "Digital Feedback", 3, juce::Colour(0xff00e5ff) },
            { NodeType::Reverb, "FDN Reverb", "8-Delay Matrix Space", 3, juce::Colour(0xff00e5ff) },
            { NodeType::ReverseReverb, "Reverse Reverb", "Bloom Granular Reverse", 3, juce::Colour(0xff00e5ff) },
            { NodeType::Tape, "Tape Saturation", "Warm Analog Flutter", 3, juce::Colour(0xff00e5ff) },
            { NodeType::TapeStop, "Tape Stop", "Analog Vinyl Spin", 3, juce::Colour(0xff00e5ff) },

            // DRIVE & LO-FI (4)
            { NodeType::Distortion, "Distortion", "Asymmetric Waveshaper", 4, juce::Colour(0xffff3355) },
            { NodeType::Bitcrusher, "Bitcrusher Lo-Fi", "Sample Rate Decimator", 4, juce::Colour(0xffff3355) },
            { NodeType::HarmonicExciter, "Harmonic Exciter", "Even/Odd Air & Sub", 4, juce::Colour(0xffff3355) },

            // MODULATION (5)
            { NodeType::Phaser, "Phaser", "6-Stage Analog Allpass", 5, juce::Colour(0xffbf55ec) },
            { NodeType::Chorus, "Chorus", "4-Voice Dimension Spread", 5, juce::Colour(0xffbf55ec) },
            { NodeType::Flanger, "Flanger", "Comb Resonance Sweep", 5, juce::Colour(0xffbf55ec) },
            { NodeType::RotarySpeaker, "Rotary Speaker", "Leslie 3D Rotor/Horn", 5, juce::Colour(0xffbf55ec) },
            { NodeType::RingModulator, "Ring Mod", "4-Quadrant Multiplier", 5, juce::Colour(0xffbf55ec) },
            { NodeType::FrequencyShifter, "Freq Shift", "Hilbert Analytic Shifter", 5, juce::Colour(0xffbf55ec) },
            { NodeType::PitchShifter, "Pitch Shifter", "Dual Crossfade Pitch", 5, juce::Colour(0xffbf55ec) },

            // SPECTRAL & GLITCH (6)
            { NodeType::Granular, "Granular Engine", "128-Grain Cloud Texture", 6, juce::Colour(0xffffd700) },
            { NodeType::AudioSlicer, "Audio Slicer", "Buffer Slice & Frozen Grain", 6, juce::Colour(0xffffd700) },
            { NodeType::Spectral, "Spectral Freeze", "FFT Phase Lock", 6, juce::Colour(0xffffd700) },
            { NodeType::SpectralProcessor, "Spectral Gate/Tilt", "FFT Magnitude Shaper", 6, juce::Colour(0xffffd700) },
            { NodeType::Resonator, "Resonator Bank", "Modal Metallic Strings", 6, juce::Colour(0xffffd700) },
            { NodeType::Glitch, "Glitch Slicer", "Stutter & Re-Trigger Buffer", 6, juce::Colour(0xffffd700) },

            // SYNTH & MIDI (7)
            { NodeType::MidiArpeggiator, "Arpeggiator", "Polyphonic Pattern Gen", 7, juce::Colour(0xffa0e000) },
            { NodeType::MidiChordEngine, "Chord Engine", "Multi-Voicing Harmonies", 7, juce::Colour(0xffa0e000) },
            { NodeType::MidiScaleQuantizer, "Scale Quantizer", "Root & Scale Conformer", 7, juce::Colour(0xffa0e000) },
            { NodeType::Vocoder, "Vocoder", "16-Band Spectral Envelope", 7, juce::Colour(0xffa0e000) },
            { NodeType::KarplusStrong, "Karplus-Strong", "Physical Pluck Model", 7, juce::Colour(0xffa0e000) },
            { NodeType::NoiseTexture, "Noise / Texture", "Pink/White/Crackle Gen", 7, juce::Colour(0xffa0e000) },

            // ROUTING & UTILITY (8)
            { NodeType::SpatialPanner, "3D Panner", "Binaural Spatializer", 8, juce::Colour(0xff5dade2) },
            { NodeType::MidSideEncoder, "M/S Encoder", "Stereo to Mid/Side Matrix", 8, juce::Colour(0xff5dade2) },
            { NodeType::MidSideDecoder, "M/S Decoder", "Mid/Side to Stereo Matrix", 8, juce::Colour(0xff5dade2) },
            { NodeType::Container, "Container", "Modular Subgraph Rack", 8, juce::Colour(0xff5dade2) },
            { NodeType::Feedback, "Feedback Loop", "Controlled Feedback Matrix", 8, juce::Colour(0xff5dade2) },
            { NodeType::EventContainer, "Event Rack", "Polyphonic Event Container", 8, juce::Colour(0xff5dade2) },
            { NodeType::ExternalSidechain, "Ext Sidechain", "Aux Key Input Router", 8, juce::Colour(0xff5dade2) },
            { NodeType::Passthrough, "Gain / Utility", "Trim, Phase & Stereo Width", 8, juce::Colour(0xff5dade2) }
        };
    }

    void updateFilter() {
        contentComponent_->removeAllChildren();
        activeCards_.clear();
        categoryHeaders_.clear();

        const juce::String query = searchBox_.getText().trim().toLowerCase();
        const int selectedCat = categoryCombo_.getSelectedId() - 1; // 0: All, 1: Dynamics, etc.

        int currentHeaderCat = -1;

        for (const auto& item : catalog_) {
            // Filtro por categoria
            if (selectedCat > 0 && item.categoryIndex != selectedCat) {
                continue;
            }

            // Filtro por busqueda
            if (query.isNotEmpty()) {
                const bool nameMatch = item.name.toLowerCase().contains(query);
                const bool subMatch = item.subtitle.toLowerCase().contains(query);
                if (!nameMatch && !subMatch) {
                    continue;
                }
            }

            // Encabezado de seccion si estamos en 'TODAS' y no hay busqueda activa
            if (selectedCat == 0 && query.isEmpty() && item.categoryIndex != currentHeaderCat) {
                currentHeaderCat = item.categoryIndex;
                juce::String catName = getCategoryName(currentHeaderCat);
                auto header = std::make_unique<CategoryHeaderComponent>(catName, item.categoryColour);
                contentComponent_->addAndMakeVisible(*header);
                categoryHeaders_.push_back(std::move(header));
            }

            auto card = std::make_unique<ModulePaletteCard>(item, [this](NodeType t) {
                if (onAddNodeRequested_) onAddNodeRequested_(t);
            });
            contentComponent_->addAndMakeVisible(*card);
            activeCards_.push_back(std::move(card));
        }

        layoutContentCards();
    }

    void layoutContentCards() {
        const int cardH = 34;
        const int headerH = 20;
        const int spacing = 4;
        const int contentW = std::max(10, viewport_.getWidth() - 10);

        int y = 2;

        for (int i = 0; i < contentComponent_->getNumChildComponents(); ++i) {
            auto* child = contentComponent_->getChildComponent(i);
            if (auto* header = dynamic_cast<CategoryHeaderComponent*>(child)) {
                header->setBounds(4, y, contentW - 8, headerH);
                y += headerH + 2;
            } else if (auto* card = dynamic_cast<ModulePaletteCard*>(child)) {
                card->setBounds(2, y, contentW - 4, cardH);
                y += cardH + spacing;
            }
        }

        contentComponent_->setBounds(0, 0, contentW, y + 8);
    }

    static juce::String getCategoryName(int catIdx) {
        switch (catIdx) {
            case 1: return "DYNAMICS";
            case 2: return "FILTERS & EQ";
            case 3: return "TIME & SPACE";
            case 4: return "DRIVE & LO-FI";
            case 5: return "MODULATION";
            case 6: return "SPECTRAL & GLITCH";
            case 7: return "SYNTH & MIDI";
            case 8: return "ROUTING & CONTAINERS";
            default: return "OTHER";
        }
    }

    juce::Label titleLabel_;
    juce::TextEditor searchBox_;
    juce::ComboBox categoryCombo_;
    juce::Viewport viewport_;
    std::unique_ptr<juce::Component> contentComponent_;

    std::vector<PaletteModuleItem> catalog_;
    std::vector<std::unique_ptr<ModulePaletteCard>> activeCards_;
    std::vector<std::unique_ptr<CategoryHeaderComponent>> categoryHeaders_;

    std::function<void(NodeType)> onAddNodeRequested_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NodePaletteComponent)
};

} // namespace audio_graph
