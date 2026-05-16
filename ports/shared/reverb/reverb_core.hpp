#pragma once

#include "SC_PlugIn.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

extern InterfaceTable* ft;

namespace chow_reverb
{
constexpr float pi = 3.14159265358979323846f;

inline int nextPowerOfTwo(int value) noexcept
{
    int power = 1;
    while (power < value)
        power <<= 1;
    return power;
}

struct XorShift32
{
    explicit XorShift32(uint32_t seed = 0x12345678u) noexcept : state(seed == 0u ? 0x12345678u : seed) {}

    uint32_t nextU32() noexcept
    {
        uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return x;
    }

    float next01() noexcept
    {
        return static_cast<float>(nextU32() >> 8) * (1.0f / 16777216.0f);
    }

    float nextSigned() noexcept
    {
        return next01() * 2.0f - 1.0f;
    }

    uint32_t state;
};

inline float delayMultiplier(int channelIndex, int nChannels, uint32_t seedBase) noexcept
{
    const float rangeLow = static_cast<float>(channelIndex + 1) / static_cast<float>(nChannels + 1);
    const float rangeHigh = static_cast<float>(channelIndex + 2) / static_cast<float>(nChannels + 1);
    XorShift32 rng(seedBase + 0x9E3779B9u * static_cast<uint32_t>(channelIndex + 1));
    return rangeLow + (rangeHigh - rangeLow) * rng.next01();
}

struct LinearSmoother
{
    float current = 0.0f;
    float target = 0.0f;
    float step = 0.0f;
    float sampleRate = 48000.0f;
    int samplesRemaining = 0;

    void reset(float newSampleRate, float initialValue) noexcept
    {
        sampleRate = newSampleRate;
        current = initialValue;
        target = initialValue;
        step = 0.0f;
        samplesRemaining = 0;
    }

    void setTarget(float newTarget, float timeSeconds) noexcept
    {
        if (newTarget == target)
            return;

        target = newTarget;
        samplesRemaining = std::max(1, static_cast<int>(std::round(timeSeconds * sampleRate)));
        step = (target - current) / static_cast<float>(samplesRemaining);
    }

    float skip(int numSamples) noexcept
    {
        if (samplesRemaining <= 0)
            return current;

        const int stepCount = std::min(numSamples, samplesRemaining);
        current += step * static_cast<float>(stepCount);
        samplesRemaining -= stepCount;

        if (samplesRemaining <= 0) {
            current = target;
            samplesRemaining = 0;
            step = 0.0f;
        }

        return current;
    }
};

struct MultiplicativeSmoother
{
    float current = 1.0f;
    float target = 1.0f;
    float ratio = 1.0f;
    float sampleRate = 48000.0f;
    int samplesRemaining = 0;

    void reset(float newSampleRate, float initialValue) noexcept
    {
        sampleRate = newSampleRate;
        current = std::max(initialValue, 1.0e-6f);
        target = current;
        ratio = 1.0f;
        samplesRemaining = 0;
    }

    void setTarget(float newTarget, float timeSeconds) noexcept
    {
        newTarget = std::max(newTarget, 1.0e-6f);
        if (newTarget == target)
            return;

        target = newTarget;
        samplesRemaining = std::max(1, static_cast<int>(std::round(timeSeconds * sampleRate)));
        ratio = std::pow(target / std::max(current, 1.0e-6f), 1.0f / static_cast<float>(samplesRemaining));
    }

    float skip(int numSamples) noexcept
    {
        if (samplesRemaining <= 0)
            return current;

        const int stepCount = std::min(numSamples, samplesRemaining);
        current *= std::pow(ratio, static_cast<float>(stepCount));
        samplesRemaining -= stepCount;

        if (samplesRemaining <= 0) {
            current = target;
            samplesRemaining = 0;
            ratio = 1.0f;
        }

        return current;
    }
};

struct SineWave
{
    float sampleRate = 48000.0f;
    float frequency = 1.0f;
    float phase = 0.0f;
    float phaseInc = 0.0f;

