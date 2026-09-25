#include "AudioVisualizerComponent.h"
#include "../plugin/PluginProcessor.h"

namespace audio_graph {

void AudioVisualizerComponent::updateTelemetry() {
    auto& ringBuffer = (sourceMode_ == SourceMode::Master)
        ? processor_.getVisualizerBuffer()
        : processor_.getProbeVisualizerBuffer();

    const size_t readSamples = ringBuffer.getLatestSamples(displayBufferL_.data(), displayBufferR_.data(), ScopeSize);
    if (readSamples > 0) {
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

        // 2b. Actualización del Waterfall 3D (Z-shift hacia el fondo)
        for (size_t z = WaterfallSlices - 1; z > 0; --z) {
            waterfallSlices_[z] = waterfallSlices_[z - 1];
        }
        auto& front = waterfallSlices_[0];
        float maxPeak = 0.0f;
        const float normFactor = 2.0f / static_cast<float>(FftSize);
        for (size_t b = 0; b < WaterfallBins; ++b) {
            const size_t fftIdx = static_cast<size_t>(std::clamp(std::pow(static_cast<float>(b) / static_cast<float>(WaterfallBins), 1.35f) * (FftSize / 2), 0.0f, static_cast<float>(FftSize / 2 - 1)));
            float mag = fftData_[fftIdx] * normFactor;
            float db = (mag > 1e-4f) ? (20.0f * std::log10(mag)) : -80.0f;
            float normMag = std::clamp((db + 70.0f) / 70.0f, 0.0f, 1.0f);
            front.magnitudes[b] = normMag;
            if (normMag > maxPeak) maxPeak = normMag;
        }
        front.peak = maxPeak;

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
    } else {
        // Decaimiento visual del espectro en ausencia de audio (Regla 49)
        for (auto& s : smoothedSpectrum_) {
            s *= 0.85f;
        }
        for (auto& slice : waterfallSlices_) {
            for (auto& m : slice.magnitudes) {
                m *= 0.92f;
            }
            slice.peak *= 0.92f;
        }
    }

    // 4. Telemetría de Eventos para el Radar 3D (Reglas 9, 23, 26) - Se procesa siempre
    auto& eventTelemetry = processor_.getDualWorldEngine().getEventManager().getTelemetryBuffer();
    EventTelemetryItem item;
    while (eventTelemetry.pop(item)) {
        size_t bestSlot = 0;
        float minAlpha = 1.0f;
        for (size_t i = 0; i < radarParticles_.size(); ++i) {
            if (radarParticles_[i].alpha < minAlpha) {
                minAlpha = radarParticles_[i].alpha;
                bestSlot = i;
            }
        }
        auto& p = radarParticles_[bestSlot];
        p.pan = item.pan;
        p.pitchRatio = item.pitchRatio;
        p.energy = item.energy;
        p.distance = item.distance;
        p.azimuth = item.azimuth;
        p.type = item.type;
        p.generation = item.generation;
        p.alpha = 1.0f;
        p.radius = 3.5f + std::clamp(item.energy * 8.0f, 0.0f, 12.0f);
    }

    // Rotación continua del haz de radar y desvanecimiento de partículas
    radarAngle_ += 0.045f;
    if (radarAngle_ > 2.0f * std::numbers::pi_v<float>) {
        radarAngle_ -= 2.0f * std::numbers::pi_v<float>;
    }
    for (auto& p : radarParticles_) {
        if (p.alpha > 0.0f) {
            p.alpha = std::max(0.0f, p.alpha - 0.035f);
        }
    }

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
        case DisplayMode::Radar:
            drawRadar(g, vizArea);
            break;
        case DisplayMode::Waterfall:
            drawWaterfall(g, vizArea);
            break;
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

void AudioVisualizerComponent::drawRadar(juce::Graphics& g, juce::Rectangle<float> bounds) {
    if (bounds.getWidth() <= 20.0f || bounds.getHeight() <= 20.0f) return;

    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getCentreY();
    const float maxRadius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.44f;

    // 1. Fondo del radar (círculo polar oscuro profundo con contorno blanco sutil)
    g.setColour(juce::Colour(0xff090c0a));
    g.fillEllipse(centreX - maxRadius, centreY - maxRadius, maxRadius * 2.0f, maxRadius * 2.0f);
    g.setColour(juce::Colours::white.withAlpha(0.25f));
    g.drawEllipse(centreX - maxRadius, centreY - maxRadius, maxRadius * 2.0f, maxRadius * 2.0f, 1.2f);

    // 2. Anillos concéntricos de distancia (33%, 66%, 100%)
    const float rings[] = { 0.33f, 0.66f, 1.0f };
    const char* ringLabels[] = { "1.0m", "3.0m", "10.0m" };
    g.setFont(juce::FontOptions(8.5f));

    for (size_t r = 0; r < 3; ++r) {
        const float rad = maxRadius * rings[r];
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawEllipse(centreX - rad, centreY - rad, rad * 2.0f, rad * 2.0f, 0.8f);

        g.setColour(juce::Colours::white.withAlpha(0.35f));
        g.drawText(ringLabels[r], static_cast<int>(centreX + 4.0f), static_cast<int>(centreY - rad - 2.0f), 35, 10, juce::Justification::centredLeft);
    }

    // 3. Ejes polares (Líneas radiales a 0°, 90°)
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.drawLine(centreX - maxRadius, centreY, centreX + maxRadius, centreY, 0.8f); // Eje horizontal L - R
    g.drawLine(centreX, centreY - maxRadius, centreX, centreY + maxRadius, 0.8f); // Eje vertical Frontal

    // Etiquetas de orientación estéreo
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.drawText("L", static_cast<int>(centreX - maxRadius - 16.0f), static_cast<int>(centreY - 8.0f), 14, 16, juce::Justification::centred);
    g.drawText("R", static_cast<int>(centreX + maxRadius + 2.0f), static_cast<int>(centreY - 8.0f), 14, 16, juce::Justification::centred);
    g.drawText("CENTER", static_cast<int>(centreX - 25.0f), static_cast<int>(centreY - maxRadius - 14.0f), 50, 12, juce::Justification::centred);

    // 4. Haz de radar giratorio (Sweep beam con estela fosforescente P4)
    const float juceAngle = radarAngle_ - 0.5f * std::numbers::pi_v<float>;
    juce::Path sweepPath;
    sweepPath.startNewSubPath(centreX, centreY);
    constexpr float sweepArc = 0.52f;
    sweepPath.addCentredArc(centreX, centreY, maxRadius, maxRadius, 0.0f, juceAngle - sweepArc, juceAngle, false);
    sweepPath.closeSubPath();

    g.setColour(juce::Colour(0xff00ff88).withAlpha(0.09f));
    g.fillPath(sweepPath);

    const float sweepEndX = centreX + std::cos(juceAngle) * maxRadius;
    const float sweepEndY = centreY + std::sin(juceAngle) * maxRadius;
    g.setColour(juce::Colour(0xff80ffb0).withAlpha(0.75f));
    g.drawLine(centreX, centreY, sweepEndX, sweepEndY, 1.5f);

    // 5. Partículas activas de eventos acústicos
    int activeParticleCount = 0;
    for (const auto& p : radarParticles_) {
        if (p.alpha <= 0.02f) continue;
        ++activeParticleCount;

        const float angle = p.pan * (0.5f * std::numbers::pi_v<float>);
        const float distNorm = std::clamp(p.distance / 10.0f, 0.15f, 0.95f);
        const float r = maxRadius * distNorm;

        const float px = centreX + std::sin(angle) * r;
        const float py = centreY - std::cos(angle) * r;

        // Proximidad al haz de barrido para efecto Phosphor Ping
        float angleDiff = std::abs(radarAngle_ - (angle + 0.5f * std::numbers::pi_v<float>));
        while (angleDiff > std::numbers::pi_v<float>) angleDiff -= 2.0f * std::numbers::pi_v<float>;
        angleDiff = std::abs(angleDiff);
        const float pingBoost = (angleDiff < 0.35f) ? (1.0f - angleDiff / 0.35f) : 0.0f;

        // Color según pitchRatio
        juce::Colour particleCol;
        if (p.pitchRatio < 0.95f) {
            const float t = std::clamp((1.0f - p.pitchRatio), 0.0f, 1.0f);
            particleCol = juce::Colour(0xff00d4ff).interpolatedWith(juce::Colour(0xff4060ff), t);
        } else if (p.pitchRatio > 1.05f) {
            const float t = std::clamp((p.pitchRatio - 1.0f), 0.0f, 1.0f);
            particleCol = juce::Colour(0xffffcc00).interpolatedWith(juce::Colour(0xffff5500), t);
        } else {
            particleCol = juce::Colours::white;
        }

        const float effectiveAlpha = std::clamp(p.alpha + pingBoost * 0.4f, 0.0f, 1.0f);
        const float partRad = (p.radius + pingBoost * 3.0f) * p.alpha;

        g.setColour(particleCol.withAlpha(effectiveAlpha * 0.35f));
        g.fillEllipse(px - partRad * 1.8f, py - partRad * 1.8f, partRad * 3.6f, partRad * 3.6f);

        g.setColour(particleCol.withAlpha(effectiveAlpha));
        g.fillEllipse(px - partRad, py - partRad, partRad * 2.0f, partRad * 2.0f);
        g.setColour(juce::Colours::white.withAlpha(effectiveAlpha * 0.9f));
        g.fillEllipse(px - partRad * 0.4f, py - partRad * 0.4f, partRad * 0.8f, partRad * 0.8f);

        if (p.energy > 0.35f && p.alpha > 0.4f) {
            g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
            g.setColour(juce::Colours::white.withAlpha(p.alpha * 0.85f));
            const char* typeTag = (p.type == EventType::Grain) ? "GRN" :
                                  (p.type == EventType::Fragment) ? "FRG" :
                                  (p.type == EventType::Transient) ? "TRN" :
                                  (p.type == EventType::Echo) ? "ECH" : "EVT";
            g.drawText(typeTag, static_cast<int>(px - 15.0f), static_cast<int>(py + partRad + 1.0f), 30, 8, juce::Justification::centred);
        }
    }

    // 6. Badge de estado en tiempo real
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    juce::String statusStr = (activeParticleCount > 0)
        ? "RADAR 3D ONLINE • " + juce::String(activeParticleCount) + " EVENTS"
        : "RADAR 3D • ACTIVE SCAN";
    g.setColour(activeParticleCount > 0 ? juce::Colour(0xff00ff88) : juce::Colours::white.withAlpha(0.4f));
    g.drawText(statusStr, bounds.removeFromTop(16.0f).removeFromRight(170.0f), juce::Justification::centredRight);
}

void AudioVisualizerComponent::drawWaterfall(juce::Graphics& g, juce::Rectangle<float> bounds) {
    if (bounds.getWidth() <= 10.0f || bounds.getHeight() <= 10.0f) return;

    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    const float x = bounds.getX();
    const float y = bounds.getY();

    // Fondo oscuro con rejilla isométrica 3D
    g.setColour(juce::Colour(0xff080b10));
    g.fillRoundedRectangle(bounds, 3.0f);

    const float frontY = y + h * 0.88f;
    const float backY = y + h * 0.20f;
    const float centerX = x + w * 0.5f;

    // Líneas guía de perspectiva
    g.setColour(juce::Colour(0xff182230).withAlpha(0.35f));
    for (float p = -0.5f; p <= 0.5f; p += 0.25f) {
        g.drawLine(centerX + p * w * 0.95f, frontY, centerX + p * w * 0.45f, backY, 0.7f);
    }
    for (float t = 0.0f; t <= 1.0f; t += 0.25f) {
        float ly = frontY - t * (frontY - backY);
        float lw = w * (0.95f - t * 0.50f);
        g.drawLine(centerX - lw * 0.5f, ly, centerX + lw * 0.5f, ly, 0.7f);
    }

    // Renderizar rodajas desde el fondo (Z = WaterfallSlices - 1) hasta el frente (Z = 0)
    for (int z = static_cast<int>(WaterfallSlices) - 1; z >= 0; --z) {
        const auto& slice = waterfallSlices_[z];
        const float zNorm = static_cast<float>(z) / static_cast<float>(WaterfallSlices);
        const float scale = 1.0f - zNorm * 0.52f;
        const float sliceW = w * 0.92f * scale;
        const float baseSliceY = frontY - zNorm * (frontY - backY);
        const float leftX = centerX - sliceW * 0.5f;
        const float maxPeakH = h * 0.42f * scale;
        const float alpha = std::clamp(1.0f - zNorm * 0.75f, 0.12f, 1.0f);

        juce::Path slicePath;
        juce::Path fillPath;
        bool started = false;

        for (size_t b = 0; b < WaterfallBins; b += 2) {
            float px = leftX + (static_cast<float>(b) / static_cast<float>(WaterfallBins)) * sliceW;
            float val = slice.magnitudes[b];
            float py = baseSliceY - val * maxPeakH;

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

            g.setColour(juce::Colour(0xff080b10).withAlpha(std::clamp(0.65f * alpha, 0.25f, 0.85f)));
            g.fillPath(fillPath);

            // Gradiente térmico reactivo
            float peak = slice.peak;
            juce::Colour col;
            if (peak < 0.25f) {
                float t = peak / 0.25f;
                col = juce::Colour::fromFloatRGBA(0.1f * (1.0f - t), 0.3f * t, 0.6f + 0.4f * t, alpha);
            } else if (peak < 0.60f) {
                float t = (peak - 0.25f) / 0.35f;
                col = juce::Colour::fromFloatRGBA(0.0f, 0.5f + 0.5f * t, 1.0f - 0.7f * t, alpha);
            } else if (peak < 0.85f) {
                float t = (peak - 0.60f) / 0.25f;
                col = juce::Colour::fromFloatRGBA(t, 1.0f - 0.3f * t, 0.1f * (1.0f - t), alpha);
            } else {
                float t = (peak - 0.85f) / 0.15f;
                col = juce::Colour::fromFloatRGBA(1.0f, 0.75f + 0.25f * t, t, alpha);
            }

            g.setColour(col);
            g.strokePath(slicePath, juce::PathStrokeType(1.1f * scale));
        }
    }

    // Badge HUD
    g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.drawText("WATERFALL 3D SONOGRAM • 36 SLICES", bounds.reduced(8.0f).removeFromTop(14.0f), juce::Justification::topLeft, false);
}

void AudioVisualizerComponent::resized() {
    auto area = getLocalBounds().reduced(6, 4);
    auto header = area.removeFromTop(20);

    const int btnWidth = 56;
    specBtn_.setBounds(header.removeFromLeft(btnWidth));
    header.removeFromLeft(2);
    scopeBtn_.setBounds(header.removeFromLeft(btnWidth));
    header.removeFromLeft(2);
    gonioBtn_.setBounds(header.removeFromLeft(btnWidth));
    header.removeFromLeft(2);
    splitBtn_.setBounds(header.removeFromLeft(btnWidth));
    header.removeFromLeft(2);
    radarBtn_.setBounds(header.removeFromLeft(btnWidth));
    header.removeFromLeft(2);
    waterfallBtn_.setBounds(header.removeFromLeft(btnWidth + 14));

    sourceToggleBtn_.setBounds(header.removeFromRight(92));
}

} // namespace audio_graph
