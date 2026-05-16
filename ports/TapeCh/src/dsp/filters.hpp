#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace tapech
{
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
        const auto f = std::clamp(normalizedFrequency, 1.0e-6f, 0.499f);
        const auto clampedQ = std::max(q, 1.0e-4f);
        const auto k = std::tan(3.14159265358979323846f * f);
        const auto norm = 1.0f / (1.0f + k / clampedQ + k * k);

        b[0] = k * k * norm;
        b[1] = 2.0f * b[0];
        b[2] = b[0];
        a[1] = 2.0f * (k * k - 1.0f) * norm;
        a[2] = (1.0f - k / clampedQ + k * k) * norm;
    }

    float process(float x) noexcept
    {
        const auto y = z[1] + x * b[0];
        z[1] = z[2] + x * b[1] - y * a[1];
        z[2] = x * b[2] - y * a[2];
        return y;
    }
};

template <int N>
class AAFilter
{
public:
    void reset(float sampleRate, int osRatio)
    {
        const auto fc = 0.98f * (sampleRate / 2.0f);
        const auto qs = calculateButterQs(2 * N);

        for (int i = 0; i < N; ++i) {
            filters[(size_t) i].reset();
            filters[(size_t) i].setLowpass(fc / (osRatio * sampleRate), qs[(size_t) i]);
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
        qs.reserve((size_t) (order / 2));

        for (int k = 1; k <= order / 2; ++k) {
            const auto b = -2.0f * std::cos((2.0f * k + order - 1) * 3.14159265358979323846f / (2.0f * order));
            qs.push_back(1.0f / b);
        }

        std::reverse(qs.begin(), qs.end());
        return qs;
    }

    std::array<BiquadFilter, N> filters {};
};
} // namespace tapech
