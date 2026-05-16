#pragma once

#include "filters.hpp"

#include <algorithm>
#include <array>
#include <vector>

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
            filters[i].setLowpass(fc / (osRatio * sampleRate), qs[(size_t) i]);
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
            const float b = -2.0f * std::cos((2.0f * k + order - 1) * static_cast<float>(M_PI) / (2.0f * order));
            qs.push_back(1.0f / b);
        }

        std::reverse(qs.begin(), qs.end());
        return qs;
    }

    std::array<BiquadFilter, N> filters {};
};

template <int Ratio, int FilterOrder = 4>
class Oversampling
{
public:
    void reset(float baseSampleRate)
    {
        aaFilter.reset(baseSampleRate, Ratio);
        aiFilter.reset(baseSampleRate, Ratio);
        std::fill(osBuffer.begin(), osBuffer.end(), 0.0f);
    }

    void upsample(float x) noexcept
    {
        osBuffer[0] = (float) Ratio * x;
        std::fill(osBuffer.begin() + 1, osBuffer.end(), 0.0f);

        for (auto& sample : osBuffer)
            sample = aiFilter.process(sample);
    }

    float downsample() noexcept
    {
        float y = 0.0f;
        for (auto sample : osBuffer)
            y = aaFilter.process(sample);

        return y;
    }

    float* getOSBuffer() noexcept
    {
        return osBuffer.data();
    }

private:
    AAFilter<FilterOrder> aaFilter;
    AAFilter<FilterOrder> aiFilter;
    std::array<float, Ratio> osBuffer {};
};
