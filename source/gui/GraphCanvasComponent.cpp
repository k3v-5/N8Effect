#include "GraphCanvasComponent.h"
#include "../plugin/PluginProcessor.h"
#include <cmath>

namespace audio_graph {

GraphCanvasComponent::GraphCanvasComponent(N8AudioProcessor& processor)
    : processor_(processor)
{
    setRepaintsOnMouseActivity(true);
    rebuildFromGraph();
}

NodeComponent* GraphCanvasComponent::findNodeComponent(NodeId id) const {
    for (const auto& comp : nodeComponents_) {
        if (comp && comp->getNodeId() == id) {
            return comp.get();
        }
    }
    return nullptr;
}

bool GraphCanvasComponent::findPinAtCanvasPos(juce::Point<float> pos,
                                              NodeId& outNodeId,
                                              PinId& outPinId,
                                              PinType& outType,
                                              PinDataType& outDataType,
                                              juce::Point<float>& outCenter,
                                              float tolerance) const
{
    for (const auto& comp : nodeComponents_) {
        if (comp != nullptr) {
            if (comp->hitTestPin(pos, outPinId, outType, outDataType, outCenter, tolerance)) {
                outNodeId = comp->getNodeId();
                return true;
            }
        }
    }
    return false;
}

ConnectionId GraphCanvasComponent::findConnectionNear(juce::Point<float> pos, float threshold) const {
    const auto& connections = processor_.getGraph().getConnections();
    const float threshSq = threshold * threshold;

    for (const auto& c : connections) {
        const NodeComponent* src = findNodeComponent(c.sourceNodeId);
        const NodeComponent* dest = findNodeComponent(c.destNodeId);
        if (src == nullptr || dest == nullptr) continue;

        const juce::Point<float> p1 = src->getPinCenterInCanvas(c.sourcePinId);
        const juce::Point<float> p2 = dest->getPinCenterInCanvas(c.destPinId);

        // Control points para curva Bézier cúbica idéntica a WireRenderer
        const float dx = std::abs(p2.x - p1.x);
        const float offset = std::max(40.0f, dx * 0.5f);
        const juce::Point<float> c1(p1.x + offset, p1.y);
        const juce::Point<float> c2(p2.x - offset, p2.y);

        // Muestrear 24 puntos a lo largo de la curva
        constexpr int numSamples = 24;
        for (int i = 0; i <= numSamples; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(numSamples);
            const float u = 1.0f - t;
            const float tt = t * t;
            const float uu = u * u;
            const float uuu = uu * u;
            const float ttt = tt * t;

            const float bx = uuu * p1.x + 3.0f * uu * t * c1.x + 3.0f * u * tt * c2.x + ttt * p2.x;
            const float by = uuu * p1.y + 3.0f * uu * t * c1.y + 3.0f * u * tt * c2.y + ttt * p2.y;

            const float distSq = (pos.x - bx) * (pos.x - bx) + (pos.y - by) * (pos.y - by);
            if (distSq <= threshSq) {
                return c.id;
            }
        }
    }
    return 0;
}

void GraphCanvasComponent::disconnectPin(NodeId nodeId, PinId pinId) {
    if (processor_.disconnectPin(nodeId, pinId)) {
        rebuildFromGraph();
    }
}

void GraphCanvasComponent::deleteConnection(ConnectionId cid) {
    if (processor_.disconnectConnection(cid)) {
        repaint();
    }
}

void GraphCanvasComponent::rebuildFromGraph() {
    nodeComponents_.clear();

    const auto& nodes = processor_.getGraph().getNodes();
    for (const auto& [nodeId, inst] : nodes) {
        if (!inst || !inst->processor) continue;

        auto comp = std::make_unique<NodeComponent>(nodeId, inst->name, inst->type, inst->processor.get());
        comp->setTopLeftPosition(static_cast<int>(inst->posX), static_cast<int>(inst->posY));
        comp->setSelected(nodeId == selectedNodeId_);

        comp->setOnNodeSelected([this](NodeId id) {
            selectNode(id);
        });

        comp->setOnNodeMoved([this](NodeId id, float x, float y) {
            auto* nodeInst = processor_.getGraph().getNode(id);
            if (nodeInst != nullptr) {
                nodeInst->posX = x;
                nodeInst->posY = y;
            }
            repaint();
        });

        comp->setOnNodeDeleted([this](NodeId id) {
            juce::MessageManager::callAsync([this, id]() {
                deleteNode(id);
            });
        });

        comp->setOnPinDragStarted([this](NodeId srcNode, PinId srcPin, PinDataType type, juce::Point<float> pt) {
            isDraggingWire_ = true;
            dragSourceNode_ = srcNode;
            dragSourcePin_ = srcPin;
            dragDataType_ = type;
            dragStartPt_ = pt;
            currentMousePt_ = pt;
            snappedTargetNode_ = InvalidNodeId;
            snappedTargetPin_ = InvalidPinId;
            repaint();
        });

        comp->setOnPinDragging([this](juce::Point<float> canvasPos) {
            currentMousePt_ = canvasPos;
            NodeId candidateNode = InvalidNodeId;
            PinId candidatePin = InvalidPinId;
            PinType candidateType = PinType::AudioInput;
            PinDataType candidateDataType = PinDataType::AudioStereo;
            juce::Point<float> candidateCenter;

            if (findPinAtCanvasPos(canvasPos, candidateNode, candidatePin, candidateType, candidateDataType, candidateCenter, 20.0f)) {
                if (candidateNode != dragSourceNode_) {
                    currentMousePt_ = candidateCenter; // Snapping magnético
                    snappedTargetNode_ = candidateNode;
                    snappedTargetPin_ = candidatePin;
                    for (auto& n : nodeComponents_) {
                        n->setHighlightedPin(n->getNodeId() == candidateNode ? candidatePin : InvalidPinId);
                    }
                }
            } else {
                snappedTargetNode_ = InvalidNodeId;
                snappedTargetPin_ = InvalidPinId;
                for (auto& n : nodeComponents_) {
                    n->setHighlightedPin(InvalidPinId);
                }
            }
            repaint();
        });

        comp->setOnPinDragEnded([this](NodeId srcNode, PinId srcPin, PinType srcPinType, PinDataType /*srcDataType*/, juce::Point<float> canvasPos) {
            for (auto& n : nodeComponents_) {
                n->setHighlightedPin(InvalidPinId);
            }
            isDraggingWire_ = false;

            NodeId destNode = snappedTargetNode_;
            PinId destPin = snappedTargetPin_;
            PinType destPinType = PinType::AudioInput;
            PinDataType destDataType = PinDataType::AudioStereo;
            juce::Point<float> pinCenter;

            if (destNode == InvalidNodeId) {
                findPinAtCanvasPos(canvasPos, destNode, destPin, destPinType, destDataType, pinCenter, 22.0f);
            }

            if (destNode != InvalidNodeId && destNode != srcNode) {
                // Conexión bidireccional automática: out -> in o in -> out
                if (srcPinType == PinType::AudioOutput || srcPinType == PinType::EventOutput) {
                    processor_.connectNodes(srcNode, srcPin, destNode, destPin);
                } else {
                    processor_.connectNodes(destNode, destPin, srcNode, srcPin);
                }
                juce::MessageManager::callAsync([this]() {
                    rebuildFromGraph();
                });
            }

            snappedTargetNode_ = InvalidNodeId;
            snappedTargetPin_ = InvalidPinId;
            repaint();
        });

        comp->setOnPinRightClicked([this](NodeId id, PinId pin) {
            juce::MessageManager::callAsync([this, id, pin]() {
                disconnectPin(id, pin);
            });
        });

        comp->setOnNodeDraggingOverCanvas([this](NodeId id, juce::Rectangle<int> bounds) {
            handleNodeDragging(id, bounds);
        });

        comp->setOnNodeDropped([this](NodeId id, juce::Rectangle<int> bounds) {
            juce::MessageManager::callAsync([this, id, bounds]() {
                handleNodeDropped(id, bounds);
            });
        });

        addAndMakeVisible(*comp);
        comp->toFront(false);
        nodeComponents_.push_back(std::move(comp));
    }

    repaint();
}

void GraphCanvasComponent::handleNodeDragging(NodeId draggedNodeId, juce::Rectangle<int> draggedBounds) {
    const auto draggedCentre = draggedBounds.getCentre().toFloat();
    NodeId candidateTarget = InvalidNodeId;

    for (const auto& comp : nodeComponents_) {
        if (comp && comp->getNodeId() != draggedNodeId) {
            const float dist = draggedCentre.getDistanceFrom(comp->getBounds().getCentre().toFloat());
            if (dist < 190.0f) {
                candidateTarget = comp->getNodeId();
                break;
            }
        }
    }

    for (auto& comp : nodeComponents_) {
        comp->setDropCandidate(comp->getNodeId() == candidateTarget);
    }
}

void GraphCanvasComponent::handleNodeDropped(NodeId draggedNodeId, juce::Rectangle<int> draggedBounds) {
    for (auto& comp : nodeComponents_) {
        comp->setDropCandidate(false);
    }

    const auto draggedCentre = draggedBounds.getCentre().toFloat();
    NodeComponent* targetComp = nullptr;

    for (const auto& comp : nodeComponents_) {
        if (comp && comp->getNodeId() != draggedNodeId) {
            const float dist = draggedCentre.getDistanceFrom(comp->getBounds().getCentre().toFloat());
            if (dist < 190.0f) {
                targetComp = comp.get();
                break;
            }
        }
    }

    if (targetComp != nullptr) {
        auto* draggedNodeInst = processor_.getGraph().getNode(draggedNodeId);
        auto* targetNodeInst = processor_.getGraph().getNode(targetComp->getNodeId());

        if (draggedNodeInst != nullptr && targetNodeInst != nullptr) {
            // Si el nodo arrastrado está a la derecha del objetivo: target -> dragged
            if (draggedBounds.getX() >= targetComp->getX()) {
                processor_.connectNodes(targetComp->getNodeId(), 2, draggedNodeId, 1);
                draggedNodeInst->posX = static_cast<float>(targetComp->getRight() + 35);
                draggedNodeInst->posY = static_cast<float>(targetComp->getY());
            } else { // Si está a la izquierda: dragged -> target
                processor_.connectNodes(draggedNodeId, 2, targetComp->getNodeId(), 1);
                targetNodeInst->posX = static_cast<float>(draggedBounds.getRight() + 35);
                targetNodeInst->posY = static_cast<float>(draggedBounds.getY());
            }
            rebuildFromGraph();
        }
    }
}

void GraphCanvasComponent::selectNode(NodeId id) {
    selectedNodeId_ = id;
    for (auto& c : nodeComponents_) {
        c->setSelected(c->getNodeId() == id);
    }
    if (onNodeSelected_) {
        onNodeSelected_(id);
    }
}

void GraphCanvasComponent::addNodeAtPosition(NodeType type, float x, float y) {
    const float minX = 15.0f;
    const float minY = 15.0f;
    const float maxX = static_cast<float>(std::max(15, getWidth() - 195));
    const float maxY = static_cast<float>(std::max(15, getHeight() - 120));

    NodeId connectFrom = InvalidNodeId;
    if (selectedNodeId_ != InvalidNodeId) {
        if (auto* sel = findNodeComponent(selectedNodeId_)) {
            connectFrom = selectedNodeId_;
            x = sel->getRight() + 30.0f;
            y = static_cast<float>(sel->getY());
            if (x > maxX) {
                x = static_cast<float>(sel->getX());
                y = static_cast<float>(sel->getBottom()) + 20.0f;
            }
        }
    }

    x = std::clamp(x, minX, maxX);
    y = std::clamp(y, minY, maxY);

    const NodeId newId = processor_.addNodeToGraph(type, x, y);
    if (newId != InvalidNodeId && connectFrom != InvalidNodeId) {
        processor_.connectNodes(connectFrom, 2, newId, 1);
    }

    selectedNodeId_ = newId;
    rebuildFromGraph();
    repaint();
}

void GraphCanvasComponent::deleteNode(NodeId id) {
    processor_.removeNodeFromGraph(id);
    rebuildFromGraph();
}

void GraphCanvasComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();