    void prepare(float newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        setFrequency(frequency);
        phase = 0.0f;
    }

    void setFrequency(float newFrequency) noexcept
    {
        frequency = newFrequency;
        phaseInc = 2.0f * pi * frequency / sampleRate;
    }

    float processSample() noexcept
    {
        const float y = std::sin(phase);
        phase += phaseInc;
        if (phase >= 2.0f * pi)
            phase -= 2.0f * pi;
        return y;
    }
};

struct OnePoleFilter
{
    float b0 = 1.0f;
    float b1 = 0.0f;
    float a1 = 0.0f;
    float z1 = 0.0f;

    void reset() noexcept
    {
        z1 = 0.0f;
    }

    float process(float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y;
        return y;
    }
};

struct ShelfFilter
{
    void reset() noexcept
    {
        filter.reset();
    }

    void calcCoefs(float lowGain, float highGain, float fc, float sampleRate) noexcept
    {
        if (std::abs(lowGain - highGain) < 1.0e-9f) {
            filter.b0 = lowGain;
            filter.b1 = 0.0f;
            filter.a1 = 0.0f;
            return;
        }

        const float wc = 2.0f * pi * fc;
        const float numerator = wc * wc * (highGain * highGain - lowGain * highGain);
        const float denominator = std::max(lowGain * highGain - lowGain * lowGain, 1.0e-12f);
        const float p = std::sqrt(std::max(numerator / denominator, 1.0e-12f));
        const float k = p / std::tan(p / (2.0f * sampleRate));

        const float b0 = highGain / p;
        const float b1 = lowGain;
        const float a0 = 1.0f / p;
        const float a1 = 1.0f;
        const float a0z = a0 * k + a1;

        filter.b0 = (b0 * k + b1) / a0z;
        filter.b1 = (-b0 * k + b1) / a0z;
        filter.a1 = (-a0 * k + a1) / a0z;
    }

    float process(float x) noexcept
    {
        return filter.process(x);
    }

    OnePoleFilter filter;
};

class IntegerDelayLine
{
public:
    bool allocate(World* world, int maxDelaySamples) noexcept
    {
        free(world);

        size = std::max(maxDelaySamples + 2, 8);
        buffer = static_cast<float*>(RTAlloc(world, static_cast<size_t>(size) * sizeof(float)));
        if (buffer == nullptr) {
            size = 0;
            return false;
        }

        reset();
        return true;
    }

    void free(World* world) noexcept
    {
        if (buffer != nullptr) {
            RTFree(world, buffer);
            buffer = nullptr;
        }

        size = 0;
        writePos = 0;
        readPos = 0;
        delayInt = 1;
    }

    void reset() noexcept
    {
        writePos = 0;
        readPos = 0;
        delayInt = 1;

        if (buffer != nullptr)
            std::fill(buffer, buffer + size, 0.0f);
    }

    void setDelay(float delaySamples) noexcept
    {
        if (buffer == nullptr || size <= 1)
            return;

        delayInt = sc_clip(static_cast<int>(std::floor(delaySamples)), 1, size - 1);
    }

    float popSample() const noexcept
    {
        if (buffer == nullptr || size == 0)
            return 0.0f;

        int index = readPos + delayInt;
        if (index >= size)
            index -= size;
        return buffer[index];
    }

    void pushSample(float x) noexcept
    {
        if (buffer == nullptr || size == 0)
            return;

        buffer[writePos] = x;
        writePos = (writePos + size - 1) % size;
    }

