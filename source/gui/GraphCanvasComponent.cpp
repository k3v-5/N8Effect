#include "GraphCanvasComponent.h"
#include "ThemeManager.h"
#include "../plugin/PluginProcessor.h"
#include "../dsp/processors/ConvolutionNode.h"
#include "../dsp/processors/SamplePlayerNode.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cmath>

namespace audio_graph {

class CanvasWorldComponent : public juce::Component {
public:
    explicit CanvasWorldComponent(GraphCanvasComponent& owner) : owner_(owner) {
        setPaintingIsUnclipped(true);
    }

    void paint(juce::Graphics& g) override {
        // 1. Renderizar todos los cables existentes en el Grafo
        const auto& connections = owner_.getProcessor().getGraph().getConnections();
        for (const auto& c : connections) {
            NodeComponent* src = owner_.findNodeComponent(c.sourceNodeId);
            NodeComponent* dest = owner_.findNodeComponent(c.destNodeId);

            if (src != nullptr && dest != nullptr && src->isVisible() && dest->isVisible()) {
                const juce::Point<float> p1 = src->getPinCenterInCanvas(c.sourcePinId);
                const juce::Point<float> p2 = dest->getPinCenterInCanvas(c.destPinId);
                WireRenderer::drawWire(g, p1, p2, PinDataType::AudioStereo, false, false);
            }
        }

        // 2. Renderizar cable temporal en arrastre activo
        if (owner_.isDraggingWire()) {
            WireRenderer::drawWire(g, owner_.getDragStartPt(), owner_.getCurrentMousePt(),
                                   owner_.getDragDataType(), true, true, owner_.isDragWireInvalid());
        }
    }

    bool hitTest(int x, int y) override {
        // Interceptar el clic únicamente si impacta sobre un nodo o caja de grupo interactiva.
        // NOTA CRÍTICA: NUNCA llamar a reallyContains() aquí, ya que reallyContains() llama a
        // top->getComponentAt(), creando una recursión infinita letal (0xc00000fd Stack Overflow).
        const juce::Point<int> pt(x, y);

        for (int i = getNumChildComponents() - 1; i >= 0; --i) {
            auto* child = getChildComponent(i);
            if (child != nullptr && child->isVisible()) {
                const auto b = child->getBounds();
                if (b.contains(pt)) {
                    const int localX = x - b.getX();
                    const int localY = y - b.getY();
                    if (child->hitTest(localX, localY)) {
                        return true;
                    }
                }
            }
        }
        // El espacio vacío y los cables caen a través hacia GraphCanvasComponent
        return false;
    }

    bool keyPressed(const juce::KeyPress& key) override {
        return owner_.keyPressed(key);
    }

    bool keyStateChanged(bool isKeyDown) override {
        return owner_.keyStateChanged(isKeyDown);
    }

private:
    GraphCanvasComponent& owner_;
};

class HudButton : public Rule48Button {
public:
    explicit HudButton(const juce::String& text, bool isAccent = false)
        : Rule48Button(text), isAccent_(isAccent)
    {
        setButtonText(text);
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        auto b = getLocalBounds().toFloat().reduced(1.0f);

        if (shouldDrawButtonAsDown) {
            g.setColour(juce::Colour(0x4400d4ff));
            g.fillRoundedRectangle(b, 3.0f);
        } else if (shouldDrawButtonAsHighlighted) {
            g.setColour(juce::Colour(0x22ffffff));
            g.fillRoundedRectangle(b, 3.0f);
        }

        juce::Colour textCol = isAccent_ ? juce::Colour(0xff00d4ff) : juce::Colour(0xffd0d8e2);
        if (shouldDrawButtonAsHighlighted) {
            textCol = juce::Colour(0xffffffff);
        }

        g.setColour(textCol);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(getButtonText(), getLocalBounds(), juce::Justification::centred, false);
    }

private:
    bool isAccent_{ false };
};

class ZoomHudOverlay : public juce::Component {
public:
    explicit ZoomHudOverlay(GraphCanvasComponent& owner) : owner_(owner) {
        setInterceptsMouseClicks(true, true);

        btnZoomOut_ = std::make_unique<HudButton>("-");
        btnZoomOut_->setTooltip("Zoom Out (Minus / Wheel Down)");
        btnZoomOut_->setOnCleanClick([this]() { owner_.zoomOut(); });
        addAndMakeVisible(*btnZoomOut_);

        btnZoomReset_ = std::make_unique<HudButton>("100%", true);
        btnZoomReset_->setTooltip("Reset Zoom to 100% (Key 0/1)");
        btnZoomReset_->setOnCleanClick([this]() { owner_.resetZoom(); });
        addAndMakeVisible(*btnZoomReset_);

        btnZoomIn_ = std::make_unique<HudButton>("+");
        btnZoomIn_->setTooltip("Zoom In (Plus / Wheel Up)");
        btnZoomIn_->setOnCleanClick([this]() { owner_.zoomIn(); });
        addAndMakeVisible(*btnZoomIn_);

        btnZoomFit_ = std::make_unique<HudButton>("FIT", true);
        btnZoomFit_->setTooltip("Fit All Nodes into View (Key F)");
        btnZoomFit_->setOnCleanClick([this]() { owner_.zoomToFit(); });
        addAndMakeVisible(*btnZoomFit_);
    }

    void updateZoom(float zoom) {
        if (btnZoomReset_) {
            const int pct = static_cast<int>(std::round(zoom * 100.0f));
            btnZoomReset_->setButtonText(juce::String(pct) + "%");
            repaint();
        }
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        // Fondo translúcido neo-analógico
        g.setColour(juce::Colour(0xf011151e));
        g.fillRoundedRectangle(bounds, 5.0f);

        // Borde elegante
        g.setColour(juce::Colour(0xff222a38));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, 1.0f);

        // Divisores verticales sutiles
        g.setColour(juce::Colour(0x22ffffff));
        if (btnZoomReset_) g.drawVerticalLine(btnZoomReset_->getX(), 4.0f, bounds.getBottom() - 4.0f);
        if (btnZoomIn_) g.drawVerticalLine(btnZoomIn_->getX(), 4.0f, bounds.getBottom() - 4.0f);
        if (btnZoomFit_) g.drawVerticalLine(btnZoomFit_->getX(), 4.0f, bounds.getBottom() - 4.0f);
    }

    void resized() override {
        auto area = getLocalBounds();
        const int outW = 26;
        const int resetW = 54;
        const int inW = 26;

        if (btnZoomOut_) btnZoomOut_->setBounds(area.removeFromLeft(outW));
        if (btnZoomReset_) btnZoomReset_->setBounds(area.removeFromLeft(resetW));
        if (btnZoomIn_) btnZoomIn_->setBounds(area.removeFromLeft(inW));
        if (btnZoomFit_) btnZoomFit_->setBounds(area);
    }

private:
    GraphCanvasComponent& owner_;
    std::unique_ptr<HudButton> btnZoomOut_;
    std::unique_ptr<HudButton> btnZoomReset_;
    std::unique_ptr<HudButton> btnZoomIn_;
    std::unique_ptr<HudButton> btnZoomFit_;
};

GraphCanvasComponent::GraphCanvasComponent(N8AudioProcessor& processor)
    : processor_(processor)
{
    setRepaintsOnMouseActivity(true);
    setWantsKeyboardFocus(true);

    worldComp_ = std::make_unique<CanvasWorldComponent>(*this);
    addAndMakeVisible(*worldComp_);

    zoomHud_ = std::make_unique<ZoomHudOverlay>(*this);
    addAndMakeVisible(*zoomHud_);
    zoomHud_->toFront(false);

    rebuildFromGraph();
}

GraphCanvasComponent::~GraphCanvasComponent() = default;

NodeComponent* GraphCanvasComponent::findNodeComponent(NodeId id) const {
    for (const auto& comp : nodeComponents_) {
        if (comp && comp->getNodeId() == id) {
            return comp.get();
        }
    }
    return nullptr;
}

NodeGroupComponent* GraphCanvasComponent::findGroupComponent(GroupId id) const {
    for (const auto& grp : nodeGroups_) {
        if (grp && grp->getGroupId() == id) {
            return grp.get();
        }
    }
    return nullptr;
}

void GraphCanvasComponent::createGroup(const std::vector<NodeId>& nodeIds, std::string_view name) {
    processor_.getGraph().addGroup(name, kGroupColorPalette[0], nodeIds);
    rebuildFromGraph();
}

void GraphCanvasComponent::removeGroup(GroupId id) {
    processor_.getGraph().removeGroup(id);
    rebuildFromGraph();
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
        if (worldComp_) worldComp_->repaint();
        repaint();
    }
}

