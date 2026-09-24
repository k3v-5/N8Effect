#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include "../core/Types.h"
#include "../graph/Graph.h"
#include "NodeComponent.h"
#include "WireRenderer.h"

namespace audio_graph {

class N8AudioProcessor;

/**
 * @brief Canvas interactivo para edición y visualización del Grafo Modular DAG (Reglas 4, 23, 24, 25).
 */
class GraphCanvasComponent : public juce::Component {
public:
    explicit GraphCanvasComponent(N8AudioProcessor& processor);
    ~GraphCanvasComponent() override = default;

    void rebuildFromGraph();
    void addNodeAtPosition(NodeType type, float x, float y);
    void deleteNode(NodeId id);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    NodeComponent* findNodeComponent(NodeId id) const;

    N8AudioProcessor& processor_;
    std::vector<std::unique_ptr<NodeComponent>> nodeComponents_;

    // Estado de arrastre de cables
    bool isDraggingWire_{ false };
    NodeId dragSourceNode_{ InvalidNodeId };
    PinId dragSourcePin_{ InvalidPinId };
    PinDataType dragDataType_{ PinDataType::AudioStereo };
    juce::Point<float> dragStartPt_;
    juce::Point<float> currentMousePt_;

    // Paneo del canvas
    juce::Point<float> canvasOffset_{ 0.0f, 0.0f };
    juce::Point<float> panStartOffset_{ 0.0f, 0.0f };
    bool isPanning_{ false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphCanvasComponent)
};

} // namespace audio_graph
