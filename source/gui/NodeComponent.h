#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>
#include <functional>
#include <cmath>
#include <algorithm>
#include <numbers>
#include "../core/Types.h"
#include "../graph/AudioProcessorNode.h"
#include "../dsp/processors/ConvolutionNode.h"
#include "../dsp/processors/SamplePlayerNode.h"
#include "ModulationSlider.h"
#include "WireRenderer.h"

namespace audio_graph {

/**
 * @brief Componente visual interactivo para un nodo en el canvas del grafo (Reglas 4, 8, 23, 24).
 * Estilo de consola de hardware modular de lujo inspirado en Arturia (Pigments/EFX) y FL Studio FLEX:
 * - Chasis de titanio cepillado con bisel superior 3D y tira de luz LED de categoría.
 * - Mini pantalla gráfica LCD interactiva (curvas de filtro biquad, ecos ping-pong, envolventes de reverb, saturación).
 * - Interruptor táctil de encendido/bypass con LED indicador luminoso.
 * - Conectores de audio estilo jack metálico mecanizado con núcleo cromático según tipo de señal.
 */
class NodeComponent : public juce::Component {
public:
    static juce::Colour getCategoryColor(NodeType type) {
        switch (type) {
            case NodeType::Filter:
            case NodeType::ParametricEQ:
            case NodeType::FormantFilter:
                return juce::Colour(0xff00d4ff); // Cyan / Blue for filters & EQ
            case NodeType::Delay:
            case NodeType::AdvancedDelay:
            case NodeType::Tape:
            case NodeType::TapeStop:
                return juce::Colour(0xff00f0aa); // Mint / Green for Delays & Time
            case NodeType::Reverb:
            case NodeType::ReverseReverb:
            case NodeType::Convolution:
            case NodeType::ShimmerReverb:
                return juce::Colour(0xffa855f7); // Purple for Reverbs & IR
            case NodeType::Compressor:
            case NodeType::Multiband:
            case NodeType::BrickwallLimiter:
            case NodeType::NoiseGate:
            case NodeType::DeEsser:
            case NodeType::TransientShaper:
                return juce::Colour(0xffff9900); // Amber / Orange for Dynamics
            case NodeType::Distortion:
            case NodeType::Bitcrusher:
            case NodeType::HarmonicExciter:
                return juce::Colour(0xffff3366); // Coral / Red for Distortion & Saturators
            case NodeType::Granular:
            case NodeType::Spectral:
            case NodeType::SpectralProcessor:
            case NodeType::Resonator:
            case NodeType::Glitch:
            case NodeType::SpectralSmear:
                return juce::Colour(0xffeab308); // Gold / Yellow for Granular & Spectral
            case NodeType::Phaser:
            case NodeType::Chorus:
            case NodeType::Flanger:
            case NodeType::RotarySpeaker:
            case NodeType::RingModulator:
            case NodeType::FrequencyShifter:
            case NodeType::PitchShifter:
            case NodeType::Refraction:
                return juce::Colour(0xff38bdf8); // Sky blue / Violet for Modulation
            case NodeType::SpatialPanner:
            case NodeType::MidSideEncoder:
            case NodeType::MidSideDecoder:
                return juce::Colour(0xff10b981); // Emerald for Spatial & Stereo
            case NodeType::Container:
            case NodeType::Feedback:
            case NodeType::EventContainer:
                return juce::Colour(0xffec4899); // Pink / Magenta for Containers
            case NodeType::Oversampler:
                return juce::Colour(0xff818cf8); // Indigo for Oversampling HQ
            case NodeType::SamplePlayer:
                return juce::Colour(0xffa0e000); // Lime green for Synth & Sampler
            default:
                return juce::Colour(0xff94a3b8); // Slate silver
        }
    }

