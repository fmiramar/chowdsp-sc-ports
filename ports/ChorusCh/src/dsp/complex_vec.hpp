#pragma once

#include "denormal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <limits>

namespace chorus
{
namespace FilterSpec
{
constexpr size_t count = 4;
}

using Complex = std::complex<float>;

struct ComplexVec
{
    std::array<Complex, FilterSpec::count> lanes {};

    ComplexVec() = default;
    explicit ComplexVec(const std::array<Complex, FilterSpec::count>& values) : lanes(values) {}

    static ComplexVec filled(Complex value) noexcept
    {
        ComplexVec out;
        out.lanes.fill(value);
        return out;
    }

    void clear() noexcept { lanes.fill(Complex { 0.0f, 0.0f }); }

    ComplexVec& operator+=(const ComplexVec& other) noexcept
    {
        for (size_t i = 0; i < FilterSpec::count; ++i)
            lanes[i] += other.lanes[i];
        return *this;
    }

    template <typename Fn>
    ComplexVec map(Fn&& fn) const
    {
        ComplexVec out;
        for (size_t i = 0; i < FilterSpec::count; ++i)
            out.lanes[i] = fn(lanes[i]);
        return out;
    }

    template <typename Fn>
    std::array<float, FilterSpec::count> mapFloat(Fn&& fn) const
    {
        std::array<float, FilterSpec::count> out {};
        for (size_t i = 0; i < FilterSpec::count; ++i)
            out[i] = fn(lanes[i]);
        return out;
    }

    float sumReal() const noexcept
    {
        float sum = 0.0f;
        for (const auto& lane : lanes)
            sum += lane.real();
        return flushSubnormal(sum);
    }

    void flushSubnormals() noexcept
    {
        for (auto& lane : lanes)
            lane = Complex { flushSubnormal(lane.real()), flushSubnormal(lane.imag()) };
    }

    int countSubnormals() const noexcept
    {
        int count = 0;

        for (const auto& lane : lanes) {
            const auto realAbs = std::abs(lane.real());
            const auto imagAbs = std::abs(lane.imag());

            if (realAbs > 0.0f && realAbs < std::numeric_limits<float>::min())
                ++count;
            if (imagAbs > 0.0f && imagAbs < std::numeric_limits<float>::min())
                ++count;
        }

        return count;
    }

    float maxAbs() const noexcept
    {
        float maxValue = 0.0f;
        for (const auto& lane : lanes) {
            maxValue = std::max(maxValue, std::abs(lane.real()));
            maxValue = std::max(maxValue, std::abs(lane.imag()));
        }

        return maxValue;
    }
};

inline ComplexVec operator+(ComplexVec lhs, const ComplexVec& rhs) noexcept
{
    lhs += rhs;
    return lhs;
}

inline ComplexVec operator*(const ComplexVec& lhs, const ComplexVec& rhs) noexcept
{
    ComplexVec out;
    for (size_t i = 0; i < FilterSpec::count; ++i)
        out.lanes[i] = lhs.lanes[i] * rhs.lanes[i];
    return out;
}

inline ComplexVec operator*(const ComplexVec& lhs, float scalar) noexcept
{
    ComplexVec out;
    for (size_t i = 0; i < FilterSpec::count; ++i)
        out.lanes[i] = lhs.lanes[i] * scalar;
    return out;
}

inline ComplexVec operator*(float scalar, const ComplexVec& rhs) noexcept
{
    return rhs * scalar;
}

inline ComplexVec expFromAngles(const std::array<float, FilterSpec::count>& angles, float factor) noexcept
{
    ComplexVec out;
    for (size_t i = 0; i < FilterSpec::count; ++i) {
        const auto phase = angles[i] * factor;
        out.lanes[i] = Complex { std::cos(phase), std::sin(phase) };
    }
    return out;
}

} // namespace chorus
