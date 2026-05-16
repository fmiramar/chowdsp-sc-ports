#pragma once

#include <algorithm>
#include <array>
#include <numeric>

namespace tapeloss
{
template <int MaxOrder>
class FIRFilter
{
public:
    void setOrder(int newOrder) noexcept
    {
        order = std::clamp(newOrder, 1, MaxOrder);
        reset();
    }

    int getOrder() const noexcept
    {
        return order;
    }

    void reset() noexcept
    {
        zPtr = 0;
        std::fill(z.begin(), z.end(), 0.0f);
    }

    void setCoefs(const float* coefs) noexcept
    {
        std::copy_n(coefs, static_cast<size_t>(order), h.begin());
        std::fill(h.begin() + order, h.end(), 0.0f);
    }

    float process(float x) noexcept
    {
        z[zPtr] = x;
        z[zPtr + order] = x;

        const auto y = std::inner_product(z.data() + zPtr, z.data() + zPtr + order, h.data(), 0.0f);
        zPtr = zPtr == 0 ? order - 1 : zPtr - 1;
        return y;
    }

private:
    std::array<float, static_cast<size_t>(MaxOrder)> h {};
    std::array<float, static_cast<size_t>(MaxOrder) * 2> z {};
    int order = 1;
    int zPtr = 0;
};
} // namespace tapeloss