    static juce::String getCategorySubtitle(NodeType type) {
        switch (type) {
            case NodeType::SamplePlayer: return "SAMPLER // WAV PLAYER";
            case NodeType::Filter: return "FILTER // BIQUAD";
            case NodeType::ParametricEQ: return "EQUALIZER // 4-BAND";
            case NodeType::FormantFilter: return "FORMANT // VOWEL";
            case NodeType::Delay: return "DELAY // DIGITAL";
            case NodeType::AdvancedDelay: return "DELAY // PING-PONG";
            case NodeType::Tape: return "TAPE ECHO // WARM";
            case NodeType::TapeStop: return "TAPE STOP // VINYL";
            case NodeType::Reverb: return "REVERB // FDN SPACE";
            case NodeType::ReverseReverb: return "REVERB // BLOOM";
            case NodeType::ShimmerReverb: return "REVERB // SHIMMER FDN";
            case NodeType::Convolution: return "CONVOLUTION // ZERO-LATENCY";
            case NodeType::Compressor: return "DYNAMICS // VCA COMP";
            case NodeType::Multiband: return "DYNAMICS // 3-WAY OTT";
            case NodeType::BrickwallLimiter: return "LIMITER // TRUE PEAK";
            case NodeType::NoiseGate: return "GATE // HYSTERESIS";
            case NodeType::DeEsser: return "DE-ESSER // SURGICAL";
            case NodeType::TransientShaper: return "DYNAMICS // TRANSIENT";
            case NodeType::Distortion: return "DRIVE // WAVESHAPER";
            case NodeType::Bitcrusher: return "LO-FI // DECIMATOR";
            case NodeType::HarmonicExciter: return "EXCITER // AIR & SUB";
            case NodeType::Granular: return "GRANULAR // 128 GRAINS";
            case NodeType::Spectral: return "SPECTRAL // FFT PROCESSOR";
            case NodeType::SpectralProcessor: return "SPECTRAL // SHAPER";
            case NodeType::SpectralSmear: return "SPECTRAL // LIQUID DIFFUSION";
            case NodeType::Resonator: return "MODAL // RESONATOR";
            case NodeType::Glitch: return "GLITCH // RE-TRIGGER";
            case NodeType::AudioSlicer: return "SLICER // FROZEN GRAIN";
            case NodeType::Phaser: return "PHASER // 6-STAGE";
            case NodeType::Chorus: return "CHORUS // 4-VOICE";
            case NodeType::Flanger: return "FLANGER // COMB";
            case NodeType::Refraction: return "REFRACTION // 8-VOICE PRISM";
            case NodeType::RotarySpeaker: return "ROTARY // LESLIE 3D";
            case NodeType::RingModulator: return "RING MOD // 4-QUADRANT";
            case NodeType::FrequencyShifter: return "FREQ SHIFT // HILBERT";
            case NodeType::PitchShifter: return "PITCH // DUAL CROSSFADE";
            case NodeType::SpatialPanner: return "3D PANNER // BINAURAL";
            case NodeType::MidSideEncoder: return "M/S ENCODER // MATRIX";
            case NodeType::MidSideDecoder: return "M/S DECODER // BASS MAKER";
            case NodeType::Oversampler: return "OVERSAMPLING // HQ PDC";
            case NodeType::Container: return "CONTAINER // SUBGRAPH";
            case NodeType::Feedback: return "CONTAINER // FEEDBACK";
            case NodeType::EventContainer: return "CONTAINER // EVENT RACK";
            default: return "DSP PROCESSOR";
        }
    }

