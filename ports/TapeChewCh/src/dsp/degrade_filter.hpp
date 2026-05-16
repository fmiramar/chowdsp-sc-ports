#pragma once

#include "smoothed_value.hpp"

#include <algorithm>
#include <cmath>

namespace tapechew
{
class DegradeFilter
{
public:
    void reset(float sampleRate, int steps = 0) noexcept
    {
        fs = sampleRate;
        z1 = 0.0f;

        if (steps > 0)
            freq.reset(steps);

        freq.setCurrentAndTargetValue(freq.getTargetValue());
        calcCoefs(freq.getCurrentValue());
    }

    void setFreq(float newFreq) noexcept
    {
        freq.setTargetValue(newFreq);
    }

    float processSample(float x) noexcept
    {
        if (freq.isSmoothing())
            calcCoefs(freq.getNextValue());

        const auto y = z1 + x * b0;
        z1 = x * b1 - y * a1;
        return y;
    }

private:
    void calcCoefs(float fc) noexcept
    {
        const auto safeFreq = std::clamp(fc, 10.0f, 0.49f * fs);
        const auto wc = 2.0f * static_cast<float>(M_PI) * safeFreq / fs;
        const auto c = 1.0f / std::tan(wc * 0.5f);
        const auto a0 = c + 1.0f;

        b0 = 1.0f / a0;
        b1 = b0;
        a1 = (1.0f - c) / a0;
    }

    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> freq { 20000.0f };
    float fs = 44100.0f;
    float b0 = 1.0f;
    float b1 = 1.0f;
    float a1 = 0.0f;
    float z1 = 0.0f;
};
} // namespace tapechew
