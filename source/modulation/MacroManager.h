#pragma once

#include <array>
#include <algorithm>
#include <cstdint>
#include "ModulationTypes.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Administrador de Macros Globales y XY Pad (Reglas 30 y 31)
 * Delega el almacenamiento y el suavizado anti-click al núcleo Rust.
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

    explicit MacroManager(RustMacroManager* existing = nullptr, bool owns = true)
        : rustCore_(existing ? existing : n8_rust_macro_manager_create()), ownsCore_(owns)
    {
    }

    ~MacroManager() {
        if (rustCore_ && ownsCore_) {
            n8_rust_macro_manager_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    MacroManager(const MacroManager&) = delete;
    MacroManager& operator=(const MacroManager&) = delete;

    MacroManager(MacroManager&& other) noexcept
        : rustCore_(other.rustCore_), ownsCore_(other.ownsCore_)
    {
        other.rustCore_ = nullptr;
        other.ownsCore_ = false;
    }

    MacroManager& operator=(MacroManager&& other) noexcept {
        if (this != &other) {
            if (rustCore_ && ownsCore_) {
                n8_rust_macro_manager_destroy(rustCore_);
            }
            rustCore_ = other.rustCore_;
            ownsCore_ = other.ownsCore_;
            other.rustCore_ = nullptr;
            other.ownsCore_ = false;
        }
        return *this;
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_macro_manager_reset(rustCore_);
        }
    }

    void setMacro(MacroIndex idx, float value) noexcept {
        if (rustCore_) {
            n8_rust_macro_manager_set_macro(rustCore_, static_cast<size_t>(idx), value);
        }
    }

    float getMacro(MacroIndex idx) const noexcept {
        if (rustCore_) {
            return n8_rust_macro_manager_get_macro(rustCore_, static_cast<size_t>(idx));
        }
        return 0.0f;
    }

    float getSmoothedMacro(MacroIndex idx) const noexcept {
        if (rustCore_) {
            return n8_rust_macro_manager_get_smoothed_macro(rustCore_, static_cast<size_t>(idx));
        }
        return 0.0f;
    }

    void setPad(float x, float y) noexcept {
        if (rustCore_) {
            n8_rust_macro_manager_set_pad(rustCore_, x, y);
        }
    }

    float getPadX() const noexcept {
        if (rustCore_) {
            return n8_rust_macro_manager_get_pad_x(rustCore_);
        }
        return 0.5f;
    }

    float getPadY() const noexcept {
        if (rustCore_) {
            return n8_rust_macro_manager_get_pad_y(rustCore_);
        }
        return 0.5f;
    }

    float getSmoothedPadX() const noexcept {
        if (rustCore_) {
            return n8_rust_macro_manager_get_smoothed_pad_x(rustCore_);
        }
        return 0.5f;
    }

    float getSmoothedPadY() const noexcept {
        if (rustCore_) {
            return n8_rust_macro_manager_get_smoothed_pad_y(rustCore_);
        }
        return 0.5f;
    }

    void processSmoothing() noexcept {
        if (rustCore_) {
            n8_rust_macro_manager_process_smoothing(rustCore_);
        }
    }

private:
    RustMacroManager* rustCore_{ nullptr };
    bool ownsCore_{ true };
};

} // namespace audio_graph
