#pragma once

#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include "../core/Types.h"
#include "../graph/AudioProcessorNode.h"
#include "ModulationTypes.h"

namespace audio_graph {

/**
 * @brief Pista individual de automatización por pasos para un parámetro de un nodo (Reglas 7, 8, 25, 37)
 */
struct NodeAutomationLane {
    bool active{ false };
    ParameterId targetParamId{ 0 };
    float baseValue{ 0.0f };
    bool hasCapturedBase{ false };

    uint32_t numSteps{ 16 };
    std::array<float, 32> steps{}; // Valores normalizados [0.0f, 1.0f]

    SyncDivision rate{ SyncDivision::Sixteenth };
    float glide{ 0.0f };   // [0.0f, 1.0f]
    float amount{ 1.0f };  // [0.0f, 1.0f] Profundidad de modulación

    // Estado en tiempo de ejecución (hilo de audio)
    float currentOutput{ 0.0f };
    float targetOutput{ 0.0f };
    int64_t lastPpqStep{ -1 };
    uint32_t currentStep{ 0 };
    float liveModulatedVal{ 0.0f };

    NodeAutomationLane() {
        steps.fill(0.5f);
    }
};

/**
 * @brief Banco de Automatización Secuenciada por Nodo (Multi-Lane Tabs) (Reglas 1, 7, 8, 9, 25, 46, 47)
 * Permite hasta 4 pistas simultáneas e independientes de automatización rítmica por efecto.
 */
class NodeAutomationBank {
public:
    static constexpr size_t MaxLanes = 4;

    enum ShapeType {
        RampUp = 0,
        RampDown = 1,
        SidechainPump = 2,
        Staccato = 3,
        Triangle = 4,
        Random = 5,
        Flat = 6
    };

    NodeAutomationBank() {
        for (size_t l = 0; l < MaxLanes; ++l) {
            lanes_[l].active = false;
            lanes_[l].numSteps = 16;
            lanes_[l].rate = SyncDivision::Sixteenth;
            lanes_[l].glide = 0.1f;
            lanes_[l].amount = 1.0f;
            applyShape(l, SidechainPump);
        }
    }

    void prepare(double /*sampleRate*/) noexcept {
        for (auto& lane : lanes_) {
            lane.currentOutput = lane.steps[0];
            lane.targetOutput = lane.steps[0];
            lane.lastPpqStep = -1;
            lane.currentStep = 0;
        }
    }

    void reset() noexcept {
        for (auto& lane : lanes_) {
            lane.currentOutput = lane.steps[0];
            lane.targetOutput = lane.steps[0];
            lane.lastPpqStep = -1;
            lane.currentStep = 0;
            lane.liveModulatedVal = lane.baseValue;
        }
    }

    NodeAutomationLane& getLane(size_t index) noexcept {
        return lanes_[index < MaxLanes ? index : 0];
    }

    const NodeAutomationLane& getLane(size_t index) const noexcept {
        return lanes_[index < MaxLanes ? index : 0];
    }

    size_t getSelectedLaneIndex() const noexcept { return selectedLane_; }
    void setSelectedLaneIndex(size_t idx) noexcept { selectedLane_ = idx < MaxLanes ? idx : 0; }

    static double getBeatsPerStep(SyncDivision div) noexcept {
        switch (div) {
            case SyncDivision::Bar_1: return 4.0;
            case SyncDivision::Half: return 2.0;
            case SyncDivision::Quarter: return 1.0;
            case SyncDivision::Eighth: return 0.5;
            case SyncDivision::Sixteenth: return 0.25;
            case SyncDivision::ThirtySecond: return 0.125;
            case SyncDivision::Triplet_Quarter: return 2.0 / 3.0;
            case SyncDivision::Triplet_Eighth: return 1.0 / 3.0;
            case SyncDivision::Dotted_Eighth: return 0.75;
            case SyncDivision::Dotted_Quarter: return 1.5;
            default: return 0.25;
        }
    }

