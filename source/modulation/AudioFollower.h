#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Seguidor de envolvente de audio (Audio Follower) para modulación dinámica (Reglas 7 y 46)
 * Delega toda la detección balística de picos al núcleo Rust de alto rendimiento.
 */
class AudioFollower {
public:
    explicit AudioFollower(RustAudioFollower* existing = nullptr, bool owns = true)
        : rustCore_(existing ? existing : n8_rust_audio_follower_create()), ownsCore_(owns)
    {
    }

    ~AudioFollower() {
        if (rustCore_ && ownsCore_) {
            n8_rust_audio_follower_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    AudioFollower(const AudioFollower&) = delete;
    AudioFollower& operator=(const AudioFollower&) = delete;

    AudioFollower(AudioFollower&& other) noexcept
        : rustCore_(other.rustCore_), ownsCore_(other.ownsCore_)
    {
        other.rustCore_ = nullptr;
        other.ownsCore_ = false;
    }

    AudioFollower& operator=(AudioFollower&& other) noexcept {
        if (this != &other) {
            if (rustCore_ && ownsCore_) {
                n8_rust_audio_follower_destroy(rustCore_);
            }
            rustCore_ = other.rustCore_;
            ownsCore_ = other.ownsCore_;
            other.rustCore_ = nullptr;
            other.ownsCore_ = false;
        }
        return *this;
    }

    void prepare(double sampleRate) noexcept {
        if (rustCore_) {
            n8_rust_audio_follower_prepare(rustCore_, sampleRate);
        }
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_audio_follower_reset(rustCore_);
        }
    }

    void setAttackMs(float ms) noexcept {
        if (rustCore_) {
            n8_rust_audio_follower_set_attack_ms(rustCore_, ms);
        }
    }

    void setReleaseMs(float ms) noexcept {
        if (rustCore_) {
            n8_rust_audio_follower_set_release_ms(rustCore_, ms);
        }
    }

    float processBlock(const float* const* input, uint32_t numChannels, uint32_t numSamples) noexcept {
        if (rustCore_) {
            return n8_rust_audio_follower_process_block(rustCore_, input, numChannels, numSamples);
        }
        return 0.0f;
    }

    float getCurrentValue() const noexcept {
        if (rustCore_) {
            return n8_rust_audio_follower_get_current_value(rustCore_);
        }
        return 0.0f;
    }

private:
    RustAudioFollower* rustCore_{ nullptr };
    bool ownsCore_{ true };
};

} // namespace audio_graph
