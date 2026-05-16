#pragma once

#include <cmath>

struct OnePoleFilter
{
    float b0 = 0.0f;
    float b1 = 0.0f;
    float a1 = 0.0f;
    float z1 = 0.0f;

    void reset() noexcept
    {
        z1 = 0.0f;
    }

    float process(float x) noexcept
    {
        const float y = z1 + x * b0;
        z1 = x * b1 - y * a1;
        return y;
    }
};
