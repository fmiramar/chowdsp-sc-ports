#pragma once

#include <cmath>
#include <limits>

namespace phase90ch
{
inline bool isSubnormal(float value) noexcept
{
    const auto absValue = std::abs(value);
    return absValue > 0.0f && absValue < std::numeric_limits<float>::min();
}

inline float flushSubnormal(float value) noexcept
{
    return isSubnormal(value) ? 0.0f : value;
}
} // namespace phase90ch