    static void drawMiniVisualCurve(juce::Graphics& g, juce::Rectangle<float> bounds, NodeType type, juce::Colour accentColour, bool isBypassed = false, const AudioProcessorNode* processor = nullptr) {
        // Marco de la pantalla LCD oscura
        g.setColour(juce::Colour(0xff06070a));
        g.fillRoundedRectangle(bounds, 3.0f);
        g.setColour(juce::Colour(0xff18202a));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

        // Cuadrícula sutil de fondo
        g.setColour(juce::Colour(0xff10151e).withAlpha(0.5f));
        g.drawHorizontalLine(static_cast<int>(bounds.getCentreY()), bounds.getX(), bounds.getRight());
        g.drawVerticalLine(static_cast<int>(bounds.getCentreX()), bounds.getY(), bounds.getBottom());

        const float w = bounds.getWidth();
        const float h = bounds.getHeight();
        const float x = bounds.getX();
        const float y = bounds.getY();
        const float midY = bounds.getCentreY();

        juce::Path curvePath;

        // Renderizado contextual según el tipo de algoritmo DSP
        switch (type) {
            case NodeType::Filter:
            case NodeType::ParametricEQ:
            case NodeType::FormantFilter: {
                // Curva de Respuesta en Frecuencia Biquad (Pasa bajos con resonancia)
                curvePath.startNewSubPath(x + 2.0f, y + 6.0f);
                curvePath.lineTo(x + w * 0.45f, y + 6.0f);
                curvePath.quadraticTo(x + w * 0.60f, y + 2.0f, x + w * 0.68f, y + 10.0f);
                curvePath.quadraticTo(x + w * 0.82f, y + h - 6.0f, x + w - 2.0f, y + h - 3.0f);
                break;
            }
            case NodeType::Delay:
            case NodeType::AdvancedDelay:
            case NodeType::Tape:
            case NodeType::TapeStop: {
                // Ecos rítmicos en ping-pong con decaimiento exponencial
                for (int t = 0; t < 5; ++t) {
                    float tapX = x + 6.0f + static_cast<float>(t) * (w - 12.0f) / 4.0f;
                    float tapH = (h - 8.0f) * std::exp(-static_cast<float>(t) * 0.55f);
                    g.setColour(accentColour.withAlpha(0.8f - static_cast<float>(t) * 0.14f));
                    g.drawLine(tapX, y + h - 4.0f, tapX, y + h - 4.0f - tapH, 1.8f);
                }
                return;
            }
            case NodeType::Reverb:
            case NodeType::ReverseReverb:
            case NodeType::ShimmerReverb: {
                // Envolvente de difusión y cola espacial
                curvePath.startNewSubPath(x + 2.0f, y + h - 3.0f);
                curvePath.quadraticTo(x + 8.0f, y + 4.0f, x + w * 0.3f, y + 10.0f);
                curvePath.quadraticTo(x + w * 0.7f, y + h * 0.6f, x + w - 2.0f, y + h - 3.0f);
                break;
            }
            case NodeType::Convolution: {
                // Miniatura de forma de onda de respuesta al impulso (IR) de 64 puntos con resplandor de decaimiento
                if (const auto* conv = dynamic_cast<const ConvolutionNode*>(processor)) {
                    const auto& thumb = conv->getIRThumbnail();
                    const float maxAmpH = (h - 8.0f) * 0.44f;
                    const float stepX = (w - 8.0f) / 63.0f;

                    // 1. Resplandor difuso de decaimiento (Relleno de envolvente espejada con gradiente)
                    juce::Path glowPath;
                    glowPath.startNewSubPath(x + 4.0f, midY);
                    for (size_t i = 0; i < 64; ++i) {
                        float px = x + 4.0f + static_cast<float>(i) * stepX;
                        float py = midY - std::clamp(thumb[i], 0.0f, 1.0f) * maxAmpH;
                        glowPath.lineTo(px, py);
                    }
                    for (int i = 63; i >= 0; --i) {
                        float px = x + 4.0f + static_cast<float>(i) * stepX;
                        float py = midY + std::clamp(thumb[static_cast<size_t>(i)], 0.0f, 1.0f) * maxAmpH;
                        glowPath.lineTo(px, py);
                    }
                    glowPath.closeSubPath();

                    juce::ColourGradient glowGrad(accentColour.withAlpha(isBypassed ? 0.05f : 0.28f), x + 4.0f, midY,
                                                  accentColour.withAlpha(isBypassed ? 0.01f : 0.04f), x + w - 4.0f, midY, false);
                    g.setGradientFill(glowGrad);
                    g.fillPath(glowPath);

                    // 2. Trazo de contorno superior e inferior de la forma de onda
                    juce::Path waveOutline;
                    waveOutline.startNewSubPath(x + 4.0f, midY);
                    for (size_t i = 0; i < 64; ++i) {
                        float px = x + 4.0f + static_cast<float>(i) * stepX;
                        float py = midY - std::clamp(thumb[i], 0.0f, 1.0f) * maxAmpH;
                        waveOutline.lineTo(px, py);
                    }
                    waveOutline.startNewSubPath(x + 4.0f, midY);
                    for (size_t i = 0; i < 64; ++i) {
                        float px = x + 4.0f + static_cast<float>(i) * stepX;
                        float py = midY + std::clamp(thumb[i], 0.0f, 1.0f) * maxAmpH;
                        waveOutline.lineTo(px, py);
                    }
                    g.setColour(accentColour.withAlpha(isBypassed ? 0.3f : 0.88f));
                    g.strokePath(waveOutline, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                    return;
                }

                // Curva de respaldo si no hay instancia concreta de ConvolutionNode
                curvePath.startNewSubPath(x + 2.0f, y + h - 3.0f);
                curvePath.quadraticTo(x + 8.0f, y + 4.0f, x + w * 0.3f, y + 10.0f);
                curvePath.quadraticTo(x + w * 0.7f, y + h * 0.6f, x + w - 2.0f, y + h - 3.0f);
                break;
            }
            case NodeType::SamplePlayer: {
                if (processor != nullptr) {
                    auto* sp = dynamic_cast<const SamplePlayerNode*>(processor);
                    if (sp != nullptr) {
                        const auto& thumb = sp->getThumbnail();
                        const float stepX = (w - 8.0f) / 63.0f;
                        const float maxAmpH = (h * 0.44f);

                        // 1. Resplandor de fondo de la forma de onda
                        juce::Path glowPath;
                        glowPath.startNewSubPath(x + 4.0f, midY);
                        for (size_t i = 0; i < 64; ++i) {
                            float px = x + 4.0f + static_cast<float>(i) * stepX;
                            float py = midY - std::clamp(thumb[i], 0.0f, 1.0f) * maxAmpH;
                            glowPath.lineTo(px, py);
                        }
                        for (int i = 63; i >= 0; --i) {
                            float px = x + 4.0f + static_cast<float>(i) * stepX;
                            float py = midY + std::clamp(thumb[static_cast<size_t>(i)], 0.0f, 1.0f) * maxAmpH;
                            glowPath.lineTo(px, py);
                        }
                        glowPath.closeSubPath();

                        juce::ColourGradient glowGrad(accentColour.withAlpha(isBypassed ? 0.05f : 0.28f), x + 4.0f, midY,
                                                      accentColour.withAlpha(isBypassed ? 0.01f : 0.04f), x + w - 4.0f, midY, false);
                        g.setGradientFill(glowGrad);
                        g.fillPath(glowPath);

                        // 2. Trazo de contorno superior e inferior de la forma de onda
                        juce::Path waveOutline;
                        waveOutline.startNewSubPath(x + 4.0f, midY);
                        for (size_t i = 0; i < 64; ++i) {
                            float px = x + 4.0f + static_cast<float>(i) * stepX;
                            float py = midY - std::clamp(thumb[i], 0.0f, 1.0f) * maxAmpH;
                            waveOutline.lineTo(px, py);
                        }
                        waveOutline.startNewSubPath(x + 4.0f, midY);
                        for (size_t i = 0; i < 64; ++i) {
                            float px = x + 4.0f + static_cast<float>(i) * stepX;
                            float py = midY + std::clamp(thumb[i], 0.0f, 1.0f) * maxAmpH;
                            waveOutline.lineTo(px, py);
                        }
                        g.setColour(accentColour.withAlpha(isBypassed ? 0.3f : 0.88f));
                        g.strokePath(waveOutline, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                        return;
                    }
                }
                curvePath.startNewSubPath(x + 4.0f, midY);
                curvePath.quadraticTo(x + w * 0.25f, y + 6.0f, x + w * 0.5f, midY);
                curvePath.quadraticTo(x + w * 0.75f, y + h - 6.0f, x + w - 4.0f, midY);
                break;
            }
            case NodeType::Compressor:
            case NodeType::Multiband:
            case NodeType::BrickwallLimiter:
            case NodeType::NoiseGate:
            case NodeType::DeEsser:
            case NodeType::TransientShaper: {
                // Curva de Transferencia Dinámica (45 grados con codo de compresión)
                curvePath.startNewSubPath(x + 4.0f, y + h - 4.0f);
                curvePath.lineTo(x + w * 0.50f, y + h * 0.50f);
                curvePath.lineTo(x + w - 4.0f, y + h * 0.32f);
                break;
            }
            case NodeType::Distortion:
            case NodeType::Bitcrusher:
            case NodeType::HarmonicExciter: {
                // Curva de Saturación Waveshaping (S-Curve Tanh)
                curvePath.startNewSubPath(x + 4.0f, y + h - 6.0f);
                curvePath.quadraticTo(x + w * 0.35f, y + h - 6.0f, x + w * 0.5f, midY);
                curvePath.quadraticTo(x + w * 0.65f, y + 6.0f, x + w - 4.0f, y + 6.0f);
                break;
            }
            case NodeType::Granular:
            case NodeType::Spectral:
            case NodeType::SpectralProcessor:
            case NodeType::Resonator:
            case NodeType::Glitch:
            case NodeType::SpectralSmear: {
                // Nube de micro-partículas o barras espectrales
                for (int b = 0; b < 7; ++b) {
                    float bx = x + 6.0f + static_cast<float>(b) * (w - 12.0f) / 6.0f;
                    float val = std::sin(static_cast<float>(b) * 1.1f) * 0.5f + 0.5f;
                    float barH = (h - 8.0f) * val;
                    g.setColour(accentColour.withAlpha(0.75f));
                    g.fillRoundedRectangle(bx - 1.5f, y + h - 4.0f - barH, 3.0f, barH, 1.0f);
                }
                return;
            }
            case NodeType::AudioSlicer: {
                // Cuadrícula de rebanadas con forma de onda rítmica
                const int numSlices = 8;
                for (int s = 1; s < numSlices; ++s) {
                    float sx = x + static_cast<float>(s) * w / static_cast<float>(numSlices);
                    g.setColour(accentColour.withAlpha(0.25f));
                    g.drawVerticalLine(static_cast<int>(sx), y + 2.0f, y + h - 2.0f);
                }
                curvePath.startNewSubPath(x + 2.0f, midY);
                for (int b = 0; b < numSlices; ++b) {
                    float bx = x + static_cast<float>(b) * w / static_cast<float>(numSlices);
                    float sw = w / static_cast<float>(numSlices);
                    float amp = (b % 2 == 0) ? (h * 0.38f) : (h * 0.22f);
                    curvePath.lineTo(bx + 2.0f, midY - amp);
                    curvePath.lineTo(bx + sw * 0.5f, midY + amp * 0.6f);
                    curvePath.lineTo(bx + sw, midY);
                }
                break;
            }
            case NodeType::Phaser:
            case NodeType::Chorus:
            case NodeType::Flanger:
            case NodeType::RotarySpeaker:
            case NodeType::RingModulator:
            case NodeType::FrequencyShifter:
            case NodeType::PitchShifter:
            case NodeType::Refraction: {
                // Modulación cíclica y dispersión estéreo
                curvePath.startNewSubPath(x + 4.0f, midY);
                for (float sx = 4.0f; sx < w - 4.0f; sx += 4.0f) {
                    float phase = (sx / w) * 4.0f * std::numbers::pi_v<float>;
                    float sy = midY - std::sin(phase) * (h * 0.35f);
                    curvePath.lineTo(x + sx, sy);
                }
                break;
            }
            default: {
                // Onda senoidal de referencia
                curvePath.startNewSubPath(x + 4.0f, midY);
                curvePath.quadraticTo(x + w * 0.25f, y + 6.0f, x + w * 0.5f, midY);
                curvePath.quadraticTo(x + w * 0.75f, y + h - 6.0f, x + w - 4.0f, midY);
                break;
            }
        }

        // Trazado de la curva luminosa
        g.setColour(accentColour.withAlpha(isBypassed ? 0.3f : 0.85f));
        g.strokePath(curvePath, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    NodeComponent(NodeId id, const juce::String& name, NodeType type, AudioProcessorNode* processor)
        : id_(id), name_(name), type_(type), processor_(processor)
    {
        setRepaintsOnMouseActivity(true);

        if (processor_ != nullptr) {
            const auto params = processor_->getParameters();
            const size_t numSliders = params.size();
            for (size_t i = 0; i < numSliders; ++i) {
                const auto& p = params[i];
                auto slider = std::make_unique<ModulationSlider>(p.name, p.minValue, p.maxValue, p.defaultValue);
                const ParameterId pid = p.id;
                slider->setBaseValue(processor_->getParameter(pid));
                slider->setOnValueChanged([this, pid](float val) {
                    if (processor_ != nullptr) {
                        processor_->setParameter(pid, val);
                    }
                    if (onParameterChanged_) {
                        onParameterChanged_(id_, pid, val);
                    }
                });
                slider->setOnModulationDropped([this, pid](ModSourceType src, float depth, juce::Colour col) {
                    juce::ignoreUnused(col);
                    if (onModulationRouteAdded_) {
                        onModulationRouteAdded_(id_, pid, src, depth);
                    }
                });
                slider->setOnModulationDepthChanged([this, pid](float depth) {
                    if (onModulationDepthChanged_) {
                        onModulationDepthChanged_(id_, pid, depth);
                    }
                });
                slider->setOnModulationRemoved([this, pid]() {
                    if (onModulationRouteRemoved_) {
                        onModulationRouteRemoved_(id_, pid);
                    }
                });
                addAndMakeVisible(*slider);
                sliders_.push_back(std::move(slider));
                sliderParamIds_.push_back(pid);
            }
        }

        updateDimensions();
    }

    NodeId getNodeId() const noexcept { return id_; }
    NodeType getNodeType() const noexcept { return type_; }
    const juce::String& getNodeName() const noexcept { return name_; }
    bool isBypassed() const noexcept { return isBypassed_; }

    void setBypassed(bool bypassed) {
        if (isBypassed_ != bypassed) {
            isBypassed_ = bypassed;
            repaint();
        }
    }

    void setSelected(bool sel) {
        isSelected_ = sel;
        repaint();
    }

    bool isSelected() const noexcept { return isSelected_; }

    void setOnNodeSelected(std::function<void(NodeId)> cb) { onNodeSelected_ = std::move(cb); }
    void setOnNodeMoved(std::function<void(NodeId, float, float)> cb) { onNodeMoved_ = std::move(cb); }
    void setOnNodeDeleted(std::function<void(NodeId)> cb) { onNodeDeleted_ = std::move(cb); }
    void setOnPinDragStarted(std::function<void(NodeId, PinId, PinDataType, juce::Point<float>)> cb) { onPinDragStarted_ = std::move(cb); }
    void setOnPinDragging(std::function<void(juce::Point<float>)> cb) { onPinDragging_ = std::move(cb); }
    void setOnPinDragEnded(std::function<void(NodeId, PinId, PinType, PinDataType, juce::Point<float>)> cb) { onPinDragEnded_ = std::move(cb); }
    void setOnPinRightClicked(std::function<void(NodeId, PinId)> cb) { onPinRightClicked_ = std::move(cb); }
    void setOnNodeDraggingOverCanvas(std::function<void(NodeId, juce::Rectangle<int>)> cb) { onNodeDraggingOverCanvas_ = std::move(cb); }
    void setOnNodeDropped(std::function<void(NodeId, juce::Rectangle<int>)> cb) { onNodeDropped_ = std::move(cb); }
    void setOnBypassToggled(std::function<void(NodeId, bool)> cb) { onBypassToggled_ = std::move(cb); }
    void setOnParameterChanged(std::function<void(NodeId, ParameterId, float)> cb) { onParameterChanged_ = std::move(cb); }
    void setOnModulationRouteAdded(std::function<void(NodeId, ParameterId, ModSourceType, float)> cb) { onModulationRouteAdded_ = std::move(cb); }
    void setOnModulationDepthChanged(std::function<void(NodeId, ParameterId, float)> cb) { onModulationDepthChanged_ = std::move(cb); }
    void setOnModulationRouteRemoved(std::function<void(NodeId, ParameterId)> cb) { onModulationRouteRemoved_ = std::move(cb); }

    ModulationSlider* getSliderForParameter(ParameterId pid) const noexcept {
        for (size_t i = 0; i < sliderParamIds_.size(); ++i) {
            if (sliderParamIds_[i] == pid && i < sliders_.size()) {
                return sliders_[i].get();
            }
        }
        return nullptr;
    }

    void setDropCandidate(bool cand) {
        if (isDropCandidate_ != cand) {
            isDropCandidate_ = cand;
            repaint();
        }
    }

    void setHighlightedPin(PinId pinId) {
        if (highlightedPinId_ != pinId) {
            highlightedPinId_ = pinId;
            repaint();
        }
    }

    bool hitTestPin(juce::Point<float> canvasPos, PinId& outPinId, PinType& outPinType, PinDataType& outDataType, juce::Point<float>& outCenter, float tolerance = 16.0f) const {
        if (processor_ == nullptr) return false;
        const auto localPos = canvasPos - getPosition().toFloat();
        const auto pins = processor_->getPins();
        for (size_t i = 0; i < pins.size(); ++i) {
            auto pinRect = getPinLocalRect(i);
            if (pinRect.expanded(tolerance).contains(localPos)) {
                outPinId = pins[i].id;
                outPinType = pins[i].type;
                outDataType = pins[i].dataType;
                outCenter = getPosition().toFloat() + pinRect.getCentre();
                return true;
            }
        }
        return false;
    }

    juce::Point<float> getPinCenterInCanvas(PinId pinId) const {
        if (processor_ == nullptr) return getBounds().getCentre().toFloat();

        const auto pins = processor_->getPins();
        for (size_t i = 0; i < pins.size(); ++i) {
            if (pins[i].id == pinId) {
                const auto r = getPinLocalRect(i);
                const auto localPt = r.getCentre();
                return getPosition().toFloat() + localPt;
            }
        }
        return getBounds().getCentre().toFloat();
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        const auto catColour = getCategoryColor(type_);

        // 1. Chasis de titanio cepillado con degradado neo-analógico Arturia/FLEX
        juce::ColourGradient bgGradient(juce::Colour(0xff141720), bounds.getTopLeft(),
                                        juce::Colour(0xff090c10), bounds.getBottomLeft(), false);
        g.setGradientFill(bgGradient);
        g.fillRoundedRectangle(bounds, 5.0f);

        // Bisel superior tridimensional (luz de borde sutil)
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawHorizontalLine(1, bounds.getX() + 5.0f, bounds.getRight() - 5.0f);

        // 2. Barra de Luz LED de Categoría en el borde superior (Glow strip)
        auto topLed = bounds.removeFromTop(3.0f).reduced(2.0f, 0.0f);
        g.setColour(catColour.withAlpha(isBypassed_ ? 0.3f : 0.95f));
        g.fillRoundedRectangle(topLed, 1.5f);

        // Resplandor difuso bajo el LED
        g.setColour(catColour.withAlpha(isBypassed_ ? 0.08f : 0.25f));
        g.fillRect(bounds.getX() + 2.0f, 3.0f, bounds.getWidth() - 4.0f, 4.0f);

        // 3. Cabecera del Nodo
        auto headerRect = bounds.removeFromTop(27.0f);

        // Interruptor Power / Bypass con LED integrado
        auto pwrRect = headerRect.removeFromLeft(24.0f).withSizeKeepingCentre(14.0f, 14.0f);
        g.setColour(juce::Colour(0xff06070a));
        g.fillEllipse(pwrRect);
        g.setColour(juce::Colour(0xff2a3240));
        g.drawEllipse(pwrRect, 1.0f);

        const auto ledCol = isBypassed_ ? juce::Colour(0xff552028) : juce::Colour(0xff00ff88);
        g.setColour(ledCol);
        g.fillEllipse(pwrRect.reduced(3.5f));
        if (!isBypassed_) {
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.fillEllipse(pwrRect.reduced(5.0f));
        }

        // Título del Nodo y Subtítulo de categoría
        auto titleArea = headerRect.removeFromLeft(headerRect.getWidth() - 22.0f);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.setColour(isBypassed_ ? juce::Colours::white.withAlpha(0.4f) : juce::Colours::white);
        g.drawText(name_.toUpperCase(), titleArea.removeFromTop(14.0f), juce::Justification::centredLeft, true);

        g.setFont(juce::FontOptions(7.5f, juce::Font::plain));
        g.setColour(catColour.withAlpha(isBypassed_ ? 0.35f : 0.75f));
        g.drawText(getCategorySubtitle(type_), titleArea, juce::Justification::centredLeft, true);

        // Botón de eliminar (X)
        auto closeBtnRect = headerRect.removeFromRight(20.0f).withSizeKeepingCentre(14.0f, 14.0f);
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.drawText("×", closeBtnRect, juce::Justification::centred, false);

        // 4. Mini Pantalla Gráfica LCD Interactiva (Curva Arturia/FLEX)
        auto lcdArea = bounds.removeFromTop(36.0f).reduced(12.0f, 2.0f);
        drawMiniVisualCurve(g, lcdArea, type_, catColour, isBypassed_, processor_);

        // 5. Renderizado de Conectores Jack Mecanizados
        if (processor_ != nullptr) {
            const auto pins = processor_->getPins();
            for (size_t i = 0; i < pins.size(); ++i) {
                auto pinRect = getPinLocalRect(i);
                const bool isHighlighted = (pins[i].id == highlightedPinId_);
                const bool isInput = (pins[i].type == PinType::AudioInput || pins[i].type == PinType::EventInput);
                drawPinSocket(g, pinRect, isInput, isHighlighted, pins[i].dataType, catColour);
            }
        }

        // 6. Marco del Chasis: Resaltado de Selección / Halo de Inserción
        if (isDropCandidate_) {
            g.setColour(catColour.withAlpha(0.35f));
            g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 5.0f);
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 5.0f, 2.0f);
        } else if (isSelected_) {
            g.setColour(catColour.withAlpha(0.25f));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 5.0f, 4.0f);
            g.setColour(catColour);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 5.0f, 1.8f);
        } else {
            g.setColour(juce::Colour(0xff222a36));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 5.0f, 1.0f);
        }

