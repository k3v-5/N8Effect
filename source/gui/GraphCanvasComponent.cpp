#include "GraphCanvasComponent.h"
#include "../plugin/PluginProcessor.h"

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

void GraphCanvasComponent::rebuildFromGraph() {
    nodeComponents_.clear();

    const auto& nodes = processor_.getGraph().getNodes();
    for (const auto& [nodeId, inst] : nodes) {
        if (!inst || !inst->processor) continue;

        auto comp = std::make_unique<NodeComponent>(nodeId, inst->name, inst->type, inst->processor.get());
        comp->setTopLeftPosition(static_cast<int>(inst->posX), static_cast<int>(inst->posY));

        comp->setOnNodeMoved([this](NodeId id, float x, float y) {
            auto* nodeInst = processor_.getGraph().getNode(id);
            if (nodeInst != nullptr) {
                nodeInst->posX = x;
                nodeInst->posY = y;
            }
            repaint();
        });

        comp->setOnNodeDeleted([this](NodeId id) {
            deleteNode(id);
        });

        comp->setOnPinDragStarted([this](NodeId srcNode, PinId srcPin, PinDataType type, juce::Point<float> pt) {
            isDraggingWire_ = true;
            dragSourceNode_ = srcNode;
            dragSourcePin_ = srcPin;
            dragDataType_ = type;
            dragStartPt_ = pt;
            currentMousePt_ = pt;
            repaint();
        });

        comp->setOnPinConnected([this](NodeId destNode, PinId destPin, PinDataType /*type*/) {
            if (isDraggingWire_ && dragSourceNode_ != InvalidNodeId && dragSourceNode_ != destNode) {
                processor_.connectNodes(dragSourceNode_, dragSourcePin_, destNode, destPin);
                isDraggingWire_ = false;
                repaint();
            }
        });

        addAndMakeVisible(*comp);
        nodeComponents_.push_back(std::move(comp));
    }

    repaint();
}

void GraphCanvasComponent::addNodeAtPosition(NodeType type, float x, float y) {
    processor_.addNodeToGraph(type, x, y);
    rebuildFromGraph();
}

void GraphCanvasComponent::deleteNode(NodeId id) {
    processor_.removeNodeFromGraph(id);
    rebuildFromGraph();
}

void GraphCanvasComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();

    // 1. Fondo oscuro del canvas
    g.setColour(juce::Colour(0xff090910));
    g.fillRect(bounds);

    // 2. Rejilla sutil de puntos de fondo
    constexpr float gridSize = 20.0f;
    g.setColour(juce::Colour(0xff181826));
    for (float x = 0.0f; x < bounds.getWidth(); x += gridSize) {
        for (float y = 0.0f; y < bounds.getHeight(); y += gridSize) {
            g.fillRect(x, y, 1.5f, 1.5f);
        }
    }

    // 3. Renderizar todos los cables existentes en el Grafo
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

    // 4. Renderizar cable temporal en arrastre
    if (isDraggingWire_) {
        WireRenderer::drawWire(g, dragStartPt_, currentMousePt_, dragDataType_, true, true);
    }
}

void GraphCanvasComponent::resized() {
    // Si la posición de los nodos es relativa o excede la vista, se adaptan
}

void GraphCanvasComponent::mouseDown(const juce::MouseEvent& e) {
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
        repaint();
    }
    isPanning_ = false;
}

} // namespace audio_graph
