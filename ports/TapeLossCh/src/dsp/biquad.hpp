#pragma once

#include <algorithm>
#include <cmath>

namespace tapeloss
{
struct BiquadFilter
{
    enum Type
    {
        PEAK
    };

    void reset() noexcept
    {
        z1 = 0.0f;
        z2 = 0.0f;
    }

    void setParameters(Type, float normalizedFrequency, float q, float gain) noexcept
    {
        const auto f = std::clamp(normalizedFrequency, 1.0e-6f, 0.499f);
        const auto K = std::tan(static_cast<float>(M_PI) * f);
        const auto c = 1.0f / K;
        const auto phi = c * c;

        auto kNum = c / q;
        auto kDenom = kNum;
        if (gain > 1.0f)
            kNum *= gain;
        else
            kDenom /= std::max(gain, 1.0e-6f);

        const auto norm = phi + kDenom + 1.0f;
        b0 = (phi + kNum + 1.0f) / norm;
        b1 = 2.0f * (1.0f - phi) / norm;
        b2 = (phi - kNum + 1.0f) / norm;
        a1 = 2.0f * (1.0f - phi) / norm;
        a2 = (phi - kDenom + 1.0f) / norm;
    }

    float process(float x) noexcept
    {
        const auto y = z1 + x * b0;
        z1 = z2 + x * b1 - y * a1;
        z2 = x * b2 - y * a2;
        return y;
    }

    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float z1 = 0.0f;
    float z2 = 0.0f;
};
} // namespace tapeloss
