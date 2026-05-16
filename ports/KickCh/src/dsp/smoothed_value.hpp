#pragma once

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace kickch
{
namespace ValueSmoothingTypes
{
struct Linear
{
};

struct Multiplicative
{
};
} // namespace ValueSmoothingTypes

template <typename FloatType, typename SmoothingType = ValueSmoothingTypes::Linear>
class SmoothedValue
{
public:
    SmoothedValue() noexcept
    {
        if constexpr (std::is_same_v<SmoothingType, ValueSmoothingTypes::Linear>)
            currentValue = target = static_cast<FloatType>(0);
        else
            currentValue = target = static_cast<FloatType>(1);
    }

    explicit SmoothedValue(FloatType initialValue) noexcept : currentValue(initialValue), target(initialValue) {}

    void reset(double sampleRate, double rampLengthInSeconds) noexcept
    {
        reset(static_cast<int>(std::floor(rampLengthInSeconds * sampleRate)));
    }

    void reset(int numSteps) noexcept
    {
        stepsToTarget = numSteps;
        setCurrentAndTargetValue(target);
    }

    bool isSmoothing() const noexcept
    {
        return countdown > 0;
    }

    FloatType getCurrentValue() const noexcept
    {
        return currentValue;
    }

    FloatType getTargetValue() const noexcept
    {
        return target;
    }

    void setCurrentAndTargetValue(FloatType newValue) noexcept
    {
        currentValue = target = newValue;
        countdown = 0;
    }

    void setTargetValue(FloatType newValue) noexcept
    {
        if (newValue == target)
            return;

        if (stepsToTarget <= 0) {
            setCurrentAndTargetValue(newValue);
            return;
        }

        target = newValue;
        countdown = stepsToTarget;
        setStepSize();
    }

    FloatType getNextValue() noexcept
    {
        if (!isSmoothing())
            return target;

        --countdown;
        if (isSmoothing())
            setNextValue();
        else
            currentValue = target;

        return currentValue;
    }

private:
    void setStepSize() noexcept
    {
        if constexpr (std::is_same_v<SmoothingType, ValueSmoothingTypes::Linear>) {
            step = (target - currentValue) / static_cast<FloatType>(countdown);
        } else {
            constexpr auto tiny = static_cast<FloatType>(1.0e-12);
            const auto start = std::max(std::abs(currentValue), tiny);
            const auto end = std::max(std::abs(target), tiny);
            step = std::exp((std::log(end) - std::log(start)) / static_cast<FloatType>(countdown));
        }
    }

    void setNextValue() noexcept
    {
        if constexpr (std::is_same_v<SmoothingType, ValueSmoothingTypes::Linear>)
            currentValue += step;
        else
            currentValue *= step;
    }

    FloatType currentValue {};
    FloatType target {};
    FloatType step {};
    int countdown = 0;
    int stepsToTarget = 0;
};
} // namespace kickch
