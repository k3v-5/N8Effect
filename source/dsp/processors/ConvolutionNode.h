#pragma once

#include <vector>
#include <array>
#include <complex>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <string_view>
#include <atomic>
#include "../../core/Types.h"
#include "../../graph/AudioProcessorNode.h"
#include "../../graph/NodeFactory.h"
#include "../core/FFTEngine.h"
#include "../core/BiquadFilter.h"
#include "../core/DenormalGuards.h"
#include "../core/FastMath.h"

namespace audio_graph {

/**
 * @brief Estructura de almacenamiento particionado de IR para doble buffering lock-free (Reglas 9 y 30)
 */
struct PartitionedIRData {
    std::array<std::vector<float>, 2> headFir;
    std::array<std::vector<std::vector<std::complex<float>>>, 2> irSpectra;
    size_t activePartitions{ 2 };
};

/**
 * @brief Motor de Convolución Particionada Zero-Latency (IR Reverb & Cab Sim)
 * Convolución híbrida multietapa: partición directa temporal en cabeza (0 latencia)
 * y particiones uniformes en frecuencia por bloques FFT (Reglas 5, 8, 9, 14, 34, 46, 47).
 */
class ConvolutionNode : public AudioProcessorNode {
public:
    enum Param : ParameterId {
        IRType = 1,       // 0: Small Room, 1: Large Hall, 2: Dark Plate, 3: Vintage Cab 4x12, 4: Spring Tank, 5: Custom
        PreDelayMs = 2,   // 0.0 a 100.0 ms
        LowCutHz = 3,     // 20.0 a 1000.0 Hz (Filtro paso-altos para limpiar graves de la IR)
        HighCutHz = 4,    // 1000.0 a 20000.0 Hz (Filtro paso-bajos para amortiguar agudos)
        StereoWidth = 5,  // 0.0 a 2.0 (Apertura Mid/Side de la reverberación / cabina)
        Mix = 6           // 0.0 a 1.0 (Mezcla Dry / Wet)
    };

    static constexpr size_t PartitionSize = 128;
    static constexpr size_t FftSize = PartitionSize * 2; // 256
    static constexpr size_t MaxPartitions = 64;          // 64 * 128 = 8192 muestras (~170ms a 48kHz o escalable)

    ConvolutionNode() {
        pins_[0] = { 1, "Audio In", PinType::AudioInput, PinDataType::AudioStereo };
        pins_[1] = { 2, "Audio Out", PinType::AudioOutput, PinDataType::AudioStereo };

        params_[0] = { IRType, "IR Model", 0.0f, 0.0f, 5.0f, false };
        params_[1] = { PreDelayMs, "Pre-Delay", 0.0f, 0.0f, 100.0f, true };
        params_[2] = { LowCutHz, "Low Cut", 40.0f, 20.0f, 1000.0f, true };
        params_[3] = { HighCutHz, "High Cut", 16000.0f, 1000.0f, 20000.0f, true };
        params_[4] = { StereoWidth, "Width", 1.0f, 0.0f, 2.0f, true };
        params_[5] = { Mix, "Mix", 0.5f, 0.0f, 1.0f, true };
    }

    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;

        // Bounded pre-delay buffer (100 ms)
        maxPreDelaySamples_ = std::max<size_t>(static_cast<size_t>(spec.sampleRate * 0.12), 64);
        for (auto& buf : preDelayBuffer_) {
            buf.assign(maxPreDelaySamples_, 0.0f);
        }
        preDelayWriteIdx_ = 0;

        // Prealocación completa de ambos slots de IR para doble buffering lock-free (Reglas 9 y 47)
        for (size_t s = 0; s < 2; ++s) {
            for (size_t ch = 0; ch < 2; ++ch) {
                irSlots_[s].headFir[ch].assign(PartitionSize, 0.0f);
                irSlots_[s].irSpectra[ch].assign(MaxPartitions, std::vector<std::complex<float>>(FftSize, { 0.0f, 0.0f }));
            }
            irSlots_[s].activePartitions = 2;
        }
        activeIRIndex_.store(0, std::memory_order_relaxed);

