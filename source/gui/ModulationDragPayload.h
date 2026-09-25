#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../modulation/ModulationTypes.h"

namespace audio_graph {

/**
 * @brief Estructura de codificación y decodificación para drag & drop de moduladores (Reglas 7, 25, 48).
 */
struct ModulationDragPayload {
    ModSourceType sourceType{ ModSourceType::None };
    uint16_t sourceIndex{ 0 };
    juce::String sourceName;
    juce::Colour sourceColor{ juce::Colours::white };

    static juce::String encode(ModSourceType type, const juce::String& name, juce::Colour col) {
        return "ModSource|" + juce::String(static_cast<int>(type)) + "|" + name + "|" + col.toDisplayString(true);
    }

    static bool decode(const juce::String& text, ModulationDragPayload& outPayload) {
        if (!text.startsWith("ModSource|")) return false;
        auto tokens = juce::StringArray::fromTokens(text, "|", "");
        if (tokens.size() < 4) return false;

        outPayload.sourceType = static_cast<ModSourceType>(tokens[1].getIntValue());
        outPayload.sourceName = tokens[2];
        outPayload.sourceColor = juce::Colour::fromString(tokens[3]);
        return true;
    }

    static juce::Colour getDefaultColor(ModSourceType type) {
        switch (type) {
            case ModSourceType::LFO1:
            case ModSourceType::LFO2:
            case ModSourceType::LFO3:
            case ModSourceType::LFO4:
                return juce::Colour(0xff00d4ff); // Cian
            case ModSourceType::Envelope1:
            case ModSourceType::Envelope2:
                return juce::Colour(0xffff3399); // Magenta
            case ModSourceType::StepSeq:
                return juce::Colour(0xff29b6f6); // Azul claro
            case ModSourceType::AudioFollower:
                return juce::Colour(0xff00e676); // Verde neón
            case ModSourceType::Random:
            case ModSourceType::ChaosX:
            case ModSourceType::ChaosY:
            case ModSourceType::ChaosZ:
                return juce::Colour(0xffff3355); // Rojo / Coral
            case ModSourceType::MacroTexture: return juce::Colour(0xffffa020); // Ámbar
            case ModSourceType::MacroMotion:  return juce::Colour(0xff00d4ff); // Cian
            case ModSourceType::MacroSpace:   return juce::Colour(0xffb040ff); // Violeta
            case ModSourceType::MacroColor:   return juce::Colour(0xffffdd00); // Oro
            case ModSourceType::MacroChaos:   return juce::Colour(0xffff3355); // Coral
            case ModSourceType::MacroDensity: return juce::Colour(0xff00e676); // Verde
            case ModSourceType::MacroEnergy:  return juce::Colour(0xffff2080); // Fucsia
            case ModSourceType::MacroMorph:   return juce::Colour(0xff2979ff); // Azul
            case ModSourceType::MSEG1:
            case ModSourceType::MSEG2:
                return juce::Colour(0xffab47bc); // Púrpura
            default:
                return juce::Colours::white;
        }
    }
};

} // namespace audio_graph
