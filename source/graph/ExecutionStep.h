#pragma once

#include <vector>
#include <cstddef>
#include "Graph.h"
#include "AudioProcessorNode.h"

namespace audio_graph {

/**
 * @brief Paso de ejecución compilado para el Runtime Graph (Regla 28)
 */
struct ExecutionStep {
    AudioProcessorNode* processor{ nullptr };
    NodeId nodeId{ InvalidNodeId };
    std::vector<NodeId> predecessorNodes;
    std::vector<NodeId> successorNodes;
    std::vector<size_t> predecessorStepIndices;
    int sidechainStepIndex{ -1 };
    int audioRateModStepIndex{ -1 };
    bool isRoot{ true };
    bool isLeaf{ true };
    bool isBypassed{ false };
};

} // namespace audio_graph