        // Estructuras de convolución para 2 canales (Stereo)
        for (size_t ch = 0; ch < 2; ++ch) {
            headInputBuffer_[ch].assign(PartitionSize * 2, 0.0f);
            headInputPos_[ch] = 0;

            // Línea de retardo de bloques de entrada en frecuencia: X_p[k]
            xSpectra_[ch].assign(MaxPartitions, std::vector<std::complex<float>>(FftSize, { 0.0f, 0.0f }));

            // Scratch buffer prealocado para FFT sin allocations en el hilo de audio (Regla 9)
            fftScratch_[ch].assign(FftSize, { 0.0f, 0.0f });

            // Acumulador de salida y overlap-add buffer
            tailAccumulator_[ch].assign(FftSize, { 0.0f, 0.0f });
            outputOverlap_[ch].assign(FftSize, 0.0f);
            inputBlockBuffer_[ch].assign(PartitionSize, 0.0f);
            inputBlockPos_[ch] = 0;

            // Filtros de modelado
            lowCutFilter_[ch].setHighpass(spec.sampleRate, 40.0f, 0.707f);
            highCutFilter_[ch].setLowpass(spec.sampleRate, 16000.0f, 0.707f);
        }

        irThumbnail_.fill(0.0f);
        blockIndex_ = 0;
        currentIrType_ = -1; // Forzar recálculo del modelo IR
        loadSyntheticIR(static_cast<int>(targetIRType_));
    }

    void reset() noexcept override {
        for (size_t ch = 0; ch < 2; ++ch) {
            std::fill(preDelayBuffer_[ch].begin(), preDelayBuffer_[ch].end(), 0.0f);
            std::fill(headInputBuffer_[ch].begin(), headInputBuffer_[ch].end(), 0.0f);
            for (auto& x : xSpectra_[ch]) {
                std::fill(x.begin(), x.end(), std::complex<float>(0.0f, 0.0f));
            }
            std::fill(fftScratch_[ch].begin(), fftScratch_[ch].end(), std::complex<float>(0.0f, 0.0f));
            std::fill(tailAccumulator_[ch].begin(), tailAccumulator_[ch].end(), std::complex<float>(0.0f, 0.0f));
            std::fill(outputOverlap_[ch].begin(), outputOverlap_[ch].end(), 0.0f);
            std::fill(inputBlockBuffer_[ch].begin(), inputBlockBuffer_[ch].end(), 0.0f);
            headInputPos_[ch] = 0;
            inputBlockPos_[ch] = 0;
            lowCutFilter_[ch].reset();
            highCutFilter_[ch].reset();
        }
        preDelayWriteIdx_ = 0;
        blockIndex_ = 0;
    }

