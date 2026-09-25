#pragma once

#include <vector>
#include <array>
#include <atomic>
#include <memory>
#include <cstdint>
#include <cassert>
#include <immintrin.h>
#include "Types.h"

namespace audio_graph {

/**
 * @brief Deshabilitador de números denormales RAII (Regla 9 y 34)
 */
class DenormalDisabler {
public:
    DenormalDisabler() noexcept {
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
        oldMxcsr_ = _mm_getcsr();
        // FTZ (Flush to Zero) y DAZ (Denormals are Zero)
        _mm_setcsr(oldMxcsr_ | 0x8040);
#endif
    }

    ~DenormalDisabler() noexcept {
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
        _mm_setcsr(oldMxcsr_);
#endif
    }

    DenormalDisabler(const DenormalDisabler&) = delete;
    DenormalDisabler& operator=(const DenormalDisabler&) = delete;

private:
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
    unsigned int oldMxcsr_{ 0 };
#endif
};

/**
 * @brief Buffer de audio plano de tamaño fijo prealocado (Regla 9 y 10)
 */
class PreallocatedBuffer {
public:
    void prepare(uint32_t numChannels, uint32_t maxSamples) {
        numChannels_ = numChannels;
        maxSamples_ = maxSamples;
        storage_.resize(static_cast<size_t>(numChannels) * maxSamples, 0.0f);
        channelPointers_.resize(numChannels);
        for (uint32_t ch = 0; ch < numChannels; ++ch) {
            channelPointers_[ch] = storage_.data() + (static_cast<size_t>(ch) * maxSamples);
        }
    }

    void clear(uint32_t numSamples) noexcept {
        assert(numSamples <= maxSamples_);
        for (uint32_t ch = 0; ch < numChannels_; ++ch) {
            std::fill_n(channelPointers_[ch], numSamples, 0.0f);
        }
    }

    void copyFrom(const float* const* src, uint32_t srcChannels, uint32_t numSamples) noexcept {
        assert(numSamples <= maxSamples_);
        uint32_t channelsToCopy = std::min(numChannels_, srcChannels);
        for (uint32_t ch = 0; ch < channelsToCopy; ++ch) {
            if (src[ch] != nullptr) {
                std::copy_n(src[ch], numSamples, channelPointers_[ch]);
            }
        }
    }

    float* getWritePointer(uint32_t channel) noexcept {
        assert(channel < numChannels_);
        return channelPointers_[channel];
    }

    const float* getReadPointer(uint32_t channel) const noexcept {
        assert(channel < numChannels_);
        return channelPointers_[channel];
    }

    float* const* getArrayOfWritePointers() noexcept {
        return channelPointers_.data();
    }

    const float* const* getArrayOfReadPointers() const noexcept {
        return const_cast<const float* const*>(channelPointers_.data());
    }

    uint32_t getNumChannels() const noexcept { return numChannels_; }
    uint32_t getMaxSamples() const noexcept { return maxSamples_; }

private:
    uint32_t numChannels_{ 0 };
    uint32_t maxSamples_{ 0 };
    std::vector<float> storage_;
    std::vector<float*> channelPointers_;
};

/**
 * @brief Pool de AudioBuffers preasignados para ejecución en tiempo real (Regla 9 y 10).
 * Sin llamadas a malloc durante el ciclo de audio.
 */
class AudioBufferPool {
public:
    AudioBufferPool() = default;

    void prepare(uint32_t poolSize, uint32_t numChannels, uint32_t maxBlockSize) {
        buffers_.clear();
        availableIndices_.clear();
        
        buffers_.resize(poolSize);
        availableIndices_.resize(poolSize);
        
        for (uint32_t i = 0; i < poolSize; ++i) {
            buffers_[i].prepare(numChannels, maxBlockSize);
            availableIndices_[i] = i;
        }
        nextAvailable_ = poolSize;
    }

    // Adquiere un buffer del pool en tiempo real sin alocaciones
    PreallocatedBuffer* acquire() noexcept {
        if (nextAvailable_ == 0) {
            return nullptr; // Pool agotado (CPU/Memory Protection)
        }
        --nextAvailable_;
        uint32_t index = availableIndices_[nextAvailable_];
        return &buffers_[index];
    }

