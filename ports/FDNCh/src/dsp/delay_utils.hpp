#pragma once

#include <array>
#include <cmath>

namespace DelayUtils
{
constexpr int maxDelays = 16;

inline bool isPrime(int n) noexcept
{
    if (n <= 1)
        return false;
    if (n <= 3)
        return true;
    if ((n % 2) == 0 || (n % 3) == 0)
        return false;

    for (int i = 5; i * i <= n; i += 6) {
        if ((n % i) == 0 || (n % (i + 2)) == 0)
            return false;
    }

    return true;
}

inline int nextPrime(int n) noexcept
{
    if (n <= 1)
        return 2;

    int prime = n;
    while (true) {
        ++prime;
        if (isPrime(prime))
            return prime;
    }
}

inline std::array<int, maxDelays> generateDelayLengths() noexcept
{
    std::array<int, maxDelays> lengths {};
    lengths[0] = 45;

    float currentLength = 45.0f;
    for (int n = 1; n < maxDelays; ++n) {
        currentLength *= 1.1f;
        currentLength = static_cast<float>(nextPrime(static_cast<int>(currentLength)));
        lengths[(size_t) n] = static_cast<int>(currentLength);
    }

    for (int i = 0; i < maxDelays / 2; ++i)
        std::swap(lengths[(size_t) i], lengths[(size_t) (maxDelays - i - 1)]);

    return lengths;
}

inline float calcGainForT60(int delaySamples, float sampleRate, float t60) noexcept
{
    const float nTimes = t60 * sampleRate / static_cast<float>(delaySamples);
    return std::pow(0.001f, 1.0f / nTimes);
}
} // namespace DelayUtils
