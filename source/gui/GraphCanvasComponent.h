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
 * Soporta cableado directo con snapping magnético, desconexión por clic, y encadenamiento automático Drag & Drop.
 */
class GraphCanvasComponent : public juce::Component, public juce::DragAndDropTarget {
public:
    explicit GraphCanvasComponent(N8AudioProcessor& processor);
    ~GraphCanvasComponent() override = default;

    void rebuildFromGraph();
    void addNodeAtPosition(NodeType type, float x, float y);
    void deleteNode(NodeId id);
    void disconnectPin(NodeId nodeId, PinId pinId);
    void deleteConnection(ConnectionId cid);
    void selectNode(NodeId id);
    NodeId getSelectedNodeId() const noexcept { return selectedNodeId_; }
    void setOnNodeSelected(std::function<void(NodeId)> cb) { onNodeSelected_ = std::move(cb); }

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Métodos Drag and Drop (juce::DragAndDropTarget)
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    // Encadenamiento automático Drag & Drop entre nodos
    void handleNodeDragging(NodeId draggedNodeId, juce::Rectangle<int> draggedBounds);
    void handleNodeDropped(NodeId draggedNodeId, juce::Rectangle<int> draggedBounds);

private:
    NodeComponent* findNodeComponent(NodeId id) const;
    bool findPinAtCanvasPos(juce::Point<float> pos, NodeId& outNodeId, PinId& outPinId, PinType& outType, PinDataType& outDataType, juce::Point<float>& outCenter, float tolerance = 18.0f) const;
    ConnectionId findConnectionNear(juce::Point<float> pos, float threshold = 8.0f) const;

    N8AudioProcessor& processor_;
    std::vector<std::unique_ptr<NodeComponent>> nodeComponents_;
    NodeId selectedNodeId_{ InvalidNodeId };
    std::function<void(NodeId)> onNodeSelected_;

    // Estado de arrastre y snapping de cables
    bool isDraggingWire_{ false };
    NodeId dragSourceNode_{ InvalidNodeId };
    PinId dragSourcePin_{ InvalidPinId };
    PinType dragSourcePinType_{ PinType::AudioOutput };
    PinDataType dragDataType_{ PinDataType::AudioStereo };
    juce::Point<float> dragStartPt_;
    juce::Point<float> currentMousePt_;
    NodeId snappedTargetNode_{ InvalidNodeId };
    PinId snappedTargetPin_{ InvalidPinId };

    // Paneo del canvas
    juce::Point<float> canvasOffset_{ 0.0f, 0.0f };
    juce::Point<float> panStartOffset_{ 0.0f, 0.0f };
    bool isPanning_{ false };

    // Vista previa fantasma Drag & Drop
    bool isShowingDragGhost_{ false };
    juce::Point<float> ghostPos_;
    juce::String ghostLabel_;
    NodeId ghostCandidateTarget_{ InvalidNodeId };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphCanvasComponent)
};

} // namespace audio_graph

