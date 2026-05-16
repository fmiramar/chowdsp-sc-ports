#pragma once

#include <cmath>
#include <limits>

namespace mousedrivech
{

inline float flushSubnormal(float value) noexcept
{
    const auto absValue = std::abs(value);
    return (absValue > 0.0f && absValue < std::numeric_limits<float>::min()) ? 0.0f : value;
}

} // namespace mousedrivech
