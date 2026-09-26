#pragma once

#include <cmath>
#include <algorithm>
#include <array>
#include "DenormalGuards.h"
#include "FastMath.h"

namespace audio_graph {

/**
 * @brief Modelo de saturación de histéresis magnética no lineal (Jiles-Atherton discretizado) (Reglas 5, 8, 34, 46, 47).
 * Emula la memoria física ferromagnética de las cintas analógicas de audio:
 * - Fuerza coercitiva y remanencia magnética (curva B-H con ciclo de histéresis abierto).
 * - Compresión suave asimétrica y saturación progresiva de partículas magnéticas.
 * - Función anhisterética de Langevin optimizada mediante aproximación racional C^inf sin trascendentes.
 * - Cero alocaciones dinámicas y protección estricta contra denormales y singularidades.
 */
class MagneticHysteresis {
public:
    MagneticHysteresis() noexcept {
        reset();
    }

    void reset() noexcept {
        for (auto& s : state_) {
            s.M = 0.0f;
            s.H_prev = 0.0f;
            s.Man_prev = 0.0f;
        }
    }

    /**
     * @brief Configura los parámetros del material ferromagnético.
     * @param drive Ganancia de excitación magnética (1.0 a 10.0)
     * @param hysteresisAmount Intensidad de memoria de histéresis / coercitividad (0.0 = saturación pura, 1.0 = máxima histéresis)
     * @param tapeSpeedFactor 0.5f para 7.5 ips, 1.0f para 15 ips, 1.5f para 30 ips
     */
    void setParameters(float drive, float hysteresisAmount, float tapeSpeedFactor = 1.0f) noexcept {
        drive_ = std::clamp(drive, 0.5f, 12.0f);
        hysteresis_ = std::clamp(hysteresisAmount, 0.0f, 1.0f);

        // Coercitividad k: ancho del lazo de histéresis dependiente de la velocidad de cinta
        k_ = (0.25f + 0.45f * hysteresis_) / std::max(0.5f, tapeSpeedFactor);
        // alpha: acoplamiento de campo medio entre dominios magnéticos
        alpha_ = 0.0015f + 0.003f * hysteresis_;
        // c: coeficiente de reversibilidad de paredes de Bloch (0.1 a 0.3)
        c_ = 0.22f - 0.08f * hysteresis_;
        // a: parámetro de anhisteresis de escala de campo
        a_ = 0.95f;
    }

    /**
     * @brief Procesa una muestra estéreo a través del simulador de histéresis magnética.
     */
    void processSample(float inL, float inR, float& outL, float& outR) noexcept {
        outL = processChannel(0, inL);
        outR = processChannel(1, inR);
    }

    float processChannel(size_t ch, float input) noexcept {
        if (ch >= 2) return input;
        ChannelState& s = state_[ch];

        // Campo magnético H de excitación proporcional al input y drive
        const float H = input * drive_;
        const float dH = H - s.H_prev;

        if (std::abs(dH) < 1.0e-7f) {
            // Sin cambio de campo magnético significativo: devolver magnetización actual
            return FastMath::flushDenormal(s.M);
        }

        const float delta = (dH > 0.0f) ? 1.0f : -1.0f;
        const float He = H + alpha_ * s.M;

        // Función anhisterética de Langevin racional suave: L(x) = x / sqrt(9 + x^2)
        const float x = He / a_;
        const float Man = Ms_ * (x / std::sqrt(9.0f + x * x));

        const float dMan = Man - s.Man_prev;
        const float dMan_dH = dMan / (dH + (dH >= 0.0f ? 1.0e-9f : -1.0e-9f));

        // Ecuación diferencial de Jiles-Atherton para dM/dH
        const float diff = Man - s.M;
        float dM = 0.0f;

        if (delta * diff > 0.0f) {
            // Movimiento irreversible de dominios magnéticos (fricción en defectos de red)
            float denom = delta * k_ - alpha_ * diff;
            const float minDenom = 0.15f * k_;
            if (std::abs(denom) < minDenom) {
                denom = (denom >= 0.0f ? minDenom : -minDenom);
            }

            const float dM_irr = (diff / denom) * (1.0f - c_);
            dM = (dM_irr + c_ * dMan_dH) * dH;
        } else {
            // Movimiento reversible únicamente (curvatura elástica de paredes de dominio)
            dM = c_ * dMan_dH * dH;
        }

        // Bounded integration step para evitar runaway numérico (Regla 47)
        const float maxStep = 0.5f;
        dM = std::clamp(dM, -maxStep, maxStep);
        s.M = std::clamp(s.M + dM, -Ms_, Ms_);

        s.H_prev = H;
        s.Man_prev = Man;

        // Salida: combinación normalizada de magnetización M y aporte directo
        const float wetMagnetic = s.M * 0.95f;
        // Mezcla suave con saturación sigmoidal estándar según parámetro de histéresis
        const float staticSat = FastMath::fastTanh(H * 0.7f);
        const float outSignal = (1.0f - hysteresis_) * staticSat + hysteresis_ * wetMagnetic;

        return FastMath::flushDenormal(outSignal);
    }

private:
    struct ChannelState {
        float M{ 0.0f };
        float H_prev{ 0.0f };
        float Man_prev{ 0.0f };
    };

    std::array<ChannelState, 2> state_;

    float drive_{ 2.0f };
    float hysteresis_{ 0.5f };
    float Ms_{ 1.25f };   // Magnetización de saturación máxima
    float k_{ 0.45f };    // Coercitividad / pinned domain density
    float alpha_{ 0.002f };// Coupling constante
    float c_{ 0.18f };    // Reversibilidad
    float a_{ 0.95f };    // Escala de campo anhisterético
};

} // namespace audio_graph
