#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <cmath>
#include <numbers>
#include <algorithm>
#include "../core/Types.h"
#include "../core/AudioVisualizerBuffer.h"

namespace audio_graph {

class N8AudioProcessor;

/**
 * @brief Visualizador Espectral 3D en Cascada / Waterfall Sonogram (Reglas 23, 24, 25, 26, 49).
 * Mantiene un historial continuo de rodajas FFT proyectadas en perspectiva 3D isométrica con
 * gradientes térmicos reactivos (violeta -> cian -> esmeralda -> ámbar -> blanco incandescente).
 * Cumple estrictamente con la Regla 49 (barrido continuo y decaimiento armónico en silencio).
 */
class WaterfallVisualizerComponent : public juce::Component {
public:
    static constexpr size_t FftOrder = 10; // 1024 muestras FFT
    static constexpr size_t FftSize = 1 << FftOrder;
    static constexpr size_t NumBins = FftSize / 4; // 256 bandas útiles
    static constexpr size_t NumSlices = 48; // 48 rodajas temporales en profundidad

    struct SpectralSlice {
        std::array<float, NumBins> magnitudes{};
        float peakEnergy{ 0.0f };
    };

    explicit WaterfallVisualizerComponent(N8AudioProcessor& processor)
        : processor_(processor),
          forwardFFT_(static_cast<int>(FftOrder)),
          window_(FftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        setOpaque(true);
        fftBuffer_.fill(0.0f);
        for (auto& slice : slices_) {
            slice.magnitudes.fill(0.0f);
            slice.peakEnergy = 0.0f;
        }
    }

    void updateTelemetry(const float* audioL, const float* audioR, size_t numSamples) {
        if (audioL != nullptr && numSamples > 0) {
            // Cargar y ventanear audio
            for (size_t i = 0; i < FftSize; ++i) {
                float sample = 0.0f;
                if (i < numSamples) {
                    sample = (audioR != nullptr) ? (0.5f * (audioL[i] + audioR[i])) : audioL[i];
                }
                fftBuffer_[i] = sample;
            }
            std::fill(fftBuffer_.begin() + FftSize, fftBuffer_.end(), 0.0f);
            window_.multiplyWithWindowingTable(fftBuffer_.data(), FftSize);
            forwardFFT_.performFrequencyOnlyForwardTransform(fftBuffer_.data());

            // Avanzar rodajas hacia el fondo (Z-shift)
            for (size_t z = NumSlices - 1; z > 0; --z) {
                slices_[z] = slices_[z - 1];
            }

            // Nueva rodaja frontal
            auto& front = slices_[0];
            float maxEnergy = 0.0f;
            const float normFactor = 2.0f / static_cast<float>(FftSize);

            for (size_t b = 0; b < NumBins; ++b) {
                // Mapeo no-lineal de frecuencias bajas a altas
                const size_t fftIdx = static_cast<size_t>(std::clamp(std::pow(static_cast<float>(b) / static_cast<float>(NumBins), 1.4f) * (FftSize / 2), 0.0f, static_cast<float>(FftSize / 2 - 1)));
                float mag = fftBuffer_[fftIdx] * normFactor;
                // Escala dB suave
                float db = (mag > 1e-4f) ? (20.0f * std::log10(mag)) : -80.0f;
                float normMag = std::clamp((db + 70.0f) / 70.0f, 0.0f, 1.0f);
                front.magnitudes[b] = normMag;
                if (normMag > maxEnergy) maxEnergy = normMag;
            }
            front.peakEnergy = maxEnergy;
        } else {
            // Regla 49: Decaimiento continuo suave en silencio (Zero Gating)
            for (auto& slice : slices_) {
                for (auto& m : slice.magnitudes) {
                    m *= 0.92f;
                }
                slice.peakEnergy *= 0.92f;
            }
        }

        // Animación de perspectiva y estela continua
        animationPhase_ += 0.02f;
        if (animationPhase_ > 2.0f * std::numbers::pi_v<float>) {
            animationPhase_ -= 2.0f * std::numbers::pi_v<float>;
        }

        repaint();
    }