    void process(ProcessContext& ctx) noexcept override {
        ScopedDenormalGuard denormalGuard;
        if (ctx.numSamples == 0 || ctx.outputChannels == nullptr) return;

        // Comprobar cambio de modelo de IR sintético
        const int requestedIR = static_cast<int>(std::round(targetIRType_));
        if (requestedIR != currentIrType_ && requestedIR >= 0 && requestedIR <= 4) {
            loadSyntheticIR(requestedIR);
        }

        // Carga lock-free del slot activo con semántica acquire (Reglas 9 y 30)
        const size_t activeIdx = activeIRIndex_.load(std::memory_order_acquire);
        const auto& currentIR = irSlots_[activeIdx];

        // Actualizar filtros de corte si han variado
        if (std::abs(lastLowCut_ - targetLowCut_) > 1.0f) {
            lastLowCut_ = targetLowCut_;
            for (size_t ch = 0; ch < 2; ++ch) {
                lowCutFilter_[ch].setHighpass(spec_.sampleRate, targetLowCut_, 0.707f);
            }
        }
        if (std::abs(lastHighCut_ - targetHighCut_) > 5.0f) {
            lastHighCut_ = targetHighCut_;
            for (size_t ch = 0; ch < 2; ++ch) {
                highCutFilter_[ch].setLowpass(spec_.sampleRate, targetHighCut_, 0.707f);
            }
        }

        const size_t preDelaySamples = static_cast<size_t>(targetPreDelayMs_ * 0.001f * spec_.sampleRate);
        const float width = targetWidth_;
        const float mix = targetMix_;

        for (size_t s = 0; s < ctx.numSamples; ++s) {
            float inL = (ctx.numInputChannels > 0 && ctx.inputChannels[0]) ? ctx.inputChannels[0][s] : 0.0f;
            float inR = (ctx.numInputChannels > 1 && ctx.inputChannels[1]) ? ctx.inputChannels[1][s] : inL;

            // Denormal protection
            inL = FastMath::flushDenormal(inL);
            inR = FastMath::flushDenormal(inR);

            // Pre-delay ring buffer
            preDelayBuffer_[0][preDelayWriteIdx_] = inL;
            preDelayBuffer_[1][preDelayWriteIdx_] = inR;

            size_t readIdx = (preDelayWriteIdx_ + maxPreDelaySamples_ - preDelaySamples) % maxPreDelaySamples_;
            float delayedInL = preDelayBuffer_[0][readIdx];
            float delayedInR = preDelayBuffer_[1][readIdx];
            preDelayWriteIdx_ = (preDelayWriteIdx_ + 1) % maxPreDelaySamples_;

            float wetL = 0.0f;
            float wetR = 0.0f;

            // Procesar canal L y canal R de forma simétrica con el slot activo
            wetL = processSample(0, delayedInL, currentIR);
            wetR = processSample(1, delayedInR, currentIR);

            // Filtrado acústico de la señal convolucionada
            wetL = lowCutFilter_[0].process(wetL);
            wetL = highCutFilter_[0].process(wetL);
            wetR = lowCutFilter_[1].process(wetR);
            wetR = highCutFilter_[1].process(wetR);

            // Control de Apertura Mid/Side
            float mid = 0.5f * (wetL + wetR);
            float side = 0.5f * (wetL - wetR) * width;
            wetL = mid + side;
            wetR = mid - side;

            // Mezcla Dry/Wet anti-click
            float outL = inL * (1.0f - mix) + wetL * mix;
            float outR = inR * (1.0f - mix) + wetR * mix;

            if (ctx.outputChannels[0]) ctx.outputChannels[0][s] = outL;
            if (ctx.numOutputChannels > 1 && ctx.outputChannels[1]) ctx.outputChannels[1][s] = outR;
        }
    }

    std::span<const PinDescriptor> getPins() const noexcept override { return pins_; }
    std::span<const ParameterInfo> getParameters() const noexcept override { return params_; }

    NodeType getType() const noexcept override { return NodeType::Convolution; }
    const char* getName() const noexcept override { return "Convolution"; }

    float getParameter(ParameterId id) const override {
        switch (id) {
            case IRType: return targetIRType_;
            case PreDelayMs: return targetPreDelayMs_;
            case LowCutHz: return targetLowCut_;
            case HighCutHz: return targetHighCut_;
            case StereoWidth: return targetWidth_;
            case Mix: return targetMix_;
            default: return 0.0f;
        }
    }

    void setParameter(ParameterId id, float value) override {
        switch (id) {
            case IRType: targetIRType_ = std::clamp(value, 0.0f, 5.0f); break;
            case PreDelayMs: targetPreDelayMs_ = std::clamp(value, 0.0f, 100.0f); break;
            case LowCutHz: targetLowCut_ = std::clamp(value, 20.0f, 1000.0f); break;
            case HighCutHz: targetHighCut_ = std::clamp(value, 1000.0f, 20000.0f); break;
            case StereoWidth: targetWidth_ = std::clamp(value, 0.0f, 2.0f); break;
            case Mix: targetMix_ = std::clamp(value, 0.0f, 1.0f); break;
            default: break;
        }
    }

