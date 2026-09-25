#include "AudioVisualizerComponent.h"
#include "../plugin/PluginProcessor.h"

namespace audio_graph {

void AudioVisualizerComponent::updateTelemetry() {
    auto& ringBuffer = (sourceMode_ == SourceMode::Master)
        ? processor_.getVisualizerBuffer()
        : processor_.getProbeVisualizerBuffer();

    const size_t readSamples = ringBuffer.getLatestSamples(displayBufferL_.data(), displayBufferR_.data(), ScopeSize);
    if (readSamples == 0) return;

    // 1. Preparar datos FFT con ventana Hann
    static juce::dsp::FFT forwardFFT(FftOrder);
    static juce::dsp::WindowingFunction<float> window(FftSize, juce::dsp::WindowingFunction<float>::hann);

    for (size_t i = 0; i < FftSize; ++i) {
        fftData_[i] = (i < ScopeSize) ? (0.5f * (displayBufferL_[i] + displayBufferR_[i])) : 0.0f;
    }
    std::fill(fftData_.begin() + FftSize, fftData_.end(), 0.0f);

    window.multiplyWithWindowingTable(fftData_.data(), FftSize);
    forwardFFT.performFrequencyOnlyForwardTransform(fftData_.data());

    // 2. Suavizado temporal de bandas espectrales
    for (size_t i = 0; i < FftSize / 2; ++i) {
        const float mag = fftData_[i] * (2.0f / static_cast<float>(FftSize));
        smoothedSpectrum_[i] = std::max(mag, smoothedSpectrum_[i] * 0.82f);
    }

    // 3. Cálculo de Correlación de Fase para el Goniometro
    double dotLR = 0.0;
    double sumL2 = 0.0;
    double sumR2 = 0.0;
    for (size_t i = 0; i < ScopeSize; ++i) {
        const double l = displayBufferL_[i];
        const double r = displayBufferR_[i];
        dotLR += l * r;
        sumL2 += l * l;
        sumR2 += r * r;
    }
    const double denom = std::sqrt(sumL2 * sumR2);
    phaseCorrelation_ = (denom > 1e-9) ? static_cast<float>(dotLR / denom) : 1.0f;

    repaint();
}

void AudioVisualizerComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();

    // Fondo negro absoluto
    g.setColour(juce::Colour(0xff060606));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Borde blanco sutil
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

    // Área de visualización reservada (dejando barra superior de botones)
    auto vizArea = bounds;
    vizArea.removeFromTop(28.0f);
    vizArea.reduce(8.0f, 6.0f);

    switch (mode_) {
        case DisplayMode::Spectrum:
            drawSpectrum(g, vizArea);
            break;
        case DisplayMode::Oscilloscope:
            drawOscilloscope(g, vizArea);
            break;
        case DisplayMode::Goniometer:
            drawGoniometer(g, vizArea);
            break;
        case DisplayMode::Split: {
            auto topArea = vizArea.removeFromTop(vizArea.getHeight() * 0.5f);
            drawSpectrum(g, topArea);
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            g.drawHorizontalLine(static_cast<int>(topArea.getBottom()), vizArea.getX(), vizArea.getRight());
            drawOscilloscope(g, vizArea);
            break;
        }
    }
}

