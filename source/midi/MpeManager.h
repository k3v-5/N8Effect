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
#include "../dsp/rust_core/include/n8_rust_core.h"

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
 * Acelerado por el motor nativo Rust n8_rust_core con cero asignaciones de memoria (Reglas 2, 3, 8, 9, 46, 47).
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
        rustCore_ = n8_rust_mpe_manager_create();
        for (size_t ch = 0; ch < MaxMpeChannels; ++ch) {
            voices_[ch].channel = static_cast<uint8_t>(ch);
        }
        syncFromRust();
    }

    ~MpeManager() {
        if (rustCore_ != nullptr) {
            n8_rust_mpe_manager_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    MpeManager(const MpeManager& other) {
        rustCore_ = n8_rust_mpe_manager_create();
        copyFrom(other);
    }

    MpeManager& operator=(const MpeManager& other) {
        if (this != &other) {
            copyFrom(other);
        }
        return *this;
    }

    MpeManager(MpeManager&& other) noexcept {
        rustCore_ = other.rustCore_;
        other.rustCore_ = nullptr;
        voices_ = other.voices_;
        sampleRate_ = other.sampleRate_;
    }

    MpeManager& operator=(MpeManager&& other) noexcept {
        if (this != &other) {
            if (rustCore_ != nullptr) {
                n8_rust_mpe_manager_destroy(rustCore_);
            }
            rustCore_ = other.rustCore_;
            other.rustCore_ = nullptr;
            voices_ = other.voices_;
            sampleRate_ = other.sampleRate_;
        }
        return *this;
    }

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 0.0) ? sampleRate : 48000.0;
        if (rustCore_ != nullptr) {
            n8_rust_mpe_manager_prepare(rustCore_, sampleRate_);
        }
        syncFromRust();
    }

    void reset() noexcept {
        if (rustCore_ != nullptr) {
            n8_rust_mpe_manager_reset(rustCore_);
        }
        syncFromRust();
    }

    void setZoneMode(ZoneMode mode) noexcept {
        if (rustCore_ != nullptr) {
            n8_rust_mpe_manager_set_zone_mode(rustCore_, static_cast<uint8_t>(mode));
        }
    }

    ZoneMode getZoneMode() const noexcept {
        if (rustCore_ != nullptr) {
            return static_cast<ZoneMode>(n8_rust_mpe_manager_get_zone_mode(rustCore_));
        }
        return ZoneMode::LowerZone;
    }

    void setPerNotePitchBendRange(float semitones) noexcept {
        if (rustCore_ != nullptr) {
            n8_rust_mpe_manager_set_per_note_pitch_bend_range(rustCore_, semitones);
        }
    }

    float getPerNotePitchBendRange() const noexcept {
        if (rustCore_ != nullptr) {
            return n8_rust_mpe_manager_get_per_note_pitch_bend_range(rustCore_);
        }
        return DefaultPitchBendRange;
    }

    void setMasterPitchBendRange(float semitones) noexcept {
        if (rustCore_ != nullptr) {
            n8_rust_mpe_manager_set_master_pitch_bend_range(rustCore_, semitones);
        }
    }

    float getMasterPitchBendRange() const noexcept {
        if (rustCore_ != nullptr) {
            return n8_rust_mpe_manager_get_master_pitch_bend_range(rustCore_);
        }
        return DefaultMasterBendRange;
    }

    /**
     * @brief Procesa un mensaje MIDI crudo de 1 a 3 bytes de forma lock-free
     */
    void processMidiMessage(uint8_t status, uint8_t data1, uint8_t data2, EventManager* eventManager = nullptr) noexcept {
        if (rustCore_ == nullptr) return;

        uint8_t actionType = 0;
        uint8_t channel = 0;
        uint8_t note = 0;
        float pitchRatio = 1.0f;
        float strikeVelocity = 0.0f;
        float pan = 0.0f;
        uint64_t maxDuration = 0;

        bool hasAction = n8_rust_mpe_manager_process_midi_message(
            rustCore_, status, data1, data2,
            &actionType, &channel, &note, &pitchRatio, &strikeVelocity, &pan, &maxDuration);

        if (hasAction && eventManager != nullptr) {
            if (actionType == 1) { // SpawnEvent
                Event* ev = eventManager->spawnEvent(
                    EventType::Note,
                    0.0,
                    maxDuration,
                    pitchRatio,
                    strikeVelocity,
                    pan,
                    0.2f, // 0.2 source follow
                    SourceDisappearanceMode::Natural
                );
                if (ev != nullptr) {
                    n8_rust_mpe_manager_set_associated_event_id(rustCore_, channel, ev->getAttributes().id);
                }
            } else if (actionType == 2) { // ReleaseEvent
                // Notificar al EventManager para release suave
                n8_rust_mpe_manager_set_associated_event_id(rustCore_, channel, InvalidEventId);
            }
        }

        syncFromRust();
    }

    /**
     * @brief Acceso a las voces activas
     */
    const std::array<MpeVoice, MaxMpeChannels>& getVoices() const noexcept { return voices_; }

    size_t getActiveVoiceCount() const noexcept {
        if (rustCore_ != nullptr) {
            return n8_rust_mpe_manager_get_active_voice_count(rustCore_);
        }
        return 0;
    }

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
        return n8_rust_mpe_manager_midi_note_to_frequency(note);
    }

private:
    void copyFrom(const MpeManager& other) {
        if (rustCore_ != nullptr && other.rustCore_ != nullptr) {
            setZoneMode(other.getZoneMode());
            setPerNotePitchBendRange(other.getPerNotePitchBendRange());
            setMasterPitchBendRange(other.getMasterPitchBendRange());
            prepare(other.sampleRate_);
        }
        voices_ = other.voices_;
        sampleRate_ = other.sampleRate_;
    }

    void syncFromRust() noexcept {
        if (rustCore_ == nullptr) return;

        for (size_t ch = 0; ch < MaxMpeChannels; ++ch) {
            bool active = false;
            uint8_t note = 60;
            float strike = 0.0f, press = 0.0f, perNoteGlide = 0.0f, masterGlide = 0.0f, slide = 0.0f, lift = 0.0f;
            float baseF = 261.63f, curF = 261.63f, ratio = 1.0f;
            uint32_t eventId = InvalidEventId;

            if (n8_rust_mpe_manager_get_voice_info(
                rustCore_, ch, &active, &note, &strike, &press, &perNoteGlide, &masterGlide, &slide, &lift,
                &baseF, &curF, &ratio, &eventId))
            {
                auto& v = voices_[ch];
                v.channel = static_cast<uint8_t>(ch);
                v.noteNumber = note;
                v.isActive = active;
                v.strikeVelocity = strike;
                v.press = press;
                v.perNoteGlideSemitones = perNoteGlide;
                v.masterGlideSemitones = masterGlide;
                v.slide = slide;
                v.liftVelocity = lift;
                v.baseFrequency = baseF;
                v.currentFrequency = curF;
                v.pitchRatio = ratio;
                v.associatedEventId = eventId;
            }
        }
    }

    RustMpeManager* rustCore_{ nullptr };
    double sampleRate_{ 48000.0 };
    std::array<MpeVoice, MaxMpeChannels> voices_;
};

} // namespace audio_graph