    /**
     * @brief Acceso a la miniatura envolvente normalizada de 64 puntos de la IR (para visualizadores LCD)
     */
    const std::array<float, 64>& getIRThumbnail() const noexcept { return irThumbnail_; }

    /**
     * @brief Permite cargar una respuesta al impulso personalizada desde memoria con doble buffering lock-free
     */
    void setCustomImpulseResponse(const float* lData, const float* rData, size_t numSamples) {
        if (!lData || numSamples == 0) return;
        targetIRType_ = 5.0f;
        currentIrType_ = 5;

        // Actualizar envolvente para renderizado en pantalla LCD
        updateThumbnail(lData, rData, numSamples);

        // Doble buffering: escribir en el slot inactivo y realizar swap atómico con release (Reglas 9 y 30)
        const size_t activeIdx = activeIRIndex_.load(std::memory_order_relaxed);
        const size_t inactiveIdx = 1 - activeIdx;

        populateIRSlot(inactiveIdx, lData, rData, numSamples);
        activeIRIndex_.store(inactiveIdx, std::memory_order_release);
    }

private:
    float processSample(size_t ch, float input, const PartitionedIRData& irData) noexcept {
        // 1. Guardar en el buffer circular de cabeza para FIR directo (Zero Latency)
        headInputBuffer_[ch][headInputPos_[ch]] = input;
        headInputBuffer_[ch][headInputPos_[ch] + PartitionSize] = input; // Duplicado para evitar módulo

        // Convolución FIR directa de la partición 0 (Head): latencia = 0 muestras
        float headOutput = 0.0f;
        const size_t offset = headInputPos_[ch] + 1;
        for (size_t i = 0; i < PartitionSize; ++i) {
            headOutput += headInputBuffer_[ch][offset + PartitionSize - 1 - i] * irData.headFir[ch][i];
        }

        headInputPos_[ch] = (headInputPos_[ch] + 1) % PartitionSize;

        // 2. Extraer salida acumulada de las particiones FFT previas (overlap-add)
        float tailOutput = outputOverlap_[ch][inputBlockPos_[ch]];
        outputOverlap_[ch][inputBlockPos_[ch]] = 0.0f; // Limpiar para el siguiente ciclo

        // 3. Guardar en el bloque acumulador de entrada
        inputBlockBuffer_[ch][inputBlockPos_[ch]] = input;
        inputBlockPos_[ch]++;

        // Cuando se completa un bloque de tamaño PartitionSize, procesar las colas FFT
        if (inputBlockPos_[ch] >= PartitionSize) {
            inputBlockPos_[ch] = 0;
            processTailBlock(ch, irData);
        }

        return headOutput + tailOutput;
    }

