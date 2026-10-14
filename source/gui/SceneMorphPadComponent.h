#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>
#include <algorithm>
#include <cmath>
#include <string>
#include "ThemeManager.h"

namespace audio_graph {

/**
 * @brief Modos de automatización cinética y física autónoma para el Pad de Escenas (Fase 7).
 */
enum class KineticMode : uint8_t {
    Manual = 0,
    Orbit = 1,
    Lissajous = 2,
    PingPong = 3,
    SpringPhysics = 4
};

/**
 * @brief Componente de Pad X/Y 2D para Morphing de Escenas Acústicas y Telemetría de Macros (Fase 7, Reglas 7, 21, 23, 25, 35, 48, 49).
 * Permite interpolación continua, no bloqueante y libre de clicks entre 4 escenas acústicas A, B, C y D.
 * Cuenta con feedback visual de los 8 macros globales, control de glide cinemático y máquina de estados Drag vs Click.
 */
class SceneMorphPadComponent : public juce::Component, private juce::Timer {
public:
    SceneMorphPadComponent() {
        setRepaintsOnMouseActivity(true);

        // Botón Snap Center
        centerBtn_.setButtonText(juce::CharPointer_UTF8("\xE2\x9C\x9F Center"));
        centerBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff121824));
        centerBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        centerBtn_.onClick = [this]() {
            setTargetXY(0.5f, 0.5f, true);
        };
        addAndMakeVisible(centerBtn_);

