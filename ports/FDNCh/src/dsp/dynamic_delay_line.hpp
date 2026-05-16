#pragma once

#include "SC_PlugIn.h"

#include <algorithm>
#include <cmath>

extern InterfaceTable* ft;

class DynamicDelayLine
{
public:
    bool allocate(World* world, int maxDelaySamples) noexcept
    {
        free(world);

        size = std::max(maxDelaySamples + 8, 8);
        buffer = static_cast<float*>(RTAlloc(world, (size_t) size * sizeof(float)));
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
        delay = 0.0f;
        delayFrac = 0.0f;
        delayInt = 0;
    }

    void reset() noexcept
    {
        writePos = 0;
        readPos = 0;
        delay = 0.0f;
        delayFrac = 0.0f;
        delayInt = 0;

        if (buffer != nullptr) {
            for (int i = 0; i < size; ++i)
                buffer[i] = 0.0f;
        }
    }

    void setDelay(float newDelayInSamples) noexcept
    {
        if (buffer == nullptr || size == 0)
            return;

        const float upperLimit = static_cast<float>(size - 1);
        delay = sc_clip(newDelayInSamples, 0.0f, upperLimit);
        delayInt = static_cast<int>(std::floor(delay));
        delayFrac = delay - static_cast<float>(delayInt);

        if (delayInt >= 1) {
            delayFrac += 1.0f;
            --delayInt;
        }
    }

    float popSample() const noexcept
    {
        if (buffer == nullptr || size == 0)
            return 0.0f;

        int index1 = readPos + delayInt;
        int index2 = index1 + 1;
        int index3 = index2 + 1;
        int index4 = index3 + 1;

        if (index4 >= size) {
            index1 %= size;
            index2 %= size;
            index3 %= size;
            index4 %= size;
        }

        const float value1 = buffer[index1];
        const float value2 = buffer[index2];
        const float value3 = buffer[index3];
        const float value4 = buffer[index4];

        const float d1 = delayFrac - 1.0f;
        const float d2 = delayFrac - 2.0f;
        const float d3 = delayFrac - 3.0f;

        const float c1 = -d1 * d2 * d3 / 6.0f;
        const float c2 = d2 * d3 * 0.5f;
        const float c3 = -d1 * d3 * 0.5f;
        const float c4 = d1 * d2 / 6.0f;

        return value1 * c1 + delayFrac * (value2 * c2 + value3 * c3 + value4 * c4);
    }

    void pushSample(float sample) noexcept
    {
        if (buffer == nullptr || size == 0)
            return;

        buffer[writePos] = sample;
        writePos = (writePos + size - 1) % size;
    }

    void updateReadPointer() noexcept
    {
        if (size == 0)
            return;

        readPos = (readPos + size - 1) % size;
    }

    float process(float x) noexcept
    {
        pushSample(x);
        const float y = popSample();
        updateReadPointer();
        return y;
    }

private:
    float* buffer = nullptr;
    int size = 0;
    int writePos = 0;
    int readPos = 0;
    float delay = 0.0f;
    float delayFrac = 0.0f;
    int delayInt = 0;
};