    // 1. Fondo negro absoluto del canvas
    g.setColour(juce::Colour(0xff000000));
    g.fillRect(bounds);

    // 2. Rejilla minimalista de micropuntos blancos de precisión
    constexpr float gridSize = 20.0f;
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    for (float x = 0.0f; x < bounds.getWidth(); x += gridSize) {
        for (float y = 0.0f; y < bounds.getHeight(); y += gridSize) {
            g.fillRect(x, y, 1.5f, 1.5f);
        }
    }

    // 3. Contorno blanco del marco del canvas
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawRect(bounds, 1.0f);

    // 4. Renderizar todos los cables existentes en el Grafo
    const auto& connections = processor_.getGraph().getConnections();
    for (const auto& c : connections) {
        NodeComponent* src = findNodeComponent(c.sourceNodeId);
        NodeComponent* dest = findNodeComponent(c.destNodeId);

        if (src != nullptr && dest != nullptr) {
            const juce::Point<float> p1 = src->getPinCenterInCanvas(c.sourcePinId);
            const juce::Point<float> p2 = dest->getPinCenterInCanvas(c.destPinId);
            WireRenderer::drawWire(g, p1, p2, PinDataType::AudioStereo, false, false);
        }
    }

    // 5. Renderizar cable temporal en arrastre activo
    if (isDraggingWire_) {
        WireRenderer::drawWire(g, dragStartPt_, currentMousePt_, dragDataType_, true, true);
    }

