#pragma once

#include "denormal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace fuzzmachinech
{

constexpr float pi = 3.14159265358979323846f;

struct BiquadFilter
{
    void reset() noexcept
    {
        z1 = 0.0f;
        z2 = 0.0f;
    }

    void setLowpass(float normalizedFrequency, float q) noexcept
    {
        const auto f = std::clamp(normalizedFrequency, 1.0e-6f, 0.499f);
        const auto k = std::tan(pi * f);
        const auto norm = 1.0f / (1.0f + k / q + k * k);

        b0 = k * k * norm;
        b1 = 2.0f * b0;
        b2 = b0;
        a1 = 2.0f * (k * k - 1.0f) * norm;
        a2 = (1.0f - k / q + k * k) * norm;
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

template <int Ratio, int FilterOrder = 4>
class Oversampling
{
public:
    void reset(float baseSampleRate)
    {
        aaFilter.reset(baseSampleRate, Ratio);
        aiFilter.reset(baseSampleRate, Ratio);
        osBuffer.fill(0.0f);
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

} // namespace fuzzmachinech