    void updateReadPointer() noexcept
    {
        if (size == 0)
            return;

        readPos = (readPos + size - 1) % size;
    }

private:
    float* buffer = nullptr;
    int size = 0;
    int writePos = 0;
    int readPos = 0;
    int delayInt = 1;
};

inline void householderInPlace(float* data, int size) noexcept
{
    float sum = 0.0f;
    for (int i = 0; i < size; ++i)
        sum += data[i];

    const float offset = sum * (-2.0f / static_cast<float>(size));
    for (int i = 0; i < size; ++i)
        data[i] += offset;
}

inline void multiplyAccumulateSpectrum(float* accum, const float* input, const float* impulse, int framesize) noexcept
{
    accum[0] += input[0] * impulse[0];
    accum[1] += input[1] * impulse[1];

    for (int i = 1; i < framesize; ++i) {
        const int realIndex = 2 * i;
        const int imagIndex = realIndex + 1;
        accum[realIndex] += input[realIndex] * impulse[realIndex] - input[imagIndex] * impulse[imagIndex];
        accum[imagIndex] += input[realIndex] * impulse[imagIndex] + input[imagIndex] * impulse[realIndex];
    }
}

class PartitionedConvolution
{
public:
    bool allocate(InterfaceTable* ft, World* world, int newBlockSize, int maxIRSamples) noexcept
    {
        free(ft, world);

        if (newBlockSize <= 0 || maxIRSamples <= 0)
            return false;

        blockSize = newBlockSize;
        fftSize = blockSize * 2;
        numPartitions = std::max(1, (maxIRSamples + blockSize - 1) / blockSize);

        const size_t spectrumBytes = static_cast<size_t>(numPartitions) * static_cast<size_t>(fftSize) * sizeof(float);
        inputSpectra = static_cast<float*>(RTAlloc(world, spectrumBytes));
        irSpectra = static_cast<float*>(RTAlloc(world, spectrumBytes));
        fftInput = static_cast<float*>(RTAlloc(world, static_cast<size_t>(fftSize) * sizeof(float)));
        fftOutput = static_cast<float*>(RTAlloc(world, static_cast<size_t>(fftSize) * sizeof(float)));
        overlap = static_cast<float*>(RTAlloc(world, static_cast<size_t>(blockSize) * sizeof(float)));

        if (inputSpectra == nullptr || irSpectra == nullptr || fftInput == nullptr || fftOutput == nullptr || overlap == nullptr) {
            free(ft, world);
            return false;
        }

        SCWorld_Allocator alloc(ft, world);
        fftForward = scfft_create(fftSize, fftSize, kRectWindow, fftInput, fftInput, kForward, alloc);
        fftInverse = scfft_create(fftSize, fftSize, kRectWindow, fftOutput, fftOutput, kBackward, alloc);
        if (fftForward == nullptr || fftInverse == nullptr) {
            free(ft, world);
            return false;
        }

        reset();
        return true;
    }

    void free(InterfaceTable* ft, World* world) noexcept
    {
        if (inputSpectra != nullptr) {
            RTFree(world, inputSpectra);
            inputSpectra = nullptr;
        }

        if (irSpectra != nullptr) {
            RTFree(world, irSpectra);
            irSpectra = nullptr;
        }

        if (fftInput != nullptr) {
            RTFree(world, fftInput);
            fftInput = nullptr;
        }

        if (fftOutput != nullptr) {
            RTFree(world, fftOutput);
            fftOutput = nullptr;
        }

        if (overlap != nullptr) {
            RTFree(world, overlap);
            overlap = nullptr;
        }

        if (ft != nullptr) {
            SCWorld_Allocator alloc(ft, world);
            if (fftForward != nullptr) {
                scfft_destroy(fftForward, alloc);
                fftForward = nullptr;
            }

            if (fftInverse != nullptr) {
                scfft_destroy(fftInverse, alloc);
                fftInverse = nullptr;
            }
        } else {
            fftForward = nullptr;
            fftInverse = nullptr;
        }

        blockSize = 0;
        fftSize = 0;
        numPartitions = 0;
        currentPartition = 0;
    }

