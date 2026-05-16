#pragma once

#include "denormal.hpp"

#include <algorithm>
#include <cmath>

namespace mousedrivech
{

constexpr float pi = 3.14159265358979323846f;

struct BiquadFilter
{
    void reset() noexcept
    {
        z1 = 0.0f;
        z2 = 0.0f;
    }

    void setHighpass(float normalizedFrequency, float q) noexcept
    {
        const auto f = std::clamp(normalizedFrequency, 1.0e-6f, 0.499f);
        const auto k = std::tan(pi * f);
        const auto norm = 1.0f / (1.0f + k / q + k * k);

        b0 = norm;
        b1 = -2.0f * b0;
        b2 = b0;
        a1 = 2.0f * (k * k - 1.0f) * norm;
        a2 = (1.0f - k / q + k * k) * norm;
    }

    float process(float x) noexcept
    {
        const auto y = z1 + x * b0;
        z1 = flushSubnormal(z2 + x * b1 - y * a1);
        z2 = flushSubnormal(x * b2 - y * a2);
        return flushSubnormal(y);
    }

    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float z1 = 0.0f;
    float z2 = 0.0f;
};

struct LinearSmoother
{
    void reset(float value) noexcept
    {
        current = value;
        target = value;
        step = 0.0f;
        samplesRemaining = 0;
    }

    void setRampTime(float sampleRate, float seconds) noexcept
    {
        rampSamples = std::max(1, static_cast<int>(std::ceil(sampleRate * seconds)));
    }

    void setTarget(float value) noexcept
    {
        target = value;
        samplesRemaining = rampSamples;
        step = (target - current) / static_cast<float>(std::max(samplesRemaining, 1));
    }

    float process() noexcept
    {
        if (samplesRemaining > 0) {
            current += step;
            --samplesRemaining;
            if (samplesRemaining == 0)
                current = target;
        }

        return current;
    }

    float current = 0.0f;
    float target = 0.0f;
    float step = 0.0f;
    int samplesRemaining = 0;
    int rampSamples = 1;
};

} // namespace mousedrivech
