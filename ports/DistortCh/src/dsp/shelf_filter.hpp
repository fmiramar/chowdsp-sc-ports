#pragma once

#include "filters.hpp"

class ShelfFilter
{
public:
    void reset() noexcept
    {
        filter.reset();
    }

    void calcCoefs(float lowGain, float highGain, float fc, float sampleRate) noexcept
    {
        if (lowGain == highGain) {
            filter.b0 = lowGain;
            filter.b1 = 0.0f;
            filter.a1 = 0.0f;
            return;
        }

        const float wc = 2.0f * static_cast<float>(M_PI) * fc;
        const float p = std::sqrt(wc * wc * (highGain * highGain - lowGain * highGain) / (lowGain * highGain - lowGain * lowGain));
        const float K = p / std::tan(p / (2.0f * sampleRate));

        const float b0 = highGain / p;
        const float b1 = lowGain;
        const float a0 = 1.0f / p;
        const float a1 = 1.0f;

        const float a0z = a0 * K + a1;

        filter.b0 = (b0 * K + b1) / a0z;
        filter.b1 = (-b0 * K + b1) / a0z;
        filter.a1 = (-a0 * K + a1) / a0z;
    }

    float process(float x) noexcept
    {
        return filter.process(x);
    }

private:
    OnePoleFilter filter;
};
