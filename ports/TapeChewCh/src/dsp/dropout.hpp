#pragma once

#include "smoothed_value.hpp"

#include <cmath>

namespace tapechew
{
class Dropout
{
public:
    void prepare(double sampleRate) noexcept
    {
        mixSmooth.reset(sampleRate, 0.01);
        mixSmooth.setCurrentAndTargetValue(mixSmooth.getTargetValue());

        powerSmooth.reset(sampleRate, 0.005);
        powerSmooth.setCurrentAndTargetValue(powerSmooth.getTargetValue());
    }

    void setMix(float newMix) noexcept
    {
        mixSmooth.setTargetValue(newMix);
    }

    void setPower(float newPower) noexcept
    {
        powerSmooth.setTargetValue(newPower);
    }

    float processSample(float x) noexcept
    {
        const auto mix = mixSmooth.getNextValue();
        if (mix == 0.0f)
            return x;

        return mix * dropout(x) + (1.0f - mix) * x;
    }

private:
    static int signum(float value) noexcept
    {
        return (0.0f < value) - (value < 0.0f);
    }

    float dropout(float x) noexcept
    {
        const auto sign = static_cast<float>(signum(x));
        return std::pow(std::abs(x), powerSmooth.getNextValue()) * sign;
    }

    SmoothedValue<float, ValueSmoothingTypes::Linear> mixSmooth;
    SmoothedValue<float, ValueSmoothingTypes::Linear> powerSmooth;
};
} // namespace tapechew
