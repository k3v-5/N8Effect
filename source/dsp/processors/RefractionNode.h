#pragma once

#include <cmath>
#include <array>
#include <span>
#include <algorithm>
#include <numbers>
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/DelayLine.h"
#include "../core/BiquadFilter.h"
#include "../core/FastMath.h"
#include "../core/DenormalGuards.h"

namespace audio_graph {

/**
 * @brief Polyphonic Refraction Dispersion Engine (Reglas 5, 8, 9, 14, 18, 45, 46, 47).
 * Divide la señal estéreo en 2 a 8 voces refractadas polifónicas con micro-desafinación (detune),
 * retardo escalonado Haas anti-peinado, paneo de potencia constante estéreo, banco de dispersión armónica resonante
 * y normalización de energía de voces 1/sqrt(N).
 */
class RefractionNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        VoiceCount = 1,        // 2.0 a 8.0 voces (default: 4.0)
        Detune = 2,            // 0.0 a 100.0 cents (default: 18.0)
        DelaySpreadMs = 3,     // 0.0 a 50.0 ms (default: 15.0)
        StereoSpread = 4,      // 0.0 a 1.0 (default: 0.85)
        HarmonicDisp = 5,      // 0.0 a 2.0 (default: 0.5)
        Resonance = 6,         // 0.1 a 10.0 Q (default: 1.2)
        DryWet = 7             // 0.0 a 1.0 (default: 0.5)
    };

    static constexpr size_t MaxVoices = 8;

    RefractionNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { VoiceCount, "Voices", 4.0f, 2.0f, 8.0f, false };
        params_[1] = { Detune, "Detune", 18.0f, 0.0f, 100.0f, true };
        params_[2] = { DelaySpreadMs, "Delay Spread", 15.0f, 0.0f, 50.0f, true };
        params_[3] = { StereoSpread, "Stereo Spread", 0.85f, 0.0f, 1.0f, true };
        params_[4] = { HarmonicDisp, "Harmonic", 0.5f, 0.0f, 2.0f, true };
        params_[5] = { Resonance, "Resonance", 1.2f, 0.1f, 10.0f, true };
        params_[6] = { DryWet, "Mix", 0.5f, 0.0f, 1.0f, true };

        for (size_t v = 0; v < MaxVoices; ++v) {
            lfoPhases_[v] = static_cast<float>(v) * (2.0f * std::numbers::pi_v<float> / static_cast<float>(MaxVoices));
        }
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        const double sr = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

        // Máximo retardo de 120ms para escalonamiento Haas y modulación LFO
        const size_t maxDelay = static_cast<size_t>(sr * 0.15);
        for (size_t v = 0; v < MaxVoices; ++v) {
            voiceDelaysL_[v].prepare(maxDelay);
            voiceDelaysR_[v].prepare(maxDelay);
            filtersL_[v].reset();
            filtersR_[v].reset();
            lfoPhases_[v] = static_cast<float>(v) * (2.0f * std::numbers::pi_v<float> / static_cast<float>(MaxVoices));
        }

        reset();
        updateFilters();
    }

    void reset() override {
        for (size_t v = 0; v < MaxVoices; ++v) {
            voiceDelaysL_[v].reset();
            voiceDelaysR_[v].reset();
            filtersL_[v].reset();
            filtersR_[v].reset();
            lfoPhases_[v] = static_cast<float>(v) * (2.0f * std::numbers::pi_v<float> / static_cast<float>(MaxVoices));
        }
    }

    void process(ProcessContext& context) override {
        ScopedDenormalGuard guard;
        updateFilters();

        const uint32_t numSamples = context.numSamples;
        if (numSamples == 0) return;

        const float sr = static_cast<float>(spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0);
        const float mix = targetMix_;
        const float detuneCents = targetDetune_;
        const float delaySpreadMs = targetDelaySpread_;
        const float stereoSpread = targetStereoSpread_;

        const int numVoices = std::clamp(static_cast<int>(std::round(targetVoices_)), 2, static_cast<int>(MaxVoices));
        const float normEnergy = 1.0f / std::sqrt(static_cast<float>(numVoices));

        // Parámetros de modulación por voz
        struct VoiceConfig {
            float baseDelaySamples{ 0.0f };
            float modDepthSamples{ 0.0f };
            float lfoPhaseInc{ 0.0f };
            float panL{ 0.7071f };
            float panR{ 0.7071f };
        };

        std::array<VoiceConfig, MaxVoices> configs;
        for (int v = 0; v < numVoices; ++v) {
            const float u = (numVoices > 1)
                ? (2.0f * static_cast<float>(v) - static_cast<float>(numVoices - 1)) / static_cast<float>(numVoices - 1)
                : 0.0f;

            // Retardo escalonado Haas anti-peinado: 2.0ms base + spread proporcional
            const float baseDelayMs = 2.0f + delaySpreadMs * (static_cast<float>(v) / static_cast<float>(numVoices - 1));
            configs[v].baseDelaySamples = (baseDelayMs * 0.001f) * sr;

            // Profundidad de micro-desafinación modulada
            configs[v].modDepthSamples = (detuneCents * 0.01f) * (0.00075f * sr);

            // Frecuencias de LFO ligeramente descorrelacionadas
            const float lfoFreqHz = 0.3f * (1.0f + 0.18f * u);
            configs[v].lfoPhaseInc = 2.0f * std::numbers::pi_v<float> * lfoFreqHz / sr;

            // Paneo de potencia constante estéreo
            const float panAngle = (std::numbers::pi_v<float> * 0.25f) * (1.0f + u * stereoSpread);
            configs[v].panL = std::cos(panAngle);
            configs[v].panR = std::sin(panAngle);
        }

        const float* inL = (context.numInputChannels > 0 && context.inputChannels[0] != nullptr) ? context.inputChannels[0] : nullptr;
        const float* inR = (context.numInputChannels > 1 && context.inputChannels[1] != nullptr) ? context.inputChannels[1] : inL;

        float* outL = (context.numOutputChannels > 0) ? context.outputChannels[0] : nullptr;
        float* outR = (context.numOutputChannels > 1) ? context.outputChannels[1] : nullptr;

        for (uint32_t s = 0; s < numSamples; ++s) {
            const float sampleL = inL ? inL[s] : 0.0f;
            const float sampleR = inR ? inR[s] : sampleL;

            float wetAccumL = 0.0f;
            float wetAccumR = 0.0f;

            for (int v = 0; v < numVoices; ++v) {
                // Escribir en las líneas de retardo de la voz
                voiceDelaysL_[v].write(sampleL);
                voiceDelaysR_[v].write(sampleR);

                // Modulación senoidal rápida de retardo
                const float modDelay = configs[v].baseDelaySamples + configs[v].modDepthSamples * FastMath::fastSin(lfoPhases_[v]);
                const float safeDelay = std::max(2.0f, modDelay);

                // Lectura con interpolación cúbica Hermite libre de aliasing
                const float readL = voiceDelaysL_[v].readCubic(safeDelay);
                const float readR = voiceDelaysR_[v].readCubic(safeDelay);

                // Dispersión armónica resonante (Filtro paso-banda + señal directa con cuerpo)
                const float filteredL = filtersL_[v].processSample(readL);
                const float filteredR = filtersR_[v].processSample(readR);

                const float voiceL = readL * 0.65f + filteredL * 0.65f;
                const float voiceR = readR * 0.65f + filteredR * 0.65f;

                // Acumulación con paneo y normalización de energía
                const float voicePannedL = voiceL * configs[v].panL;
                const float voicePannedR = voiceR * configs[v].panR;

                wetAccumL += voicePannedL * normEnergy;
                wetAccumR += voicePannedR * normEnergy;

                // Avanzar fase LFO
                lfoPhases_[v] += configs[v].lfoPhaseInc;
                if (lfoPhases_[v] >= 2.0f * std::numbers::pi_v<float>) {
                    lfoPhases_[v] -= 2.0f * std::numbers::pi_v<float>;
                }
            }

            // Limitador suave para protección analógica
            const float wetFinalL = FastMath::fastTanh(wetAccumL);
            const float wetFinalR = FastMath::fastTanh(wetAccumR);

            if (outL) outL[s] = sampleL * (1.0f - mix) + wetFinalL * mix;
            if (outR) outR[s] = sampleR * (1.0f - mix) + wetFinalR * mix;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case VoiceCount: targetVoices_ = std::clamp(value, 2.0f, 8.0f); break;
            case Detune: targetDetune_ = std::clamp(value, 0.0f, 100.0f); break;
            case DelaySpreadMs: targetDelaySpread_ = std::clamp(value, 0.0f, 50.0f); break;
            case StereoSpread: targetStereoSpread_ = std::clamp(value, 0.0f, 1.0f); break;
            case HarmonicDisp: targetHarmonicDisp_ = std::clamp(value, 0.0f, 2.0f); break;
            case Resonance: targetResonance_ = std::clamp(value, 0.1f, 10.0f); break;
            case DryWet: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
        }
    }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case VoiceCount: return targetVoices_;
            case Detune: return targetDetune_;
            case DelaySpreadMs: return targetDelaySpread_;
            case StereoSpread: return targetStereoSpread_;
            case HarmonicDisp: return targetHarmonicDisp_;
            case Resonance: return targetResonance_;
            case DryWet: return targetMix_;
            default: return 0.0f;
        }
    }

    NodeType getType() const override { return NodeType::Refraction; }
    const char* getName() const override { return "Polyphonic Refraction"; }
    bool supportsTail() const override { return true; }
    uint32_t getTailSamples() const override {
        return static_cast<uint32_t>(spec_.sampleRate * 0.5); // 500ms
    }

    std::span<const PinDescriptor> getPins() const override { return pins_; }
    std::span<const ParameterInfo> getParameters() const override { return params_; }