    void applyShape(size_t laneIdx, ShapeType shape) noexcept {
        if (laneIdx >= MaxLanes) return;
        auto& lane = lanes_[laneIdx];
        const uint32_t n = std::clamp<uint32_t>(lane.numSteps, 1, 32);

        switch (shape) {
            case RampUp: {
                for (uint32_t i = 0; i < n; ++i) {
                    lane.steps[i] = static_cast<float>(i) / static_cast<float>(std::max(1u, n - 1));
                }
                break;
            }
            case RampDown: {
                for (uint32_t i = 0; i < n; ++i) {
                    lane.steps[i] = 1.0f - static_cast<float>(i) / static_cast<float>(std::max(1u, n - 1));
                }
                break;
            }
            case SidechainPump: {
                // Curva de bombeo tipo 4-al-piso (repite cada 4 pasos en un patrón de 16)
                const uint32_t period = (n >= 8) ? 4 : n;
                for (uint32_t i = 0; i < n; ++i) {
                    const float t = static_cast<float>(i % period) / static_cast<float>(period);
                    lane.steps[i] = t * t; // Curva cuadrática creciente
                }
                break;
            }
            case Staccato: {
                for (uint32_t i = 0; i < n; ++i) {
                    lane.steps[i] = (i % 2 == 0) ? 0.95f : 0.05f;
                }
                break;
            }
            case Triangle: {
                const uint32_t half = n / 2;
                for (uint32_t i = 0; i < n; ++i) {
                    if (i <= half) {
                        lane.steps[i] = static_cast<float>(i) / static_cast<float>(std::max(1u, half));
                    } else {
                        lane.steps[i] = 1.0f - static_cast<float>(i - half) / static_cast<float>(std::max(1u, n - half));
                    }
                }
                break;
            }
            case Random: {
                uint32_t seed = 0x55AA1234u + static_cast<uint32_t>(laneIdx * 100);
                for (uint32_t i = 0; i < n; ++i) {
                    seed = seed * 1664525u + 1013904223u;
                    lane.steps[i] = static_cast<float>(seed & 0xFFFF) / 65535.0f;
                }
                break;
            }
            case Flat: {
                for (uint32_t i = 0; i < n; ++i) {
                    lane.steps[i] = 0.5f;
                }
                break;
            }
        }
    }

    // Procesamiento en tiempo real por bloque en el hilo de audio (Reglas 9, 26, 47)
    void processBlock(const ProcessContext& ctx, AudioProcessorNode* node) noexcept {
        if (!node) return;

        const auto params = node->getParameters();

        for (size_t l = 0; l < MaxLanes; ++l) {
            auto& lane = lanes_[l];
            if (!lane.active || lane.targetParamId == 0) continue;

            const ParameterInfo* targetInfo = nullptr;
            for (const auto& p : params) {
                if (p.id == lane.targetParamId) {
                    targetInfo = &p;
                    break;
                }
            }
            if (!targetInfo) continue;

            // Capturar valor base inicial si no se había capturado
            if (!lane.hasCapturedBase) {
                lane.baseValue = node->getParameter(lane.targetParamId);
                lane.hasCapturedBase = true;
            }

            // Avance rítmico según el PPQ del host DAW
            if (ctx.isPlaying) {
                const double beatsPerStep = getBeatsPerStep(lane.rate);
                if (beatsPerStep > 0.0) {
                    int64_t stepIdx = static_cast<int64_t>(std::floor(ctx.ppqPosition / beatsPerStep));
                    if (stepIdx != lane.lastPpqStep) {
                        lane.lastPpqStep = stepIdx;
                        const uint32_t n = std::max(1u, lane.numSteps);
                        int64_t wrapped = (stepIdx % n + n) % n;
                        lane.currentStep = static_cast<uint32_t>(wrapped);
                        lane.targetOutput = lane.steps[lane.currentStep];
                    }
                }
            } else {
                lane.targetOutput = lane.steps[lane.currentStep < lane.numSteps ? lane.currentStep : 0];
            }

            // Suavizado Glide exponencial anti-clic
            const float glideCoeff = std::clamp(1.0f - lane.glide * 0.95f, 0.05f, 1.0f);
            lane.currentOutput += glideCoeff * (lane.targetOutput - lane.currentOutput);

            // Interpolación de modulación entre Base Value y Valor de la Secuencia
            const float range = targetInfo->maxValue - targetInfo->minValue;
            const float seqValue = targetInfo->minValue + std::clamp(lane.currentOutput, 0.0f, 1.0f) * range;
            const float modulatedVal = std::clamp(
                (1.0f - lane.amount) * lane.baseValue + lane.amount * seqValue,
                targetInfo->minValue,
                targetInfo->maxValue
            );

            lane.liveModulatedVal = modulatedVal;
            node->setParameter(lane.targetParamId, modulatedVal);
        }
    }

private:
    std::array<NodeAutomationLane, MaxLanes> lanes_;
    size_t selectedLane_{ 0 };
};

} // namespace audio_graph