void AudioVisualizerComponent::drawSpectrum(juce::Graphics& g, juce::Rectangle<float> bounds) {
    if (bounds.getWidth() <= 10.0f || bounds.getHeight() <= 10.0f) return;

    // Cuadrícula de frecuencias de referencia (100Hz, 1kHz, 10kHz)
    g.setFont(juce::FontOptions(9.0f));
    g.setColour(juce::Colours::white.withAlpha(0.12f));

    const float sampleRate = static_cast<float>(processor_.getSampleRate() > 0.0 ? processor_.getSampleRate() : 44100.0);
    const float minFreq = 20.0f;
    const float maxFreq = 20000.0f;
    const float logMin = std::log10(minFreq);
    const float logMax = std::log10(maxFreq);

    auto freqToX = [&](float f) -> float {
        const float norm = (std::log10(std::clamp(f, minFreq, maxFreq)) - logMin) / (logMax - logMin);
        return bounds.getX() + norm * bounds.getWidth();
    };

    const float gridFreqs[] = { 50.0f, 100.0f, 250.0f, 500.0f, 1000.0f, 2500.0f, 5000.0f, 10000.0f, 20000.0f };
    for (float f : gridFreqs) {
        const float x = freqToX(f);
        g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
        if (f == 100.0f || f == 1000.0f || f == 10000.0f) {
            g.setColour(juce::Colours::white.withAlpha(0.35f));
            juce::String txt = (f >= 1000.0f) ? juce::String(static_cast<int>(f / 1000.0f)) + "k" : juce::String(static_cast<int>(f));
            g.drawText(txt, static_cast<int>(x + 3.0f), static_cast<int>(bounds.getY() + 2.0f), 30, 12, juce::Justification::left);
            g.setColour(juce::Colours::white.withAlpha(0.12f));
        }
    }

    // Curva del espectro FFT
    juce::Path spectrumPath;
    juce::Path fillPath;
    spectrumPath.preallocateSpace(256);

    const size_t numBins = FftSize / 2;
    const float binWidthHz = (sampleRate * 0.5f) / static_cast<float>(numBins);

    bool firstPoint = true;
    for (size_t b = 1; b < numBins; ++b) {
        const float freq = static_cast<float>(b) * binWidthHz;
        if (freq < minFreq || freq > maxFreq) continue;

        const float x = freqToX(freq);
        const float mag = smoothedSpectrum_[b];
        const float magDb = (mag > 1e-5f) ? (20.0f * std::log10(mag)) : -100.0f;
        // Rango de -70 dB a +6 dB
        const float normY = std::clamp((magDb - (-70.0f)) / 76.0f, 0.0f, 1.0f);
        const float y = bounds.getBottom() - normY * bounds.getHeight();

        if (firstPoint) {
            spectrumPath.startNewSubPath(x, y);
            fillPath.startNewSubPath(x, bounds.getBottom());
            fillPath.lineTo(x, y);
            firstPoint = false;
        } else {
            spectrumPath.lineTo(x, y);
            fillPath.lineTo(x, y);
        }
    }

    if (!firstPoint) {
        fillPath.lineTo(bounds.getRight(), bounds.getBottom());
        fillPath.closeSubPath();

        // Relleno degradado sutil
        juce::ColourGradient grad(juce::Colours::white.withAlpha(0.18f), 0.0f, bounds.getY(),
                                  juce::Colours::white.withAlpha(0.01f), 0.0f, bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillPath(fillPath);

        // Trazo de la curva nítida
        g.setColour(juce::Colours::white);
        g.strokePath(spectrumPath, juce::PathStrokeType(1.4f));
    }
}

void AudioVisualizerComponent::drawOscilloscope(juce::Graphics& g, juce::Rectangle<float> bounds) {
    if (bounds.getWidth() <= 10.0f || bounds.getHeight() <= 10.0f) return;

    // Eje central de cero
    const float midY = bounds.getCentreY();
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawHorizontalLine(static_cast<int>(midY), bounds.getX(), bounds.getRight());

    // Disparo Zero-Crossing para estabilizar forma de onda
    size_t triggerIdx = 0;
    for (size_t i = 1; i < ScopeSize / 2; ++i) {
        if (displayBufferL_[i - 1] < 0.0f && displayBufferL_[i] >= 0.0f) {
            triggerIdx = i;
            break;
        }
    }

    const size_t visibleSamples = std::min<size_t>(ScopeSize - triggerIdx, 512);
    if (visibleSamples < 2) return;

    const float dx = bounds.getWidth() / static_cast<float>(visibleSamples - 1);
    const float amp = bounds.getHeight() * 0.45f;

    juce::Path pathL;
    juce::Path pathR;

    pathL.startNewSubPath(bounds.getX(), midY - std::clamp(displayBufferL_[triggerIdx] * amp, -amp, amp));
    pathR.startNewSubPath(bounds.getX(), midY - std::clamp(displayBufferR_[triggerIdx] * amp, -amp, amp));

    for (size_t i = 1; i < visibleSamples; ++i) {
        const float x = bounds.getX() + static_cast<float>(i) * dx;
        const float yL = midY - std::clamp(displayBufferL_[triggerIdx + i] * amp, -amp, amp);
        const float yR = midY - std::clamp(displayBufferR_[triggerIdx + i] * amp, -amp, amp);
        pathL.lineTo(x, yL);
        pathR.lineTo(x, yR);
    }

    // Canal R en gris suave
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.strokePath(pathR, juce::PathStrokeType(1.0f));

    // Canal L en blanco brillante
    g.setColour(juce::Colours::white);
    g.strokePath(pathL, juce::PathStrokeType(1.4f));
}

void AudioVisualizerComponent::drawGoniometer(juce::Graphics& g, juce::Rectangle<float> bounds) {
    if (bounds.getWidth() <= 10.0f || bounds.getHeight() <= 10.0f) return;

    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getCentreY();
    const float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.42f;

    // Círculo y retícula de 45 grados (Mid / Side)
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 1.0f);
    g.drawLine(centreX - radius, centreY, centreX + radius, centreY, 1.0f); // Side
    g.drawLine(centreX, centreY - radius, centreX, centreY + radius, 1.0f); // Mid

    // Trazo de Lissajous XY: X = (L - R) * 0.707, Y = (L + R) * 0.707
    juce::Path lissajousPath;
    constexpr float InvSqrt2 = 0.70710678f;

    bool first = true;
    const size_t step = 2; // Decimación ligera para alta tasa de refresco
    for (size_t i = 0; i < ScopeSize; i += step) {
        const float l = displayBufferL_[i];
        const float r = displayBufferR_[i];

        const float side = (l - r) * InvSqrt2;
        const float mid = (l + r) * InvSqrt2;

        const float px = centreX + std::clamp(side * radius, -radius, radius);
        const float py = centreY - std::clamp(mid * radius, -radius, radius);

        if (first) {
            lissajousPath.startNewSubPath(px, py);
            first = false;
        } else {
            lissajousPath.lineTo(px, py);
        }
    }

    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.strokePath(lissajousPath, juce::PathStrokeType(1.2f));

    // Barra de Correlación de Fase (-1.0 a +1.0)
    auto barRect = bounds.removeFromRight(18.0f).removeFromBottom(bounds.getHeight() * 0.75f).reduced(2.0f, 0.0f);
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawRoundedRectangle(barRect, 2.0f, 1.0f);

    const float midBarY = barRect.getCentreY();
    const float corrY = midBarY - phaseCorrelation_ * (barRect.getHeight() * 0.5f - 2.0f);

    g.setColour(phaseCorrelation_ >= 0.0f ? juce::Colours::white : juce::Colours::white.withAlpha(0.4f));
    g.fillRect(barRect.getX() + 2.0f, std::min(midBarY, corrY), barRect.getWidth() - 4.0f, std::abs(corrY - midBarY));

    g.setFont(juce::FontOptions(8.0f));
    g.drawText("+1", static_cast<int>(barRect.getX() - 14.0f), static_cast<int>(barRect.getY()), 12, 10, juce::Justification::centredRight);
    g.drawText("-1", static_cast<int>(barRect.getX() - 14.0f), static_cast<int>(barRect.getBottom() - 10.0f), 12, 10, juce::Justification::centredRight);
}

void AudioVisualizerComponent::resized() {
    auto area = getLocalBounds().reduced(6, 4);
    auto header = area.removeFromTop(20);

    const int btnWidth = 72;
    specBtn_.setBounds(header.removeFromLeft(btnWidth));
    header.removeFromLeft(4);
    scopeBtn_.setBounds(header.removeFromLeft(btnWidth));
    header.removeFromLeft(4);
    gonioBtn_.setBounds(header.removeFromLeft(btnWidth));
    header.removeFromLeft(4);
    splitBtn_.setBounds(header.removeFromLeft(btnWidth));

    sourceToggleBtn_.setBounds(header.removeFromRight(96));
}

} // namespace audio_graph