private:
    void updateFilters() noexcept {
        const double sr = spec_.sampleRate > 0.0 ? spec_.sampleRate : 44100.0;
        const float q = std::clamp(0.707f + targetResonance_ * 0.75f, 0.5f, 12.0f);

        for (size_t v = 0; v < MaxVoices; ++v) {
            // Escalonamiento armónico de frecuencias de corte
            const float baseFreq = 350.0f * (1.0f + static_cast<float>(v) * targetHarmonicDisp_ * 0.75f);
            const float cutoff = std::clamp(baseFreq, 60.0f, static_cast<float>(sr * 0.44));

            filtersL_[v].setCoefficients(BiquadFilter::Type::Bandpass, sr, cutoff, q);
            filtersR_[v].setCoefficients(BiquadFilter::Type::Bandpass, sr, cutoff, q);
        }
    }

    ProcessSpec spec_;

    std::array<DelayLine, MaxVoices> voiceDelaysL_;
    std::array<DelayLine, MaxVoices> voiceDelaysR_;
    std::array<BiquadFilter, MaxVoices> filtersL_;
    std::array<BiquadFilter, MaxVoices> filtersR_;
    std::array<float, MaxVoices> lfoPhases_{};

    float targetVoices_{ 4.0f };
    float targetDetune_{ 18.0f };
    float targetDelaySpread_{ 15.0f };
    float targetStereoSpread_{ 0.85f };
    float targetHarmonicDisp_{ 0.5f };
    float targetResonance_{ 1.2f };
    float targetMix_{ 0.5f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 7> params_;
};

inline AutoRegisterNode<RefractionNode> registerRefraction(NodeType::Refraction, "refraction", "Modulation");

} // namespace audio_graph
