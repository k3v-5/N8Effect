#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>
#include "../core/Types.h"
#include "SequentialSlotComponent.h"

namespace audio_graph {

class N8AudioProcessor;

/**
 * @brief Vista de Tira Secuencial / Cola de Efectos Modular (Estilo Arturia Efx MOTIONS).
 * Organiza los nodos en una cola horizontal interactiva con flujo de señal visible,
 * botones de inserción entre slots, reordenamiento instantáneo y adición dinámica.
 */
class SequentialStripComponent : public juce::Component {
public:
    explicit SequentialStripComponent(N8AudioProcessor& processor);
    ~SequentialStripComponent() override = default;

    void rebuild();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void showAddEffectMenu(int insertIndex);

    N8AudioProcessor& processor_;

    juce::Viewport viewport_;
    juce::Component contentContainer_;

    std::vector<std::unique_ptr<SequentialSlotComponent>> slots_;
    std::vector<std::unique_ptr<juce::Button>> insertBetweenButtons_;
    std::unique_ptr<juce::TextButton> addSlotEndBtn_;

    // Notificación de estado vacío
    juce::Label emptyStateLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequentialStripComponent)
};

} // namespace audio_graph
