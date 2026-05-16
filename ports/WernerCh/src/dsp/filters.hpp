#pragma once

#include <algorithm>
#include <cmath>

struct BiquadFilter
{
    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float z1 = 0.0f;
    float z2 = 0.0f;

    void reset() noexcept
    {
        z1 = 0.0f;
        z2 = 0.0f;
    }

    void setLowpass(float normalizedFrequency, float q) noexcept
    {
        const float f = std::clamp(normalizedFrequency, 1.0e-6f, 0.499f);
        const float k = std::tan(static_cast<float>(M_PI) * f);
        const float norm = 1.0f / (1.0f + k / q + k * k);

        b0 = k * k * norm;
        b1 = 2.0f * b0;
        b2 = b0;
        a1 = 2.0f * (k * k - 1.0f) * norm;
        a2 = (1.0f - k / q + k * k) * norm;
    }

    float process(float x) noexcept
    {
        const float y = z1 + x * b0;
        z1 = z2 + x * b1 - y * a1;
        z2 = x * b2 - y * a2;
        return y;
    }
};
