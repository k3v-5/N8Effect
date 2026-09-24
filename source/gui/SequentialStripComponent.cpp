#include "SequentialStripComponent.h"
#include "../plugin/PluginProcessor.h"

namespace audio_graph {

SequentialStripComponent::SequentialStripComponent(N8AudioProcessor& processor)
    : processor_(processor)
{
    setRepaintsOnMouseActivity(true);

    viewport_.setViewedComponent(&contentContainer_, false);
    viewport_.setScrollBarsShown(false, true);
    viewport_.setScrollBarThickness(8);
    addAndMakeVisible(viewport_);

    emptyStateLabel_.setText("LA COLA SECUENCIAL ESTA VACIA\nHaz clic en [+ AGREGAR EFECTO] para comenzar tu cadena de efectos.", juce::dontSendNotification);
    emptyStateLabel_.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    emptyStateLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff60647a));
    emptyStateLabel_.setJustificationType(juce::Justification::centred);
    contentContainer_.addAndMakeVisible(emptyStateLabel_);

    rebuild();
}

void SequentialStripComponent::rebuild() {
    slots_.clear();
    insertBetweenButtons_.clear();
    contentContainer_.removeAllChildren();

    const auto chain = processor_.getLinearNodeChain();
    const bool isEmpty = chain.empty();

    emptyStateLabel_.setVisible(isEmpty);
    if (isEmpty) {
        contentContainer_.addAndMakeVisible(emptyStateLabel_);
    }

    const int numSlots = static_cast<int>(chain.size());

    for (int i = 0; i < numSlots; ++i) {
        NodeId id = chain[static_cast<size_t>(i)];
        auto* inst = processor_.getGraph().getNode(id);
        if (inst == nullptr || inst->processor == nullptr) continue;

        auto slot = std::make_unique<SequentialSlotComponent>(
            id, i, inst->name, inst->type, inst->processor.get());

        slot->setSlotIndex(i, i == 0, i == numSlots - 1);

        slot->setOnMoveLeftRequested([this](int idx) {
            if (idx > 0) {
                processor_.moveNodeInLinearChain(idx, idx - 1);
                rebuild();
            }
        });

        slot->setOnMoveRightRequested([this](int idx) {
            processor_.moveNodeInLinearChain(idx, idx + 1);
            rebuild();
        });

        slot->setOnDeleteRequested([this](NodeId idToDelete) {
            processor_.removeNodeFromLinearChain(idToDelete);
            rebuild();
        });

        slot->setOnBypassToggled([this, id](NodeId /*nodeId*/, bool isBypassed) {
            // Manejo de bypass en el nodo
            if (auto* nodeInst = processor_.getGraph().getNode(id)) {
                if (nodeInst->processor != nullptr) {
                    // Si el nodo tiene parámetro Mix (ID 1 o 100), se ajusta según corresponda
                    const auto params = nodeInst->processor->getParameters();
                    for (const auto& p : params) {
                        if (juce::String(p.name).equalsIgnoreCase("Mix")) {
                            nodeInst->processor->setParameter(p.id, isBypassed ? 0.0f : p.defaultValue);
                            break;
                        }
                    }
                }
            }
        });

        contentContainer_.addAndMakeVisible(*slot);
        slots_.push_back(std::move(slot));

        // Botón intermedio de inserción entre slots [+]
        if (i < numSlots - 1) {
            auto insertBtn = std::make_unique<juce::TextButton>("+");
            insertBtn->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181824));
            insertBtn->setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00f0ff));
            const int insertIdx = i + 1;
            insertBtn->onClick = [this, insertIdx]() {
                showAddEffectMenu(insertIdx);
            };
            contentContainer_.addAndMakeVisible(*insertBtn);
            insertBetweenButtons_.push_back(std::move(insertBtn));
        }
    }

    // Botón para agregar al final [+ ADD EFFECT]
    addSlotEndBtn_ = std::make_unique<juce::TextButton>("+ AGREGAR EFECTO");
    addSlotEndBtn_->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff141524));
    addSlotEndBtn_->setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00f0ff));
    addSlotEndBtn_->onClick = [this]() {
        showAddEffectMenu(-1);
    };
    contentContainer_.addAndMakeVisible(*addSlotEndBtn_);

    resized();
    repaint();
}

