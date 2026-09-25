#pragma once

#include <cmath>
#include <algorithm>

namespace audio_graph {

/**
 * @brief Modulador de Caos No Lineal Continuo basado en el Atractor de Lorenz (Reglas 7, 8, 25, 46, 47)
 * Genera 3 señales dinámicas correlacionadas pero perpetuamente no repetitivas (X, Y, Z).
 */
class ChaosModulator {
public:
    ChaosModulator() {
        reset();
    }

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 44100.0;
        reset();
    }

    void reset() noexcept {
        x_ = 0.1f;
        y_ = 0.0f;
        z_ = 0.0f;
        normX_ = 0.0f;
        normY_ = 0.0f;
        normZ_ = 0.0f;
    }

    void setSpeed(float hz) noexcept {
        speedHz_ = std::clamp(hz, 0.01f, 30.0f);
    }

    void processSample() noexcept {
        // Constantes canónicas del Atractor Caótico de Lorenz
        constexpr float sigma = 10.0f;
        constexpr float rho   = 28.0f;
        constexpr float beta  = 8.0f / 3.0f;

        const float dt = (speedHz_ * 0.005f) / static_cast<float>(sampleRate_ > 0.0 ? sampleRate_ : 44100.0);

        // Integración numérica Runge-Kutta de 4º Orden (RK4) para máxima estabilidad numérica
        auto derivs = [](float x, float y, float z, float& dx, float& dy, float& dz) noexcept {
            dx = sigma * (y - x);
            dy = x * (rho - z) - y;
            dz = x * y - beta * z;
        };

        float k1x, k1y, k1z;
        derivs(x_, y_, z_, k1x, k1y, k1z);

        float k2x, k2y, k2z;
        derivs(x_ + 0.5f * dt * k1x, y_ + 0.5f * dt * k1y, z_ + 0.5f * dt * k1z, k2x, k2y, k2z);

        float k3x, k3y, k3z;
        derivs(x_ + 0.5f * dt * k2x, y_ + 0.5f * dt * k2y, z_ + 0.5f * dt * k2z, k3x, k3y, k3z);

        float k4x, k4y, k4z;
        derivs(x_ + dt * k3x, y_ + dt * k3y, z_ + dt * k3z, k4x, k4y, k4z);

        x_ += (dt / 6.0f) * (k1x + 2.0f * k2x + 2.0f * k3x + k4x);
        y_ += (dt / 6.0f) * (k1y + 2.0f * k2y + 2.0f * k3y + k4y);
        z_ += (dt / 6.0f) * (k1z + 2.0f * k2z + 2.0f * k3z + k4z);

        // Protección contra NaNs / Infs y divergencia extrema
        if (!std::isfinite(x_) || !std::isfinite(y_) || !std::isfinite(z_) ||
            std::abs(x_) > 100.0f || std::abs(y_) > 100.0f || std::abs(z_) > 120.0f) {
            reset();
        }

        // Normalización bipolar a [-1.0, +1.0]
        normX_ = std::clamp(x_ / 20.0f, -1.0f, 1.0f);
        normY_ = std::clamp(y_ / 30.0f, -1.0f, 1.0f);
        normZ_ = std::clamp((z_ - 25.0f) / 25.0f, -1.0f, 1.0f);
    }

    float getNormalizedX() const noexcept { return normX_; }
    float getNormalizedY() const noexcept { return normY_; }
    float getNormalizedZ() const noexcept { return normZ_; }

private:
    double sampleRate_{ 44100.0 };
    float speedHz_{ 1.0f };

    float x_{ 0.1f };
    float y_{ 0.0f };
    float z_{ 0.0f };

    float normX_{ 0.0f };
    float normY_{ 0.0f };
    float normZ_{ 0.0f };
};

} // namespace audio_graph
