#pragma once

#include <cmath>
#include <algorithm>
#include "EventTypes.h"
#include "AudioFragment.h"
#include "../dsp/core/SpatialPannerCore.h"

namespace audio_graph {

/**
 * @brief Entidad de Evento individual con ciclo de vida explícito (Reglas 2, 3, 18, 46)
 */
class Event {
public:
    Event() = default;

    void initialize(const EventAttributes& attrs, double captureOffset, uint64_t durationSamples, bool reverse = false) {
        attrs_ = attrs;
        attrs_.state = EventLifecycle::Created;
        attrs_.ageSamples = 0;
        attrs_.lifetimeSamples = durationSamples;

        fragment_.configure(captureOffset, durationSamples, attrs.pitchRatio, reverse);
        spatialPanner_.prepare(44100.0);
        spatialPanner_.setCoordinates(attrs.azimuth, attrs.elevation, attrs.distance);
        currentEnvelope_ = 0.0f;
        isActive_ = true;
    }

    void start() noexcept {
        if (attrs_.state == EventLifecycle::Created) {
            attrs_.state = EventLifecycle::Playing;
        }
    }

    void kill() noexcept {
        attrs_.state = EventLifecycle::Killed;
        isActive_ = false;
    }

    void release() noexcept {
        if (attrs_.state == EventLifecycle::Playing || attrs_.state == EventLifecycle::Frozen || attrs_.state == EventLifecycle::Looping) {
            attrs_.state = EventLifecycle::Releasing;
        }
    }

    // Renderiza muestras de audio del evento y gestiona su ciclo de vida (Reglas 2, 3, 9)
    void render(const EventCaptureBuffer& capture, float* outL, float* outR, uint32_t numSamples, float sourceLevel) noexcept {
        if (!isActive_ || attrs_.state == EventLifecycle::Killed || attrs_.state == EventLifecycle::Destroyed) {
            return;
        }

        // Evaluar dependencia de la fuente (Regla 3: Source Following)
        const bool sourceIsSilent = (sourceLevel < 1e-4f);
        if (sourceIsSilent && attrs_.sourceFollow > 0.05f) {
            handleSourceDisappearance();
        }

        // Factores de paneo de ley de potencia constante (-3dB pan law)
        const float panClamped = std::clamp(attrs_.pan, -1.0f, 1.0f);
        const float panAngle = (panClamped + 1.0f) * 0.25f * std::numbers::pi_v<float>;
        const float panGainL = std::cos(panAngle) * attrs_.gain;
        const float panGainR = std::sin(panAngle) * attrs_.gain;

        for (uint32_t s = 0; s < numSamples; ++s) {
            if (attrs_.state == EventLifecycle::Killed || attrs_.state == EventLifecycle::Destroyed) {
                isActive_ = false;
                break;
            }

            // Cálculo explícito de envolvente según el estado del ciclo de vida (Regla 2)
            updateEnvelope();

            float sampleL = 0.0f;
            float sampleR = 0.0f;
            bool hasMore = fragment_.getNextSample(capture, sampleL, sampleR, currentEnvelope_);

            if (!hasMore && attrs_.state != EventLifecycle::Looping && attrs_.state != EventLifecycle::Frozen) {
                if (attrs_.state == EventLifecycle::Playing) {
                    attrs_.state = EventLifecycle::Releasing;
                } else if (attrs_.state == EventLifecycle::Releasing && currentEnvelope_ <= 0.001f) {
                    attrs_.state = EventLifecycle::Killed;
                    isActive_ = false;
                    break;
                }
            }

            // Aplicar ganancia efectiva considerando sourceFollow
            float effectiveGain = 1.0f;
            if (attrs_.sourceFollow > 0.0f) {
                // Interpola entre 1.0 (autónomo) y sourceLevel (dependiente)
                effectiveGain = (1.0f - attrs_.sourceFollow) + attrs_.sourceFollow * std::clamp(sourceLevel * 2.0f, 0.0f, 1.0f);
            }

            const bool has3d = (std::abs(attrs_.azimuth) > 0.01f || std::abs(attrs_.elevation) > 0.01f || std::abs(attrs_.distance - 1.0f) > 0.01f);
            if (has3d) {
                float spatL = 0.0f;
                float spatR = 0.0f;
                spatialPanner_.processSample(sampleL, sampleR, spatL, spatR);
                outL[s] += spatL * attrs_.gain * effectiveGain;
                outR[s] += spatR * attrs_.gain * effectiveGain;
            } else {
                outL[s] += sampleL * panGainL * effectiveGain;
                outR[s] += sampleR * panGainR * effectiveGain;
            }

            attrs_.ageSamples++;
            if (attrs_.lifetimeSamples > 0 && attrs_.ageSamples >= attrs_.lifetimeSamples && attrs_.state == EventLifecycle::Playing) {
                attrs_.state = EventLifecycle::Releasing;
            }
        }
    }

    bool isActive() const noexcept { return isActive_; }
    const EventAttributes& getAttributes() const noexcept { return attrs_; }
    EventAttributes& getAttributes() noexcept { return attrs_; }
    EventLifecycle getState() const noexcept { return attrs_.state; }
    void setState(EventLifecycle state) noexcept { attrs_.state = state; }

private:
    void handleSourceDisappearance() noexcept {
        switch (attrs_.disappearanceMode) {
            case SourceDisappearanceMode::Cut:
                attrs_.state = EventLifecycle::Killed;
                isActive_ = false;
                break;
            case SourceDisappearanceMode::Short:
                if (attrs_.lifetimeSamples > attrs_.ageSamples + 256) {
                    attrs_.lifetimeSamples = attrs_.ageSamples + 256;
                }
                break;
            case SourceDisappearanceMode::Fade:
                if (attrs_.state == EventLifecycle::Playing) {
                    attrs_.state = EventLifecycle::Releasing;
                }
                break;
            case SourceDisappearanceMode::Hold:
                // Permanece en el nivel actual sin avanzar liberación
                break;
            case SourceDisappearanceMode::Freeze:
                attrs_.state = EventLifecycle::Frozen;
                break;
            case SourceDisappearanceMode::Natural:
            default:
                // Continúa su ciclo de vida natural (Regla 18: Tails independientes)
                break;
        }
    }

    void updateEnvelope() noexcept {
        const float attackStep = 1.0f / static_cast<float>(std::max<uint32_t>(1, attrs_.attackSamples));
        const float releaseStep = 1.0f / static_cast<float>(std::max<uint32_t>(1, attrs_.releaseSamples));

        switch (attrs_.state) {
            case EventLifecycle::Created:
            case EventLifecycle::Playing:
            case EventLifecycle::Looping:
            case EventLifecycle::Frozen:
                if (currentEnvelope_ < 1.0f) {
                    currentEnvelope_ = std::min(1.0f, currentEnvelope_ + attackStep);
                }
                break;

            case EventLifecycle::Releasing:
                if (currentEnvelope_ > 0.0f) {
                    currentEnvelope_ = std::max(0.0f, currentEnvelope_ - releaseStep);
                }
                if (currentEnvelope_ <= 0.0001f) {
                    attrs_.state = EventLifecycle::Killed;
                    isActive_ = false;
                }
                break;

            case EventLifecycle::Killed:
            case EventLifecycle::Destroyed:
                currentEnvelope_ = 0.0f;
                isActive_ = false;
                break;
        }
    }

    EventAttributes attrs_;
    AudioFragment fragment_;
    SpatialPannerCore spatialPanner_;
    float currentEnvelope_{ 0.0f };
    bool isActive_{ false };
};

} // namespace audio_graph