    // 6. Renderizar previsualización fantasma al arrastrar módulos desde la paleta
    if (isShowingDragGhost_) {
        juce::Rectangle<float> ghostBox(ghostPos_.x - 90.0f, ghostPos_.y - 40.0f, 180.0f, 80.0f);
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.fillRoundedRectangle(ghostBox, 4.0f);
        g.setColour(juce::Colours::white);
        const float dashes[] = { 4.0f, 3.0f };
        g.drawRoundedRectangle(ghostBox, 4.0f, 1.5f);

        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText("+ " + ghostLabel_.toUpperCase(), ghostBox, juce::Justification::centred, false);
    }
}

void GraphCanvasComponent::resized() {
}

void GraphCanvasComponent::mouseDown(const juce::MouseEvent& e) {
    selectNode(InvalidNodeId);

    // Si se hace clic sobre un cable existente, seleccionarlo/eliminarlo
    const ConnectionId hitConn = findConnectionNear(e.position, 8.0f);
    if (hitConn != 0) {
        deleteConnection(hitConn);
        return;
    }

    if (e.mods.isRightButtonDown()) {
        isPanning_ = true;
        panStartOffset_ = e.position;
    }
}

void GraphCanvasComponent::mouseDrag(const juce::MouseEvent& e) {
    if (isDraggingWire_) {
        currentMousePt_ = e.position;
        repaint();
    }
}