    void processTailBlock(size_t ch, const PartitionedIRData& irData) noexcept {
        const size_t partitions = irData.activePartitions;

        // 1. Desplazar la línea de retardo de espectros de entrada X_p: X_p = X_{p-1}
        for (size_t p = partitions - 1; p > 0; --p) {
            std::copy(xSpectra_[ch][p - 1].begin(), xSpectra_[ch][p - 1].end(), xSpectra_[ch][p].begin());
        }

        // 2. Calcular FFT del bloque de entrada recién completado en scratch prealocado (Regla 9: Zero Malloc)
        for (size_t i = 0; i < PartitionSize; ++i) {
            fftScratch_[ch][i] = { inputBlockBuffer_[ch][i], 0.0f };
        }
        for (size_t i = PartitionSize; i < FftSize; ++i) {
            fftScratch_[ch][i] = { 0.0f, 0.0f };
        }
        FFTEngine::computeRadix2FFT(fftScratch_[ch], false);
        std::copy(fftScratch_[ch].begin(), fftScratch_[ch].end(), xSpectra_[ch][0].begin());

        // 3. Convolución espectral con acumulación: Y[k] = sum_{p=1}^{active-1} X_{p-1}[k] * H_p[k]
        std::fill(tailAccumulator_[ch].begin(), tailAccumulator_[ch].end(), std::complex<float>(0.0f, 0.0f));

        for (size_t p = 1; p < partitions; ++p) {
            const auto& Xp = xSpectra_[ch][p - 1]; // X_0 es el bloque de entrada actual para la partición 1 (H_1)
            const auto& Hp = irData.irSpectra[ch][p];
            for (size_t k = 0; k < FftSize; ++k) {
                tailAccumulator_[ch][k] += Xp[k] * Hp[k];
            }
        }

        // 4. IFFT del acumulador de colas
        FFTEngine::computeRadix2FFT(tailAccumulator_[ch], true);

        // 5. Desplazar la mitad superior de outputOverlap_ a la mitad inferior y limpiar la mitad superior
        for (size_t i = 0; i < PartitionSize; ++i) {
            outputOverlap_[ch][i] = outputOverlap_[ch][i + PartitionSize];
            outputOverlap_[ch][i + PartitionSize] = 0.0f;
        }

        // 6. Overlap-Add hacia el buffer de salida
        for (size_t i = 0; i < FftSize; ++i) {
            outputOverlap_[ch][i] += tailAccumulator_[ch][i].real();
        }
    }