    void setTilt(float tilt) noexcept { tilt_ = std::clamp(tilt, 0.2f, 0.85f); repaint(); }
    void setDepth(float depth) noexcept { depthScale_ = std::clamp(depth, 0.4f, 1.5f); repaint(); }
    void setHeightScale(float h) noexcept { heightScale_ = std::clamp(h, 0.5f, 2.5f); repaint(); }

    void mouseDown(const juce::MouseEvent& e) override {
        lastMousePos_ = e.getPosition();
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        auto delta = e.getPosition() - lastMousePos_;
        lastMousePos_ = e.getPosition();

        tilt_ = std::clamp(tilt_ - delta.y * 0.003f, 0.2f, 0.85f);
        perspectiveAngle_ = std::clamp(perspectiveAngle_ + delta.x * 0.003f, -0.6f, 0.6f);
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // Fondo oscuro de ciberespacio
        g.setColour(juce::Colour(0xff06070a));
        g.fillRoundedRectangle(bounds, 4.0f);

        // Borde
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

        const float width = bounds.getWidth() - 20.0f;
        const float height = bounds.getHeight() - 20.0f;
        const float originX = bounds.getX() + 10.0f;
        const float originY = bounds.getY() + 10.0f;

        // Cuadrícula isométrica de fondo
        drawFloorGrid(g, originX, originY, width, height);

        // Dibujar las rodajas desde el fondo (Z = NumSlices - 1) hacia el frente (Z = 0)
        for (int z = static_cast<int>(NumSlices) - 1; z >= 0; --z) {
            const float zNorm = static_cast<float>(z) / static_cast<float>(NumSlices);
            drawSlice(g, static_cast<size_t>(z), zNorm, originX, originY, width, height);
        }

        // Overlay de HUD interactivo
        drawHud(g, bounds);
    }

private:
    void drawFloorGrid(juce::Graphics& g, float x, float y, float w, float h) {
        g.setColour(juce::Colour(0xff182028).withAlpha(0.35f));
        const float backW = w * 0.55f;
        const float frontY = y + h * 0.90f;
        const float backY = y + h * (1.0f - tilt_);
        const float centerX = x + w * 0.5f + perspectiveAngle_ * w * 0.25f;

        // Líneas guía de profundidad
        for (float p = -0.5f; p <= 0.5f; p += 0.25f) {
            float fx = centerX + p * w;
            float bx = centerX + p * backW;
            g.drawLine(fx, frontY, bx, backY, 0.8f);
        }

        // Líneas horizontales de tiempo
        for (float t = 0.0f; t <= 1.0f; t += 0.2f) {
            float curY = frontY - t * (frontY - backY);
            float curW = w - t * (w - backW);
            g.drawLine(centerX - curW * 0.5f, curY, centerX + curW * 0.5f, curY, 0.8f);
        }
    }