    void reset() noexcept
    {
        if (inputSpectra != nullptr)
            std::memset(inputSpectra, 0, static_cast<size_t>(numPartitions) * static_cast<size_t>(fftSize) * sizeof(float));
        if (irSpectra != nullptr)
            std::memset(irSpectra, 0, static_cast<size_t>(numPartitions) * static_cast<size_t>(fftSize) * sizeof(float));
        if (fftInput != nullptr)
            std::memset(fftInput, 0, static_cast<size_t>(fftSize) * sizeof(float));
        if (fftOutput != nullptr)
            std::memset(fftOutput, 0, static_cast<size_t>(fftSize) * sizeof(float));
        if (overlap != nullptr)
            std::fill(overlap, overlap + blockSize, 0.0f);
        currentPartition = 0;
    }

    void clearState() noexcept
    {
        if (inputSpectra != nullptr)
            std::memset(inputSpectra, 0, static_cast<size_t>(numPartitions) * static_cast<size_t>(fftSize) * sizeof(float));
        if (fftOutput != nullptr)
            std::memset(fftOutput, 0, static_cast<size_t>(fftSize) * sizeof(float));
        if (overlap != nullptr)
            std::fill(overlap, overlap + blockSize, 0.0f);
        currentPartition = 0;
    }

    void setIR(const float* ir, int irSamples) noexcept
    {
        if (irSpectra == nullptr || fftInput == nullptr)
            return;

        std::memset(irSpectra, 0, static_cast<size_t>(numPartitions) * static_cast<size_t>(fftSize) * sizeof(float));

        for (int partition = 0; partition < numPartitions; ++partition) {
            std::fill(fftInput, fftInput + fftSize, 0.0f);

            const int offset = partition * blockSize;
            if (offset < irSamples) {
                const int copySamples = std::min(blockSize, irSamples - offset);
                std::memcpy(fftInput, ir + offset, static_cast<size_t>(copySamples) * sizeof(float));
            }

            scfft_dofft(fftForward);
            std::memcpy(irSpectra + partition * fftSize, fftInput, static_cast<size_t>(fftSize) * sizeof(float));
        }

        clearState();
    }

    void processBlock(const float* input, float* output, int numSamples) noexcept
    {
        if (fftInput == nullptr || fftOutput == nullptr || overlap == nullptr)
            return;

        std::fill(fftInput, fftInput + fftSize, 0.0f);
        std::memcpy(fftInput, input, static_cast<size_t>(numSamples) * sizeof(float));
        scfft_dofft(fftForward);

        std::memcpy(inputSpectra + currentPartition * fftSize, fftInput, static_cast<size_t>(fftSize) * sizeof(float));
        std::fill(fftOutput, fftOutput + fftSize, 0.0f);

        for (int partition = 0; partition < numPartitions; ++partition) {
            int historyIndex = currentPartition - partition;
            if (historyIndex < 0)
                historyIndex += numPartitions;

            multiplyAccumulateSpectrum(fftOutput,
                                       inputSpectra + historyIndex * fftSize,
                                       irSpectra + partition * fftSize,
                                       blockSize);
        }

        scfft_doifft(fftInverse);

        for (int i = 0; i < numSamples; ++i)
            output[i] = fftOutput[i] + overlap[i];

        std::memcpy(overlap, fftOutput + blockSize, static_cast<size_t>(blockSize) * sizeof(float));
        currentPartition = (currentPartition + 1) % numPartitions;
    }

private:
    float* inputSpectra = nullptr;
    float* irSpectra = nullptr;
    float* fftInput = nullptr;
    float* fftOutput = nullptr;
    float* overlap = nullptr;
    scfft* fftForward = nullptr;
    scfft* fftInverse = nullptr;
    int blockSize = 0;
    int fftSize = 0;
    int numPartitions = 0;
    int currentPartition = 0;
};

class ConvolutionDiffuserCore
{
public:
    static constexpr int maxChannels = 2;