    void loadSyntheticIR(int type) {
        currentIrType_ = type;
        const size_t totalSamples = std::min<size_t>(static_cast<size_t>(spec_.sampleRate * 2.5), MaxPartitions * PartitionSize);
        std::vector<float> irL(totalSamples, 0.0f);
        std::vector<float> irR(totalSamples, 0.0f);

        switch (type) {
            case 0: { // Small Room (Estudio limpio y brillante)
                float decayRate = 18.0f / static_cast<float>(spec_.sampleRate);
                irL[0] = 1.0f; irR[0] = 0.95f;
                // Reflexiones primarias discretas
                const std::array<size_t, 6> early = { 47, 89, 131, 211, 283, 379 };
                const std::array<float, 6> earlyGains = { 0.45f, -0.38f, 0.32f, -0.28f, 0.22f, -0.18f };
                for (size_t i = 0; i < 6; ++i) {
                    if (early[i] < totalSamples) {
                        irL[early[i]] += earlyGains[i];
                        irR[early[i] + 7] += earlyGains[i] * 0.9f;
                    }
                }
                // Cola exponencial difusa
                for (size_t n = 1; n < totalSamples; ++n) {
                    float env = std::exp(-decayRate * static_cast<float>(n));
                    float noiseL = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                    float noiseR = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                    irL[n] += noiseL * env * 0.35f;
                    irR[n] += noiseR * env * 0.35f;
                }
                break;
            }
            case 1: { // Large Hall (Catedral exuberante)
                float decayRate = 3.5f / static_cast<float>(spec_.sampleRate);
                irL[0] = 0.7f; irR[0] = 0.7f;
                for (size_t n = 1; n < totalSamples; ++n) {
                    float env = std::exp(-decayRate * static_cast<float>(n));
                    float noiseL = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                    float noiseR = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                    irL[n] += noiseL * env * 0.4f;
                    irR[n] += noiseR * env * 0.4f;
                }
                break;
            }
            case 2: { // Dark Plate (Placa de acero cálida)
                float decayRate = 5.0f / static_cast<float>(spec_.sampleRate);
                irL[0] = 0.9f; irR[0] = 0.85f;
                float filterStateL = 0.0f, filterStateR = 0.0f;
                for (size_t n = 1; n < totalSamples; ++n) {
                    float env = std::exp(-decayRate * static_cast<float>(n));
                    float noiseL = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                    float noiseR = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                    filterStateL = filterStateL * 0.65f + noiseL * 0.35f;
                    filterStateR = filterStateR * 0.65f + noiseR * 0.35f;
                    irL[n] += filterStateL * env * 0.5f;
                    irR[n] += filterStateR * env * 0.5f;
                }
                break;
            }
            case 3: { // Vintage Cab 4x12 (Simulación de altavoz celestion)
                // Impulso corto con pico en 2.8 kHz y caídas abruptas
                const size_t cabLen = std::min<size_t>(totalSamples, 1024);
                float f0 = 2800.0f;
                float omega = 2.0f * std::numbers::pi_v<float> * f0 / static_cast<float>(spec_.sampleRate);
                float decay = 120.0f / static_cast<float>(spec_.sampleRate);
                for (size_t n = 0; n < cabLen; ++n) {
                    float env = std::exp(-decay * static_cast<float>(n));
                    float sample = std::sin(omega * static_cast<float>(n)) * env;
                    irL[n] = sample * 0.85f;
                    irR[n] = sample * 0.85f;
                }
                break;
            }
            case 4: { // Spring Tank (Resonancia metálica dispersiva)
                const size_t springLen = std::min<size_t>(totalSamples, 4096);
                float decay = 15.0f / static_cast<float>(spec_.sampleRate);
                for (size_t n = 0; n < springLen; ++n) {
                    float env = std::exp(-decay * static_cast<float>(n));
                    // Chirp con frecuencia ascendente
                    float phase = (0.05f * static_cast<float>(n) + 0.0001f * static_cast<float>(n * n));
                    float sL = std::sin(phase) * env;
                    float sR = std::sin(phase * 1.04f + 0.5f) * env;
                    irL[n] = sL * 0.6f;
                    irR[n] = sR * 0.6f;
                }
                break;
            }
            default:
                irL[0] = 1.0f;
                irR[0] = 1.0f;
                break;
        }

        // Actualizar miniatura y cargar ambas particiones en slot inactivo con swap
        updateThumbnail(irL.data(), irR.data(), totalSamples);

        const size_t activeIdx = activeIRIndex_.load(std::memory_order_relaxed);
        const size_t inactiveIdx = 1 - activeIdx;
        populateIRSlot(inactiveIdx, irL.data(), irR.data(), totalSamples);
        activeIRIndex_.store(inactiveIdx, std::memory_order_release);
    }

    void updateThumbnail(const float* lData, const float* rData, size_t numSamples) {
        if (!lData || numSamples == 0) {
            irThumbnail_.fill(0.0f);
            return;
        }

        float globalMax = 0.0f;
        for (size_t pt = 0; pt < 64; ++pt) {
            const size_t start = pt * numSamples / 64;
            size_t end = (pt + 1) * numSamples / 64;
            if (end <= start) end = start + 1;
            if (end > numSamples) end = numSamples;

            float peak = 0.0f;
            for (size_t i = start; i < end; ++i) {
                float valL = std::abs(lData[i]);
                float valR = rData ? std::abs(rData[i]) : valL;
                float val = std::max(valL, valR);
                if (val > peak) peak = val;
            }
            irThumbnail_[pt] = peak;
            if (peak > globalMax) globalMax = peak;
        }

        if (globalMax > 1e-5f) {
            for (float& v : irThumbnail_) {
                v /= globalMax;
            }
        }
    }

