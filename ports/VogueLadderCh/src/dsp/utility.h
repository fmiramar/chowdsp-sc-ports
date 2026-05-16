#pragma once

#include <cmath>

namespace vogue_ladder_utility
{
constexpr double PI = 3.14159265358979323846264338327950288419716939937510;
constexpr double TWO_PI = 2.0 * PI;
constexpr double MIN_FILTER_FREQ = 20.0;
constexpr double MAX_FILTER_FREQ = 20480.0;
constexpr double ZERO_VOLT_FREQ = MIN_FILTER_FREQ * 32.0;

template <typename T = double>
inline T mapLinearNormalized(T val, T outMin, T outMax)
{
    return val * (outMax - outMin) + outMin;
}

inline double prewarp(double omegaDigital, double sampleRate)
{
    return 2.0 * sampleRate * std::tan(omegaDigital / (2.0 * sampleRate));
}

inline double decibelToRawGain(double decibel)
{
    return std::pow(10.0, decibel / 20.0);
}

inline double skewNormalized(double normalizedVal, double skew)
{
    return std::pow(normalizedVal, 1.0 / skew);
}

inline double limitUpper(double valueToLimit, double limit)
{
    return valueToLimit >= limit ? limit : valueToLimit;
}

inline double voltToFreq(double volt)
{
    return ZERO_VOLT_FREQ * std::pow(2.0, volt);
}

template <typename T>
inline T fastTanh2(T x)
{
    const auto ax = std::abs(x);
    const auto x2 = x * x;

    return x * ((T) 2.45550750702956 + (T) 2.45550750702956 * ax + ((T) 0.893229853513558 + (T) 0.821226666969744 * ax) * x2)
        / ((T) 2.44506634652299 + ((T) 2.44506634652299 + x2) * std::abs(x + (T) 0.814642734961073 * x * ax));
}
} // namespace vogue_ladder_utility
