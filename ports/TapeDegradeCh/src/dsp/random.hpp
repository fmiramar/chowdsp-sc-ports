#pragma once

#include <cstdint>

namespace tapedegrade
{
class Random
{
public:
    explicit Random(std::uint32_t seed = 0x7f4a7c15u) noexcept
        : state(seed == 0 ? 0x7f4a7c15u : seed)
    {
    }

    float uniform() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float>((state >> 8) & 0x00ffffffu) * (1.0f / 16777216.0f);
    }

private:
    std::uint32_t state;
};
} // namespace tapedegrade
