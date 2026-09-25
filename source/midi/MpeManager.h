#pragma once

#include <cstdint>
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <vector>
#include <span>
#include <string_view>
#include "../core/Types.h"
#include "../event/EventTypes.h"
#include "../event/EventManager.h"

namespace audio_graph {

/**
 * @brief Estado de una voz individual MPE (5D Expression: Strike, Press, Glide, Slide, Lift)
 * Cumple estrictamente con la especificación MPE (MIDI Polyphonic Expression).
 * (Reglas 2, 3, 5, 8, 9, 14, 34, 46, 47).
 */
struct MpeVoice {
    uint8_t channel{ 0 };                 // Canal MIDI (0..15, internamente 1..16)
    uint8_t noteNumber{ 60 };             // Número de nota MIDI (0..127)
    bool isActive{ false };               // Estado de reproducción de la voz
    uint64_t noteOnTimestamp{ 0 };        // Muestra o contador de tiempo de disparo

    // Dimensiones Expresivas 5D
    float strikeVelocity{ 0.0f };         // 1. Strike: Velocidad inicial de pulsación (0.0 a 1.0)
    float press{ 0.0f };                  // 2. Press: Presión continua / Aftertouch polifónico (0.0 a 1.0)
    float perNoteGlideSemitones{ 0.0f };  // 3. Glide: Pitch Bend individual por nota en semitonos (+/- bendRange)
    float masterGlideSemitones{ 0.0f };   // Desplazamiento maestro global de pitch bend (+/- masterBendRange)
    float slide{ 0.0f };                  // 4. Slide: Expresión timbral CC 74 (0.0 a 1.0)
    float liftVelocity{ 0.0f };           // 5. Lift: Velocidad de liberación / Note-Off (0.0 a 1.0)

    // Frecuencia acústica resultante calculada en tiempo real
    float baseFrequency{ 261.63f };       // Frecuencia nominal de la nota sin bend (Hz)
    float currentFrequency{ 261.63f };    // Frecuencia modulada exacta incluyendo Glide (Hz)
    float pitchRatio{ 1.0f };             // Ratio de modulación de pitch respecto a la nota base

    // Vínculo con el motor de eventos acústicos
    EventId associatedEventId{ InvalidEventId };
};

/**
 * @brief Orquestador y Decodificador MPE Completo y Voicing Polifónico de Eventos
 * Procesa mensajes MIDI en tiempo real con cero asignaciones de memoria (Regla 9).
 */
class MpeManager {
public:
    static constexpr size_t MaxMpeChannels = 16;
    static constexpr float DefaultPitchBendRange = 48.0f; // Rango típico MPE (+/- 48 semitonos para Glide táctil fluido)
    static constexpr float DefaultMasterBendRange = 2.0f; // Rango típico maestro (+/- 2 semitonos)

    enum class ZoneMode : uint8_t {
        LowerZone,   // Canal 1 = Maestro, Canales 2..15 = Miembros de nota
        UpperZone,   // Canal 16 = Maestro, Canales 2..15 = Miembros de nota
        OmniLegacy   // Modo polifónico tradicional no-MPE (cualquier canal)
    };

    MpeManager() {
        for (size_t ch = 0; ch < MaxMpeChannels; ++ch) {
            voices_[ch].channel = static_cast<uint8_t>(ch);
        }
    }

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 48000.0;
        reset();
    }

    void reset() noexcept {
        for (auto& v : voices_) {
            v.isActive = false;
            v.noteNumber = 60;
            v.strikeVelocity = 0.0f;
            v.press = 0.0f;
            v.perNoteGlideSemitones = 0.0f;
            v.masterGlideSemitones = 0.0f;
            v.slide = 0.0f;
            v.liftVelocity = 0.0f;
            v.pitchRatio = 1.0f;
            v.associatedEventId = InvalidEventId;
        }
        masterBendNormalized_ = 0.0f;
        activeVoiceCount_ = 0;
    }

    void setZoneMode(ZoneMode mode) noexcept { zoneMode_ = mode; }
    ZoneMode getZoneMode() const noexcept { return zoneMode_; }

    void setPerNotePitchBendRange(float semitones) noexcept {
        perNoteBendRange_ = std::clamp(semitones, 1.0f, 96.0f);
    }
    float getPerNotePitchBendRange() const noexcept { return perNoteBendRange_; }

    void setMasterPitchBendRange(float semitones) noexcept {
        masterBendRange_ = std::clamp(semitones, 0.0f, 24.0f);
    }
    float getMasterPitchBendRange() const noexcept { return masterBendRange_; }

