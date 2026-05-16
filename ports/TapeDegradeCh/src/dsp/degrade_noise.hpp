#pragma once

#include "random.hpp"
#include "smoothed_value.hpp"

namespace tapedegrade
{
class DegradeNoise
{
public:
    explicit DegradeNoise(std::uint32_t seed = 0x3b92dc14u) noexcept
        : rng(seed)
    {
    }

    void setGain(float newGain) noexcept
    {
        gain.setTargetValue(newGain);
    }

    void prepare(float sampleRate) noexcept
    {
        gain.reset(sampleRate, 0.05);
    }

    float processSample(float x) noexcept
    {
        return x + (rng.uniform() - 0.5f) * gain.getNextValue();
    }

private:
    Random rng;
    SmoothedValue<float, ValueSmoothingTypes::Linear> gain;
};
} // namespace tapedegrade
