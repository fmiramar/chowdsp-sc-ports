#pragma once

#include "filters.hpp"

#include <algorithm>
#include <array>

namespace centaurch
{
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
        osBuffer[0] = static_cast<float>(Ratio) * x;
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
} // namespace centaurch