void GraphCanvasComponent::rebuildFromGraph() {
    nodeComponents_.clear();
    nodeGroups_.clear();

    const auto& nodes = processor_.getGraph().getNodes();
    for (const auto& [nodeId, inst] : nodes) {
        if (!inst || !inst->processor) continue;

        auto comp = std::make_unique<NodeComponent>(nodeId, inst->name, inst->type, inst->processor.get());
        comp->setTopLeftPosition(static_cast<int>(inst->posX), static_cast<int>(inst->posY));
        comp->setSelected(nodeId == selectedNodeId_ || selectedNodeIds_.contains(nodeId));
        comp->setBypassed(inst->isBypassed);

        comp->setOnBypassToggled([this](NodeId id, bool bypassed) {
            processor_.setNodeBypassed(id, bypassed);
        });

        comp->setOnNodeSelected([this](NodeId id) {
            selectNode(id);
        });

        comp->setOnNodeMoved([this](NodeId id, float x, float y) {
            auto* nodeInst = processor_.getGraph().getNode(id);
            if (nodeInst != nullptr) {
                nodeInst->posX = x;
                nodeInst->posY = y;
            }
            for (auto& grpComp : nodeGroups_) {
                if (grpComp) grpComp->updateBoundsFromMembers();
            }
            if (worldComp_) worldComp_->repaint();
            repaint();
        });

        comp->setOnCanvasPanRequested([this](const juce::MouseEvent& e, bool isStart, bool isEnd) {
            handleExternalPan(e, isStart, isEnd);
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
            if (worldComp_) worldComp_->repaint();
            repaint();
        });

        comp->setOnPinDragging([this](juce::Point<float> canvasPos) {
            currentMousePt_ = canvasPos;
            NodeId candidateNode = InvalidNodeId;
            PinId candidatePin = InvalidPinId;
            PinType candidateType = PinType::AudioInput;
            PinDataType candidateDataType = PinDataType::AudioStereo;
            juce::Point<float> candidateCenter;

            const float snapTol = 20.0f / zoomScale_;
            if (findPinAtCanvasPos(canvasPos, candidateNode, candidatePin, candidateType, candidateDataType, candidateCenter, snapTol)) {
                if (candidateNode != dragSourceNode_) {
                    currentMousePt_ = candidateCenter; // Snapping magnetico
                    snappedTargetNode_ = candidateNode;
                    snappedTargetPin_ = candidatePin;

                    // Validacion predictiva de ciclo e incompatibilidad de tipo (Reglas 4, 15, 28)
                    const NodeId effSrc = (dragSourcePinType_ == PinType::AudioOutput || dragSourcePinType_ == PinType::EventOutput) ? dragSourceNode_ : candidateNode;
                    const NodeId effDest = (effSrc == dragSourceNode_) ? candidateNode : dragSourceNode_;
                    const bool cycle = processor_.getGraph().wouldCreateCycle(effSrc, effDest);
                    const bool typeConflict = (dragSourcePinType_ == candidateType) || (dragDataType_ != candidateDataType);
                    isDragWireInvalid_ = cycle || typeConflict;

                    for (auto& n : nodeComponents_) {
                        n->setHighlightedPin(n->getNodeId() == candidateNode ? candidatePin : InvalidPinId);
                    }
                }
            } else {
                snappedTargetNode_ = InvalidNodeId;
                snappedTargetPin_ = InvalidPinId;
                isDragWireInvalid_ = false;
                for (auto& n : nodeComponents_) {
                    n->setHighlightedPin(InvalidPinId);
                }
            }
            if (worldComp_) worldComp_->repaint();
            repaint();
        });

        comp->setOnPinDragEnded([this](NodeId srcNode, PinId srcPin, PinType srcPinType, PinDataType /*srcDataType*/, juce::Point<float> canvasPos) {
            for (auto& n : nodeComponents_) {
                n->setHighlightedPin(InvalidPinId);
            }
            isDraggingWire_ = false;

            if (isDragWireInvalid_) {
                // Rechazo visual y funcional inmediato: no crea la conexion invalida (Reglas 15, 28)
                snappedTargetNode_ = InvalidNodeId;
                snappedTargetPin_ = InvalidPinId;
                isDragWireInvalid_ = false;
                if (worldComp_) worldComp_->repaint();
                repaint();
                return;
            }

            NodeId destNode = snappedTargetNode_;
            PinId destPin = snappedTargetPin_;
            PinType destPinType = PinType::AudioInput;
            PinDataType destDataType = PinDataType::AudioStereo;
            juce::Point<float> pinCenter;

            const float connectTol = 22.0f / zoomScale_;
            if (destNode == InvalidNodeId) {
                findPinAtCanvasPos(canvasPos, destNode, destPin, destPinType, destDataType, pinCenter, connectTol);
            }

            if (destNode != InvalidNodeId && destNode != srcNode) {
                // Conexion bidireccional automatica: out -> in o in -> out
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
            isDragWireInvalid_ = false;
            if (worldComp_) worldComp_->repaint();
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

        comp->setOnModulationRouteAdded([this](NodeId nid, ParameterId pid, ModSourceType src, float depth) {
            auto& mat = processor_.getDualWorldEngine().getModulationEngine().getMatrix();
            mat.addRoute(src, nid, pid, depth, true);
        });

        comp->setOnModulationDepthChanged([this](NodeId nid, ParameterId pid, float depth) {
            auto& mat = processor_.getDualWorldEngine().getModulationEngine().getMatrix();
            for (size_t r = 0; r < mat.getMaxRoutes(); ++r) {
                auto& route = mat.getRoute(r);
                if (route.isActive && route.targetNodeId == nid && route.targetParamId == pid) {
                    mat.setRouteAmount(r, depth);
                }
            }
        });

        comp->setOnModulationRouteRemoved([this](NodeId nid, ParameterId pid) {
            auto& mat = processor_.getDualWorldEngine().getModulationEngine().getMatrix();
            for (size_t r = 0; r < mat.getMaxRoutes(); ++r) {
                const auto& route = mat.getRoute(r);
                if (route.isActive && route.targetNodeId == nid && route.targetParamId == pid) {
                    mat.removeRoute(r);
                }
            }
        });

        // Restaurar estado visual de modulación existente
        const auto& mat = processor_.getDualWorldEngine().getModulationEngine().getMatrix();
        for (size_t r = 0; r < mat.getMaxRoutes(); ++r) {
            const auto& route = mat.getRoute(r);
            if (route.isActive && route.targetNodeId == nodeId) {
                if (auto* slider = comp->getSliderForParameter(route.targetParamId)) {
                    const auto col = ModulationDragPayload::getDefaultColor(route.source);
                    slider->setModulationDepth(route.amount, col);
                }
            }
        }

        if (worldComp_) {
            worldComp_->addAndMakeVisible(*comp);
        }
        comp->toFront(false);
        nodeComponents_.push_back(std::move(comp));
    }

    // Instanciar y sincronizar Cajas de Grupo de Nodos (Regla R2)
    const auto& groups = processor_.getGraph().getGroups();
    for (const auto& [gid, grp] : groups) {
        if (!grp) continue;
        auto groupComp = std::make_unique<NodeGroupComponent>(gid, *this, processor_);
        if (worldComp_) {
            worldComp_->addAndMakeVisible(*groupComp);
        }
        groupComp->toBack(); // Colocar siempre detrás de los nodos y cables
        nodeGroups_.push_back(std::move(groupComp));
    }

    // Asegurar que todos los nodos queden al frente de los grupos
    for (auto& comp : nodeComponents_) {
        comp->toFront(false);
    }

    updateWorldTransform();
    if (worldComp_) worldComp_->repaint();
    repaint();
}

void GraphCanvasComponent::handleExternalPan(const juce::MouseEvent& e, bool isStart, bool isEnd) {
    const auto canvasPos = (e.eventComponent != this)
        ? getLocalPoint(e.eventComponent, e.position).toFloat()
        : e.position;

    if (isStart) {
        isPanning_ = true;
        panStartOffset_ = panOffset_;
        panStartMouse_ = canvasPos;
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        return;
    }

    if (isEnd) {
        isPanning_ = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
        return;
    }

    if (isPanning_) {
        panOffset_ = panStartOffset_ + (canvasPos - panStartMouse_);
        updateWorldTransform();
        repaint();
    }
}

void GraphCanvasComponent::handleNodeDragging(NodeId draggedNodeId, juce::Rectangle<int> draggedBounds) {
    const auto draggedCentre = draggedBounds.getCentre().toFloat();
    NodeId candidateTarget = InvalidNodeId;

    for (const auto& comp : nodeComponents_) {
        if (comp && comp->getNodeId() != draggedNodeId) {
            // Solo considerar acople automático si se solapa directamente sobre el otro nodo
            if (draggedBounds.intersects(comp->getBounds().reduced(20))) {
                const float dist = draggedCentre.getDistanceFrom(comp->getBounds().getCentre().toFloat());
                if (dist < 60.0f) {
                    candidateTarget = comp->getNodeId();
                    break;
                }
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

    // Preservar fielmente las coordenadas exactas donde el usuario colocó el nodo (sin saltos forzados)
    auto* draggedNodeInst = processor_.getGraph().getNode(draggedNodeId);
    if (draggedNodeInst != nullptr) {
        draggedNodeInst->posX = static_cast<float>(draggedBounds.getX());
        draggedNodeInst->posY = static_cast<float>(draggedBounds.getY());
    }

    const auto draggedCentre = draggedBounds.getCentre().toFloat();
    NodeComponent* targetComp = nullptr;

    for (const auto& comp : nodeComponents_) {
        if (comp && comp->getNodeId() != draggedNodeId) {
            if (draggedBounds.intersects(comp->getBounds().reduced(20))) {
                const float dist = draggedCentre.getDistanceFrom(comp->getBounds().getCentre().toFloat());
                if (dist < 60.0f) {
                    targetComp = comp.get();
                    break;
                }
            }
        }
    }

    if (targetComp != nullptr) {
        if (draggedBounds.getX() >= targetComp->getX()) {
            processor_.connectNodes(targetComp->getNodeId(), 2, draggedNodeId, 1);
        } else {
            processor_.connectNodes(draggedNodeId, 2, targetComp->getNodeId(), 1);
        }
        rebuildFromGraph();
    }
}

void GraphCanvasComponent::selectNode(NodeId id, bool additive) {
    if (!additive) {
        selectedNodeIds_.clear();
    }
    if (id != InvalidNodeId) {
        selectedNodeIds_.insert(id);
    }
    selectedNodeId_ = id;
    for (auto& c : nodeComponents_) {
        c->setSelected(selectedNodeIds_.contains(c->getNodeId()));
    }
    if (onNodeSelected_) {
        onNodeSelected_(id);
    }
    repaint();
}

void GraphCanvasComponent::clearSelection() {
    selectedNodeIds_.clear();
    selectedNodeId_ = InvalidNodeId;
    for (auto& c : nodeComponents_) {
        c->setSelected(false);
    }
    if (onNodeSelected_) {
        onNodeSelected_(InvalidNodeId);
    }
    repaint();
}

void GraphCanvasComponent::addNodeAtPosition(NodeType type, float x, float y) {
    NodeId connectFrom = InvalidNodeId;
    if (selectedNodeId_ != InvalidNodeId) {
        if (auto* sel = findNodeComponent(selectedNodeId_)) {
            connectFrom = selectedNodeId_;
            x = sel->getRight() + 30.0f;
            y = static_cast<float>(sel->getY());
        }
    } else {
        const auto viewCenterWorld = screenToWorld(getLocalBounds().getCentre().toFloat());
        if (std::abs(x - 80.0f) < 250.0f && std::abs(y - 60.0f) < 250.0f) {
            x = viewCenterWorld.x - 90.0f;
            y = viewCenterWorld.y - 40.0f;
        }
    }

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
    const auto& theme = ThemeManager::getInstance().getColors();

    // 1. Fondo según tema activo
    g.setColour(theme.backgroundDark);
    g.fillRect(bounds);

    // 2. Rejilla interactiva de micropuntos animada y sincronizada con el paneo y zoom
    float dotStep = 24.0f * zoomScale_;
    while (dotStep < 16.0f) dotStep *= 2.0f;
    while (dotStep > 48.0f) dotStep *= 0.5f;

    float startX = std::fmod(panOffset_.x, dotStep);
    if (startX < 0.0f) startX += dotStep;
    float startY = std::fmod(panOffset_.y, dotStep);
    if (startY < 0.0f) startY += dotStep;

    const float dotAlpha = std::clamp(0.12f * zoomScale_, 0.07f, 0.22f);
    g.setColour(theme.accentPrimary.withAlpha(dotAlpha));
    for (float x = startX; x < bounds.getWidth(); x += dotStep) {
        for (float y = startY; y < bounds.getHeight(); y += dotStep) {
            g.fillRect(x, y, 1.5f, 1.5f);
        }
    }

    // 3. Contorno del marco del canvas
    g.setColour(theme.borderMuted);
    g.drawRect(bounds, 1.0f);

    // 4. Selección elástica múltiple (Marquee / Rubberband Box, Regla 16)
    if (isMarqueeSelecting_) {
        g.setColour(juce::Colour(0x2200E5FF));
        g.fillRect(marqueeRect_);
        g.setColour(juce::Colour(0xAA00E5FF));
        juce::PathStrokeType stroke(1.2f);
        float dashes[] = { 4.0f, 4.0f };
        juce::Path p;
        p.addRectangle(marqueeRect_);
        stroke.createDashedStroke(p, p, dashes, 2);
        g.strokePath(p, stroke);
    }

    // 5. Renderizar previsualización fantasma al arrastrar módulos desde la paleta
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
    if (worldComp_) {
        worldComp_->setBounds(0, 0, 100000, 100000);
        updateWorldTransform();
    }
    if (zoomHud_) {
        zoomHud_->setBounds(std::max(10, getWidth() - 164), 10, 154, 26);
    }

    // Al abrir por primera vez con dimensiones válidas, ajustar vista automáticamente a todos los nodos
    if (!hasAutoFittedInitial_ && getWidth() > 50 && getHeight() > 50 && !nodeComponents_.empty()) {
        hasAutoFittedInitial_ = true;
        zoomToFit();
    }
}

void GraphCanvasComponent::mouseDown(const juce::MouseEvent& e) {
    grabKeyboardFocus();

    // Si se hace clic sobre un cable existente, seleccionarlo/eliminarlo
    const auto worldPos = screenToWorld(e.position);
    const ConnectionId hitConn = findConnectionNear(worldPos, 10.0f / zoomScale_);
    if (hitConn != 0) {
        deleteConnection(hitConn);
        return;
    }

    // Paneo con botón central, Alt+clic, o Barra espaciadora
    if (e.mods.isMiddleButtonDown() || e.mods.isAltDown() || juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::spaceKey)) {
        isPanning_ = true;
        panStartOffset_ = panOffset_;
        panStartMouse_ = e.position;
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        return;
    }

    // Clic derecho: iniciar seguimiento para distinguir Arrastre vs Clic (Regla 48)
    if (e.mods.isRightButtonDown()) {
        isRightMouseDown_ = true;
        rightMouseDownPos_ = e.position;
        panStartOffset_ = panOffset_;
        panStartMouse_ = e.position;
        isPanning_ = false;
        return;
    }

    // Clic izquierdo en el fondo:
    // Si se mantiene pulsado Shift, Ctrl o Cmd -> selección elástica por rectángulo (Marquee)
    if (e.mods.isShiftDown() || e.mods.isCtrlDown() || e.mods.isCommandDown()) {
        isMarqueeSelecting_ = true;
        marqueeStartPt_ = e.position;
        marqueeRect_ = juce::Rectangle<float>(e.position, e.position);
        repaint();
        return;
    }

    // Clic izquierdo normal en el fondo -> Paneo directo y fluido del lienzo
    isPanning_ = true;
    panStartOffset_ = panOffset_;
    panStartMouse_ = e.position;
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void GraphCanvasComponent::mouseDrag(const juce::MouseEvent& e) {
    if (isPanning_) {
        panOffset_ = panStartOffset_ + (e.position - panStartMouse_);
        updateWorldTransform();
        repaint();
        return;
    }

    if (isRightMouseDown_) {
        const float dist = static_cast<float>(e.getDistanceFromDragStart());
        if (!isPanning_ && dist > 4.0f) {
            isPanning_ = true; // Regla 48: Superado umbral de arrastre
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }
        if (isPanning_) {
            panOffset_ = panStartOffset_ + (e.position - panStartMouse_);
            updateWorldTransform();
            repaint();
            return;
        }
    }

    if (isMarqueeSelecting_) {
        const float x = std::min(marqueeStartPt_.x, e.position.x);
        const float y = std::min(marqueeStartPt_.y, e.position.y);
        const float w = std::abs(e.position.x - marqueeStartPt_.x);
        const float h = std::abs(e.position.y - marqueeStartPt_.y);
        marqueeRect_ = juce::Rectangle<float>(x, y, w, h);

        const auto worldMarquee = screenToWorld(marqueeRect_);

        for (auto& comp : nodeComponents_) {
            if (comp != nullptr) {
                if (worldMarquee.intersects(comp->getBounds().toFloat())) {
                    selectedNodeIds_.insert(comp->getNodeId());
                    comp->setSelected(true);
                    selectedNodeId_ = comp->getNodeId();
                }
            }
        }
        repaint();
    }
}

void GraphCanvasComponent::mouseUp(const juce::MouseEvent& e) {
    if (isPanning_) {
        isPanning_ = false;
        isRightMouseDown_ = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        // Si fue un clic limpio sin arrastre en fondo vacío (distancia <= 4px), limpiar selección
        if (e.getDistanceFromDragStart() <= 4) {
            clearSelection();
        }
        repaint();
        return;
    }

    if (isRightMouseDown_) {
        isRightMouseDown_ = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        // Clic derecho limpio (<= 4px): desplegar menú contextual
        showCanvasContextMenu(e.position);
        return;
    }

    isMarqueeSelecting_ = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

void GraphCanvasComponent::mouseDoubleClick(const juce::MouseEvent& e) {
    if (e.mods.isLeftButtonDown()) {
        if (std::abs(zoomScale_ - 1.0f) > 0.05f) {
            resetZoom();
        } else {
            zoomToFit();
        }
    }
}

void GraphCanvasComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    if (wheel.deltaY == 0.0f) return;

    const auto anchorScreen = e.position;
    const auto anchorWorld = screenToWorld(anchorScreen);

    const float zoomFactor = (wheel.deltaY > 0.0f) ? 1.10f : (1.0f / 1.10f);
    const float targetZoom = std::clamp(zoomScale_ * zoomFactor, 0.20f, 2.5f);

    if (std::abs(targetZoom - zoomScale_) < 1e-4f) return;

    zoomScale_ = targetZoom;
    panOffset_ = anchorScreen - anchorWorld * zoomScale_;

    updateWorldTransform();
    updateZoomHud();
    repaint();
}

void GraphCanvasComponent::mouseMagnify(const juce::MouseEvent& e, float scaleFactor) {
    if (scaleFactor <= 0.0f) return;

    const auto anchorScreen = e.position;
    const auto anchorWorld = screenToWorld(anchorScreen);

    const float targetZoom = std::clamp(zoomScale_ * scaleFactor, 0.20f, 2.5f);
    if (std::abs(targetZoom - zoomScale_) < 1e-4f) return;

    zoomScale_ = targetZoom;
    panOffset_ = anchorScreen - anchorWorld * zoomScale_;

    updateWorldTransform();
    updateZoomHud();
    repaint();
}

bool GraphCanvasComponent::keyPressed(const juce::KeyPress& key) {
    const int kc = key.getKeyCode();
    const bool hasModifiers = key.getModifiers().isShiftDown() || key.getModifiers().isCtrlDown() || key.getModifiers().isCommandDown();

    // 1. Reenvío prioritario de teclas al sintetizador/piano de prueba si no hay modificadores Ctrl/Cmd
    if (!key.getModifiers().isCtrlDown() && !key.getModifiers().isCommandDown()) {
        if (onPianoKeyForward_ && onPianoKeyForward_(key)) {
            return true;
        }
    }

    // 2. Ctrl+G / Cmd+G: Colapso de subgrafo seleccionado en contenedor
    if ((kc == 'G' || kc == 'g') && (key.getModifiers().isCtrlDown() || key.getModifiers().isCommandDown())) {
        collapseSelectedNodesIntoContainer();
        return true;
    }

    // 'F' o 'f': Zoom to Fit (requiere Shift o Ctrl para no interceptar la nota F de piano)
    if (kc == 'F' || kc == 'f') {
        if (hasModifiers) {
            zoomToFit();
            return true;
        }
    }

    if (kc == '0' || kc == '1') {
        resetZoom();
        return true;
    }
    if (kc == '+' || kc == '=') {
        zoomIn();
        return true;
    }
    if (kc == '-' || kc == '_') {
        zoomOut();
        return true;
    }

    return Component::keyPressed(key);
}

bool GraphCanvasComponent::keyStateChanged(bool isKeyDown) {
    if (onPianoKeyStateChanged_ && onPianoKeyStateChanged_(isKeyDown)) {
        return true;
    }
    return Component::keyStateChanged(isKeyDown);
}

void GraphCanvasComponent::setZoom(float newZoom, juce::Point<float> anchorScreen) {
    const float targetZoom = std::clamp(newZoom, 0.20f, 2.5f);
    if (std::abs(targetZoom - zoomScale_) < 1e-4f) return;

    const auto anchorWorld = screenToWorld(anchorScreen);
    zoomScale_ = targetZoom;
    panOffset_ = anchorScreen - anchorWorld * zoomScale_;

    updateWorldTransform();
    updateZoomHud();
    repaint();
}

void GraphCanvasComponent::zoomBy(float factor) {
    const auto centerScreen = getLocalBounds().getCentre().toFloat();
    setZoom(zoomScale_ * factor, centerScreen);
}

void GraphCanvasComponent::zoomIn() {
    zoomBy(1.15f);
}

void GraphCanvasComponent::zoomOut() {
    zoomBy(0.85f);
}

void GraphCanvasComponent::setPanOffset(juce::Point<float> offset) {
    panOffset_ = offset;
    updateWorldTransform();
    repaint();
}

void GraphCanvasComponent::resetZoom() {
    zoomScale_ = 1.0f;
    if (nodeComponents_.empty()) {
        panOffset_ = { 0.0f, 0.0f };
    } else {
        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float maxY = std::numeric_limits<float>::lowest();

        for (const auto& comp : nodeComponents_) {
            if (comp) {
                auto b = comp->getBounds().toFloat();
                minX = std::min(minX, b.getX());
                minY = std::min(minY, b.getY());
                maxX = std::max(maxX, b.getRight());
                maxY = std::max(maxY, b.getBottom());
            }
        }

        const float worldW = maxX - minX;
        const float worldH = maxY - minY;
        const float canvasW = static_cast<float>(getWidth());
        const float canvasH = static_cast<float>(getHeight());

        panOffset_.x = (canvasW - worldW) * 0.5f - minX;
        panOffset_.y = (canvasH - worldH) * 0.5f - minY;
    }

    updateWorldTransform();
    updateZoomHud();
    repaint();
}

void GraphCanvasComponent::zoomToFit() {
    if (nodeComponents_.empty()) {
        resetZoom();
        return;
    }

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    for (const auto& comp : nodeComponents_) {
        if (comp != nullptr) {
            auto b = comp->getBounds().toFloat();
            minX = std::min(minX, b.getX());
            minY = std::min(minY, b.getY());
            maxX = std::max(maxX, b.getRight());
            maxY = std::max(maxY, b.getBottom());
        }
    }

    if (minX >= maxX || minY >= maxY) {
        resetZoom();
        return;
    }

    constexpr float padding = 60.0f;
    minX -= padding;
    minY -= padding;
    maxX += padding;
    maxY += padding;

    const float worldW = maxX - minX;
    const float worldH = maxY - minY;
    const float canvasW = static_cast<float>(getWidth());
    const float canvasH = static_cast<float>(getHeight());

    if (canvasW <= 20.0f || canvasH <= 20.0f) return;

    const float scaleX = canvasW / worldW;
    const float scaleY = canvasH / worldH;
    const float fitScale = std::clamp(std::min(scaleX, scaleY), 0.20f, 1.50f);

    zoomScale_ = fitScale;
    const float scaledW = worldW * zoomScale_;
    const float scaledH = worldH * zoomScale_;
    panOffset_.x = (canvasW - scaledW) * 0.5f - minX * zoomScale_;
    panOffset_.y = (canvasH - scaledH) * 0.5f - minY * zoomScale_;

    updateWorldTransform();
    updateZoomHud();
    repaint();
}

juce::Point<float> GraphCanvasComponent::screenToWorld(juce::Point<float> screenPt) const noexcept {
    return (screenPt - panOffset_) / zoomScale_;
}

juce::Point<float> GraphCanvasComponent::worldToScreen(juce::Point<float> worldPt) const noexcept {
    return worldPt * zoomScale_ + panOffset_;
}

juce::Rectangle<float> GraphCanvasComponent::screenToWorld(juce::Rectangle<float> screenRect) const noexcept {
    const auto tl = screenToWorld(screenRect.getTopLeft());
    const auto br = screenToWorld(screenRect.getBottomRight());
    return juce::Rectangle<float>(tl, br);
}

juce::Rectangle<float> GraphCanvasComponent::worldToScreen(juce::Rectangle<float> worldRect) const noexcept {
    const auto tl = worldToScreen(worldRect.getTopLeft());
    const auto br = worldToScreen(worldRect.getBottomRight());
    return juce::Rectangle<float>(tl, br);
}

void GraphCanvasComponent::updateWorldTransform() {
    if (worldComp_) {
        const auto t = juce::AffineTransform::scale(zoomScale_).translated(panOffset_.x, panOffset_.y);
        worldComp_->setTransform(t);
    }
}

void GraphCanvasComponent::updateZoomHud() {
    if (zoomHud_) {
        zoomHud_->updateZoom(zoomScale_);
    }
}

void GraphCanvasComponent::showCanvasContextMenu(juce::Point<float> /*screenPos*/) {
    juce::PopupMenu canvasMenu;
    if (!selectedNodeIds_.empty() || selectedNodeId_ != InvalidNodeId) {
        canvasMenu.addItem(1, "Collapse into Container (Ctrl+G)", true, false);
        canvasMenu.addItem(2, "Group Selected Nodes (Chassis)", true, false);
        canvasMenu.addSeparator();
        canvasMenu.addItem(3, "Export Selected SubGraph (.n8module)", true, false);
        canvasMenu.addSeparator();
        canvasMenu.addItem(4, "Deselect All", true, false);
    } else {
        canvasMenu.addItem(5, "Import SubGraph Module (.n8module)", true, false);
    }
    canvasMenu.addSeparator();
    canvasMenu.addItem(6, "Zoom to Fit All Nodes (F)", true, false);
    canvasMenu.addItem(7, "Reset Zoom to 100% (0)", true, false);

    canvasMenu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [this](int res) {
            if (res == 1) {
                collapseSelectedNodesIntoContainer();
            } else if (res == 2) {
                std::vector<NodeId> nids(selectedNodeIds_.begin(), selectedNodeIds_.end());
                if (nids.empty() && selectedNodeId_ != InvalidNodeId) nids.push_back(selectedNodeId_);
                createGroup(nids, "Group");
            } else if (res == 3) {
                exportSelectedSubGraphModule();
            } else if (res == 4) {
                clearSelection();
            } else if (res == 5) {
                importSubGraphModule();
            } else if (res == 6) {
                zoomToFit();
            } else if (res == 7) {
                resetZoom();
            }
        });
}

void GraphCanvasComponent::collapseSelectedNodesIntoContainer() {
    std::vector<NodeId> targetIds(selectedNodeIds_.begin(), selectedNodeIds_.end());
    if (targetIds.empty() && selectedNodeId_ != InvalidNodeId) {
        targetIds.push_back(selectedNodeId_);
    }
    if (targetIds.empty()) return;

    auto& graph = processor_.getGraph();

    // 1. Calcular centroide de los nodos seleccionados
    float sumX = 0.0f;
    float sumY = 0.0f;
    int count = 0;
    for (NodeId id : targetIds) {
        if (auto* inst = graph.getNode(id)) {
            sumX += inst->posX;
            sumY += inst->posY;
            count++;
        }
    }
    if (count == 0) return;
    const float centerX = sumX / static_cast<float>(count);
    const float centerY = sumY / static_cast<float>(count);

    // 2. Instanciar nuevo ContainerNode en la posición centroide
    auto containerProc = std::make_unique<ContainerNode>();
    ContainerNode* containerPtr = containerProc.get();
    NodeId containerId = graph.addNode(std::move(containerProc), "Macro Container", centerX, centerY);
    if (containerId == InvalidNodeId) return;

    std::unordered_set<NodeId> targetSet(targetIds.begin(), targetIds.end());
    std::unordered_map<NodeId, NodeId> oldToInnerMap;

    // 3. Mover procesadores y configuración al subgrafo interno
    for (NodeId oldId : targetIds) {
        auto* oldInst = graph.getNode(oldId);
        if (oldInst && oldInst->processor) {
            float relX = oldInst->posX - centerX + 120.0f;
            float relY = oldInst->posY - centerY + 80.0f;
            NodeId innerId = containerPtr->getInnerGraph().addNode(std::move(oldInst->processor), oldInst->name, relX, relY);
            oldToInnerMap[oldId] = innerId;
        }
    }

    // 4. Mapear conexiones internas y externas
    const auto oldConns = graph.getConnections(); // Copia inmutable de conexiones
    for (const auto& c : oldConns) {
        const bool srcIn = targetSet.contains(c.sourceNodeId);
        const bool destIn = targetSet.contains(c.destNodeId);

        if (srcIn && destIn) {
            // Cable interno: mover al subgrafo
            containerPtr->getInnerGraph().connect(oldToInnerMap[c.sourceNodeId], c.sourcePinId,
                                                  oldToInnerMap[c.destNodeId], c.destPinId);
        } else if (!srcIn && destIn) {
            // Cable entrante desde el exterior: enrutar al pin de entrada del contenedor
            graph.connect(c.sourceNodeId, c.sourcePinId, containerId, 1);
        } else if (srcIn && !destIn) {
            // Cable saliente hacia el exterior: enrutar desde el pin de salida del contenedor
            graph.connect(containerId, 2, c.destNodeId, c.destPinId);
        }
    }

    // 5. Eliminar nodos originales del grafo principal
    for (NodeId oldId : targetIds) {
        graph.removeNode(oldId);
    }

    // 6. Compilar el subgrafo interno
    std::string err;
    containerPtr->compileInnerGraph(err);

    // 7. Actualizar selección y refrescar interfaz
    clearSelection();
    selectedNodeId_ = containerId;
    selectedNodeIds_.insert(containerId);
    processor_.recompilePlan();
    rebuildFromGraph();
}

void GraphCanvasComponent::exportSelectedSubGraphModule() {
    NodeId targetId = selectedNodeId_;
    if (targetId == InvalidNodeId && !selectedNodeIds_.empty()) {
        targetId = *selectedNodeIds_.begin();
    }
    if (targetId == InvalidNodeId) return;

    auto* node = processor_.getGraph().getNode(targetId);
    if (!node || node->type != NodeType::Container) return;

    auto* container = dynamic_cast<ContainerNode*>(node->processor.get());
    if (!container) return;

    const std::string json = container->exportSubGraphJson(node->name);
    auto moduleDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                         .getChildFile("N8Audio")
                         .getChildFile("Modules");
    moduleDir.createDirectory();
    auto file = moduleDir.getChildFile(node->name + ".n8module");
    file.replaceWithText(juce::String(json));
}

void GraphCanvasComponent::importSubGraphModule() {
    auto moduleDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                         .getChildFile("N8Audio")
                         .getChildFile("Modules");
    if (!moduleDir.isDirectory()) return;

    auto files = moduleDir.findChildFiles(juce::File::findFiles, false, "*.n8module");
    if (files.isEmpty()) return;

    const juce::String json = files[0].loadFileAsString();
    if (json.isEmpty()) return;

    auto containerProc = std::make_unique<ContainerNode>();
    ContainerNode* containerPtr = containerProc.get();
    std::string err;
    if (containerPtr->importSubGraphJson(json.toStdString(), err)) {
        const float x = static_cast<float>(getWidth() / 2 - 90);
        const float y = static_cast<float>(getHeight() / 2 - 40);
        NodeId newId = processor_.getGraph().addNode(std::move(containerProc), files[0].getFileNameWithoutExtension().toStdString(), x, y);
        selectedNodeId_ = newId;
        processor_.recompilePlan();
        rebuildFromGraph();
    }
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

    const auto worldGhostPos = screenToWorld(ghostPos_);
    NodeComponent* targetComp = nullptr;
    for (const auto& comp : nodeComponents_) {
        if (comp && comp->getBounds().toFloat().expanded(16.0f).contains(worldGhostPos)) {
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
    const auto worldDropPos = screenToWorld(dropPos);

    // 1. Verificar si se soltó directamente sobre un nodo existente para auto-encadenamiento
    NodeComponent* targetComp = nullptr;
    for (const auto& comp : nodeComponents_) {
        if (comp && comp->getBounds().toFloat().contains(worldDropPos)) {
            targetComp = comp.get();
            break;
        }
    }

    float spawnX = 0.0f;
    float spawnY = 0.0f;

    if (targetComp != nullptr) {
        // Auto-encadenar: ubicar a la derecha
        spawnX = targetComp->getRight() + 30.0f;
        spawnY = static_cast<float>(targetComp->getY());

        const NodeId newId = processor_.addNodeToGraph(type, spawnX, spawnY);
        if (newId != InvalidNodeId) {
            processor_.connectNodes(targetComp->getNodeId(), 2, newId, 1);
        }
        selectedNodeId_ = newId;
    } else {
        // Posición libre: ubicar exactamente centrado bajo el cursor donde el usuario soltó
        spawnX = worldDropPos.x - 90.0f;
        spawnY = worldDropPos.y - 30.0f;

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

// ==============================================================================
// Implementación de juce::FileDragAndDropTarget (Respuestas al Impulso IR)
// ==============================================================================
bool GraphCanvasComponent::isInterestedInFileDrag(const juce::StringArray& files) {
    for (const auto& f : files) {
        if (f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".wave") ||
            f.endsWithIgnoreCase(".aif") || f.endsWithIgnoreCase(".aiff")) {
            return true;
        }
    }
    return false;
}

void GraphCanvasComponent::fileDragEnter(const juce::StringArray& files, int x, int y) {
    fileDragMove(files, x, y);
}

void GraphCanvasComponent::fileDragMove(const juce::StringArray& /*files*/, int x, int y) {
    NodeComponent* targetComp = nullptr;
    for (const auto& comp : nodeComponents_) {
        if (comp && comp->getBounds().contains(x, y) && comp->getNodeType() == NodeType::Convolution) {
            targetComp = comp.get();
            break;
        }
    }

    for (auto& comp : nodeComponents_) {
        comp->setDropCandidate(comp.get() == targetComp);
    }
    repaint();
}

void GraphCanvasComponent::fileDragExit(const juce::StringArray& /*files*/) {
    for (auto& comp : nodeComponents_) {
        comp->setDropCandidate(false);
    }
    repaint();
}

void GraphCanvasComponent::filesDropped(const juce::StringArray& files, int x, int y) {
    for (auto& comp : nodeComponents_) {
        comp->setDropCandidate(false);
    }

    if (files.isEmpty()) return;
    juce::File audioFile(files[0]);
    if (!audioFile.existsAsFile()) return;

    const auto worldPt = screenToWorld(juce::Point<float>(static_cast<float>(x), static_cast<float>(y)));

    // 1. Comprobar si se soltó sobre un nodo existente (Convolution o SamplePlayer)
    for (const auto& comp : nodeComponents_) {
        if (comp && comp->getBounds().contains(static_cast<int>(worldPt.x), static_cast<int>(worldPt.y))) {
            if (comp->getNodeType() == NodeType::Convolution) {
                loadIRFileIntoNode(comp->getNodeId(), audioFile);
                repaint();
                return;
            } else if (comp->getNodeType() == NodeType::SamplePlayer) {
                loadSampleFileIntoNode(comp->getNodeId(), audioFile);
                repaint();
                return;
            }
        }
    }

    // 2. Si se soltó en lienzo vacío, desplegar menú contextual para elegir modo de carga
    juce::PopupMenu menu;
    menu.addSectionHeader("AUDIO: " + audioFile.getFileName());
    menu.addItem(1, "🎹 Cargar como Sample Player (Tocar con teclado / MIDI)");
    menu.addItem(2, "🌌 Cargar como Convolución (Reverb / Respuesta de Impulso)");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [this, audioFile, worldPt](int result) {
            if (result == 0) return;

            const float spawnX = worldPt.x - 90.0f;
            const float spawnY = worldPt.y - 30.0f;

            const NodeId prevSelected = selectedNodeId_;

            if (result == 1) { // 🎹 Cargar como Sample Player
                NodeId newId = processor_.addNodeToGraph(NodeType::SamplePlayer, spawnX, spawnY);
                if (newId != InvalidNodeId) {
                    loadSampleFileIntoNode(newId, audioFile);
                    // Auto-conectar con el nodo seleccionado previo si existe para que suene de inmediato
                    if (prevSelected != InvalidNodeId && prevSelected != newId) {
                        processor_.connectNodes(prevSelected, 2, newId, 1);
                    }
                    selectedNodeId_ = newId;
                    rebuildFromGraph();
                }
            } else if (result == 2) { // 🌌 Cargar como Convolución
                NodeId newId = processor_.addNodeToGraph(NodeType::Convolution, spawnX, spawnY);
                if (newId != InvalidNodeId) {
                    loadIRFileIntoNode(newId, audioFile);
                    if (prevSelected != InvalidNodeId && prevSelected != newId) {
                        processor_.connectNodes(prevSelected, 2, newId, 1);
                    }
                    selectedNodeId_ = newId;
                    rebuildFromGraph();
                }
            }

            repaint();
            juce::MessageManager::callAsync([this]() {
                rebuildFromGraph();
                repaint();
                if (auto* p = getParentComponent()) {
                    p->repaint();
                }
            });
        });
}

void GraphCanvasComponent::loadIRFileIntoNode(NodeId id, const juce::File& file) {
    juce::AudioFormatManager formatMgr;
    formatMgr.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatMgr.createReaderFor(file));
    if (reader == nullptr) return;

    const double hostSampleRate = processor_.getCurrentSpec().sampleRate > 0.0 ? processor_.getCurrentSpec().sampleRate : 48000.0;
    const int numChannels = static_cast<int>(reader->numChannels);
    if (numChannels <= 0) return;

    // Limitar a un máximo razonable (10 segundos a la tasa de muestreo original) para evitar desbordamientos
    const int maxSamplesToRead = static_cast<int>(std::min<int64_t>(reader->lengthInSamples, static_cast<int64_t>(reader->sampleRate * 10.0)));
    if (maxSamplesToRead <= 0) return;

    juce::AudioBuffer<float> tempBuffer(numChannels, maxSamplesToRead);
    reader->read(&tempBuffer, 0, maxSamplesToRead, 0, true, true);

    juce::AudioBuffer<float> resampledBuffer;
    if (std::abs(reader->sampleRate - hostSampleRate) > 1.0) {
        const double ratio = reader->sampleRate / hostSampleRate;
        const int newLength = static_cast<int>(std::round(static_cast<double>(maxSamplesToRead) / ratio));
        resampledBuffer.setSize(numChannels, newLength);

        juce::LagrangeInterpolator interpolator;
        for (int ch = 0; ch < numChannels; ++ch) {
            interpolator.reset();
            interpolator.process(ratio, tempBuffer.getReadPointer(ch),
                                 resampledBuffer.getWritePointer(ch), newLength);
        }
    } else {
        resampledBuffer.makeCopyOf(tempBuffer);
    }

    auto* nodeInst = processor_.getGraph().getNode(id);
    if (nodeInst && nodeInst->processor) {
        if (auto* conv = dynamic_cast<ConvolutionNode*>(nodeInst->processor.get())) {
            const float* lData = resampledBuffer.getReadPointer(0);
            const float* rData = (resampledBuffer.getNumChannels() > 1) ? resampledBuffer.getReadPointer(1) : lData;
            conv->setCustomImpulseResponse(lData, rData, static_cast<size_t>(resampledBuffer.getNumSamples()));
            repaint();
        }
    }
}

void GraphCanvasComponent::loadSampleFileIntoNode(NodeId id, const juce::File& file) {
    juce::AudioFormatManager formatMgr;
    formatMgr.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatMgr.createReaderFor(file));
    if (reader == nullptr) return;

    const double hostSampleRate = processor_.getCurrentSpec().sampleRate > 0.0 ? processor_.getCurrentSpec().sampleRate : 48000.0;
    const int numChannels = static_cast<int>(reader->numChannels);
    if (numChannels <= 0) return;

    // Limitar a 30 segundos de muestra para control estricto de memoria (Regla 47)
    const int maxSamplesToRead = static_cast<int>(std::min<int64_t>(reader->lengthInSamples, static_cast<int64_t>(reader->sampleRate * 30.0)));
    if (maxSamplesToRead <= 0) return;

    juce::AudioBuffer<float> tempBuffer(numChannels, maxSamplesToRead);
    reader->read(&tempBuffer, 0, maxSamplesToRead, 0, true, true);

    juce::AudioBuffer<float> resampledBuffer;
    if (std::abs(reader->sampleRate - hostSampleRate) > 1.0) {
        const double ratio = reader->sampleRate / hostSampleRate;
        const int newLength = static_cast<int>(std::round(static_cast<double>(maxSamplesToRead) / ratio));
        resampledBuffer.setSize(numChannels, newLength);

        juce::LagrangeInterpolator interpolator;
        for (int ch = 0; ch < numChannels; ++ch) {
            interpolator.reset();
            interpolator.process(ratio, tempBuffer.getReadPointer(ch),
                                 resampledBuffer.getWritePointer(ch), newLength);
        }
    } else {
        resampledBuffer.makeCopyOf(tempBuffer);
    }

    const float* lData = resampledBuffer.getReadPointer(0);
    const float* rData = (resampledBuffer.getNumChannels() > 1) ? resampledBuffer.getReadPointer(1) : lData;
    const size_t totalSamples = static_cast<size_t>(resampledBuffer.getNumSamples());

    auto* nodeInst = processor_.getGraph().getNode(id);
    if (nodeInst && nodeInst->processor) {
        if (auto* sp = dynamic_cast<SamplePlayerNode*>(nodeInst->processor.get())) {
            sp->loadAudioSample(lData, rData, totalSamples, hostSampleRate);
            repaint();
        }
    }

    // Cargar también en el sintetizador polifónico de prueba para disparar con el teclado visual o MIDI
    processor_.getTestSynthesizer().setLoadedSample(lData, totalSamples, hostSampleRate);

    if (onSampleLoaded_) {
        onSampleLoaded_();
    }

    repaint();
}

// ==============================================================================
// Implementación de GroupMacroKnob (Reglas 48 y R2)
// ==============================================================================
void GroupMacroKnob::refreshFromEngine() {
    auto* grp = processor_.getGraph().getGroup(groupId_);
    if (grp && macroIndex_ < grp->macros.size()) {
        value_ = grp->macros[macroIndex_].value;
        repaint();
    }
}

void GroupMacroKnob::applyValueToEngine() {
    processor_.getGraph().applyGroupMacroValue(groupId_, macroIndex_, value_);
    canvas_.repaint();
}

void GroupMacroKnob::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();

    constexpr float dialSize = 22.0f;
    const float dialX = (w - dialSize) * 0.5f;
    const float dialY = 2.0f;
    const juce::Rectangle<float> dialBounds(dialX, dialY, dialSize, dialSize);

    juce::Colour groupCol = juce::Colour(0xff00d4ff);
    if (const auto* grp = processor_.getGraph().getGroup(groupId_)) {
        groupCol = rgbaToJuceColour(grp->colorRgba);
    }

    // Fondo del dial
    g.setColour(juce::Colour(0xff080b10));
    g.fillEllipse(dialBounds);
    g.setColour(juce::Colour(0xff1e2634));
    g.drawEllipse(dialBounds, 1.2f);

    // Arco bipolar [0.0 .. 1.0], centro en 0.5f (12 o'clock = -pi/2)
    constexpr float angleCenter = -juce::MathConstants<float>::halfPi;
    constexpr float angleRange = juce::MathConstants<float>::pi * 1.4f;
    const float angleCurrent = angleCenter + (value_ - 0.5f) * angleRange;

    juce::Path arcPath;
    const float arcRadius = dialSize * 0.5f - 2.5f;
    const auto centre = dialBounds.getCentre();

    if (std::abs(value_ - 0.5f) > 0.01f) {
        arcPath.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                              std::min(angleCenter, angleCurrent),
                              std::max(angleCenter, angleCurrent), true);
        g.setColour(groupCol);
        g.strokePath(arcPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    } else {
        g.setColour(groupCol.withAlpha(0.7f));
        g.fillEllipse(centre.x - 1.5f, dialBounds.getY() + 1.0f, 3.0f, 3.0f);
    }

    // Aguja indicadora
    juce::Line<float> needle(centre, centre.getPointOnCircumference(arcRadius - 1.0f, angleCurrent));
    g.setColour(juce::Colours::white);
    g.drawLine(needle, 1.5f);

    // Centro del dial
    g.setColour(juce::Colour(0xff121822));
    g.fillEllipse(centre.x - 2.5f, centre.y - 2.5f, 5.0f, 5.0f);

    // Etiqueta de texto debajo del dial
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    juce::String labelText = "CTRL " + juce::String(macroIndex_ + 1);
    if (const auto* grp = processor_.getGraph().getGroup(groupId_)) {
        if (macroIndex_ < grp->macros.size() && !grp->macros[macroIndex_].name.empty()) {
            labelText = grp->macros[macroIndex_].name;
        }
    }
    g.drawText(labelText, 0, static_cast<int>(dialBounds.getBottom() + 1), static_cast<int>(w), 12,
               juce::Justification::centred, false);
}

void GroupMacroKnob::showMappingMenu() {
    auto* grp = processor_.getGraph().getGroup(groupId_);
    if (!grp || macroIndex_ >= grp->macros.size()) return;

    auto& macro = grp->macros[macroIndex_];
    juce::PopupMenu menu;

    menu.addSectionHeader(juce::String(macro.name) + " (" + juce::String(std::round(value_ * 100.0f)) + "%)");
    menu.addItem(1, "Reset to Center (50%)", true, false);
    menu.addSeparator();

    if (!macro.mappings.empty()) {
        juce::PopupMenu currentMappingsMenu;
        int mapId = 100;
        for (size_t i = 0; i < macro.mappings.size(); ++i) {
            const auto& m = macro.mappings[i];
            juce::String nodeName = "Node " + juce::String(m.targetNodeId);
            juce::String paramName = "Param " + juce::String(m.targetParamId);

            if (auto* node = processor_.getGraph().getNode(m.targetNodeId)) {
                nodeName = node->name;
                if (node->processor) {
                    for (const auto& p : node->processor->getParameters()) {
                        if (p.id == m.targetParamId) {
                            paramName = p.name;
                            break;
                        }
                    }
                }
            }

            juce::String depthStr = (m.depth >= 0 ? "+" : "") + juce::String(std::round(m.depth * 100.0f)) + "%";
            juce::PopupMenu depthMenu;
            depthMenu.addItem(mapId + 1, "Depth: +100%", true, std::abs(m.depth - 1.0f) < 0.05f);
            depthMenu.addItem(mapId + 2, "Depth: +75%", true, std::abs(m.depth - 0.75f) < 0.05f);
            depthMenu.addItem(mapId + 3, "Depth: +50%", true, std::abs(m.depth - 0.50f) < 0.05f);
            depthMenu.addItem(mapId + 4, "Depth: +25%", true, std::abs(m.depth - 0.25f) < 0.05f);
            depthMenu.addItem(mapId + 5, "Depth: -25%", true, std::abs(m.depth - -0.25f) < 0.05f);
            depthMenu.addItem(mapId + 6, "Depth: -50%", true, std::abs(m.depth - -0.50f) < 0.05f);
            depthMenu.addItem(mapId + 7, "Depth: -75%", true, std::abs(m.depth - -0.75f) < 0.05f);
            depthMenu.addItem(mapId + 8, "Depth: -100%", true, std::abs(m.depth - -1.0f) < 0.05f);
            depthMenu.addSeparator();
            depthMenu.addItem(mapId + 9, "Remove Mapping", true, false);

            currentMappingsMenu.addSubMenu(nodeName + " -> " + paramName + " [" + depthStr + "]", depthMenu);
            mapId += 10;
        }
        menu.addSubMenu("Active Mappings (" + juce::String(macro.mappings.size()) + ")", currentMappingsMenu);
        menu.addItem(2, "Clear All Mappings", true, false);
        menu.addSeparator();
    }

    juce::PopupMenu addMappingMenu;
    int addId = 1000;
    std::vector<std::pair<NodeId, ParameterId>> addChoices;

    for (NodeId nid : grp->memberNodeIds) {
        auto* node = processor_.getGraph().getNode(nid);
        if (!node || !node->processor) continue;

        juce::PopupMenu nodeParamsMenu;
        for (const auto& p : node->processor->getParameters()) {
            nodeParamsMenu.addItem(addId, p.name, true, false);
            addChoices.push_back({ nid, p.id });
            addId++;
        }
        addMappingMenu.addSubMenu(node->name, nodeParamsMenu);
    }

    if (!grp->memberNodeIds.empty()) {
        menu.addSubMenu("Add Mapping...", addMappingMenu);
    } else {
        menu.addItem(9999, "No member nodes in group", false, false);
    }

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [this, addChoices](int result) {
            if (result == 0) return;
            auto* g = processor_.getGraph().getGroup(groupId_);
            if (!g || macroIndex_ >= g->macros.size()) return;

            auto& m = g->macros[macroIndex_];

            if (result == 1) {
                setValue(0.5f);
            } else if (result == 2) {
                m.mappings.clear();
                applyValueToEngine();
                repaint();
            } else if (result >= 100 && result < 1000) {
                int mapIdx = (result - 100) / 10;
                int action = (result - 100) % 10;
                if (static_cast<size_t>(mapIdx) < m.mappings.size()) {
                    if (action == 1) m.mappings[mapIdx].depth = 1.0f;
                    else if (action == 2) m.mappings[mapIdx].depth = 0.75f;
                    else if (action == 3) m.mappings[mapIdx].depth = 0.50f;
                    else if (action == 4) m.mappings[mapIdx].depth = 0.25f;
                    else if (action == 5) m.mappings[mapIdx].depth = -0.25f;
                    else if (action == 6) m.mappings[mapIdx].depth = -0.50f;
                    else if (action == 7) m.mappings[mapIdx].depth = -0.75f;
                    else if (action == 8) m.mappings[mapIdx].depth = -1.0f;
                    else if (action == 9) m.mappings.erase(m.mappings.begin() + mapIdx);

                    applyValueToEngine();
                    repaint();
                }
            } else if (result >= 1000) {
                size_t choiceIdx = static_cast<size_t>(result - 1000);
                if (choiceIdx < addChoices.size()) {
                    NodeId targetNid = addChoices[choiceIdx].first;
                    ParameterId targetPid = addChoices[choiceIdx].second;

                    float baseVal = 0.0f;
                    if (auto* node = processor_.getGraph().getNode(targetNid)) {
                        if (node->processor) {
                            baseVal = node->processor->getParameter(targetPid);
                        }
                    }

                    GroupMacroMapping mapping;
                    mapping.targetNodeId = targetNid;
                    mapping.targetParamId = targetPid;
                    mapping.depth = 1.0f;
                    mapping.baseValue = baseVal;

                    m.mappings.push_back(mapping);
                    applyValueToEngine();
                    repaint();
                }
            }
        });
}

// ==============================================================================
// Implementación de NodeGroupComponent (Regla R2)
// ==============================================================================
NodeGroupComponent::NodeGroupComponent(GroupId groupId, GraphCanvasComponent& canvas, N8AudioProcessor& processor)
    : groupId_(groupId), canvas_(canvas), processor_(processor)
{
    setInterceptsMouseClicks(true, true);
    setRepaintsOnMouseActivity(true);

    addAndMakeVisible(titleLabel_);
    titleLabel_.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel_.setEditable(false, true, false);
    titleLabel_.onTextChange = [this]() {
        if (auto* grp = getGroup()) {
            grp->name = titleLabel_.getText().toStdString();
        }
    };

    bypassButton_ = std::make_unique<Rule48Button>("PWR");
    bypassButton_->setTooltip("Collective Group Bypass");
    bypassButton_->setOnCleanClick([this]() { toggleBypass(); });
    addAndMakeVisible(*bypassButton_);

    collapseButton_ = std::make_unique<Rule48Button>("-");
    collapseButton_->setTooltip("Collapse / Expand Group");
    collapseButton_->setOnCleanClick([this]() { toggleCollapse(); });
    addAndMakeVisible(*collapseButton_);

    colorButton_ = std::make_unique<Rule48Button>("");
    colorButton_->setTooltip("Cycle Group Accent Color");
    colorButton_->setOnCleanClick([this]() { cycleColor(); });
    addAndMakeVisible(*colorButton_);

    for (size_t i = 0; i < 3; ++i) {
        macroKnobs_[i] = std::make_unique<GroupMacroKnob>(groupId_, i, canvas_, processor_);
        addAndMakeVisible(*macroKnobs_[i]);
    }

    syncWithGraph();
}

NodeGroup* NodeGroupComponent::getGroup() {
    return processor_.getGraph().getGroup(groupId_);
}

const NodeGroup* NodeGroupComponent::getGroup() const {
    return processor_.getGraph().getGroup(groupId_);
}

juce::Colour NodeGroupComponent::getGroupColour() const {
    if (const auto* grp = getGroup()) {
        return rgbaToJuceColour(grp->colorRgba);
    }
    return juce::Colour(0xff00d4ff);
}

void NodeGroupComponent::syncWithGraph() {
    const auto* grp = getGroup();
    if (!grp) return;

    isCollapsed_ = grp->isCollapsed;
    isBypassed_ = grp->isBypassed;
    colorRgba_ = grp->colorRgba;
    titleLabel_.setText(grp->name, juce::dontSendNotification);
    collapseButton_->setButtonText(isCollapsed_ ? "+" : "-");

    for (size_t i = 0; i < 3; ++i) {
        if (macroKnobs_[i]) {
            macroKnobs_[i]->refreshFromEngine();
        }
    }

    updateBoundsFromMembers();
    repaint();
}

void NodeGroupComponent::toggleCollapse() {
    isCollapsed_ = !isCollapsed_;
    processor_.getGraph().setGroupCollapsed(groupId_, isCollapsed_);
    collapseButton_->setButtonText(isCollapsed_ ? "+" : "-");

    if (auto* grp = getGroup()) {
        for (NodeId nid : grp->memberNodeIds) {
            if (auto* comp = canvas_.findNodeComponent(nid)) {
                comp->setVisible(!isCollapsed_);
            }
        }
    }

    updateBoundsFromMembers();
    canvas_.repaint();
}

void NodeGroupComponent::toggleBypass() {
    isBypassed_ = !isBypassed_;
    processor_.getGraph().setGroupBypassed(groupId_, isBypassed_);

    if (auto* grp = getGroup()) {
        for (NodeId nid : grp->memberNodeIds) {
            if (auto* comp = canvas_.findNodeComponent(nid)) {
                comp->setBypassed(isBypassed_);
            }
        }
    }

    repaint();
    canvas_.repaint();
}

void NodeGroupComponent::cycleColor() {
    paletteIndex_ = (paletteIndex_ + 1) % kGroupColorPalette.size();
    colorRgba_ = kGroupColorPalette[paletteIndex_];
    if (auto* grp = getGroup()) {
        grp->colorRgba = colorRgba_;
    }
    repaint();
}

void NodeGroupComponent::updateBoundsFromMembers() {
    auto* grp = getGroup();
    if (!grp) return;

    int minX = std::numeric_limits<int>::max();
    int minY = std::numeric_limits<int>::max();
    int maxX = std::numeric_limits<int>::min();
    int maxY = std::numeric_limits<int>::min();
    bool anyFound = false;

    for (NodeId nid : grp->memberNodeIds) {
        if (auto* comp = canvas_.findNodeComponent(nid)) {
            anyFound = true;
            minX = std::min(minX, comp->getX());
            minY = std::min(minY, comp->getY());
            maxX = std::max(maxX, comp->getRight());
            maxY = std::max(maxY, comp->getBottom());
            comp->setVisible(!isCollapsed_);
        }
    }

    if (anyFound) {
        constexpr int padding = 16;
        int gx = minX - padding;
        int gy = minY - kHeaderHeight - padding;
        int gw = std::max(370, (maxX - minX) + 2 * padding);
        int gh = isCollapsed_ ? kHeaderHeight : ((maxY - minY) + kHeaderHeight + 2 * padding);
        setBounds(gx, gy, gw, gh);
    } else {
        if (getWidth() <= 0 || getHeight() <= 0) {
            setBounds(100, 100, 370, isCollapsed_ ? kHeaderHeight : 160);
        } else {
            setSize(std::max(370, getWidth()), isCollapsed_ ? kHeaderHeight : 160);
        }
    }
}

void NodeGroupComponent::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();
    const auto groupCol = getGroupColour();

    // 1. Chasis principal translúcido
    if (!isCollapsed_) {
        g.setColour(groupCol.withAlpha(isBypassed_ ? 0.03f : 0.08f));
        g.fillRoundedRectangle(bounds, 8.0f);

        g.setColour(groupCol.withAlpha(isBypassed_ ? 0.25f : 0.65f));
        g.drawRoundedRectangle(bounds, 8.0f, 1.5f);

        if (!isBypassed_) {
            g.setColour(groupCol.withAlpha(0.12f));
            g.drawRoundedRectangle(bounds.expanded(1.5f), 9.0f, 1.0f);
        }
    }

    // 2. Barra de encabezado
    const juce::Rectangle<float> headerBounds(0.0f, 0.0f, bounds.getWidth(), static_cast<float>(kHeaderHeight));
    juce::Path headerPath;
    if (isCollapsed_) {
        headerPath.addRoundedRectangle(headerBounds, 8.0f);
    } else {
        headerPath.addRoundedRectangle(headerBounds.getX(), headerBounds.getY(),
                                       headerBounds.getWidth(), headerBounds.getHeight(),
                                       8.0f, 8.0f, true, true, false, false);
    }

    g.setColour(juce::Colour(0xff0e121a).interpolatedWith(groupCol, 0.15f));
    g.fillPath(headerPath);

    if (!isCollapsed_) {
        g.setColour(groupCol.withAlpha(0.35f));
        g.drawHorizontalLine(kHeaderHeight, 0.0f, bounds.getWidth());
    } else {
        g.setColour(groupCol.withAlpha(isBypassed_ ? 0.25f : 0.65f));
        g.drawRoundedRectangle(headerBounds, 8.0f, 1.5f);
    }

    // 3. LED indicador de Bypass
    const float ledX = 18.0f;
    const float ledY = 22.0f;
    constexpr float ledR = 4.0f;

    if (!isBypassed_) {
        g.setColour(juce::Colour(0xff00ff88)); // LED verde activo
        g.fillEllipse(ledX - ledR, ledY - ledR, ledR * 2.0f, ledR * 2.0f);
        g.setColour(juce::Colour(0xff00ff88).withAlpha(0.4f));
        g.drawEllipse(ledX - ledR - 1.5f, ledY - ledR - 1.5f, (ledR + 1.5f) * 2.0f, (ledR + 1.5f) * 2.0f, 1.0f);
    } else {
        g.setColour(juce::Colour(0xffcc2233)); // LED rojo bypassed
        g.fillEllipse(ledX - ledR, ledY - ledR, ledR * 2.0f, ledR * 2.0f);
    }

    // 4. Muestra de color (swatch circular)
    if (colorButton_) {
        const auto cb = colorButton_->getBounds().toFloat();
        g.setColour(groupCol);
        g.fillEllipse(cb.reduced(2.0f));
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawEllipse(cb.reduced(2.0f), 1.0f);
    }

    // 5. Botón colapso icono
    if (collapseButton_) {
        const auto clb = collapseButton_->getBounds().toFloat();
        g.setColour(juce::Colour(0xff18202c));
        g.fillRoundedRectangle(clb, 3.0f);
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(isCollapsed_ ? "+" : "-", clb, juce::Justification::centred, false);
    }
}

void NodeGroupComponent::resized() {
    if (bypassButton_) {
        bypassButton_->setBounds(6, 10, 24, 24);
        bypassButton_->setAlpha(0.01f);
    }

    if (colorButton_) {
        colorButton_->setBounds(34, 13, 18, 18);
        colorButton_->setAlpha(0.01f);
    }

    if (collapseButton_) {
        collapseButton_->setBounds(58, 12, 22, 20);
        collapseButton_->setAlpha(0.01f);
    }

    constexpr int knobW = 42;
    constexpr int knobH = 40;
    int rightX = getWidth() - 6;

    for (int i = 2; i >= 0; --i) {
        rightX -= knobW;
        if (macroKnobs_[static_cast<size_t>(i)]) {
            macroKnobs_[static_cast<size_t>(i)]->setBounds(rightX, 2, knobW, knobH);
        }
        rightX -= 4;
    }

    int titleLeft = 86;
    int titleRight = rightX - 6;
    titleLabel_.setBounds(titleLeft, 8, std::max(60, titleRight - titleLeft), 28);
}

void NodeGroupComponent::mouseDown(const juce::MouseEvent& e) {
    hasDragged_ = false;
    dragStartWorldPos_ = (getParentComponent() != nullptr)
        ? e.getEventRelativeTo(getParentComponent()).position
        : canvas_.screenToWorld(e.getEventRelativeTo(&canvas_).position);
    dragStartGroupPos_ = getPosition();
    initialNodePositions_.clear();

    if (auto* grp = getGroup()) {
        for (NodeId nid : grp->memberNodeIds) {
            if (auto* comp = canvas_.findNodeComponent(nid)) {
                initialNodePositions_[nid] = comp->getPosition();
            }
        }
    }
}

void NodeGroupComponent::mouseDrag(const juce::MouseEvent& e) {
    const auto curWorldPos = (getParentComponent() != nullptr)
        ? e.getEventRelativeTo(getParentComponent()).position
        : canvas_.screenToWorld(e.getEventRelativeTo(&canvas_).position);
    const float dx = curWorldPos.x - dragStartWorldPos_.x;
    const float dy = curWorldPos.y - dragStartWorldPos_.y;

    if (!hasDragged_ && (std::hypot(dx, dy) * canvas_.getZoomScale() > 4.0f)) {
        hasDragged_ = true; // Regla 48: Superado umbral de 4px en pantalla
    }

    if (hasDragged_) {
        setTopLeftPosition(dragStartGroupPos_.x + static_cast<int>(dx),
                           dragStartGroupPos_.y + static_cast<int>(dy));

        for (const auto& [nid, pos] : initialNodePositions_) {
            if (auto* comp = canvas_.findNodeComponent(nid)) {
                const int nx = pos.x + static_cast<int>(dx);
                const int ny = pos.y + static_cast<int>(dy);
                comp->setTopLeftPosition(nx, ny);

                if (auto* nodeInst = processor_.getGraph().getNode(nid)) {
                    nodeInst->posX = static_cast<float>(nx);
                    nodeInst->posY = static_cast<float>(ny);
                }
            }
        }

        canvas_.repaint();
    }
}

void NodeGroupComponent::mouseUp(const juce::MouseEvent& e) {
    if (hasDragged_) {
        hasDragged_ = false;
        canvas_.repaint();
        return; // Regla 48: Supresión absoluta de clic tras arrastre
    }

    // Clic limpio <= 4px: seleccionar grupo o abrir menú contextual
    if (e.mods.isPopupMenu() || e.mods.isRightButtonDown()) {
        juce::PopupMenu groupMenu;
        groupMenu.addSectionHeader(titleLabel_.getText());
        groupMenu.addItem(1, "Toggle Bypass", true, isBypassed_);
        groupMenu.addItem(2, isCollapsed_ ? "Expand Group" : "Collapse Group", true, false);
        groupMenu.addItem(3, "Cycle Accent Color", true, false);
        groupMenu.addSeparator();
        groupMenu.addItem(4, "Ungroup Nodes", true, false);

        groupMenu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [this](int res) {
                if (res == 1) toggleBypass();
                else if (res == 2) toggleCollapse();
                else if (res == 3) cycleColor();
                else if (res == 4) {
                    processor_.getGraph().removeGroup(groupId_);
                    canvas_.rebuildFromGraph();
                }
            });
    }
}

} // namespace audio_graph

