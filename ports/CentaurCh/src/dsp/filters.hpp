#pragma once

#include "denormal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace centaurch
{
constexpr float pi = 3.14159265358979323846f;

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
        const auto y = z1 + x * b0;
        z1 = flushSubnormal(x * b1 - y * a1);
        return flushSubnormal(y);
    }
};

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
        const float K = std::tan(pi * f);
        const float norm = 1.0f / (1.0f + K / q + K * K);

        b0 = K * K * norm;
        b1 = 2.0f * b0;
        b2 = b0;
        a1 = 2.0f * (K * K - 1.0f) * norm;
        a2 = (1.0f - K / q + K * K) * norm;
    }

    void setHighpass(float normalizedFrequency, float q) noexcept
    {
        const float f = std::clamp(normalizedFrequency, 1.0e-6f, 0.499f);
        const float K = std::tan(pi * f);
        const float norm = 1.0f / (1.0f + K / q + K * K);

        b0 = norm;
        b1 = -2.0f * b0;
        b2 = b0;
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
};

inline void bilinearTransformOrder1(OnePoleFilter& filter,
                                    float b0s,
                                    float b1s,
                                    float a0s,
                                    float a1s,
                                    float K) noexcept
{
    const auto a0 = a0s * K + a1s;
    filter.b0 = (b0s * K + b1s) / a0;
    filter.b1 = (-b0s * K + b1s) / a0;
    filter.a1 = (-a0s * K + a1s) / a0;
}

inline void bilinearTransformOrder2(BiquadFilter& filter,
                                    float b0s,
                                    float b1s,
                                    float b2s,
                                    float a0s,
                                    float a1s,
                                    float a2s,
                                    float K) noexcept
{
    const auto KSq = K * K;
    const auto a0 = a0s * KSq + a1s * K + a2s;
    filter.b0 = (b0s * KSq + b1s * K + b2s) / a0;
    filter.b1 = 2.0f * (b2s - b0s * KSq) / a0;
    filter.b2 = (b0s * KSq - b1s * K + b2s) / a0;
    filter.a1 = 2.0f * (a2s - a0s * KSq) / a0;
    filter.a2 = (a0s * KSq - a1s * K + a2s) / a0;
}

inline float calcPoleFreq(float a0s, float a1s, float a2s) noexcept
{
    const auto radicand = a1s * a1s - 4.0f * a0s * a2s;
    if (radicand >= 0.0f)
        return 0.0f;
    return std::sqrt(-radicand) / (2.0f * a0s);
}

template <int N>
class AAFilter
{
public:
    void reset(float sampleRate, int osRatio)
    {
        const float fc = 0.98f * (sampleRate / 2.0f);
        const auto qs = calculateButterQs(2 * N);

        for (int i = 0; i < N; ++i) {
            filters[i].reset();
            filters[i].setLowpass(fc / (osRatio * sampleRate), qs[static_cast<size_t>(i)]);
        }
    }

    float process(float x) noexcept
    {
        for (auto& filter : filters)
            x = filter.process(x);
        return x;
    }

private:
    static std::vector<float> calculateButterQs(int order)
    {
        std::vector<float> qs;
        qs.reserve(static_cast<size_t>(order / 2));

        for (int k = 1; k <= order / 2; ++k) {
            const float b = -2.0f * std::cos((2.0f * k + order - 1) * pi / (2.0f * order));
            qs.push_back(1.0f / b);
        }

        std::reverse(qs.begin(), qs.end());
        return qs;
    }

    std::array<BiquadFilter, N> filters {};
};

} // namespace centaurch
