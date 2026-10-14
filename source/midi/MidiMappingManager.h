#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <sstream>
#include <iomanip>
#include "../core/Types.h"
#include "../graph/Graph.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Curvas de respuesta para el mapeo MIDI CC
 */
enum class MidiCurve : uint8_t {
    Linear = 0,
    Exponential,  // x^2: mayor resolución en valores bajos
    Logarithmic,  // sqrt(x): mayor resolución en valores altos
    SShaped       // Smoothstep (3x^2 - 2x^3): zona muerta suave en extremos
};

/**
 * @brief Mapeo individual entre un mensaje MIDI CC y un parámetro objetivo
 */
struct MidiMapping {
    uint8_t ccNumber{ 0 };                 // 0 a 127
    uint8_t channel{ 0 };                  // 0 = Omni (cualquier canal), 1..16 = canal específico
    ParameterId paramId{ 0 };              // ID numérico del parámetro
    NodeId targetNodeId{ InvalidNodeId };  // InvalidNodeId para macros o parámetros globales
    float minVal{ 0.0f };                  // Valor mínimo asignado
    float maxVal{ 1.0f };                  // Valor máximo asignado
    bool inverted{ false };                // Invierte el sentido del recorrido
    MidiCurve curve{ MidiCurve::Linear };  // Curva de interpolación
    std::string customLabel{};             // Etiqueta legible para la GUI
};

/**
 * @brief Administrador universal de MIDI Learn y Perfiles de Controladores (.n8midi)
 * Impulsado por el núcleo nativo Rust n8_rust_core con latencia cero y sin alocaciones en el hilo de audio (Reglas 8, 9, 21, 46).
 */
class MidiMappingManager {
public:
    static constexpr size_t MaxMappings = 128;

    struct LearnTarget {
        ParameterId paramId{ 0 };
        NodeId targetNodeId{ InvalidNodeId };
        float minVal{ 0.0f };
        float maxVal{ 1.0f };
        MidiCurve curve{ MidiCurve::Linear };
        bool inverted{ false };
    };

    MidiMappingManager() {
        rustCore_ = n8_rust_midi_mapping_manager_create();
        mappings_.reserve(MaxMappings);
    }

    ~MidiMappingManager() {
        if (rustCore_ != nullptr) {
            n8_rust_midi_mapping_manager_destroy(rustCore_);
            rustCore_ = nullptr;
        }
    }

    MidiMappingManager(const MidiMappingManager& other) {
        rustCore_ = n8_rust_midi_mapping_manager_create();
        mappings_.reserve(MaxMappings);
        copyFrom(other);
    }

    MidiMappingManager& operator=(const MidiMappingManager& other) {
        if (this != &other) {
            copyFrom(other);
        }
        return *this;
    }

    MidiMappingManager(MidiMappingManager&& other) noexcept {
        rustCore_ = other.rustCore_;
        other.rustCore_ = nullptr;
        mappings_ = std::move(other.mappings_);
        pendingTarget_ = other.pendingTarget_;
    }

    MidiMappingManager& operator=(MidiMappingManager&& other) noexcept {
        if (this != &other) {
            if (rustCore_ != nullptr) {
                n8_rust_midi_mapping_manager_destroy(rustCore_);
            }
            rustCore_ = other.rustCore_;
            other.rustCore_ = nullptr;
            mappings_ = std::move(other.mappings_);
            pendingTarget_ = other.pendingTarget_;
        }
        return *this;
    }

    void reset() noexcept {
        cancelLearning();
        if (rustCore_ != nullptr) {
            n8_rust_midi_mapping_manager_reset(rustCore_);
        }
        mappings_.clear();
    }

    // Modo MIDI Learn
    void startLearning(ParameterId paramId,
                       NodeId targetNodeId = InvalidNodeId,
                       float minVal = 0.0f,
                       float maxVal = 1.0f,
                       MidiCurve curve = MidiCurve::Linear,
                       bool inverted = false) noexcept
    {
        pendingTarget_ = LearnTarget{
            .paramId = paramId,
            .targetNodeId = targetNodeId,
            .minVal = minVal,
            .maxVal = maxVal,
            .curve = curve,
            .inverted = inverted
        };
        if (rustCore_ != nullptr) {
            n8_rust_midi_mapping_manager_start_learning(
                rustCore_, paramId, targetNodeId, minVal, maxVal, static_cast<uint8_t>(curve), inverted);
        }
    }

    void cancelLearning() noexcept {
        if (rustCore_ != nullptr) {
            n8_rust_midi_mapping_manager_cancel_learning(rustCore_);
        }
    }

    bool isLearning() const noexcept {
        if (rustCore_ != nullptr) {
            return n8_rust_midi_mapping_manager_is_learning(rustCore_);
        }
        return false;
    }

    const LearnTarget& getLearnTarget() const noexcept {
        return pendingTarget_;
    }

    // Vinculación manual de mapeo
    bool addMapping(const MidiMapping& mapping) {
        if (rustCore_ != nullptr) {
            bool ok = n8_rust_midi_mapping_manager_add_mapping(
                rustCore_,
                mapping.ccNumber,
                mapping.channel,
                mapping.paramId,
                mapping.targetNodeId,
                mapping.minVal,
                mapping.maxVal,
                mapping.inverted,
                static_cast<uint8_t>(mapping.curve),
                mapping.customLabel.c_str());
            if (ok) {
                syncFromRust();
                return true;
            }
        }
        return false;
    }

