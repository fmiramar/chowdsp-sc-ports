#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace fracfilter
{
constexpr int numStages = 4;
constexpr float pi = 3.14159265358979323846f;

inline float clampFreq(float freq, float sampleRate) noexcept
{
    return std::clamp(freq, 20.0f, sampleRate * 0.499f);
}

inline float clampAlpha(float alpha) noexcept
{
    return std::clamp(alpha, 0.0f, 1.0f);
}

inline float flushSubnormal(float value) noexcept
{
    return std::abs(value) < 1.0e-30f ? 0.0f : value;
}

struct FirstOrderSection
{
    void reset() noexcept
    {
        z1 = 0.0f;
    }

    void setCoefs(float newB0, float newB1, float newA1) noexcept
    {
        b0 = newB0;
        b1 = newB1;
        a1 = newA1;
    }

    float process(float x) noexcept
    {
        const float y = z1 + (x * b0);
        z1 = flushSubnormal((x * b1) - (y * a1));
        return y;
    }

    float b0 = 1.0f;
    float b1 = 0.0f;
    float a1 = 0.0f;
    float z1 = 0.0f;
};

class FractionalOrderLowpass
{
public:
    void prepare(float newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        fRef = newSampleRate * 0.5f;
        reset();
        updateCoefs(currentFreq, currentAlpha);
    }

    void reset() noexcept
    {
        for (auto& section : sections)
            section.reset();
    }

    void updateCoefs(float freq, float alpha) noexcept
    {
        currentFreq = clampFreq(freq, sampleRate);
        currentAlpha = clampAlpha(alpha);

        const float wc = 2.0f * pi * currentFreq;
        const float tanArg = pi * currentFreq / sampleRate;
        const float tanValue = std::tan(tanArg);
        const float k = std::abs(tanValue) > 1.0e-12f ? wc / tanValue : 2.0f * sampleRate;
        const float freqRatio = fRef / currentFreq;
        const float denominator = static_cast<float>((2 * numStages) + 1) - currentAlpha;

        for (int stage = 0; stage < numStages; ++stage) {
            const float stageIndex = static_cast<float>((2 * stage) + 1);
            const float poleExponent = (stageIndex - currentAlpha) / denominator;
            const float zeroExponent = (stageIndex + currentAlpha) / denominator;

            const float pole = -wc * std::pow(freqRatio, poleExponent);
            const float zero = -wc * std::pow(freqRatio, zeroExponent);

            const float as0 = 1.0f / pole;
            const float as1 = -1.0f;
            const float bs0 = 1.0f / zero;
            const float bs1 = -1.0f;

            const float a0Inv = 1.0f / ((as0 * k) + as1);
            const float b0 = ((bs0 * k) + bs1) * a0Inv;
            const float b1 = ((-bs0 * k) + bs1) * a0Inv;
            const float a1 = ((-as0 * k) + as1) * a0Inv;

            sections[stage].setCoefs(b0, b1, a1);
        }
    }

    float process(float x) noexcept
    {
        for (auto& section : sections)
            x = section.process(x);
        return x;
    }

    float getCurrentFreq() const noexcept { return currentFreq; }
    float getCurrentAlpha() const noexcept { return currentAlpha; }

private:
    std::array<FirstOrderSection, numStages> sections {};
    float sampleRate = 48000.0f;
    float fRef = 24000.0f;
    float currentFreq = 1000.0f;
    float currentAlpha = 0.5f;
};
} // namespace fracfilter
