#pragma once

#include <algorithm>
#include <cmath>

struct BiquadFilter
{
    float b[3] { 1.0f, 0.0f, 0.0f };
    float a[3] { 1.0f, 0.0f, 0.0f };
    float z[3] { 0.0f, 0.0f, 0.0f };

    void reset() noexcept
    {
        z[1] = 0.0f;
        z[2] = 0.0f;
    }

    void setLowpass(float normalizedFrequency, float q) noexcept
    {
        const float f = std::clamp(normalizedFrequency, 1.0e-6f, 0.499f);
        const float clampedQ = std::max(q, 1.0e-4f);
        const float k = std::tan(static_cast<float>(M_PI) * f);
        const float norm = 1.0f / (1.0f + k / clampedQ + k * k);

        b[0] = k * k * norm;
        b[1] = 2.0f * b[0];
        b[2] = b[0];
        a[1] = 2.0f * (k * k - 1.0f) * norm;
        a[2] = (1.0f - k / clampedQ + k * k) * norm;
    }

    void setPeak(float normalizedFrequency, float q, float gain) noexcept
    {
        const float f = std::clamp(normalizedFrequency, 1.0e-6f, 0.499f);
        const float clampedQ = std::max(q, 1.0e-4f);
        const float clampedGain = std::max(gain, 1.0e-6f);
        const float k = std::tan(static_cast<float>(M_PI) * f);
        const float c = 1.0f / k;
        const float phi = c * c;

        float kNum = c / clampedQ;
        float kDenom = kNum;

        if (clampedGain > 1.0f)
            kNum *= clampedGain;
        else
            kDenom /= clampedGain;

        const float norm = phi + kDenom + 1.0f;

        b[0] = (phi + kNum + 1.0f) / norm;
        b[1] = 2.0f * (1.0f - phi) / norm;
        b[2] = (phi - kNum + 1.0f) / norm;
        a[1] = 2.0f * (1.0f - phi) / norm;
        a[2] = (phi - kDenom + 1.0f) / norm;
    }

    float process(float x) noexcept
    {
        const float y = z[1] + x * b[0];
        z[1] = z[2] + x * b[1] - y * a[1];
        z[2] = x * b[2] - y * a[2];
        return y;
    }
};