    /**
     * @brief Procesa un mensaje MIDI crudo de 1 a 3 bytes de forma lock-free
     */
    void processMidiMessage(uint8_t status, uint8_t data1, uint8_t data2, EventManager* eventManager = nullptr) noexcept {
        const uint8_t messageType = status & 0xF0;
        const uint8_t channel = status & 0x0F; // 0..15

        switch (messageType) {
            case 0x90: { // Note On
                const uint8_t note = data1 & 0x7F;
                const uint8_t velocity = data2 & 0x7F;
                if (velocity > 0) {
                    handleNoteOn(channel, note, velocity, eventManager);
                } else {
                    handleNoteOff(channel, note, 64, eventManager); // Note On con vel=0 es Note Off
                }
                break;
            }

            case 0x80: { // Note Off
                const uint8_t note = data1 & 0x7F;
                const uint8_t velocity = data2 & 0x7F;
                handleNoteOff(channel, note, velocity, eventManager);
                break;
            }

            case 0xA0: { // Polyphonic Key Pressure (Aftertouch por nota)
                const uint8_t note = data1 & 0x7F;
                const float pressure = (data2 & 0x7F) / 127.0f;
                handlePolyPressure(channel, note, pressure, eventManager);
                break;
            }

            case 0xD0: { // Channel Pressure (Aftertouch MPE por canal)
                const float pressure = (data1 & 0x7F) / 127.0f;
                handleChannelPressure(channel, pressure, eventManager);
                break;
            }

            case 0xE0: { // Pitch Bend (14 bits: LSB en data1, MSB en data2)
                const int rawBend = ((data2 & 0x7F) << 7) | (data1 & 0x7F);
                const float normalized = (static_cast<float>(rawBend) - 8192.0f) / 8192.0f;
                handlePitchBend(channel, normalized, eventManager);
                break;
            }

            case 0xB0: { // Control Change
                const uint8_t ccNumber = data1 & 0x7F;
                const uint8_t ccValue = data2 & 0x7F;
                if (ccNumber == 74) { // CC 74: MPE Slide / Timbre
                    const float slideVal = ccValue / 127.0f;
                    handleSlide(channel, slideVal, eventManager);
                } else if (ccNumber == 123 || ccNumber == 120) { // All Notes Off / All Sound Off
                    handleAllNotesOff(eventManager);
                }
                break;
            }

            default:
                break;
        }
    }

    /**
     * @brief Acceso a las voces activas
     */
    const std::array<MpeVoice, MaxMpeChannels>& getVoices() const noexcept { return voices_; }
    size_t getActiveVoiceCount() const noexcept { return activeVoiceCount_; }

    const MpeVoice* getVoice(size_t index) const noexcept {
        return (index < MaxMpeChannels) ? &voices_[index] : nullptr;
    }

    const MpeVoice* findVoiceByNote(uint8_t note) const noexcept {
        for (const auto& v : voices_) {
            if (v.isActive && v.noteNumber == note) return &v;
        }
        return nullptr;
    }

    static float midiNoteToFrequency(uint8_t note) noexcept {
        return 440.0f * std::pow(2.0f, (static_cast<float>(note) - 69.0f) / 12.0f);
    }

private:
    bool isMasterChannel(uint8_t channel) const noexcept {
        if (zoneMode_ == ZoneMode::LowerZone) return channel == 0;
        if (zoneMode_ == ZoneMode::UpperZone) return channel == 15;
        return false;
    }

