#pragma once

#include "SC_PlugIn.h"

#include "delay_utils.hpp"
#include "dynamic_delay_line.hpp"
#include "shelf_filter.hpp"

#include <array>
#include <cmath>
#include <numeric>

class FDNCore
{
public:
    static constexpr int maxDelays = DelayUtils::maxDelays;

    bool allocate(World* world, float sampleRate) noexcept
    {
        free(world);

        delayLensMs = DelayUtils::generateDelayLengths();
        fillMixMatrix();

        for (int i = 0; i < maxDelays; ++i) {
            const int maxSamples = static_cast<int>(std::ceil(delayLensMs[(size_t) i] * sampleRate / 1000.0f));
            if (!delayLines[(size_t) i].allocate(world, maxSamples)) {
                free(world);
                return false;
            }
        }

        reset();
        return true;
    }

    void free(World* world) noexcept
    {
        for (auto& delayLine : delayLines)
            delayLine.free(world);
    }

    void reset() noexcept
    {
        for (auto& delayLine : delayLines)
            delayLine.reset();

        for (auto& shelf : shelves)
            shelf.reset();

        delayReads.fill(0.0f);
        curDelaySamples.fill(1.0f);
        gLow.fill(0.0f);
        gHigh.fill(0.0f);

        oldSize = -1.0f;
        oldT60Low = -1.0f;
        oldT60High = -1.0f;
        oldCurDelays = -1;
    }

    void prepare(float sampleRate, float size, float t60Low, float t60High, int curDelays) noexcept
    {
        const bool needsSizeUpdate = (size != oldSize) || (curDelays != oldCurDelays);
        const bool needsShelfUpdate = (t60Low != oldT60Low) || (t60High != oldT60High) || needsSizeUpdate;

        if (!needsShelfUpdate)
            return;

        if (needsSizeUpdate) {
            for (int dInd = 0; dInd < curDelays; ++dInd) {
                const float currentDelayLenMs = static_cast<float>(delayLensMs[(size_t) dInd]) * size;
                curDelaySamples[(size_t) dInd] = (currentDelayLenMs / 1000.0f) * sampleRate;
                delayLines[(size_t) dInd].setDelay(curDelaySamples[(size_t) dInd]);
            }

            oldSize = size;
        }

        if ((t60Low != oldT60Low) || needsSizeUpdate) {
            for (int dInd = 0; dInd < curDelays; ++dInd)
                gLow[(size_t) dInd] = DelayUtils::calcGainForT60(static_cast<int>(curDelaySamples[(size_t) dInd]), sampleRate, t60Low);

            oldT60Low = t60Low;
        }

        if ((t60High != oldT60High) || needsSizeUpdate) {
            for (int dInd = 0; dInd < curDelays; ++dInd)
                gHigh[(size_t) dInd] = DelayUtils::calcGainForT60(static_cast<int>(curDelaySamples[(size_t) dInd]), sampleRate, t60High);

            oldT60High = t60High;
        }

        for (int dInd = 0; dInd < curDelays; ++dInd)
            shelves[(size_t) dInd].calcCoefs(sc_clip(gLow[(size_t) dInd], -1.0f, 1.0f), sc_clip(gHigh[(size_t) dInd], -1.0f, 1.0f), 800.0f, sampleRate);

        oldCurDelays = curDelays;
    }

    float process(float x, int curDelays) noexcept
    {
        float y = 0.0f;

        for (int sumInd = 0; sumInd < curDelays; ++sumInd)
            delayReads[(size_t) sumInd] = delayLines[(size_t) sumInd].popSample();

        for (int dInd = 0; dInd < curDelays; ++dInd) {
            float accum = 0.0f;
            for (int sumInd = 0; sumInd < curDelays; ++sumInd)
                accum += matrix[(size_t) dInd][(size_t) sumInd] * delayReads[(size_t) sumInd];

            y += accum;
            accum += x;
            accum = shelves[(size_t) dInd].process(accum);
            delayLines[(size_t) dInd].pushSample(accum);
            delayLines[(size_t) dInd].updateReadPointer();
        }

        return y;
    }

private:
    static int parity(unsigned value) noexcept
    {
        int result = 0;
        while (value != 0u) {
            result ^= static_cast<int>(value & 1u);
            value >>= 1u;
        }
        return result;
    }

    void fillMixMatrix() noexcept
    {
        constexpr float scale = 0.25f;

        for (int row = 0; row < maxDelays; ++row) {
            for (int col = 0; col < maxDelays; ++col) {
                const bool negative = parity(static_cast<unsigned>(row & col)) != 0;
                matrix[(size_t) row][(size_t) col] = negative ? -scale : scale;
            }
        }
    }

    std::array<DynamicDelayLine, maxDelays> delayLines {};
    std::array<int, maxDelays> delayLensMs {};
    std::array<float, maxDelays> curDelaySamples {};
    std::array<ShelfFilter, maxDelays> shelves {};
    std::array<float, maxDelays> gLow {};
    std::array<float, maxDelays> gHigh {};
    std::array<std::array<float, maxDelays>, maxDelays> matrix {};
    std::array<float, maxDelays> delayReads {};

    float oldSize = -1.0f;
    float oldT60Low = -1.0f;
    float oldT60High = -1.0f;
    int oldCurDelays = -1;
};