void GraphCanvasComponent::mouseUp(const juce::MouseEvent& /*e*/) {
    if (isDraggingWire_) {
        isDraggingWire_ = false;
        for (auto& n : nodeComponents_) {
            n->setHighlightedPin(InvalidPinId);
        }
        repaint();
    }
    isPanning_ = false;
}

// Implementación de juce::DragAndDropTarget
bool GraphCanvasComponent::isInterestedInDragSource(const SourceDetails& dragSourceDetails) {
    return dragSourceDetails.description.toString().startsWith("N8_MODULE_TYPE:");
}

void GraphCanvasComponent::itemDragEnter(const SourceDetails& dragSourceDetails) {
    isShowingDragGhost_ = true;
    ghostLabel_ = dragSourceDetails.description.toString().fromFirstOccurrenceOf("N8_MODULE_NAME:", false, false);
    ghostPos_ = dragSourceDetails.localPosition.toFloat();
    repaint();
}

void GraphCanvasComponent::itemDragMove(const SourceDetails& dragSourceDetails) {
    ghostPos_ = dragSourceDetails.localPosition.toFloat();

    NodeComponent* targetComp = nullptr;
    for (const auto& comp : nodeComponents_) {
        if (comp && comp->getBounds().toFloat().expanded(16.0f).contains(ghostPos_)) {
            targetComp = comp.get();
            break;
        }
    }

    for (auto& comp : nodeComponents_) {
        comp->setDropCandidate(comp.get() == targetComp);
    }

    repaint();
}