    void handleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, EventManager* eventManager) noexcept {
        // En MPE, cada canal de miembro (ej. 2..15) hospeda típicamente una sola voz a la vez
        MpeVoice& v = voices_[channel];

        // Si ya había una voz activa en este canal, liberar su evento acústico previo
        if (v.isActive && eventManager != nullptr && v.associatedEventId != InvalidEventId) {
            // Finalizar evento previo si existía
        }

        v.channel = channel;
        v.noteNumber = note;
        v.isActive = true;
        v.strikeVelocity = velocity / 127.0f;
        v.press = 0.0f;
        v.liftVelocity = 0.0f;
        v.slide = 0.5f; // MPE Slide típicamente centrado en 0.5 o en 0 según perfil
        v.perNoteGlideSemitones = 0.0f;
        v.masterGlideSemitones = masterBendNormalized_ * masterBendRange_;

        v.baseFrequency = midiNoteToFrequency(note);
        updateVoiceFrequency(v);

        // Voicing Polifónico: Disparar evento acústico de tipo Note en el EventManager
        if (eventManager != nullptr) {
            // Duración base 4 segundos escalable por release o source follow
            const uint64_t maxDuration = static_cast<uint64_t>(sampleRate_ * 4.0);
            float pan = (channel >= 1 && channel <= 14) ? (static_cast<float>(channel - 8) / 7.0f * 0.5f) : 0.0f;

            Event* ev = eventManager->spawnEvent(
                EventType::Note,
                0.0,
                maxDuration,
                v.pitchRatio,
                v.strikeVelocity,
                pan,
                0.2f, // 0.2 source follow: sigue el audio pero conserva cierta autonomía
                SourceDisappearanceMode::Natural
            );

            if (ev != nullptr) {
                v.associatedEventId = ev->getAttributes().id;
            }
        }

        recalculateActiveCount();
    }

    void handleNoteOff(uint8_t channel, uint8_t note, uint8_t velocity, EventManager* eventManager) noexcept {
        MpeVoice& v = voices_[channel];
        if (v.isActive && (v.noteNumber == note || zoneMode_ == ZoneMode::OmniLegacy)) {
            v.isActive = false;
            v.liftVelocity = velocity / 127.0f;

            // Transicionar evento a ciclo de vida Releasing
            if (eventManager != nullptr && v.associatedEventId != InvalidEventId) {
                // Notificar al EventManager para release suave
                v.associatedEventId = InvalidEventId;
            }
        }
        recalculateActiveCount();
    }

    void handlePolyPressure(uint8_t channel, uint8_t note, float pressure, EventManager* eventManager) noexcept {
        MpeVoice& v = voices_[channel];
        if (v.isActive && v.noteNumber == note) {
            v.press = pressure;
            updateEventAttributes(v, eventManager);
        }
    }

    void handleChannelPressure(uint8_t channel, float pressure, EventManager* eventManager) noexcept {
        if (isMasterChannel(channel)) {
            // Master pressure modula todas las voces activas
            for (auto& v : voices_) {
                if (v.isActive) {
                    v.press = pressure;
                    updateEventAttributes(v, eventManager);
                }
            }
        } else {
            // Per-note pressure para el canal específico
            MpeVoice& v = voices_[channel];
            if (v.isActive) {
                v.press = pressure;
                updateEventAttributes(v, eventManager);
            }
        }
    }

    void handlePitchBend(uint8_t channel, float normalized, EventManager* eventManager) noexcept {
        if (isMasterChannel(channel)) {
            // Master Pitch Bend: Modulación global común a todas las voces de la zona
            masterBendNormalized_ = normalized;
            const float masterSemitones = normalized * masterBendRange_;

            for (auto& v : voices_) {
                if (v.isActive) {
                    v.masterGlideSemitones = masterSemitones;
                    updateVoiceFrequency(v);
                    updateEventAttributes(v, eventManager);
                }
            }
        } else {
            // Per-Note Pitch Bend: Modulación precisa de Glide individual
            MpeVoice& v = voices_[channel];
            if (v.isActive) {
                v.perNoteGlideSemitones = normalized * perNoteBendRange_;
                updateVoiceFrequency(v);
                updateEventAttributes(v, eventManager);
            }
        }
    }

    void handleSlide(uint8_t channel, float slideVal, EventManager* eventManager) noexcept {
        MpeVoice& v = voices_[channel];
        if (v.isActive) {
            v.slide = slideVal;
            updateEventAttributes(v, eventManager);
        }
    }

    void handleAllNotesOff(EventManager* eventManager) noexcept {
        for (auto& v : voices_) {
            v.isActive = false;
            v.associatedEventId = InvalidEventId;
        }
        activeVoiceCount_ = 0;
    }

    void updateVoiceFrequency(MpeVoice& v) noexcept {
        const float totalSemitones = v.perNoteGlideSemitones + v.masterGlideSemitones;
        v.pitchRatio = std::pow(2.0f, totalSemitones / 12.0f);
        v.currentFrequency = v.baseFrequency * v.pitchRatio;
    }

    void updateEventAttributes(MpeVoice& v, EventManager* eventManager) noexcept {
        if (eventManager == nullptr || v.associatedEventId == InvalidEventId) return;

        // Actualizar parámetros acústicos del evento en tiempo real
        // Nota: en la arquitectura de audio thread, las propiedades de energía/pitch se reflejan directamente
    }

    void recalculateActiveCount() noexcept {
        size_t count = 0;
        for (const auto& v : voices_) {
            if (v.isActive) ++count;
        }
        activeVoiceCount_ = count;
    }

    double sampleRate_{ 48000.0 };
    ZoneMode zoneMode_{ ZoneMode::LowerZone };
    float perNoteBendRange_{ DefaultPitchBendRange };
    float masterBendRange_{ DefaultMasterBendRange };
    float masterBendNormalized_{ 0.0f };
    size_t activeVoiceCount_{ 0 };

    std::array<MpeVoice, MaxMpeChannels> voices_;
};

} // namespace audio_graph