    bool allocate(InterfaceTable* ft, World* world, float newSampleRate, int blockSize, int newNumChannels, float newMaxDiffusionSeconds) noexcept
    {
        free(ft, world);

        sampleRate = newSampleRate;
        numChannels = sc_clip(newNumChannels, 1, maxChannels);
        maxDiffusionSeconds = std::max(newMaxDiffusionSeconds, 0.001f);
        maxKernelSamples = nextPowerOfTwo(static_cast<int>(std::round(sampleRate * maxDiffusionSeconds)));

        for (int ch = 0; ch < numChannels; ++ch) {
            baseKernel[ch] = static_cast<float*>(RTAlloc(world, static_cast<size_t>(maxKernelSamples) * sizeof(float)));
            scratchKernel[ch] = static_cast<float*>(RTAlloc(world, static_cast<size_t>(maxKernelSamples) * sizeof(float)));
            if (baseKernel[ch] == nullptr || scratchKernel[ch] == nullptr)
                return false;

            if (!engines[ch].allocate(ft, world, blockSize, maxKernelSamples))
                return false;
        }

        generateKernels();
        setDiffusionTimeMs(100.0f);
        return true;
    }

    void free(InterfaceTable* ft, World* world) noexcept
    {
        for (int ch = 0; ch < maxChannels; ++ch) {
            engines[ch].free(ft, world);

            if (baseKernel[ch] != nullptr) {
                RTFree(world, baseKernel[ch]);
                baseKernel[ch] = nullptr;
            }

            if (scratchKernel[ch] != nullptr) {
                RTFree(world, scratchKernel[ch]);
                scratchKernel[ch] = nullptr;
            }
        }

        sampleRate = 48000.0f;
        maxDiffusionSeconds = 0.1f;
        maxKernelSamples = 0;
        numChannels = 0;
        currentDiffusionSamples = -1;
    }

    void setDiffusionTimeMs(float diffusionTimeMs) noexcept
    {
        if (maxKernelSamples <= 0)
            return;

        const float clampedMs = sc_clip(diffusionTimeMs, 1.0f, maxDiffusionSeconds * 1000.0f);
        const int diffusionSamples = std::max(1, static_cast<int>(std::round(sampleRate * clampedMs * 0.001f)));
        if (diffusionSamples == currentDiffusionSamples)
            return;

        currentDiffusionSamples = diffusionSamples;
        const float makeupGain = 32.0f / std::sqrt(static_cast<float>(diffusionSamples));

        for (int ch = 0; ch < numChannels; ++ch) {
            std::fill(scratchKernel[ch], scratchKernel[ch] + maxKernelSamples, 0.0f);
            std::memcpy(scratchKernel[ch], baseKernel[ch], static_cast<size_t>(diffusionSamples) * sizeof(float));
            for (int i = 0; i < diffusionSamples; ++i)
                scratchKernel[ch][i] *= makeupGain;

            engines[ch].setIR(scratchKernel[ch], maxKernelSamples);
        }
    }

    void processBlock(const float* inL, const float* inR, float* outL, float* outR, int numSamples) noexcept
    {
        engines[0].processBlock(inL, outL, numSamples);
        if (numChannels > 1)
            engines[1].processBlock(inR, outR, numSamples);
    }

private:
    void generateKernels() noexcept
    {
        const int rampSamples = std::max(1, std::min(maxKernelSamples, static_cast<int>(std::round(sampleRate * 0.05f))));

        for (int ch = 0; ch < numChannels; ++ch) {
            XorShift32 rng(0xC0FFEE11u + 0x9E3779B9u * static_cast<uint32_t>(ch + 1));
            for (int i = 0; i < maxKernelSamples; ++i) {
                const float ramp = i < rampSamples ? static_cast<float>(i + 1) / static_cast<float>(rampSamples) : 1.0f;
                baseKernel[ch][i] = rng.nextSigned() * ramp;
            }
        }
    }

    std::array<PartitionedConvolution, maxChannels> engines {};
    std::array<float*, maxChannels> baseKernel { nullptr, nullptr };
    std::array<float*, maxChannels> scratchKernel { nullptr, nullptr };
    float sampleRate = 48000.0f;
    float maxDiffusionSeconds = 0.1f;
    int maxKernelSamples = 0;
    int numChannels = 0;
    int currentDiffusionSamples = -1;
};

class SimpleReverbFDNCore
{
public:
    static constexpr int numChannels = 8;

