/*
 * Copyright (C) 2019 Stefano D'Angelo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * NOTE: code has been modified significantly by Jatin Chowdhury
 */

#pragma once

#include <algorithm>
#include <cstdint>

namespace chowdsp::Omega
{
template <typename T>
inline T log2_approx(T x)
{
    constexpr T alpha = static_cast<T>(0.1640425613334452);
    constexpr T beta = static_cast<T>(-1.098865286222744);
    constexpr T gamma = static_cast<T>(3.148297929334117);
    constexpr T zeta = static_cast<T>(-2.213475204444817);

    return zeta + x * (gamma + x * (beta + x * alpha));
}

template <typename T>
T log_approx(T x);

template <>
inline float log_approx(float x)
{
    union
    {
        int32_t i;
        float f;
    } v {};
    v.f = x;
    const int32_t ex = v.i & 0x7f800000;
    const int32_t e = (ex >> 23) - 127;
    v.i = (v.i - ex) | 0x3f800000;

    return 0.693147180559945f * (static_cast<float>(e) + log2_approx<float>(v.f));
}

template <>
inline double log_approx(double x)
{
    union
    {
        int64_t i;
        double d;
    } v {};
    v.d = x;
    const int64_t ex = v.i & 0x7ff0000000000000LL;
    const int64_t e = (ex >> 52) - 1023;
    v.i = (v.i - ex) | 0x3ff0000000000000LL;

    return 0.693147180559945 * (static_cast<double>(e) + log2_approx<double>(v.d));
}

template <typename T>
inline T pow2_approx(T x)
{
    constexpr T alpha = static_cast<T>(0.07944154167983575);
    constexpr T beta = static_cast<T>(0.2274112777602189);
    constexpr T gamma = static_cast<T>(0.6931471805599453);
    constexpr T zeta = static_cast<T>(1.0);

    return zeta + x * (gamma + x * (beta + x * alpha));
}

template <typename T>
T exp_approx(T x);

template <>
inline float exp_approx(float x)
{
    x = std::max(-126.0f, 1.442695040888963f * x);

    union
    {
        int32_t i;
        float f;
    } v {};

    const auto xi = static_cast<int32_t>(x);
    const int32_t l = x < 0.0f ? xi - 1 : xi;
    const float f = x - static_cast<float>(l);
    v.i = (l + 127) << 23;

    return v.f * pow2_approx<float>(f);
}

template <>
inline double exp_approx(double x)
{
    x = std::max(-1022.0, 1.442695040888963 * x);

    union
    {
        int64_t i;
        double d;
    } v {};

    const auto xi = static_cast<int64_t>(x);
    const int64_t l = x < 0.0 ? xi - 1 : xi;
    const double d = x - static_cast<double>(l);
    v.i = (l + 1023) << 52;

    return v.d * pow2_approx<double>(d);
}

template <typename T>
inline T omega3(T x)
{
    constexpr T x1 = static_cast<T>(-3.341459552768620);
    constexpr T x2 = static_cast<T>(8.0);
    constexpr T a = static_cast<T>(-1.314293149877800e-3);
    constexpr T b = static_cast<T>(4.775931364975583e-2);
    constexpr T c = static_cast<T>(3.631952663804445e-1);
    constexpr T d = static_cast<T>(6.313183464296682e-1);
    return x < x1 ? static_cast<T>(0) : (x < x2 ? d + x * (c + x * (b + x * a)) : x - log_approx<T>(x));
}

template <typename T>
inline T omega4(T x)
{
    const T y = omega3<T>(x);
    return y - (y - exp_approx<T>(x - y)) / (y + static_cast<T>(1));
}

} // namespace chowdsp::Omega