        // Si está en bypass, atenuar visualmente los parámetros
        if (isBypassed_) {
            g.setColour(juce::Colour(0x6606080c));
            g.fillRoundedRectangle(bounds, 4.0f);
            g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.drawText("BYPASS", getLocalBounds().reduced(6).removeFromBottom(16), juce::Justification::centredRight);
        }
    }

    void resized() override {
        auto area = getLocalBounds();
        area.removeFromTop(30); // Cabecera LED
        area.removeFromTop(38); // Mini pantalla LCD
        area.removeFromBottom(8);
        area.reduce(14, 0);

        for (auto& slider : sliders_) {
            slider->setBounds(area.removeFromTop(24));
            area.removeFromTop(6); // Espaciado entre diales
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        // Clic en botón cerrar
        if (e.position.y < 30.0f && e.position.x > getWidth() - 28.0f) {
            if (onNodeDeleted_) {
                onNodeDeleted_(id_);
            }
            return;
        }

        // Clic en botón de encendido / bypass
        if (e.position.y < 30.0f && e.position.x < 28.0f) {
            isBypassed_ = !isBypassed_;
            if (onBypassToggled_) {
                onBypassToggled_(id_, isBypassed_);
            }
            repaint();
            return;
        }

        // Clic en pin (Izquierdo para cablear, Derecho para desconectar)
        if (processor_ != nullptr) {
            const auto pins = processor_->getPins();
            for (size_t i = 0; i < pins.size(); ++i) {
                auto pinRect = getPinLocalRect(i);
                if (pinRect.expanded(6.0f).contains(e.position)) {
                    if (e.mods.isRightButtonDown()) {
                        if (onPinRightClicked_) {
                            onPinRightClicked_(id_, pins[i].id);
                        }
                        return;
                    }
                    isDraggingPin_ = true;
                    draggedPinId_ = pins[i].id;
                    draggedPinType_ = pins[i].type;
                    draggedPinDataType_ = pins[i].dataType;
                    if (onPinDragStarted_) {
                        const auto canvasPt = getPosition().toFloat() + pinRect.getCentre();
                        onPinDragStarted_(id_, pins[i].id, pins[i].dataType, canvasPt);
                    }
                    return;
                }
            }
        }

        isDraggingPin_ = false;
        if (onNodeSelected_) {
            onNodeSelected_(id_);
        }
        dragger_.startDraggingComponent(this, e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (isDraggingPin_) {
            auto canvasPos = (getParentComponent() != nullptr)
                ? getParentComponent()->getLocalPoint(this, e.position).toFloat()
                : (getPosition().toFloat() + e.position);
            if (onPinDragging_) {
                onPinDragging_(canvasPos);
            }
            return;
        }

        dragger_.dragComponent(this, e, nullptr);
        if (onNodeMoved_) {
            onNodeMoved_(id_, static_cast<float>(getX()), static_cast<float>(getY()));
        }
        if (onNodeDraggingOverCanvas_) {
            onNodeDraggingOverCanvas_(id_, getBounds());
        }
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (isDraggingPin_) {
            isDraggingPin_ = false;
            auto canvasPos = (getParentComponent() != nullptr)
                ? getParentComponent()->getLocalPoint(this, e.position).toFloat()
                : (getPosition().toFloat() + e.position);
            if (onPinDragEnded_) {
                onPinDragEnded_(id_, draggedPinId_, draggedPinType_, draggedPinDataType_, canvasPos);
            }
            return;
        }

        if (onNodeDropped_) {
            onNodeDropped_(id_, getBounds());
        }
    }

private:
    void drawPinSocket(juce::Graphics& g, juce::Rectangle<float> pinRect, bool isInput, bool isHighlighted, PinDataType dataType, juce::Colour catColour) {
        juce::ignoreUnused(isInput);

        // Halo de acoplamiento al arrastrar cables
        if (isHighlighted) {
            g.setColour(catColour.withAlpha(0.35f));
            g.fillEllipse(pinRect.expanded(4.0f));
        }

        // Anillo exterior de aluminio mecanizado
        g.setColour(juce::Colour(0xff2a3240));
        g.fillEllipse(pinRect);

        // Agujero interior del jack oscuro
        auto innerRect = pinRect.reduced(2.0f);
        g.setColour(juce::Colour(0xff06070a));
        g.fillEllipse(innerRect);

        // Núcleo de terminal cromático según tipo de señal
        juce::Colour coreColour;
        if (dataType == PinDataType::AudioRateSignal) {
            coreColour = juce::Colour(0xffff00cc); // Magenta vibrante para FM Audio-Rate
        } else if (dataType == PinDataType::EventMessage) {
            coreColour = juce::Colour(0xff00ff88); // Esmeralda para Eventos
        } else if (dataType == PinDataType::ModulationScalar) {
            coreColour = juce::Colour(0xffffaa00); // Ámbar para Modulación
        } else {
            coreColour = juce::Colour(0xff00d4ff); // Cian para Audio Stereo/Mono
        }

        g.setColour(coreColour);
        g.fillEllipse(pinRect.getCentreX() - 1.5f, pinRect.getCentreY() - 1.5f, 3.0f, 3.0f);
        g.drawEllipse(innerRect, 1.0f);
    }

    juce::Rectangle<float> getPinLocalRect(size_t pinIndex) const {
        if (processor_ == nullptr) return { 4.0f, 40.0f, 10.0f, 10.0f };
        const auto pins = processor_->getPins();
        if (pinIndex >= pins.size()) return { 4.0f, 40.0f, 10.0f, 10.0f };

        const bool isInput = (pins[pinIndex].type == PinType::AudioInput || pins[pinIndex].type == PinType::EventInput);
        int slot = 0;
        for (size_t i = 0; i < pinIndex; ++i) {
            const bool otherIsInput = (pins[i].type == PinType::AudioInput || pins[i].type == PinType::EventInput);
            if (otherIsInput == isInput) ++slot;
        }

        const float pinY = 74.0f + static_cast<float>(slot) * 26.0f;
        const float pinX = isInput ? 4.0f : static_cast<float>(getWidth() - 14.0f);
        return { pinX, pinY, 10.0f, 10.0f };
    }

    void updateDimensions() {
        const size_t numParams = sliders_.size();
        const size_t numPins = (processor_ != nullptr) ? processor_->getPins().size() : 2;
        const int heightFromPins = 80 + static_cast<int>(numPins) * 26;
        const int heightFromParams = 76 + static_cast<int>(numParams) * 30;
        const int totalH = std::max({ 120, heightFromPins, heightFromParams });
        setSize(204, totalH);
    }

    NodeId id_{ InvalidNodeId };
    juce::String name_;
    NodeType type_{ NodeType::Unknown };
    AudioProcessorNode* processor_{ nullptr };

    bool isSelected_{ false };
    bool isBypassed_{ false };
    bool isDropCandidate_{ false };
    PinId highlightedPinId_{ InvalidPinId };

    bool isDraggingPin_{ false };
    PinId draggedPinId_{ InvalidPinId };
    PinType draggedPinType_{ PinType::AudioOutput };
    PinDataType draggedPinDataType_{ PinDataType::AudioStereo };

    juce::ComponentDragger dragger_;
    std::vector<std::unique_ptr<ModulationSlider>> sliders_;
    std::vector<ParameterId> sliderParamIds_;

    std::function<void(NodeId)> onNodeSelected_;
    std::function<void(NodeId, float, float)> onNodeMoved_;
    std::function<void(NodeId)> onNodeDeleted_;
    std::function<void(NodeId, PinId, PinDataType, juce::Point<float>)> onPinDragStarted_;
    std::function<void(juce::Point<float>)> onPinDragging_;
    std::function<void(NodeId, PinId, PinType, PinDataType, juce::Point<float>)> onPinDragEnded_;
    std::function<void(NodeId, PinId)> onPinRightClicked_;
    std::function<void(NodeId, juce::Rectangle<int>)> onNodeDraggingOverCanvas_;
    std::function<void(NodeId, juce::Rectangle<int>)> onNodeDropped_;
    std::function<void(NodeId, ParameterId, float)> onParameterChanged_;
    std::function<void(NodeId, ParameterId, ModSourceType, float)> onModulationRouteAdded_;
    std::function<void(NodeId, ParameterId, float)> onModulationDepthChanged_;
    std::function<void(NodeId, ParameterId)> onModulationRouteRemoved_;
    std::function<void(NodeId, bool)> onBypassToggled_;
};

} // namespace audio_graph