void GraphCanvasComponent::itemDragExit(const SourceDetails& /*dragSourceDetails*/) {
    isShowingDragGhost_ = false;
    for (auto& comp : nodeComponents_) {
        comp->setDropCandidate(false);
    }
    repaint();
}

void GraphCanvasComponent::itemDropped(const SourceDetails& dragSourceDetails) {
    isShowingDragGhost_ = false;
    for (auto& comp : nodeComponents_) {
        comp->setDropCandidate(false);
    }

    const auto desc = dragSourceDetails.description.toString();
    const auto typeStr = desc.fromFirstOccurrenceOf("N8_MODULE_TYPE:", false, false).upToFirstOccurrenceOf("|", false, false);
    if (typeStr.isEmpty()) return;

    const NodeType type = static_cast<NodeType>(typeStr.getIntValue());
    if (type == NodeType::Unknown) return;

    const auto dropPos = dragSourceDetails.localPosition.toFloat();

    const float minX = 15.0f;
    const float minY = 15.0f;
    const float maxX = static_cast<float>(std::max(15, getWidth() - 195));
    const float maxY = static_cast<float>(std::max(15, getHeight() - 120));

    // 1. Verificar si se soltó directamente sobre un nodo existente para auto-encadenamiento
    NodeComponent* targetComp = nullptr;
    for (const auto& comp : nodeComponents_) {
        if (comp && comp->getBounds().toFloat().contains(dropPos)) {
            targetComp = comp.get();
            break;
        }
    }

    float spawnX = 0.0f;
    float spawnY = 0.0f;

    if (targetComp != nullptr) {
        // Auto-encadenar: ubicar a la derecha si cabe, o debajo si no cabe a la derecha
        spawnX = targetComp->getRight() + 30.0f;
        spawnY = static_cast<float>(targetComp->getY());

        if (spawnX > maxX) {
            spawnX = static_cast<float>(targetComp->getX());
            spawnY = static_cast<float>(targetComp->getBottom()) + 20.0f;
        }

        spawnX = std::clamp(spawnX, minX, maxX);
        spawnY = std::clamp(spawnY, minY, maxY);

        const NodeId newId = processor_.addNodeToGraph(type, spawnX, spawnY);
        if (newId != InvalidNodeId) {
            processor_.connectNodes(targetComp->getNodeId(), 2, newId, 1);
        }
        selectedNodeId_ = newId;
    } else {
        // Posición libre: ubicar exactamente centrado bajo el cursor donde el usuario soltó
        spawnX = std::clamp(dropPos.x - 90.0f, minX, maxX);
        spawnY = std::clamp(dropPos.y - 30.0f, minY, maxY);

        const NodeId newId = processor_.addNodeToGraph(type, spawnX, spawnY);
        selectedNodeId_ = newId;
    }

    rebuildFromGraph();
    repaint();

    // Sincronización asíncrona garantizada con el MessageManager tras el ciclo DragAndDrop de JUCE
    juce::MessageManager::callAsync([this]() {
        rebuildFromGraph();
        repaint();
        if (auto* p = getParentComponent()) {
            p->repaint();
        }
    });
}

} // namespace audio_graph