    // Libera un buffer al pool en tiempo real
    void release(PreallocatedBuffer* buffer) noexcept {
        if (buffer == nullptr) return;
        
        for (size_t i = 0; i < buffers_.size(); ++i) {
            if (&buffers_[i] == buffer) {
                assert(nextAvailable_ < availableIndices_.size());
                availableIndices_[nextAvailable_] = static_cast<uint32_t>(i);
                ++nextAvailable_;
                return;
            }
        }
    }

    // Resetea todos los buffers asignados al pool
    void releaseAll() noexcept {
        nextAvailable_ = static_cast<uint32_t>(buffers_.size());
        for (uint32_t i = 0; i < nextAvailable_; ++i) {
            availableIndices_[i] = i;
        }
    }

    uint32_t getAvailableCount() const noexcept { return nextAvailable_; }
    uint32_t getTotalCapacity() const noexcept { return static_cast<uint32_t>(buffers_.size()); }

private:
    std::vector<PreallocatedBuffer> buffers_;
    std::vector<uint32_t> availableIndices_;
    uint32_t nextAvailable_{ 0 };
};

/**
 * @brief Cola circular SPSC (Single Producer Single Consumer) lock-free (Regla 9 y 26)
 */
template <typename T, size_t Capacity>
class FixedSPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

public:
    FixedSPSCQueue() : head_(0), tail_(0) {}

    bool push(const T& item) noexcept {
        const size_t currentTail = tail_.load(std::memory_order_relaxed);
        const size_t currentHead = head_.load(std::memory_order_acquire);
        
        if (((currentTail + 1) & (Capacity - 1)) == currentHead) {
            return false; // Cola llena
        }

        buffer_[currentTail] = item;
        tail_.store((currentTail + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }

    bool pop(T& item) noexcept {
        const size_t currentHead = head_.load(std::memory_order_relaxed);
        const size_t currentTail = tail_.load(std::memory_order_acquire);

        if (currentHead == currentTail) {
            return false; // Cola vacía
        }

        item = buffer_[currentHead];
        head_.store((currentHead + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }

    bool isEmpty() const noexcept {
        return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
    }

private:
    T buffer_[Capacity];
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
};

/**
 * @brief Buffer circular lock-free SPSC para telemetría visual de audio (Reglas 9, 23, 26, 47).
 * Permite al audio thread volcar bloques de audio sin bloqueo ni allocations,
 * y al GUI thread leerlos a 30/60 fps para alimentar el analizador FFT, osciloscopio y goniometro.
 */
class AudioVisualizerBuffer {
public:
    static constexpr size_t Capacity = 4096;
    static constexpr size_t IndexMask = Capacity - 1;

    AudioVisualizerBuffer() {
        bufferL_.fill(0.0f);
        bufferR_.fill(0.0f);
        writeIndex_.store(0, std::memory_order_relaxed);
    }

    void writeBlock(const float* l, const float* r, size_t numSamples) noexcept {
        if (numSamples == 0 || l == nullptr) return;
        size_t writePos = writeIndex_.load(std::memory_order_relaxed);
        for (size_t i = 0; i < numSamples; ++i) {
            const size_t idx = (writePos + i) & IndexMask;
            bufferL_[idx] = l[i];
            bufferR_[idx] = (r != nullptr) ? r[i] : l[i];
        }
        writeIndex_.store((writePos + numSamples) & IndexMask, std::memory_order_release);
    }

    size_t getLatestSamples(float* destL, float* destR, size_t numSamplesToRead) const noexcept {
        if (numSamplesToRead == 0 || destL == nullptr) return 0;
        const size_t count = std::min(numSamplesToRead, Capacity);
        const size_t currentWrite = writeIndex_.load(std::memory_order_acquire);
        const size_t startPos = (currentWrite + Capacity - count) & IndexMask;
        for (size_t i = 0; i < count; ++i) {
            const size_t idx = (startPos + i) & IndexMask;
            destL[i] = bufferL_[idx];
            if (destR != nullptr) destR[i] = bufferR_[idx];
        }
        return count;
    }

private:
    std::array<float, Capacity> bufferL_{};
    std::array<float, Capacity> bufferR_{};
    alignas(64) std::atomic<size_t> writeIndex_{ 0 };
};

} // namespace audio_graph
