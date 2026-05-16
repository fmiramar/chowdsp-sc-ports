#pragma once

#include "denormal.hpp"

#include <cmath>

namespace rnnch
{

struct BiquadFilter
{
    enum Type
    {
        LOWPASS,
        HIGHPASS
    };

    void reset() noexcept
    {
        z1 = 0.0f;
        z2 = 0.0f;
    }

    void setParameters(Type type, float f, float q) noexcept
    {
        constexpr float pi = 3.14159265358979323846f;
        const auto K = std::tan(pi * f);
        const auto norm = 1.0f / (1.0f + K / q + K * K);

        if (type == LOWPASS) {
            b0 = K * K * norm;
            b1 = 2.0f * b0;
            b2 = b0;
        } else {
            b0 = norm;
            b1 = -2.0f * b0;
            b2 = b0;
        }

        a1 = 2.0f * (K * K - 1.0f) * norm;
        a2 = (1.0f - K / q + K * K) * norm;
    }

    float process(float x) noexcept
    {
        const auto y = z1 + x * b0;
        z1 = flushSubnormal(z2 + x * b1 - y * a1);
        z2 = flushSubnormal(x * b2 - y * a2);
        return flushSubnormal(y);
    }

    float b0 = 0.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float z1 = 0.0f;
    float z2 = 0.0f;
};

} // namespace rnnch
