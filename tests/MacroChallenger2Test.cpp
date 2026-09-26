#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "../source/modulation/ModulationTypes.h"
#include "../source/gui/ModulationDragPayload.h"
#include "../source/gui/MacroDashboardComponent.h"

using namespace audio_graph;

// Helper to construct juce::MouseEvent with specific coordinates and modifiers
static juce::MouseEvent createMouseEvent(juce::Component* comp,
                                         juce::Point<float> currentPos,
                                         juce::ModifierKeys mods,
                                         juce::Point<float> startPos,
                                         int numClicks = 1)
{
    auto mouseSrc = juce::Desktop::getInstance().getMainMouseSource();
    auto now = juce::Time::getCurrentTime();
    return juce::MouseEvent(mouseSrc, currentPos, mods, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                            comp, comp, now, startPos, now, numClicks, false);
}

// Test harness container that implements DragAndDropContainer
class TestDragContainer : public juce::Component, public juce::DragAndDropContainer {
public:
    TestDragContainer() {
        setSize(400, 300);
    }
};

static int gFailedTests = 0;
#define CHALLENGE_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] " << msg << " (line " << __LINE__ << ")\n"; \
            gFailedTests++; \
        } else { \
            std::cout << "  [PASS] " << msg << "\n"; \
        } \
    } while(0)

// ==============================================================================
// 1. ModulationDragPayload Integrity Tests
// ==============================================================================
void testModulationDragPayloadIntegrity() {
    std::cout << "\n======================================================\n";
    std::cout << "[CHALLENGE 1] ModulationDragPayload Integrity (8 Macros)\n";
    std::cout << "======================================================\n";

    struct MacroDef {
        ModSourceType type;
        const char* name;
        uint16_t actualEnumVal;
        int fileLineNumber; // Line number in ModulationTypes.h
    };

    // Note: In ModulationTypes.h, MacroTexture is declared on line 21, but its
    // actual C++ enum integer value is 10 (consecutive after Random = 9).
    const MacroDef macros[8] = {
        { ModSourceType::MacroTexture, "TEXTURE", 10, 21 },
        { ModSourceType::MacroMotion,  "MOTION",  11, 22 },
        { ModSourceType::MacroSpace,   "SPACE",   12, 23 },
        { ModSourceType::MacroColor,   "COLOR",   13, 24 },
        { ModSourceType::MacroChaos,   "CHAOS",   14, 25 },
        { ModSourceType::MacroDensity, "DENSITY", 15, 26 },
        { ModSourceType::MacroEnergy,  "ENERGY",  16, 27 },
        { ModSourceType::MacroMorph,   "MORPH",   17, 28 }
    };

    for (int i = 0; i < 8; ++i) {
        const auto& m = macros[i];
        CHALLENGE_ASSERT(static_cast<uint16_t>(m.type) == m.actualEnumVal,
                         std::string("Enum value for ") + m.name + " matches actual contiguous index " + std::to_string(m.actualEnumVal) + " (defined at line " + std::to_string(m.fileLineNumber) + ")");

        juce::Colour col = ModulationDragPayload::getDefaultColor(m.type);
        CHALLENGE_ASSERT(col != juce::Colours::transparentBlack,
                         std::string("Default color for ") + m.name + " is non-transparent");

        // Encode payload
        juce::String encoded = ModulationDragPayload::encode(m.type, m.name, col);
        std::cout << "    Encoded [" << m.name << "]: " << encoded.toStdString() << "\n";

        CHALLENGE_ASSERT(encoded.startsWith("ModSource|"),
                         std::string("Encoded payload for ") + m.name + " starts with 'ModSource|'");

        juce::String expectedTypeToken = "|" + juce::String(m.actualEnumVal) + "|";
        CHALLENGE_ASSERT(encoded.contains(expectedTypeToken),
                         std::string("Encoded payload contains type token ") + expectedTypeToken.toStdString());

        // Decode payload
        ModulationDragPayload decoded;
        bool ok = ModulationDragPayload::decode(encoded, decoded);
        CHALLENGE_ASSERT(ok, std::string("Payload decode succeeded for ") + m.name);
        CHALLENGE_ASSERT(decoded.sourceType == m.type,
                         std::string("Decoded sourceType matches ") + m.name);
        CHALLENGE_ASSERT(decoded.sourceName == m.name,
                         std::string("Decoded sourceName matches ") + m.name);
        CHALLENGE_ASSERT(decoded.sourceColor == col,
                         std::string("Decoded sourceColor matches ") + m.name);
    }

    // Adversarial tests for decode robustness
    std::cout << "  -- Adversarial stress on decode() malformed inputs --\n";
    ModulationDragPayload dummy;
    CHALLENGE_ASSERT(!ModulationDragPayload::decode("", dummy), "Rejects empty string");
    CHALLENGE_ASSERT(!ModulationDragPayload::decode("InvalidHeader|10|TEXTURE|FFFFA020", dummy), "Rejects wrong header prefix");
    CHALLENGE_ASSERT(!ModulationDragPayload::decode("ModSource|10|TEXTURE", dummy), "Rejects truncated token count (<4)");
    CHALLENGE_ASSERT(!ModulationDragPayload::decode("ModSource|", dummy), "Rejects incomplete prefix only");
    CHALLENGE_ASSERT(!ModulationDragPayload::decode("ModSource", dummy), "Rejects string without delimiter");
}

