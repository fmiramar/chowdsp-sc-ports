#pragma once

#include <cmath>
#include <limits>

namespace dirtytubech
{

inline float flushSubnormal(float value) noexcept
{
    const auto absValue = std::abs(value);
    return (absValue > 0.0f && absValue < std::numeric_limits<float>::min()) ? 0.0f : value;
}

inline double flushSubnormal(double value) noexcept
{
    const auto absValue = std::abs(value);
    return (absValue > 0.0 && absValue < std::numeric_limits<double>::min()) ? 0.0 : value;
}

} // namespace dirtytubech
