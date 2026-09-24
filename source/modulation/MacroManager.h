#pragma once

#include <array>
#include <algorithm>
#include <cstdint>
#include "ModulationTypes.h"

namespace audio_graph {

/**
 * @brief Administrador de Macros Globales y XY Pad (Reglas 30 y 31)
 */
class MacroManager {
public:
    enum MacroIndex : size_t {
        Texture = 0,
        Motion = 1,
        Space = 2,
        Color = 3,
        Chaos = 4,
        Density = 5,
        Energy = 6,
        Morph = 7,
        Count = 8
    };

    MacroManager() {
        for (auto& m : macros_) m = 0.5f;
        for (auto& s : smoothedMacros_) s = 0.5f;
    }

    void reset() noexcept {
        for (size_t i = 0; i < Count; ++i) {
            smoothedMacros_[i] = macros_[i];
        }
        smoothedPadX_ = padX_;
        smoothedPadY_ = padY_;
    }

    void setMacro(MacroIndex idx, float value) noexcept {
        if (idx < Count) {
            macros_[idx] = std::clamp(value, 0.0f, 1.0f);
        }
    }

    float getMacro(MacroIndex idx) const noexcept {
        return (idx < Count) ? macros_[idx] : 0.0f;
    }

    float getSmoothedMacro(MacroIndex idx) const noexcept {
        return (idx < Count) ? smoothedMacros_[idx] : 0.0f;
    }

    void setPad(float x, float y) noexcept {
        padX_ = std::clamp(x, 0.0f, 1.0f);
        padY_ = std::clamp(y, 0.0f, 1.0f);
    }

    float getPadX() const noexcept { return padX_; }
    float getPadY() const noexcept { return padY_; }
    float getSmoothedPadX() const noexcept { return smoothedPadX_; }
    float getSmoothedPadY() const noexcept { return smoothedPadY_; }

    // Actualiza el suavizado anti-click de los macros y el XY Pad (Regla 35)
    void processSmoothing() noexcept {
        const float alpha = 0.01f;
        for (size_t i = 0; i < Count; ++i) {
            smoothedMacros_[i] += alpha * (macros_[i] - smoothedMacros_[i]);
        }
        smoothedPadX_ += alpha * (padX_ - smoothedPadX_);
        smoothedPadY_ += alpha * (padY_ - smoothedPadY_);
    }

private:
    std::array<float, Count> macros_{ 0.5f };
    std::array<float, Count> smoothedMacros_{ 0.5f };

    float padX_{ 0.5f };
    float padY_{ 0.5f };
    float smoothedPadX_{ 0.5f };
    float smoothedPadY_{ 0.5f };
};

} // namespace audio_graph