// ==============================================================================
// 2. MacroDragPin Rule 48 State Machine Verification
// ==============================================================================
void testMacroDragPinRule48() {
    std::cout << "\n======================================================\n";
    std::cout << "[CHALLENGE 2] MacroDragPin Rule 48 State Machine\n";
    std::cout << "======================================================\n";

    TestDragContainer container;
    auto pin = std::make_unique<MacroDragPin>(ModSourceType::MacroTexture, "TEXTURE", juce::Colours::orange);
    pin->setBounds(10, 10, 20, 20);
    container.addAndMakeVisible(*pin);

    int clickCount = 0;
    pin->setOnClick([&clickCount]() {
        clickCount++;
    });

    const juce::Point<float> startPos(10.0f, 10.0f);

    // Test A: Click with zero movement (distance = 0px <= 4px) -> MUST click
    clickCount = 0;
    pin->mouseDown(createMouseEvent(pin.get(), startPos, juce::ModifierKeys(), startPos));
    pin->mouseUp(createMouseEvent(pin.get(), startPos, juce::ModifierKeys(), startPos));
    CHALLENGE_ASSERT(clickCount == 1, "Pure click (dist = 0px) invokes onClick");

    // Test B: Click with tiny jitter <= 4px (distance = 3.0px) -> MUST click
    clickCount = 0;
    juce::Point<float> jitterPos(10.0f + 3.0f, 10.0f);
    pin->mouseDown(createMouseEvent(pin.get(), startPos, juce::ModifierKeys(), startPos));
    pin->mouseDrag(createMouseEvent(pin.get(), jitterPos, juce::ModifierKeys(), startPos));
    pin->mouseUp(createMouseEvent(pin.get(), jitterPos, juce::ModifierKeys(), startPos));
    CHALLENGE_ASSERT(clickCount == 1, "Micro-movement (dist = 3.0px <= 4px) executes onClick");

    // Test C: Exact threshold boundary: 4.00f -> MUST click
    clickCount = 0;
    juce::Point<float> boundaryPos40(10.0f + 4.000f, 10.0f);
    pin->mouseDown(createMouseEvent(pin.get(), startPos, juce::ModifierKeys(), startPos));
    pin->mouseDrag(createMouseEvent(pin.get(), boundaryPos40, juce::ModifierKeys(), startPos));
    pin->mouseUp(createMouseEvent(pin.get(), boundaryPos40, juce::ModifierKeys(), startPos));
    CHALLENGE_ASSERT(clickCount == 1, "Exact threshold (dist = 4.000f <= 4px) executes onClick");

    // Test D: Drag exceeding threshold: 4.10px -> MUST SUPPRESS click
    clickCount = 0;
    juce::Point<float> dragPos(10.0f + 4.10f, 10.0f);
    pin->mouseDown(createMouseEvent(pin.get(), startPos, juce::ModifierKeys(), startPos));
    pin->mouseDrag(createMouseEvent(pin.get(), dragPos, juce::ModifierKeys(), startPos));
    pin->mouseUp(createMouseEvent(pin.get(), dragPos, juce::ModifierKeys(), startPos));
    CHALLENGE_ASSERT(clickCount == 0, "Drag (dist = 4.10px > 4px) triggers drag and SUPPRESSES onClick");

    // Test E: Large drag (dist = 25px) -> MUST SUPPRESS click
    clickCount = 0;
    juce::Point<float> largeDragPos(10.0f + 25.0f, 10.0f);
    pin->mouseDown(createMouseEvent(pin.get(), startPos, juce::ModifierKeys(), startPos));
    pin->mouseDrag(createMouseEvent(pin.get(), largeDragPos, juce::ModifierKeys(), startPos));
    pin->mouseUp(createMouseEvent(pin.get(), largeDragPos, juce::ModifierKeys(), startPos));
    CHALLENGE_ASSERT(clickCount == 0, "Large drag (dist = 25px) SUPPRESSES onClick");

    // Test F: Adversarial Drag Boundary Return (Drag > 4px, then return to startPos before mouseUp)
    // Rule 48 requires hasDragged_ to remain latched so releasing at startPos STILL does NOT click!
    clickCount = 0;
    pin->mouseDown(createMouseEvent(pin.get(), startPos, juce::ModifierKeys(), startPos));
    pin->mouseDrag(createMouseEvent(pin.get(), largeDragPos, juce::ModifierKeys(), startPos)); // trigger hasDragged_
    pin->mouseDrag(createMouseEvent(pin.get(), startPos, juce::ModifierKeys(), startPos));      // move back to start!
    pin->mouseUp(createMouseEvent(pin.get(), startPos, juce::ModifierKeys(), startPos));        // release at start!
    CHALLENGE_ASSERT(clickCount == 0, "Adversarial drag-out and return to origin: onClick STRICTLY SUPPRESSED");
}