    void drawSlice(juce::Graphics& g, size_t sliceIdx, float zNorm, float x, float y, float w, float h) {
        const auto& slice = slices_[sliceIdx];
        const float backScale = 0.55f;
        const float scale = 1.0f - zNorm * (1.0f - backScale);
        const float sliceW = w * scale;

        const float frontY = y + h * 0.90f;
        const float backY = y + h * (1.0f - tilt_);
        const float baseSliceY = frontY - zNorm * (frontY - backY);
        const float centerX = x + w * 0.5f + perspectiveAngle_ * w * 0.25f * (1.0f - zNorm * 0.5f);
        const float leftX = centerX - sliceW * 0.5f;

        const float maxPeakY = h * 0.45f * heightScale_ * scale;
        const float alpha = std::clamp(1.0f - zNorm * 0.75f, 0.12f, 1.0f);

        juce::Path slicePath;
        juce::Path fillPath;

        const size_t step = 2; // Bandas agrupadas para fluidez a 60 FPS
        bool started = false;

        for (size_t b = 0; b < NumBins; b += step) {
            float px = leftX + (static_cast<float>(b) / static_cast<float>(NumBins)) * sliceW;
            float val = slice.magnitudes[b];
            float py = baseSliceY - val * maxPeakY;

            if (!started) {
                slicePath.startNewSubPath(px, py);
                fillPath.startNewSubPath(px, baseSliceY);
                fillPath.lineTo(px, py);
                started = true;
            } else {
                slicePath.lineTo(px, py);
                fillPath.lineTo(px, py);
            }
        }

        if (started) {
            fillPath.lineTo(leftX + sliceW, baseSliceY);
            fillPath.closeSubPath();

            // Sombra inferior semi-transparente para oclusión 3D
            g.setColour(juce::Colour(0xff06070a).withAlpha(std::clamp(0.65f * alpha, 0.2f, 0.9f)));
            g.fillPath(fillPath);

            // Gradiente térmico para la cresta espectral
            juce::Colour crestColour = getThermalColour(slice.peakEnergy).withAlpha(alpha);
            g.setColour(crestColour);
            g.strokePath(slicePath, juce::PathStrokeType(1.2f * scale));
        }
    }

    [[nodiscard]] juce::Colour getThermalColour(float val) const noexcept {
        val = std::clamp(val, 0.0f, 1.0f);
        if (val < 0.25f) {
            // Negro-violeta -> Cian profundo
            float t = val / 0.25f;
            return juce::Colour::fromFloatRGBA(0.1f * (1.0f - t), 0.3f * t, 0.5f + 0.5f * t, 1.0f);
        } else if (val < 0.55f) {
            // Cian -> Esmeralda vibrante
            float t = (val - 0.25f) / 0.30f;
            return juce::Colour::fromFloatRGBA(0.0f, 0.4f + 0.6f * t, 1.0f - 0.6f * t, 1.0f);
        } else if (val < 0.85f) {
            // Esmeralda -> Ámbar / Neón Naranja
            float t = (val - 0.55f) / 0.30f;
            return juce::Colour::fromFloatRGBA(t, 1.0f - 0.25f * t, 0.1f * (1.0f - t), 1.0f);
        } else {
            // Neón Naranja -> Blanco incandescente
            float t = (val - 0.85f) / 0.15f;
            return juce::Colour::fromFloatRGBA(1.0f, 0.75f + 0.25f * t, t, 1.0f);
        }
    }

    void drawHud(juce::Graphics& g, juce::Rectangle<float> bounds) {
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.setColour(juce::Colours::white.withAlpha(0.65f));
        g.drawText("3D WATERFALL SONOGRAM", bounds.reduced(10.0f).removeFromTop(16.0f), juce::Justification::topLeft, false);

        g.setFont(juce::Font(9.0f, juce::Font::plain));
        g.setColour(juce::Colours::white.withAlpha(0.40f));
        juce::String info = "TILT: " + juce::String(static_cast<int>(tilt_ * 100)) + "% | DRAG TO ROTATE 3D";
        g.drawText(info, bounds.reduced(10.0f).removeFromBottom(14.0f), juce::Justification::bottomRight, false);
    }

    N8AudioProcessor& processor_;
    juce::dsp::FFT forwardFFT_;
    juce::dsp::WindowingFunction<float> window_;

    std::array<float, FftSize * 2> fftBuffer_{};
    std::array<SpectralSlice, NumSlices> slices_{};

    float tilt_{ 0.55f };
    float perspectiveAngle_{ 0.0f };
    float depthScale_{ 1.0f };
    float heightScale_{ 1.25f };
    float animationPhase_{ 0.0f };
    juce::Point<int> lastMousePos_{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaterfallVisualizerComponent)
};

} // namespace audio_graph