    bool removeMapping(ParameterId paramId, NodeId targetNodeId = InvalidNodeId) {
        if (rustCore_ != nullptr) {
            bool ok = n8_rust_midi_mapping_manager_remove_mapping(rustCore_, paramId, targetNodeId);
            if (ok) {
                syncFromRust();
                return true;
            }
        }
        return false;
    }

    void removeMappingByCC(uint8_t ccNumber, uint8_t channel = 0) {
        if (rustCore_ != nullptr) {
            n8_rust_midi_mapping_manager_remove_mapping_by_cc(rustCore_, ccNumber, channel);
            syncFromRust();
        }
    }

    const std::vector<MidiMapping>& getMappings() const noexcept {
        return mappings_;
    }

    /**
     * @brief Procesa un mensaje CC entrante en tiempo real (Audio Thread Safe)
     */
    void processControlChange(uint8_t channel, uint8_t ccNumber, uint8_t ccValue, Graph* graph = nullptr) noexcept {
        if (rustCore_ == nullptr) return;

        const bool wasLearning = isLearning();
        uint32_t targets[64];
        uint32_t paramIds[64];
        float values[64];
        size_t dispatches = n8_rust_midi_mapping_manager_process_control_change(
            rustCore_, channel, ccNumber, ccValue, targets, paramIds, values, 64);

        if (wasLearning && !isLearning()) {
            syncFromRust();
        }

        if (graph != nullptr) {
            for (size_t i = 0; i < dispatches; ++i) {
                if (targets[i] != InvalidNodeId) {
                    auto* proc = graph->getNodeProcessor(targets[i]);
                    if (proc != nullptr) {
                        proc->setParameter(paramIds[i], values[i]);
                    }
                }
            }
        }
    }

    /**
     * @brief Transforma un valor CC (0..127) según la curva, inversión y rango configurado
     */
    static float calculateMappedValue(const MidiMapping& map, uint8_t ccValue) noexcept {
        return n8_rust_midi_mapping_manager_calculate_mapped_value(
            map.minVal, map.maxVal, map.inverted, static_cast<uint8_t>(map.curve), ccValue);
    }

    // =========================================================================
    // Plantillas de Controladores Hardware Preconfiguradas
    // =========================================================================
    void loadFactoryProfile(std::string_view profileName) {
        if (rustCore_ == nullptr) return;
        std::string name(profileName);
        n8_rust_midi_mapping_manager_load_factory_profile(rustCore_, name.c_str());
        syncFromRust();
    }

    // =========================================================================
    // Serialización / Deserialización JSON de Perfiles (.n8midi)
    // =========================================================================
    std::string exportToJson(std::string_view profileName = "Custom Hardware Profile") const {
        if (rustCore_ == nullptr) return {};
        std::string name(profileName);
        std::vector<char> buf(16384, 0);
        size_t len = n8_rust_midi_mapping_manager_export_json(rustCore_, name.c_str(), buf.data(), buf.size());
        if (len >= buf.size()) {
            buf.resize(len + 1, 0);
            n8_rust_midi_mapping_manager_export_json(rustCore_, name.c_str(), buf.data(), buf.size());
        }
        return std::string(buf.data());
    }

    bool importFromJson(std::string_view jsonStr) {
        if (rustCore_ == nullptr || jsonStr.empty()) return false;
        std::string str(jsonStr);
        bool ok = n8_rust_midi_mapping_manager_import_json(rustCore_, str.c_str());
        if (ok) {
            syncFromRust();
        }
        return ok;
    }

private:
    void copyFrom(const MidiMappingManager& other) {
        reset();
        for (const auto& m : other.mappings_) {
            addMapping(m);
        }
        if (other.isLearning()) {
            const auto& t = other.getLearnTarget();
            startLearning(t.paramId, t.targetNodeId, t.minVal, t.maxVal, t.curve, t.inverted);
        }
    }

    void syncFromRust() {
        if (rustCore_ == nullptr) return;
        size_t count = n8_rust_midi_mapping_manager_get_count(rustCore_);
        mappings_.clear();
        mappings_.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            uint8_t cc = 0, ch = 0, curve = 0;
            uint32_t param = 0, node = 0;
            float minV = 0.0f, maxV = 1.0f;
            bool inv = false;
            char labelBuf[128] = { 0 };
            if (n8_rust_midi_mapping_manager_get_mapping(
                rustCore_, i, &cc, &ch, &param, &node, &minV, &maxV, &inv, &curve, labelBuf, sizeof(labelBuf)))
            {
                MidiMapping m;
                m.ccNumber = cc;
                m.channel = ch;
                m.paramId = static_cast<ParameterId>(param);
                m.targetNodeId = static_cast<NodeId>(node);
                m.minVal = minV;
                m.maxVal = maxV;
                m.inverted = inv;
                m.curve = static_cast<MidiCurve>(curve);
                m.customLabel = labelBuf;
                mappings_.push_back(std::move(m));
            }
        }
    }

    RustMidiMappingManager* rustCore_{ nullptr };
    std::vector<MidiMapping> mappings_;
    LearnTarget pendingTarget_{};
};

} // namespace audio_graph