// ==============================================================================
// 3. MacroKnob Drag Sensitivity, Shift Precision Ratio & Gestures
// ==============================================================================
void testMacroKnobInteractions() {
    std::cout << "\n======================================================\n";
    std::cout << "[CHALLENGE 3] MacroKnob Sensitivity, Precision & Gestures\n";
    std::cout << "======================================================\n";

    TestDragContainer container;
    auto knob = std::make_unique<MacroKnob>("TEXTURE", nullptr, ModSourceType::MacroTexture, juce::Colours::orange);
    knob->setBounds(0, 0, 100, 42);
    container.addAndMakeVisible(*knob);

    const juce::Point<float> startPos(50.0f, 21.0f);

    // Initial state
    knob->setValue(0.50f);
    CHALLENGE_ASSERT(std::abs(knob->getValue() - 0.50f) < 1e-4f, "Initial value is 0.50f");
    CHALLENGE_ASSERT(!knob->isDragging(), "isDragging is false initially");

    // 3.1 Normal vertical drag sensitivity (0.005f per pixel)
    // Drag upwards by 20 pixels (deltaY = +20)
    knob->mouseDown(createMouseEvent(knob.get(), startPos, juce::ModifierKeys(), startPos));
    CHALLENGE_ASSERT(!knob->isDragging(), "isDragging is false immediately on mouseDown before move");

    // Tiny jitter <= 4px does not engage drag
    knob->mouseDrag(createMouseEvent(knob.get(), juce::Point<float>(50.0f, 21.0f - 3.0f), juce::ModifierKeys(), startPos));
    CHALLENGE_ASSERT(!knob->isDragging(), "isDragging is false when vertical delta <= 4px");
    CHALLENGE_ASSERT(std::abs(knob->getValue() - 0.50f) < 1e-4f, "Value unchanged under threshold");

    // Drag 20px upwards (> 4px)
    const juce::Point<float> move20Up(50.0f, 21.0f - 20.0f);
    knob->mouseDrag(createMouseEvent(knob.get(), move20Up, juce::ModifierKeys(), startPos));
    CHALLENGE_ASSERT(knob->isDragging(), "isDragging is true after 20px vertical movement");

    const float normalVal = knob->getValue();
    const float expectedNormal = 0.50f + 20.0f * 0.005f; // 0.60f
    CHALLENGE_ASSERT(std::abs(normalVal - expectedNormal) < 1e-4f,
                     "Normal drag 20px up produced value: " + std::to_string(normalVal) + " (expected: " + std::to_string(expectedNormal) + ")");

    knob->mouseUp(createMouseEvent(knob.get(), move20Up, juce::ModifierKeys(), startPos));
    CHALLENGE_ASSERT(!knob->isDragging(), "isDragging is false after mouseUp");

    // 3.2 Shift+drag precision mode (0.001f per pixel -> 0.2x ratio)
    knob->setValue(0.50f);
    auto shiftMods = juce::ModifierKeys(juce::ModifierKeys::shiftModifier);
    knob->mouseDown(createMouseEvent(knob.get(), startPos, shiftMods, startPos));
    knob->mouseDrag(createMouseEvent(knob.get(), move20Up, shiftMods, startPos));
    CHALLENGE_ASSERT(knob->isDragging(), "isDragging is true during shift drag");

    const float shiftVal = knob->getValue();
    const float expectedShift = 0.50f + 20.0f * 0.001f; // 0.52f
    CHALLENGE_ASSERT(std::abs(shiftVal - expectedShift) < 1e-4f,
                     "Shift drag 20px up produced value: " + std::to_string(shiftVal) + " (expected: " + std::to_string(expectedShift) + ")");

    const float deltaNormal = normalVal - 0.50f;
    const float deltaShift = shiftVal - 0.50f;
    const float ratio = deltaShift / deltaNormal;
    CHALLENGE_ASSERT(std::abs(ratio - 0.20f) < 1e-4f,
                     "Empirical precision speed ratio = " + std::to_string(ratio) + " (EXACTLY 0.2x!)");

    knob->mouseUp(createMouseEvent(knob.get(), move20Up, shiftMods, startPos));

    // 3.3 Double-click reset to default 50% (0.50f)
    knob->setValue(0.123f);
    CHALLENGE_ASSERT(std::abs(knob->getValue() - 0.123f) < 1e-4f, "Value set to 0.123f");
    knob->mouseDoubleClick(createMouseEvent(knob.get(), startPos, juce::ModifierKeys(), startPos, 2));
    CHALLENGE_ASSERT(std::abs(knob->getValue() - 0.50f) < 1e-4f,
                     "Double-click from 0.123f successfully reset value to 0.50f");

    knob->setValue(0.987f);
    knob->mouseDoubleClick(createMouseEvent(knob.get(), startPos, juce::ModifierKeys(), startPos, 2));
    CHALLENGE_ASSERT(std::abs(knob->getValue() - 0.50f) < 1e-4f,
                     "Double-click from 0.987f successfully reset value to 0.50f");

    // 3.4 Mouse wheel increments/decrements (+/- 0.02f) and boundary clamping [0.0f, 1.0f]
    knob->setValue(0.50f);
    juce::MouseWheelDetails wheelUp;
    wheelUp.deltaY = 1.0f;
    wheelUp.isReversed = false;
    knob->mouseWheelMove(createMouseEvent(knob.get(), startPos, juce::ModifierKeys(), startPos), wheelUp);
    CHALLENGE_ASSERT(std::abs(knob->getValue() - 0.52f) < 1e-4f, "Mouse wheel up incremented value to 0.52f (+0.02f)");

    juce::MouseWheelDetails wheelDown;
    wheelDown.deltaY = -1.0f;
    wheelDown.isReversed = false;
    knob->mouseWheelMove(createMouseEvent(knob.get(), startPos, juce::ModifierKeys(), startPos), wheelDown);
    CHALLENGE_ASSERT(std::abs(knob->getValue() - 0.50f) < 1e-4f, "Mouse wheel down decremented value to 0.50f (-0.02f)");

    // Test upper clamping at 1.0f
    knob->setValue(0.99f);
    knob->mouseWheelMove(createMouseEvent(knob.get(), startPos, juce::ModifierKeys(), startPos), wheelUp);
    CHALLENGE_ASSERT(std::abs(knob->getValue() - 1.00f) < 1e-4f, "Mouse wheel clamped at upper boundary 1.0f");

    // Test lower clamping at 0.0f
    knob->setValue(0.01f);
    knob->mouseWheelMove(createMouseEvent(knob.get(), startPos, juce::ModifierKeys(), startPos), wheelDown);
    CHALLENGE_ASSERT(std::abs(knob->getValue() - 0.00f) < 1e-4f, "Mouse wheel clamped at lower boundary 0.0f");

    // Test reversed wheel flag
    knob->setValue(0.50f);
    juce::MouseWheelDetails wheelReversed;
    wheelReversed.deltaY = 1.0f;
    wheelReversed.isReversed = true; // reversed means upwards delta produces negative step
    knob->mouseWheelMove(createMouseEvent(knob.get(), startPos, juce::ModifierKeys(), startPos), wheelReversed);
    CHALLENGE_ASSERT(std::abs(knob->getValue() - 0.48f) < 1e-4f, "Reversed mouse wheel correctly inverts delta to -0.02f (0.48f)");

    // 3.5 Host Parameter Update Shielding via isDragging()
    knob->setValue(0.40f);
    knob->mouseDown(createMouseEvent(knob.get(), startPos, juce::ModifierKeys(), startPos));
    knob->mouseDrag(createMouseEvent(knob.get(), move20Up, juce::ModifierKeys(), startPos));
    CHALLENGE_ASSERT(knob->isDragging(), "Knob is currently actively being dragged");

    // Simulate host polling/timer update logic in updateKnobValues:
    // "if (knobs_[i] != nullptr && !knobs_[i]->isDragging()) { knobs_[i]->setValue(hostVal); }"
    const float hostAutomatedValue = 0.999f;
    if (!knob->isDragging()) {
        knob->setValue(hostAutomatedValue);
    }
    CHALLENGE_ASSERT(std::abs(knob->getValue() - hostAutomatedValue) > 0.1f,
                     "Host parameter automation was properly BLOCKED from clobbering active user drag");
    knob->mouseUp(createMouseEvent(knob.get(), move20Up, juce::ModifierKeys(), startPos));
    CHALLENGE_ASSERT(!knob->isDragging(), "Knob released from dragging");
}

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::cout << "======================================================\n";
    std::cout << "CHALLENGER 2: EMPIRICAL STRESS TEST SUITE\n";
    std::cout << "Target: MacroDashboardComponent, MacroDragPin, MacroKnob\n";
    std::cout << "======================================================\n";

    testModulationDragPayloadIntegrity();
    testMacroDragPinRule48();
    testMacroKnobInteractions();

    std::cout << "\n======================================================\n";
    if (gFailedTests == 0) {
        std::cout << "CHALLENGER 2 VERDICT: ALL TESTS PASSED (0 FAILURES)\n";
        std::cout << "======================================================\n";
        return 0;
    } else {
        std::cerr << "CHALLENGER 2 VERDICT: " << gFailedTests << " FAILURES DETECTED!\n";
        std::cout << "======================================================\n";
        return 1;
    }
}