    bool allocate(World* world, float newSampleRate, float maxDelayMs = 625.0f) noexcept
    {
        free(world);

        sampleRate = newSampleRate;
        fsOver1000 = sampleRate / 1000.0f;

        for (int i = 0; i < numChannels; ++i)
            delayRelativeMults[i] = delayMultiplier(i, numChannels, 0x13579BDFu);

        for (int i = 0; i < numChannels; ++i) {
            const int maxDelaySamples = static_cast<int>(std::ceil(maxDelayMs * delayRelativeMults[i] * fsOver1000)) + 4;
            if (!delays[i].allocate(world, maxDelaySamples)) {
                free(world);
                return false;
            }
        }

        reset();
        return true;
    }

    void free(World* world) noexcept
    {
        for (auto& delay : delays)
            delay.free(world);
    }

    void reset() noexcept
    {
        for (auto& delay : delays)
            delay.reset();
        for (auto& shelf : shelves)
            shelf.reset();

        delayTimesSamples.fill(1.0f);
        outData.fill(0.0f);
        fbData.fill(0.0f);
    }

    void setDelayTimeMsWithModulators(float delayTimeMs, const float* modulators, int numModulators) noexcept
    {
        for (int i = 0; i < numChannels; ++i) {
            const float mod = i < numModulators ? modulators[i] : 1.0f;
            delayTimesSamples[i] = delayRelativeMults[i] * delayTimeMs * fsOver1000 * mod;
            delays[i].setDelay(delayTimesSamples[i]);
        }
    }

    void setDecayTimeMs(float decayTimeLowMs, float decayTimeHighMs, float crossoverFreqHz) noexcept
    {
        for (int i = 0; i < numChannels; ++i) {
            const float channelDelayMs = delayTimesSamples[i] / fsOver1000;
            const float lowGain = calcGainForT60(decayTimeLowMs, channelDelayMs);
            const float highGain = calcGainForT60(decayTimeHighMs, channelDelayMs);
            shelves[i].calcCoefs(lowGain, highGain, crossoverFreqHz, sampleRate);
        }
    }

    void processStereo(float& xL, float& xR) noexcept
    {
        float inData[numChannels] {};
        for (int i = 0; i < numChannels; i += 2) {
            inData[i] = xL;
            inData[i + 1] = xR;
        }

        for (int i = 0; i < numChannels; ++i) {
            outData[i] = delays[i].popSample();
            delays[i].updateReadPointer();
        }

        householderInPlace(outData.data(), numChannels);

        for (int i = 0; i < numChannels; ++i)
            fbData[i] = shelves[i].process(outData[i]);

        for (int i = 0; i < numChannels; ++i)
            delays[i].pushSample(inData[i] + fbData[i]);

        float sumL = 0.0f;
        float sumR = 0.0f;
        for (int i = 0; i < numChannels; i += 2) {
            sumL += outData[i];
            sumR += outData[i + 1];
        }

        xL = sumL * 0.125f;
        xR = sumR * 0.125f;
    }

private:
    static float calcGainForT60(float decayTimeMs, float delayTimeMs) noexcept
    {
        const float safeDecay = std::max(decayTimeMs, 1.0e-6f);
        const float safeDelay = std::max(delayTimeMs, 1.0e-6f);
        return std::pow(0.001f, safeDelay / safeDecay);
    }

    std::array<IntegerDelayLine, numChannels> delays {};
    std::array<float, numChannels> delayRelativeMults {};
    std::array<float, numChannels> delayTimesSamples {};
    std::array<ShelfFilter, numChannels> shelves {};
    std::array<float, numChannels> outData {};
    std::array<float, numChannels> fbData {};
    float sampleRate = 48000.0f;
    float fsOver1000 = 48.0f;
};
} // namespace chow_reverb
