#pragma once

#include <algorithm>
#include <cmath>

namespace phasermodch
{
namespace ldr
{
inline float lightShape(float x, float skewPow) noexcept
{
    x = std::clamp(x, -1.0f, 1.0f);
    return (std::pow((x + 1.0f) * 0.5f, skewPow) * 2.0f) - 1.0f;
}

inline float getResistance(float lfo, float skew) noexcept
{
    constexpr float maxDepth = 20.0f;
    const float skewVal = std::pow(2.0f, skew);
    const float lfoVal = lightShape(lfo, skewVal);
    const float lightVal = (maxDepth + 0.1f) - (lfoVal * maxDepth);

    return 100000.0f * std::pow(lightVal / 0.1f, -0.75f);
}
} // namespace ldr
} // namespace phasermodch
