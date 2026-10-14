#pragma once

#include <cmath>
#include <algorithm>
#include <span>
#include "BiquadFilter.h"
#include "../rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Crossover Linkwitz-Riley LR4 (24 dB/octava) estéreo de 3 bandas (Reglas 32, 33, 34, 46).
 * Delegado al Rust Core n8_rust_core con compensación de fase allpass en graves y suma plana 0 dB.
 */
class LinkwitzRileyCrossover3Way {
public:
    struct OutputSample {
        float low;
        float mid;
        float high;
    };

    LinkwitzRileyCrossover3Way() noexcept {
        rustCore_ = n8_rust_lr_crossover_create();
    }

    ~LinkwitzRileyCrossover3Way() noexcept {
        if (rustCore_) {
            n8_rust_lr_crossover_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    LinkwitzRileyCrossover3Way(const LinkwitzRileyCrossover3Way&) = delete;
    LinkwitzRileyCrossover3Way& operator=(const LinkwitzRileyCrossover3Way&) = delete;

    LinkwitzRileyCrossover3Way(LinkwitzRileyCrossover3Way&& other) noexcept : rustCore_(other.rustCore_) {
        other.rustCore_ = nullptr;
    }

    LinkwitzRileyCrossover3Way& operator=(LinkwitzRileyCrossover3Way&& other) noexcept {
        if (this != &other) {
            if (rustCore_) n8_rust_lr_crossover_destroy(rustCore_);
            rustCore_ = other.rustCore_;
            other.rustCore_ = nullptr;
        }
        return *this;
    }

    void prepare(double sampleRate) noexcept {
        if (rustCore_) {
            n8_rust_lr_crossover_prepare(rustCore_, sampleRate);
        }
    }

    void reset() noexcept {
        if (rustCore_) {
            n8_rust_lr_crossover_reset(rustCore_);
        }
    }

    void setCrossoverFrequencies(float fLowMidHz, float fMidHighHz) noexcept {
        if (rustCore_) {
            n8_rust_lr_crossover_set_crossover_frequencies(rustCore_, fLowMidHz, fMidHighHz);
        }
    }

    inline OutputSample processSample(int ch, float in) noexcept {
        OutputSample out{ 0.0f, 0.0f, 0.0f };
        if (rustCore_) {
            n8_rust_lr_crossover_process_sample(rustCore_, static_cast<size_t>(ch), in, &out.low, &out.mid, &out.high);
        }
        return out;
    }

    void processBlock(const float* const* inChannels,
                      float* const* lowChannels,
                      float* const* midChannels,
                      float* const* highChannels,
                      size_t numSamples) noexcept {
        if (!rustCore_) return;
        const float* inL = (inChannels != nullptr) ? inChannels[0] : nullptr;
        const float* inR = (inChannels != nullptr) ? inChannels[1] : nullptr;
        float* lowL = (lowChannels != nullptr) ? lowChannels[0] : nullptr;
        float* lowR = (lowChannels != nullptr) ? lowChannels[1] : nullptr;
        float* midL = (midChannels != nullptr) ? midChannels[0] : nullptr;
        float* midR = (midChannels != nullptr) ? midChannels[1] : nullptr;
        float* highL = (highChannels != nullptr) ? highChannels[0] : nullptr;
        float* highR = (highChannels != nullptr) ? highChannels[1] : nullptr;

        n8_rust_lr_crossover_process_block(
            rustCore_,
            inL, inR,
            lowL, lowR,
            midL, midR,
            highL, highR,
            numSamples
        );
    }

    RustLinkwitzRileyCrossover3Way* getRustCrossover() noexcept { return rustCore_; }
    const RustLinkwitzRileyCrossover3Way* getRustCrossover() const noexcept { return rustCore_; }

private:
    RustLinkwitzRileyCrossover3Way* rustCore_{ nullptr };
};

} // namespace audio_graph
