#pragma once

#include <cstdint>

namespace tapechew
{
class Random
{
public:
    explicit Random(std::uint32_t seed = 0x42f0e1ebu) noexcept
        : state(seed == 0 ? 0x42f0e1ebu : seed)
    {
    }

    std::uint32_t u32() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    float uniform() noexcept
    {
        return static_cast<float>((u32() >> 8) & 0x00ffffffu) * (1.0f / 16777216.0f);
    }

private:
    std::uint32_t state;
};
} // namespace tapechew