        // Botón Smart Procedural Mutate
        randomBtn_.setButtonText(juce::CharPointer_UTF8("\xF0\x9F\x8E\xB2 Mutate"));
        randomBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1d28));
        randomBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffaa00));
        randomBtn_.onClick = [this]() {
            if (onRandomizeRequested_) onRandomizeRequested_();
        };
        addAndMakeVisible(randomBtn_);

        // Botones de Captura de Escena en las 4 esquinas
        const char* capLabels[4] = { "CAP A", "CAP B", "CAP C", "CAP D" };
        const juce::Colour capCols[4] = {
            juce::Colour(0xffffaa00), // Scene A: Amber
            juce::Colour(0xff00d4ff), // Scene B: Cyan
            juce::Colour(0xffd040ff), // Scene C: Magenta
            juce::Colour(0xff00ff88)  // Scene D: Emerald
        };

        for (size_t i = 0; i < 4; ++i) {
            capButtons_[i].setButtonText(capLabels[i]);
            capButtons_[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0f141f));
            capButtons_[i].setColour(juce::TextButton::textColourOffId, capCols[i]);
            capButtons_[i].onClick = [this, i]() {
                if (onCaptureScene_) onCaptureScene_(i);
                sceneCaptured_[i] = true;
                repaint();
            };
            addAndMakeVisible(capButtons_[i]);
        }

        // Slider de Glide (anti-click glide speed)
        glideSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
        glideSlider_.setRange(0.01, 1.0, 0.01);
        glideSlider_.setValue(0.12);
        glideSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        glideSlider_.setTooltip("Glide Smoothing Rate");
        glideSlider_.onValueChange = [this]() {
            smoothingSpeed_ = static_cast<float>(glideSlider_.getValue());
            if (onGlideSpeedChanged_) onGlideSpeedChanged_(smoothingSpeed_);
        };
        addAndMakeVisible(glideSlider_);

        // Combo de Modo Cinético Autónomo
        kineticModeCombo_.addItem("Manual", 1);
        kineticModeCombo_.addItem("Orbit", 2);
        kineticModeCombo_.addItem("Lissajous", 3);
        kineticModeCombo_.addItem("Ping-Pong", 4);
        kineticModeCombo_.addItem("Spring / Inertia", 5);
        kineticModeCombo_.setSelectedId(1, juce::dontSendNotification);
        kineticModeCombo_.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff121824));
        kineticModeCombo_.setColour(juce::ComboBox::textColourId, juce::Colour(0xff00d4ff));
        kineticModeCombo_.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff222a38));
        kineticModeCombo_.setTooltip("Autonomous Kinetic Automation Mode");
        kineticModeCombo_.onChange = [this]() {
            int id = kineticModeCombo_.getSelectedId();
            if (id >= 1 && id <= 5) {
                kineticMode_ = static_cast<KineticMode>(id - 1);
                if (kineticMode_ == KineticMode::Orbit || kineticMode_ == KineticMode::Lissajous) {
                    kineticPhase_ = 0.0f;
                }
            }
        };
        addAndMakeVisible(kineticModeCombo_);

        // Slider de Velocidad Cinética
        kineticRateSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
        kineticRateSlider_.setRange(0.1, 3.0, 0.05);
        kineticRateSlider_.setValue(1.0);
        kineticRateSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        kineticRateSlider_.setTooltip("Kinetic Rate / Speed Multiplier");
        kineticRateSlider_.onValueChange = [this]() {
            kineticRate_ = static_cast<float>(kineticRateSlider_.getValue());
        };
        addAndMakeVisible(kineticRateSlider_);

        startTimerHz(60); // 60 FPS de animación continua fluida (Regla 49)
    }

    ~SceneMorphPadComponent() override {
        stopTimer();
    }

    // --- Callbacks ---
    void setOnMorphXYChanged(std::function<void(float, float)> cb) { onMorphXYChanged_ = std::move(cb); }
    void setOnCaptureScene(std::function<void(size_t)> cb) { onCaptureScene_ = std::move(cb); }
    void setOnClearScene(std::function<void(size_t)> cb) { onClearScene_ = std::move(cb); }
    void setOnRandomizeRequested(std::function<void()> cb) { onRandomizeRequested_ = std::move(cb); }
    void setOnGlideSpeedChanged(std::function<void(float)> cb) { onGlideSpeedChanged_ = std::move(cb); }

    // --- Sincronización de Estado ---
    void setTargetXY(float x, float y, bool notify = true) {
        targetX_ = std::clamp(x, 0.0f, 1.0f);
        targetY_ = std::clamp(y, 0.0f, 1.0f);
        if (notify && onMorphXYChanged_) {
            onMorphXYChanged_(targetX_, targetY_);
        }
        repaint();
    }

    void setSmoothedXY(float x, float y) noexcept {
        currentX_ = std::clamp(x, 0.0f, 1.0f);
        currentY_ = std::clamp(y, 0.0f, 1.0f);
    }

    void setSceneCaptured(size_t sceneIdx, bool isCaptured) noexcept {
        if (sceneIdx < 4) {
            sceneCaptured_[sceneIdx] = isCaptured;
            repaint();
        }
    }

    void updateMacroValues(const std::array<float, 8>& macros) noexcept {
        macroValues_ = macros;
    }

    [[nodiscard]] float getTargetX() const noexcept { return targetX_; }
    [[nodiscard]] float getTargetY() const noexcept { return targetY_; }
    [[nodiscard]] float getCurrentX() const noexcept { return currentX_; }
    [[nodiscard]] float getCurrentY() const noexcept { return currentY_; }
    [[nodiscard]] float getSmoothingSpeed() const noexcept { return smoothingSpeed_; }

    void setKineticMode(KineticMode mode) {
        kineticMode_ = mode;
        kineticModeCombo_.setSelectedId(static_cast<int>(mode) + 1, juce::dontSendNotification);
        if (mode == KineticMode::Orbit || mode == KineticMode::Lissajous) {
            kineticPhase_ = 0.0f;
        }
    }

    [[nodiscard]] KineticMode getKineticMode() const noexcept { return kineticMode_; }

    void setKineticRate(float rate) noexcept {
        kineticRate_ = std::clamp(rate, 0.05f, 5.0f);
        kineticRateSlider_.setValue(kineticRate_, juce::dontSendNotification);
    }

    [[nodiscard]] float getKineticRate() const noexcept { return kineticRate_; }

    void triggerPhysicsImpulse(float vx, float vy) noexcept {
        velocityX_ = vx;
        velocityY_ = vy;
    }

    // --- Máquina de Estados Drag vs Click (Regla 48) y Física Inercial ---
    void mouseDown(const juce::MouseEvent& e) override {
        hasDragged_ = false;
        isMouseDown_ = true;
        dragStartPos_ = e.position;
        lastDragPos_ = e.position;
        velocityX_ = 0.0f;
        velocityY_ = 0.0f;

        auto padBounds = getPadBounds();
        if (padBounds.contains(e.position)) {
            float normX = (e.position.x - padBounds.getX()) / padBounds.getWidth();
            float normY = (e.position.y - padBounds.getY()) / padBounds.getHeight();
            setTargetXY(normX, normY, true);
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        const float dist = e.position.getDistanceFrom(dragStartPos_);
        if (!hasDragged_ && dist > 4.0f) {
            hasDragged_ = true;
        }

        auto padBounds = getPadBounds();
        if (padBounds.getWidth() > 0.0f && padBounds.getHeight() > 0.0f) {
            float normX = (e.position.x - padBounds.getX()) / padBounds.getWidth();
            float normY = (e.position.y - padBounds.getY()) / padBounds.getHeight();

            // Seguimiento de velocidad para física inercial de suelta (SpringPhysics)
            float dNormX = (e.position.x - lastDragPos_.x) / padBounds.getWidth();
            float dNormY = (e.position.y - lastDragPos_.y) / padBounds.getHeight();
            velocityX_ = 0.6f * velocityX_ + 0.4f * dNormX;
            velocityY_ = 0.6f * velocityY_ + 0.4f * dNormY;
            lastDragPos_ = e.position;

            setTargetXY(normX, normY, true);
        }
    }

    void mouseUp(const juce::MouseEvent& /*e*/) override {
        isMouseDown_ = false;
        velocityX_ = std::clamp(velocityX_, -0.06f, 0.06f);
        velocityY_ = std::clamp(velocityY_, -0.06f, 0.06f);

        if (hasDragged_) {
            hasDragged_ = false;
            // Supresión estricta de clics fantasmas tras arrastre (Regla 48)
            return;
        }
    }

    void timerCallback() override {
        // 1. Automatización Cinética Autónoma (60 FPS continuo, Regla 49)
        if (!isMouseDown_) {
            switch (kineticMode_) {
                case KineticMode::Manual:
                    break;

                case KineticMode::Orbit: {
                    const float omega = 2.0f * juce::MathConstants<float>::pi * (0.08f + kineticRate_ * 0.72f) * (1.0f / 60.0f);
                    kineticPhase_ += omega;
                    if (kineticPhase_ > 2.0f * juce::MathConstants<float>::pi) kineticPhase_ -= 2.0f * juce::MathConstants<float>::pi;
                    const float r = 0.36f;
                    targetX_ = 0.5f + r * std::cos(kineticPhase_);
                    targetY_ = 0.5f + r * std::sin(kineticPhase_);
                    if (onMorphXYChanged_) onMorphXYChanged_(targetX_, targetY_);
                    break;
                }

                case KineticMode::Lissajous: {
                    const float omega = 2.0f * juce::MathConstants<float>::pi * (0.08f + kineticRate_ * 0.60f) * (1.0f / 60.0f);
                    kineticPhase_ += omega;
                    if (kineticPhase_ > 2.0f * juce::MathConstants<float>::pi) kineticPhase_ -= 2.0f * juce::MathConstants<float>::pi;
                    targetX_ = 0.5f + 0.38f * std::sin(kineticPhase_);
                    targetY_ = 0.5f + 0.34f * std::sin(kineticPhase_ * 2.0f + 0.5f);
                    if (onMorphXYChanged_) onMorphXYChanged_(targetX_, targetY_);
                    break;
                }

                case KineticMode::PingPong: {
                    if (std::abs(velocityX_) < 0.001f && std::abs(velocityY_) < 0.001f) {
                        velocityX_ = 0.006f * (0.5f + kineticRate_ * 0.8f);
                        velocityY_ = 0.0045f * (0.5f + kineticRate_ * 0.8f);
                    }
                    targetX_ += velocityX_;
                    targetY_ += velocityY_;
                    if (targetX_ <= 0.02f) { targetX_ = 0.02f; velocityX_ = std::abs(velocityX_); }
                    else if (targetX_ >= 0.98f) { targetX_ = 0.98f; velocityX_ = -std::abs(velocityX_); }
                    if (targetY_ <= 0.02f) { targetY_ = 0.02f; velocityY_ = std::abs(velocityY_); }
                    else if (targetY_ >= 0.98f) { targetY_ = 0.98f; velocityY_ = -std::abs(velocityY_); }
                    if (onMorphXYChanged_) onMorphXYChanged_(targetX_, targetY_);
                    break;
                }

                case KineticMode::SpringPhysics: {
                    targetX_ += velocityX_;
                    targetY_ += velocityY_;
                    // Rebote contra paredes con restitución elástica
                    if (targetX_ <= 0.01f) { targetX_ = 0.01f; velocityX_ = -velocityX_ * 0.85f; }
                    else if (targetX_ >= 0.99f) { targetX_ = 0.99f; velocityX_ = -velocityX_ * 0.85f; }
                    if (targetY_ <= 0.01f) { targetY_ = 0.01f; velocityY_ = -velocityY_ * 0.85f; }
                    else if (targetY_ >= 0.99f) { targetY_ = 0.99f; velocityY_ = -velocityY_ * 0.85f; }
                    // Amortiguamiento por fricción aerodinámica
                    velocityX_ *= 0.985f;
                    velocityY_ *= 0.985f;
                    if (std::abs(velocityX_) > 0.0001f || std::abs(velocityY_) > 0.0001f) {
                        if (onMorphXYChanged_) onMorphXYChanged_(targetX_, targetY_);
                    }
                    break;
                }
            }
        }

        // 2. Suavizado cinemático visual continuo (Anti-click Rule 35)
        currentX_ += smoothingSpeed_ * (targetX_ - currentX_);
        currentY_ += smoothingSpeed_ * (targetY_ - currentY_);

        // 3. Almacenamiento en buffer circular de estela cometaria (Zero Allocations Rule 9)
        auto pad = getPadBounds();
        if (pad.getWidth() > 0.0f && pad.getHeight() > 0.0f) {
            const float px = pad.getX() + currentX_ * pad.getWidth();
            const float py = pad.getY() + currentY_ * pad.getHeight();
            trailPoints_[trailHead_] = { px, py };
            trailHead_ = (trailHead_ + 1) % TrailLength;
        }

        repaint();
    }

    void paint(juce::Graphics& g) override {
        const auto& theme = ThemeManager::getInstance().getColors();
        auto bounds = getLocalBounds().toFloat();

        // 1. Chasis exterior estilo consola de modulación aeroespacial
        g.setColour(theme.panelSurface);
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(theme.borderMuted);
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

        // 2. Cabecera informativa y controles
        auto topBar = bounds.removeFromTop(24.0f).reduced(8.0f, 2.0f);
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.setColour(theme.accentPrimary);
        g.drawText(juce::CharPointer_UTF8("\xE2\x97\x88 2D VECTOR SCENE MORPH"), topBar.removeFromLeft(170.0f), juce::Justification::centredLeft, true);

        // Lectura de coordenadas X/Y
        juce::String coordsStr = juce::String::formatted("X: %.2f  |  Y: %.2f  |  GLIDE: %d%%",
            currentX_, currentY_, static_cast<int>(smoothingSpeed_ * 100.0f));
        g.setFont(juce::FontOptions(8.5f, juce::Font::plain));
        g.setColour(theme.textSecondary);
        g.drawText(coordsStr, topBar.removeFromLeft(190.0f), juce::Justification::centredLeft, true);

        // 3. Superficie del Pad Vectorial 2D
        auto padBounds = getPadBounds();
        paintPadSurface(g, padBounds);

        // 4. Panel lateral de Telemetría de los 8 Macros Globales
        auto macroArea = getMacroArea();
        paintMacroTelemetry(g, macroArea);
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(6, 4);
        auto header = bounds.removeFromTop(22);

        // Controles de cabecera a la derecha
        centerBtn_.setBounds(header.removeFromRight(54).withSizeKeepingCentre(52, 18));
        header.removeFromRight(4);
        randomBtn_.setBounds(header.removeFromRight(58).withSizeKeepingCentre(56, 18));
        header.removeFromRight(8);
        glideSlider_.setBounds(header.removeFromRight(74).withSizeKeepingCentre(70, 14));
        header.removeFromRight(8);
        kineticRateSlider_.setBounds(header.removeFromRight(64).withSizeKeepingCentre(60, 14));
        header.removeFromRight(4);
        kineticModeCombo_.setBounds(header.removeFromRight(106).withSizeKeepingCentre(102, 18));

        bounds.removeFromTop(4);

        // Posicionar botones de captura en las esquinas del Pad
        auto padBounds = getPadBounds();
        const int btnW = 44;
        const int btnH = 16;
        const int padMargin = 4;

        // Top-Left (A)
        capButtons_[0].setBounds(static_cast<int>(padBounds.getX()) + padMargin,
                                 static_cast<int>(padBounds.getY()) + padMargin, btnW, btnH);
        // Top-Right (B)
        capButtons_[1].setBounds(static_cast<int>(padBounds.getRight()) - btnW - padMargin,
                                 static_cast<int>(padBounds.getY()) + padMargin, btnW, btnH);
        // Bottom-Left (C)
        capButtons_[2].setBounds(static_cast<int>(padBounds.getX()) + padMargin,
                                 static_cast<int>(padBounds.getBottom()) - btnH - padMargin, btnW, btnH);
        // Bottom-Right (D)
        capButtons_[3].setBounds(static_cast<int>(padBounds.getRight()) - btnW - padMargin,
                                 static_cast<int>(padBounds.getBottom()) - btnH - padMargin, btnW, btnH);
    }

private:
    juce::Rectangle<float> getPadBounds() const {
        auto bounds = getLocalBounds().toFloat().reduced(8.0f, 6.0f);
        bounds.removeFromTop(24.0f);
        // Reservar espacio a la derecha para la tira de macros
        const float macroW = 210.0f;
        auto padArea = bounds.removeFromLeft(bounds.getWidth() - macroW);
        padArea.reduce(4.0f, 2.0f);
        return padArea;
    }

    juce::Rectangle<float> getMacroArea() const {
        auto bounds = getLocalBounds().toFloat().reduced(8.0f, 6.0f);
        bounds.removeFromTop(24.0f);
        const float macroW = 200.0f;
        auto area = bounds.removeFromRight(macroW);
        area.reduce(4.0f, 2.0f);
        return area;
    }

    void paintPadSurface(juce::Graphics& g, juce::Rectangle<float> pad) {
        if (pad.getWidth() <= 10.0f || pad.getHeight() <= 10.0f) return;

        // Fondo del Pad con cuadrícula oscura
        g.setColour(juce::Colour(0xff080b11));
        g.fillRoundedRectangle(pad, 4.0f);
        g.setColour(juce::Colour(0xff182030));
        g.drawRoundedRectangle(pad, 4.0f, 1.0f);

        // Cuadrantes y ejes centrales
        const float cx = pad.getCentreX();
        const float cy = pad.getCentreY();
        g.setColour(juce::Colour(0xff131924));
        g.drawHorizontalLine(static_cast<int>(cy), pad.getX(), pad.getRight());
        g.drawVerticalLine(static_cast<int>(cx), pad.getY(), pad.getBottom());

        // Colores temáticos de los 4 cuadrantes
        const juce::Colour colA(0xffffaa00); // Amber (Top-Left)
        const juce::Colour colB(0xff00d4ff); // Cyan (Top-Right)
        const juce::Colour colC(0xffd040ff); // Magenta (Bottom-Left)
        const juce::Colour colD(0xff00ff88); // Emerald (Bottom-Right)

        // Resplandor en las cuatro esquinas
        drawCornerGlow(g, pad.getX(), pad.getY(), colA, sceneCaptured_[0]);
        drawCornerGlow(g, pad.getRight(), pad.getY(), colB, sceneCaptured_[1]);
        drawCornerGlow(g, pad.getX(), pad.getBottom(), colC, sceneCaptured_[2]);
        drawCornerGlow(g, pad.getRight(), pad.getBottom(), colD, sceneCaptured_[3]);

        // Cálculo de pesos bilineales en tiempo real
        const float x = currentX_;
        const float y = currentY_;
        float wA = (1.0f - x) * (1.0f - y);
        float wB = x * (1.0f - y);
        float wC = (1.0f - x) * y;
        float wD = x * y;

        // Posición en píxeles del cursor / puck
        const float px = pad.getX() + x * pad.getWidth();
        const float py = pad.getY() + y * pad.getHeight();

        // Líneas guía vectoriales desde el puck a las 4 esquinas con alfa ponderado
        drawVectorTether(g, px, py, pad.getX(), pad.getY(), colA, wA);
        drawVectorTether(g, px, py, pad.getRight(), pad.getY(), colB, wB);
        drawVectorTether(g, px, py, pad.getX(), pad.getBottom(), colC, wC);
        drawVectorTether(g, px, py, pad.getRight(), pad.getBottom(), colD, wD);

        // Cruz reticular sutil hacia los bordes
        g.setColour(juce::Colour(0x3300d4ff));
        g.drawHorizontalLine(static_cast<int>(py), pad.getX(), pad.getRight());
        g.drawVerticalLine(static_cast<int>(px), pad.getY(), pad.getBottom());

        const juce::Colour puckAccent = colA.interpolatedWith(colB, x).interpolatedWith(colC.interpolatedWith(colD, x), y);

        // 1. Estela cometaria luminosa con partículas y halo difuso (Kinetic Orbit / Physics Trail)
        for (size_t i = 1; i < TrailLength; ++i) {
            size_t prevIdx = (trailHead_ + i - 1) % TrailLength;
            size_t currIdx = (trailHead_ + i) % TrailLength;
            auto p1 = trailPoints_[prevIdx];
            auto p2 = trailPoints_[currIdx];
            if (p1.x > 0.0f && p2.x > 0.0f && p1.getDistanceFrom(p2) < pad.getWidth() * 0.45f) {
                float progress = static_cast<float>(i) / static_cast<float>(TrailLength);
                g.setColour(puckAccent.withAlpha(progress * 0.35f));
                g.drawLine(p1.x, p1.y, p2.x, p2.y, 0.8f + progress * 2.2f);
            }
        }
        for (size_t i = 0; i < TrailLength; ++i) {
            size_t idx = (trailHead_ + i) % TrailLength;
            auto pt = trailPoints_[idx];
            if (pt.x <= 0.0f && pt.y <= 0.0f) continue;
            float progress = static_cast<float>(i) / static_cast<float>(TrailLength);
            float radius = 1.0f + progress * 3.2f;
            float alpha = progress * 0.45f;
            g.setColour(puckAccent.withAlpha(alpha));
            g.fillEllipse(pt.x - radius, pt.y - radius, radius * 2.0f, radius * 2.0f);
        }

        // Puck / Retículo 2D con resplandor radial
        {
            const float puckR = 9.0f;

            // Halo de resplandor exterior
            juce::ColourGradient halo(puckAccent.withAlpha(0.45f), px, py,
                                      puckAccent.withAlpha(0.0f), px + puckR * 2.2f, py + puckR * 2.2f, true);
            g.setGradientFill(halo);
            g.fillEllipse(px - puckR * 2.2f, py - puckR * 2.2f, puckR * 4.4f, puckR * 4.4f);

            // Corona exterior del puck
            g.setColour(juce::Colour(0xff0c1018));
            g.fillEllipse(px - puckR, py - puckR, puckR * 2.0f, puckR * 2.0f);
            g.setColour(puckAccent);
            g.drawEllipse(px - puckR, py - puckR, puckR * 2.0f, puckR * 2.0f, 1.6f);

            // Núcleo brillante
            g.setColour(juce::Colours::white);
            g.fillEllipse(px - 2.5f, py - 2.5f, 5.0f, 5.0f);
        }

        // Badges de porcentaje en las 4 esquinas
        paintCornerBadge(g, pad.getX() + 52.0f, pad.getY() + 6.0f, "A", wA, colA, sceneCaptured_[0]);
        paintCornerBadge(g, pad.getRight() - 96.0f, pad.getY() + 6.0f, "B", wB, colB, sceneCaptured_[1]);
        paintCornerBadge(g, pad.getX() + 52.0f, pad.getBottom() - 18.0f, "C", wC, colC, sceneCaptured_[2]);
        paintCornerBadge(g, pad.getRight() - 96.0f, pad.getBottom() - 18.0f, "D", wD, colD, sceneCaptured_[3]);
    }

    void drawCornerGlow(juce::Graphics& g, float cx, float cy, juce::Colour col, bool active) {
        const float r = 40.0f;
        const float alpha = active ? 0.22f : 0.06f;
        juce::ColourGradient grad(col.withAlpha(alpha), cx, cy, col.withAlpha(0.0f), cx + (cx > 0 ? -r : r), cy + (cy > 0 ? -r : r), true);
        g.setGradientFill(grad);
        g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
    }

    void drawVectorTether(juce::Graphics& g, float x1, float y1, float x2, float y2, juce::Colour col, float weight) {
        if (weight <= 0.01f) return;
        g.setColour(col.withAlpha(std::clamp(weight * 0.7f, 0.08f, 0.75f)));
        g.drawLine(x1, y1, x2, y2, std::clamp(weight * 2.2f, 0.6f, 2.4f));
    }

    void paintCornerBadge(juce::Graphics& g, float x, float y, const juce::String& name, float weight, juce::Colour col, bool captured) {
        auto badge = juce::Rectangle<float>(x, y, 40.0f, 13.0f);
        g.setColour(juce::Colour(0xff090c13).withAlpha(0.85f));
        g.fillRoundedRectangle(badge, 2.0f);
        g.setColour(col.withAlpha(captured ? 0.6f : 0.2f));
        g.drawRoundedRectangle(badge, 2.0f, 0.8f);

        juce::String txt = juce::String::formatted("%s: %d%%", name.toRawUTF8(), static_cast<int>(weight * 100.0f + 0.5f));
        g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
        g.setColour(captured ? col : juce::Colour(0xff606c80));
        g.drawText(txt, badge, juce::Justification::centred, false);
    }

    void paintMacroTelemetry(juce::Graphics& g, juce::Rectangle<float> area) {
        if (area.getWidth() <= 20.0f) return;

        // Chasis del panel de macros
        g.setColour(juce::Colour(0xff080b11));
        g.fillRoundedRectangle(area, 4.0f);
        g.setColour(juce::Colour(0xff182030));
        g.drawRoundedRectangle(area, 4.0f, 1.0f);

        auto inner = area.reduced(6.0f, 4.0f);

        // Cabecera de telemetría de macros
        auto titleArea = inner.removeFromTop(14.0f);
        g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff8c96a5));
        g.drawText(juce::CharPointer_UTF8("\xE2\x89\x8B GLOBAL MACROS TELEMETRY"), titleArea, juce::Justification::centredLeft, true);

        const char* macroNames[8] = {
            "TEXTURE", "MOTION", "SPACE", "COLOR",
            "CHAOS", "DENSITY", "ENERGY", "MORPH"
        };

        const float rowH = (inner.getHeight() - 4.0f) / 8.0f;

        for (size_t i = 0; i < 8; ++i) {
            auto row = inner.removeFromTop(rowH).reduced(0.0f, 1.0f);

            // Etiqueta del Macro
            auto labelArea = row.removeFromLeft(48.0f);
            g.setFont(juce::FontOptions(7.2f, juce::Font::bold));
            g.setColour(juce::Colour(0xff6e7a8e));
            g.drawText(macroNames[i], labelArea, juce::Justification::centredLeft, true);

            // Barra de nivel
            const float val = std::clamp(macroValues_[i], 0.0f, 1.0f);
            auto barArea = row.reduced(1.0f, 1.0f);

            g.setColour(juce::Colour(0xff101520));
            g.fillRoundedRectangle(barArea, 1.5f);

            auto fillArea = barArea.withWidth(barArea.getWidth() * val);
            if (fillArea.getWidth() > 1.0f) {
                juce::Colour barCol = (i == 7) ? juce::Colour(0xff00ff88) : juce::Colour(0xff00d4ff);
                g.setColour(barCol.withAlpha(0.85f));
                g.fillRoundedRectangle(fillArea, 1.5f);
            }

            // Lectura numérica porcentual
            g.setFont(juce::FontOptions(6.8f, juce::Font::plain));
            g.setColour(juce::Colour(0xffc5d0e0));
            juce::String valStr = juce::String::formatted("%d%%", static_cast<int>(val * 100.0f + 0.5f));
            g.drawText(valStr, row, juce::Justification::centredRight, false);
        }
    }

    // --- Variables de Estado ---
    float targetX_{ 0.5f };
    float targetY_{ 0.5f };
    float currentX_{ 0.5f };
    float currentY_{ 0.5f };
    float smoothingSpeed_{ 0.12f };

    KineticMode kineticMode_{ KineticMode::Manual };
    float kineticRate_{ 1.0f };
    float kineticPhase_{ 0.0f };
    float velocityX_{ 0.0f };
    float velocityY_{ 0.0f };
    juce::Point<float> lastDragPos_{};
    bool isMouseDown_{ false };

    static constexpr size_t TrailLength = 24;
    std::array<juce::Point<float>, TrailLength> trailPoints_{};
    size_t trailHead_{ 0 };

    bool sceneCaptured_[4]{ false, false, false, false };
    std::array<float, 8> macroValues_{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };

    // Máquina de estados Drag vs Click (Regla 48)
    bool hasDragged_{ false };
    juce::Point<float> dragStartPos_{};

    // Subcomponentes
    juce::TextButton centerBtn_;
    juce::TextButton randomBtn_;
    std::array<juce::TextButton, 4> capButtons_;
    juce::Slider glideSlider_;
    juce::ComboBox kineticModeCombo_;
    juce::Slider kineticRateSlider_;

    std::function<void(float, float)> onMorphXYChanged_;
    std::function<void(size_t)> onCaptureScene_;
    std::function<void(size_t)> onClearScene_;
    std::function<void()> onRandomizeRequested_;
    std::function<void(float)> onGlideSpeedChanged_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SceneMorphPadComponent)
};

} // namespace audio_graph