    void populateIRSlot(size_t slotIdx, const float* lData, const float* rData, size_t numSamples) {
        if (!lData || numSamples == 0 || slotIdx >= 2) return;
        auto& slot = irSlots_[slotIdx];

        // Normalización de energía del impulso
        float maxPeak = 0.0f;
        for (size_t i = 0; i < numSamples; ++i) {
            float valL = std::abs(lData[i]);
            float valR = rData ? std::abs(rData[i]) : valL;
            maxPeak = std::max({ maxPeak, valL, valR });
        }
        const float gain = (maxPeak > 1e-4f) ? (0.95f / maxPeak) : 1.0f;

        const size_t totalPartitions = std::min<size_t>(MaxPartitions, (numSamples + PartitionSize - 1) / PartitionSize);
        slot.activePartitions = std::max<size_t>(totalPartitions, 2);

        std::vector<std::complex<float>> hp(FftSize, { 0.0f, 0.0f });

        for (size_t ch = 0; ch < 2; ++ch) {
            const float* src = (ch == 0) ? lData : (rData ? rData : lData);

            // 1. Partición 0 (Head): direct FIR
            for (size_t i = 0; i < PartitionSize; ++i) {
                slot.headFir[ch][i] = (i < numSamples) ? (src[i] * gain) : 0.0f;
            }

            // 2. Particiones 1 a P-1 (Tail): FFT transform
            for (size_t p = 1; p < slot.activePartitions; ++p) {
                const size_t startIdx = p * PartitionSize;
                for (size_t i = 0; i < PartitionSize; ++i) {
                    size_t idx = startIdx + i;
                    hp[i] = (idx < numSamples) ? std::complex<float>(src[idx] * gain, 0.0f) : std::complex<float>(0.0f, 0.0f);
                }
                for (size_t i = PartitionSize; i < FftSize; ++i) {
                    hp[i] = { 0.0f, 0.0f };
                }
                FFTEngine::computeRadix2FFT(hp, false);
                std::copy(hp.begin(), hp.end(), slot.irSpectra[ch][p].begin());
            }

            // Limpiar particiones no activas
            for (size_t p = slot.activePartitions; p < MaxPartitions; ++p) {
                std::fill(slot.irSpectra[ch][p].begin(), slot.irSpectra[ch][p].end(), std::complex<float>(0.0f, 0.0f));
            }
        }
    }

    ProcessSpec spec_{ 48000.0, 256, 2, 2 };

    // Pre-delay
    std::array<std::vector<float>, 2> preDelayBuffer_;
    size_t preDelayWriteIdx_{ 0 };
    size_t maxPreDelaySamples_{ 1024 };

    // Head FIR (Partición 0) & Tail FFT Partitions double-buffered (Reglas 9 y 30)
    std::array<PartitionedIRData, 2> irSlots_;
    std::atomic<size_t> activeIRIndex_{ 0 };
    std::array<float, 64> irThumbnail_{};

    std::array<std::vector<float>, 2> headInputBuffer_;
    std::array<size_t, 2> headInputPos_{};

    // Tail FFT Buffers
    std::array<std::vector<std::vector<std::complex<float>>>, 2> xSpectra_;
    std::array<std::vector<std::complex<float>>, 2> fftScratch_;
    std::array<std::vector<std::complex<float>>, 2> tailAccumulator_;
    std::array<std::vector<float>, 2> outputOverlap_;
    std::array<std::vector<float>, 2> inputBlockBuffer_;
    std::array<size_t, 2> inputBlockPos_{};

    size_t blockIndex_{ 0 };
    int currentIrType_{ -1 };

    // Filtros de modelado
    std::array<BiquadFilter, 2> lowCutFilter_;
    std::array<BiquadFilter, 2> highCutFilter_;
    float lastLowCut_{ 40.0f };
    float lastHighCut_{ 16000.0f };

    float targetIRType_{ 0.0f };
    float targetPreDelayMs_{ 0.0f };
    float targetLowCut_{ 40.0f };
    float targetHighCut_{ 16000.0f };
    float targetWidth_{ 1.0f };
    float targetMix_{ 0.5f };

    std::array<PinDescriptor, 2> pins_;
    std::array<ParameterInfo, 6> params_;
};

inline AutoRegisterNode<ConvolutionNode> registerConvolution(NodeType::Convolution, "convolution", "Reverb");

} // namespace audio_graph
