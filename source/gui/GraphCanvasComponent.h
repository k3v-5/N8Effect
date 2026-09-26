#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "../core/Types.h"
#include "../graph/Graph.h"
#include "NodeComponent.h"
#include "WireRenderer.h"
#include "NodeGroupComponent.h"

namespace audio_graph {

class N8AudioProcessor;
class CanvasWorldComponent;
class ZoomHudOverlay;

/**
 * @brief Canvas interactivo con Zoom & Pan para edición y visualización del Grafo Modular DAG (Reglas 4, 23, 24, 25).
 * Soporta zoom continuo centrado en el cursor (0.35x a 2.0x), Zoom-to-Fit automático, paneo multi-método,
 * cableado directo con snapping magnético, desconexión por clic, y encadenamiento automático Drag & Drop.
 */
class GraphCanvasComponent : public juce::Component,
                             public juce::DragAndDropTarget,
                             public juce::FileDragAndDropTarget {
public:
    explicit GraphCanvasComponent(N8AudioProcessor& processor);
    ~GraphCanvasComponent() override;

    void rebuildFromGraph();
    void addNodeAtPosition(NodeType type, float x, float y);
    void deleteNode(NodeId id);
    void disconnectPin(NodeId nodeId, PinId pinId);
    void deleteConnection(ConnectionId cid);
    void selectNode(NodeId id, bool additive = false);
    void clearSelection();
    bool isNodeSelected(NodeId id) const noexcept { return selectedNodeIds_.contains(id); }
    const std::unordered_set<NodeId>& getSelectedNodeIds() const noexcept { return selectedNodeIds_; }
    NodeId getSelectedNodeId() const noexcept { return selectedNodeId_; }
    void setOnNodeSelected(std::function<void(NodeId)> cb) { onNodeSelected_ = std::move(cb); }

    // Colapso y exportación de subgrafos jerárquicos (Reglas 6, 16, 21, 22)
    void collapseSelectedNodesIntoContainer();
    void exportSelectedSubGraphModule();
    void importSubGraphModule();
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;
    void setOnPianoKeyForward(std::function<bool(const juce::KeyPress&)> cb) { onPianoKeyForward_ = std::move(cb); }
    void setOnPianoKeyStateChanged(std::function<bool(bool)> cb) { onPianoKeyStateChanged_ = std::move(cb); }
    void setIsPianoVisibleCallback(std::function<bool()> cb) { isPianoVisibleCallback_ = std::move(cb); }

    // Gestión de Nodos y Cajas de Grupo (Regla R2)
    NodeComponent* findNodeComponent(NodeId id) const;
    NodeGroupComponent* findGroupComponent(GroupId id) const;
    const std::vector<std::unique_ptr<NodeGroupComponent>>& getNodeGroups() const noexcept { return nodeGroups_; }
    void createGroup(const std::vector<NodeId>& nodeIds, std::string_view name = "Group");
    void removeGroup(GroupId id);

    // Sistema de Zoom & Paneo Virtual (Reglas 23, 24, 48)
    float getZoomScale() const noexcept { return zoomScale_; }
    juce::Point<float> getPanOffset() const noexcept { return panOffset_; }
    void setZoom(float newZoom, juce::Point<float> anchorScreen);
    void zoomBy(float factor);
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void zoomToFit();
    void setPanOffset(juce::Point<float> offset);

    // Transformaciones bidireccionales Screen <-> World
    juce::Point<float> screenToWorld(juce::Point<float> screenPt) const noexcept;
    juce::Point<float> worldToScreen(juce::Point<float> worldPt) const noexcept;
    juce::Rectangle<float> screenToWorld(juce::Rectangle<float> screenRect) const noexcept;
    juce::Rectangle<float> worldToScreen(juce::Rectangle<float> worldRect) const noexcept;

    // Métodos Component
    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseMagnify(const juce::MouseEvent& e, float scaleFactor) override;

    // Métodos Drag and Drop de Módulos (juce::DragAndDropTarget)
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    // Métodos File Drag and Drop para Respuestas al Impulso IR (juce::FileDragAndDropTarget)
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragMove(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void setOnSampleLoaded(std::function<void()> cb) { onSampleLoaded_ = std::move(cb); }

    // Encadenamiento automático Drag & Drop entre nodos
    void handleNodeDragging(NodeId draggedNodeId, juce::Rectangle<int> draggedBounds);
    void handleNodeDropped(NodeId draggedNodeId, juce::Rectangle<int> draggedBounds);
    void handleExternalPan(const juce::MouseEvent& e, bool isStart, bool isEnd);

    // Accesores internos para CanvasWorldComponent
    N8AudioProcessor& getProcessor() const noexcept { return processor_; }
    bool isDraggingWire() const noexcept { return isDraggingWire_; }
    bool isDragWireInvalid() const noexcept { return isDragWireInvalid_; }
    juce::Point<float> getDragStartPt() const noexcept { return dragStartPt_; }
    juce::Point<float> getCurrentMousePt() const noexcept { return currentMousePt_; }
    PinDataType getDragDataType() const noexcept { return dragDataType_; }

private:
    friend class CanvasWorldComponent;
    friend class ZoomHudOverlay;

    void updateWorldTransform();
    void updateZoomHud();
    void showCanvasContextMenu(juce::Point<float> screenPos);
    void loadIRFileIntoNode(NodeId id, const juce::File& file);
    void loadSampleFileIntoNode(NodeId id, const juce::File& file);
    bool findPinAtCanvasPos(juce::Point<float> pos, NodeId& outNodeId, PinId& outPinId, PinType& outType, PinDataType& outDataType, juce::Point<float>& outCenter, float tolerance = 18.0f) const;
    ConnectionId findConnectionNear(juce::Point<float> pos, float threshold = 8.0f) const;

    N8AudioProcessor& processor_;

    // Capa virtual del mundo (alberga nodos, grupos y cables transformados)
    std::unique_ptr<CanvasWorldComponent> worldComp_;
    std::unique_ptr<ZoomHudOverlay> zoomHud_;

    std::vector<std::unique_ptr<NodeComponent>> nodeComponents_;
    std::vector<std::unique_ptr<NodeGroupComponent>> nodeGroups_;
    NodeId selectedNodeId_{ InvalidNodeId };
    std::unordered_set<NodeId> selectedNodeIds_;
    std::function<void(NodeId)> onNodeSelected_;
    std::function<void()> onSampleLoaded_;
    std::function<bool(const juce::KeyPress&)> onPianoKeyForward_;
    std::function<bool(bool)> onPianoKeyStateChanged_;
    std::function<bool()> isPianoVisibleCallback_;

    // Parámetros de Zoom & Paneo
    float zoomScale_{ 1.0f };
    juce::Point<float> panOffset_{ 0.0f, 0.0f };
    bool hasAutoFittedInitial_{ false };

    // Estado de arrastre y snapping de cables
    bool isDraggingWire_{ false };
    bool isDragWireInvalid_{ false };
    NodeId dragSourceNode_{ InvalidNodeId };
    PinId dragSourcePin_{ InvalidPinId };
    PinType dragSourcePinType_{ PinType::AudioOutput };
    PinDataType dragDataType_{ PinDataType::AudioStereo };
    juce::Point<float> dragStartPt_;
    juce::Point<float> currentMousePt_;
    NodeId snappedTargetNode_{ InvalidNodeId };
    PinId snappedTargetPin_{ InvalidPinId };

    // Selección elástica múltiple (Marquee / Rubberband)
    bool isMarqueeSelecting_{ false };
    juce::Point<float> marqueeStartPt_;
    juce::Rectangle<float> marqueeRect_;

    // Paneo del canvas (Regla 48)
    bool isPanning_{ false };
    juce::Point<float> panStartOffset_{ 0.0f, 0.0f };
    juce::Point<float> panStartMouse_{ 0.0f, 0.0f };
    bool isRightMouseDown_{ false };
    juce::Point<float> rightMouseDownPos_{ 0.0f, 0.0f };

    // Vista previa fantasma Drag & Drop
    bool isShowingDragGhost_{ false };
    juce::Point<float> ghostPos_;
    juce::String ghostLabel_;
    NodeId ghostCandidateTarget_{ InvalidNodeId };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphCanvasComponent)
};

} // namespace audio_graph