void SequentialStripComponent::showAddEffectMenu(int insertIndex) {
    juce::PopupMenu menu;

    // 1. Dinámica
    juce::PopupMenu dynMenu;
    dynMenu.addItem(1, "Compresor VCA (Soft-Knee)");
    dynMenu.addItem(2, "Multibanda OTT (3-Band Dynamics)");
    menu.addSubMenu("Dinámica & Ganancia", dynMenu);

    // 2. Filtros y EQ
    juce::PopupMenu filterMenu;
    filterMenu.addItem(3, "EQ Paramétrico (3 Bandas)");
    filterMenu.addItem(4, "Filtro Simple (State Variable)");
    filterMenu.addItem(5, "Banco de Resonadores (6 Modos)");
    menu.addSubMenu("Filtros & EQ", filterMenu);

    // 3. Tiempo y Espacio
    juce::PopupMenu timeMenu;
    timeMenu.addItem(6, "Stereo Delay (Ping-Pong)");
    timeMenu.addItem(7, "Reverb FDN (4-Line Householder)");
    timeMenu.addItem(8, "Spatial Panner 3D (Woodworth ITD/ILD)");
    menu.addSubMenu("Tiempo & Espacio", timeMenu);

    // 4. Modulación & Tono
    juce::PopupMenu modMenu;
    modMenu.addItem(9, "Phaser (6 Etapas Allpass)");
    modMenu.addItem(10, "Chorus (4 Voces Cuadratura)");
    modMenu.addItem(11, "Flanger (Comb Bipolar)");
    modMenu.addItem(12, "Modulador en Anillo (4 Cuadrantes)");
    modMenu.addItem(13, "Desplazador de Frecuencia (Hilbert SSB)");
    modMenu.addItem(14, "Pitch Shifter (Doble Cabezal)");
    menu.addSubMenu("Modulación & Pitch", modMenu);

    // 5. Granular & Glitch
    juce::PopupMenu grainMenu;
    grainMenu.addItem(15, "Sintetizador Granular en Nube");
    grainMenu.addItem(16, "Glitch Beat Slicer");
    grainMenu.addItem(17, "Spectral Freeze (STFT Drone)");
    menu.addSubMenu("Granular & Glitch", grainMenu);

    // 6. Saturación & Calor
    juce::PopupMenu distMenu;
    distMenu.addItem(18, "Distorsión (5 Shapers Analógicos)");
    distMenu.addItem(19, "Saturación de Cinta (Wow & Flutter)");
    menu.addSubMenu("Color & Distorsión", distMenu);

    // 7. Ruteo & Contenedores
    juce::PopupMenu routeMenu;
    routeMenu.addItem(20, "Codificador Mid/Side");
    routeMenu.addItem(21, "Decodificador Mid/Side (Mono Bass Maker)");
    routeMenu.addItem(22, "Contenedor de Subgrafo");
    routeMenu.addItem(23, "Lazo de Feedback Controlado");
    routeMenu.addItem(24, "Rack de Eventos Acústicos");
    menu.addSubMenu("Ruteo & Contenedores", routeMenu);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this, insertIndex](int result) {
        if (result <= 0) return;

        NodeType type = NodeType::Unknown;
        switch (result) {
            case 1:  type = NodeType::Compressor; break;
            case 2:  type = NodeType::Multiband; break;
            case 3:  type = NodeType::Filter; break;
            case 4:  type = NodeType::Passthrough; break; // o SimpleFilter
            case 5:  type = NodeType::Resonator; break;
            case 6:  type = NodeType::Delay; break;
            case 7:  type = NodeType::Reverb; break;
            case 8:  type = NodeType::SpatialPanner; break;
            case 9:  type = NodeType::Phaser; break;
            case 10: type = NodeType::Chorus; break;
            case 11: type = NodeType::Flanger; break;
            case 12: type = NodeType::RingModulator; break;
            case 13: type = NodeType::FrequencyShifter; break;
            case 14: type = NodeType::PitchShifter; break;
            case 15: type = NodeType::Granular; break;
            case 16: type = NodeType::Glitch; break;
            case 17: type = NodeType::Spectral; break;
            case 18: type = NodeType::Distortion; break;
            case 19: type = NodeType::Tape; break;
            case 20: type = NodeType::MidSideEncoder; break;
            case 21: type = NodeType::MidSideDecoder; break;
            case 22: type = NodeType::Container; break;
            case 23: type = NodeType::Feedback; break;
            case 24: type = NodeType::EventContainer; break;
            default: break;
        }

        if (type != NodeType::Unknown) {
            processor_.insertNodeInLinearChain(type, insertIndex);
            rebuild();
        }
    });
}

void SequentialStripComponent::paint(juce::Graphics& g) {
    // Fondo de la franja de slots
    g.setColour(juce::Colour(0xff0a0b12));
    g.fillRect(getLocalBounds());

    // Patrón sutil de puntos de fondo
    g.setColour(juce::Colour(0xff141520));
    for (int x = 0; x < getWidth(); x += 24) {
        for (int y = 0; y < getHeight(); y += 24) {
            g.fillRect(x, y, 1, 1);
        }
    }
}

void SequentialStripComponent::resized() {
    viewport_.setBounds(getLocalBounds());

    const int slotW = 184;
    const int slotH = 260;
    const int arrowW = 38;
    const int startX = 110; // Espacio tras la placa de entrada
    const int numSlots = static_cast<int>(slots_.size());

    const int totalContentWidth = std::max(getWidth(), startX + numSlots * (slotW + arrowW) + 240);
    const int contentH = std::max(getHeight(), slotH + 40);

    contentContainer_.setBounds(0, 0, totalContentWidth, contentH);
    emptyStateLabel_.setBounds(0, 0, totalContentWidth, contentH);

    int curX = startX;
    const int curY = (contentH - slotH) / 2;

    for (int i = 0; i < numSlots; ++i) {
        slots_[static_cast<size_t>(i)]->setBounds(curX, curY, slotW, slotH);
        curX += slotW;

        if (i < static_cast<int>(insertBetweenButtons_.size())) {
            // Posicionar botón de inserción centrado en la flecha de flujo
            insertBetweenButtons_[static_cast<size_t>(i)]->setBounds(curX + 9, curY + (slotH / 2) - 10, 20, 20);
        }
        curX += arrowW;
    }

    if (addSlotEndBtn_ != nullptr) {
        addSlotEndBtn_->setBounds(curX + 10, curY, 150, slotH);
    }
}

} // namespace audio_graph
