#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace waveshapers
{
inline double sign(double x) noexcept
{
    return (x > 0.0) - (x < 0.0);
}

inline double square(double x) noexcept
{
    return x * x;
}

inline double cube(double x) noexcept
{
    return x * x * x;
}

inline double clamp(double x, double low, double high) noexcept
{
    return std::max(low, std::min(high, x));
}

inline double logCoshStable(double x) noexcept
{
    const auto ax = std::abs(x);
    return ax + std::log1p(std::exp(-2.0 * ax)) - std::log(2.0);
}

inline double li2(double x) noexcept
{
    static constexpr double pi = 3.14159265358979323846;
    static constexpr double p[] = {
        0.9999999999999999502,
        -2.6883926818565423430,
        2.6477222699473109692,
        -1.1538559607887416355,
        0.20886077795020607837,
        -0.010859777134152463084
    };
    static constexpr double q[] = {
        1.0000000000000000000,
        -2.9383926818565635485,
        3.2712093293018635389,
        -1.7076702173954289421,
        0.41596017228400603836,
        -0.039801343754084482956,
        0.00082743668974466659035
    };

    double y = 0.0;
    double r = 0.0;
    double s = 1.0;

    if (x < -1.0) {
        const auto l = std::log(1.0 - x);
        y = 1.0 / (1.0 - x);
        r = -(pi * pi) / 6.0 + l * (0.5 * l - std::log(-x));
        s = 1.0;
    } else if (x == -1.0) {
        return -(pi * pi) / 12.0;
    } else if (x < 0.0) {
        const auto l = std::log1p(-x);
        y = x / (x - 1.0);
        r = -0.5 * l * l;
        s = -1.0;
    } else if (x == 0.0) {
        return 0.0;
    } else if (x < 0.5) {
        y = x;
        r = 0.0;
        s = 1.0;
    } else if (x < 1.0) {
        y = 1.0 - x;
        r = (pi * pi) / 6.0 - std::log(x) * std::log(y);
        s = -1.0;
    } else if (x == 1.0) {
        return (pi * pi) / 6.0;
    } else if (x < 2.0) {
        const auto l = std::log(x);
        y = 1.0 - 1.0 / x;
        r = (pi * pi) / 6.0 - l * (std::log(y) + 0.5 * l);
        s = 1.0;
    } else {
        const auto l = std::log(x);
        y = 1.0 / x;
        r = (pi * pi) / 3.0 - 0.5 * l * l;
        s = -1.0;
    }

    const auto y2 = y * y;
    const auto y4 = y2 * y2;
    const auto polyP = p[0] + y * p[1] + y2 * (p[2] + y * p[3]) + y4 * (p[4] + y * p[5]);
    const auto polyQ = q[0] + y * q[1] + y2 * (q[2] + y * q[3]) + y4 * (q[4] + y * q[5] + y2 * q[6]);
    return r + s * y * polyP / polyQ;
}

inline double tanhAD1(double x) noexcept
{
    return logCoshStable(x);
}

inline double tanhAD2(double x) noexcept
{
    static constexpr double pi = 3.14159265358979323846;
    if (x < 0.0)
        return -tanhAD2(-x);

    const auto expVal = std::exp(-2.0 * x);
    const auto logCosh = logCoshStable(x);
    return 0.5 * (li2(-expVal) - x * (x + 2.0 * std::log1p(expVal) - 2.0 * logCosh)) + ((pi * pi) / 24.0);
}

template <typename Policy>
struct ADAAShaper
{
    double x1 = 0.0;
    double x2 = 0.0;
    double ad2_x0 = 0.0;
    double ad2_x1 = 0.0;
    double d2 = 0.0;

    static constexpr double tolerance = 1.0e-2;

    void reset() noexcept
    {
        x1 = 0.0;
        x2 = 0.0;
        ad2_x0 = 0.0;
        ad2_x1 = 0.0;
        d2 = 0.0;
    }

    void sanitize() noexcept
    {
        x1 = sanitizeValue(x1);
        x2 = sanitizeValue(x2);
        ad2_x0 = sanitizeValue(ad2_x0);
        ad2_x1 = sanitizeValue(ad2_x1);
        d2 = sanitizeValue(d2);
    }

    double processSample(double input) noexcept
    {
        const auto x = input;
        const auto illCondition = std::abs(x - x2) < tolerance;
        const auto d1 = calcD1(x);
        auto y = illCondition ? fallback(x) : (2.0 / (x - x2)) * (d1 - d2);
        y += x1;

        d2 = d1;
        x2 = x1;
        x1 = x;
        ad2_x1 = ad2_x0;
        return y;
    }

private:
    static double sanitizeValue(double x) noexcept
    {
        return !std::isfinite(x) || std::abs(x) < 1.0e-30 ? 0.0 : x;
    }

    double calcD1(double x0) noexcept
    {
        const auto illCondition = std::abs(x0 - x1) < tolerance;
        ad2_x0 = Policy::ad2(x0);
        return illCondition ? Policy::ad1(0.5 * (x0 + x1)) : (ad2_x0 - ad2_x1) / (x0 - x1);
    }

    double fallback(double x) noexcept
    {
        const auto xBar = 0.5 * (x + x2);
        const auto delta = xBar - x1;
        const auto illCondition = std::abs(delta) < tolerance;
        return illCondition ? Policy::nl(0.5 * (xBar + x1))
                            : (2.0 / delta) * (Policy::ad1(xBar) + (ad2_x1 - Policy::ad2(xBar)) / delta);
    }
};

struct HardClipPolicy
{
    static double nl(double x) noexcept
    {
        return clamp(x, -1.0, 1.0);
    }

    static double ad1(double x) noexcept
    {
        return std::abs(x) <= 1.0 ? 0.5 * square(x) : x * sign(x) - 0.5;
    }

    static double ad2(double x) noexcept
    {
        return std::abs(x) <= 1.0 ? cube(x) / 6.0 : (0.5 * square(x) + (1.0 / 6.0)) * sign(x) - 0.5 * x;
    }
};

struct TanhClipPolicy
{
    static double nl(double x) noexcept
    {
        return std::tanh(x);
    }

    static double ad1(double x) noexcept
    {
        return tanhAD1(x);
    }

    static double ad2(double x) noexcept
    {
        return tanhAD2(x);
    }
};

struct WestCoastFoldPolicy
{
    struct FolderCell
    {
        double G;
        double B;
        double thresh;
        double mix;
        double Bp;
        double Bpp;

        FolderCell(double g, double b, double threshold, double mixFactor)
            : G(g)
            , B(b)
            , thresh(threshold)
            , mix(mixFactor)
            , Bp(0.5 * g * threshold * threshold - b * threshold)
            , Bpp((1.0 / 6.0) * g * threshold * threshold * threshold - 0.5 * b * threshold * threshold - threshold * Bp)
        {
        }

        double func(double x) const noexcept
        {
            return std::abs(x) > thresh ? (G * x - B * sign(x)) : 0.0;
        }

        double funcAD1(double x) const noexcept
        {
            return std::abs(x) > thresh ? 0.5 * G * square(x) - B * x * sign(x) - Bp : 0.0;
        }

        double funcAD2(double x) const noexcept
        {
            return std::abs(x) > thresh ? (1.0 / 6.0) * G * cube(x) - 0.5 * B * square(x) * sign(x) - Bp * x - Bpp * sign(x) : 0.0;
        }
    };

    static inline const std::array<FolderCell, 5> cells {
        FolderCell { 0.8333, 0.5, 0.6, -12.0 },
        FolderCell { 0.3768, 1.1281, 2.994, -27.777 },
        FolderCell { 0.2829, 1.5446, 5.46, -21.428 },
        FolderCell { 0.5743, 1.0338, 1.8, 17.647 },
        FolderCell { 0.2673, 1.0907, 4.08, 36.363 },
    };

    static double nl(double x) noexcept
    {
        double y = 5.0 * x;
        for (const auto& cell : cells)
            y += cell.mix * cell.func(x);
        return y;
    }

    static double ad1(double x) noexcept
    {
        double y = 2.5 * square(x);
        for (const auto& cell : cells)
            y += cell.mix * cell.funcAD1(x);
        return y;
    }

    static double ad2(double x) noexcept
    {
        double y = (5.0 / 6.0) * cube(x);
        for (const auto& cell : cells)
            y += cell.mix * cell.funcAD2(x);
        return y;
    }
};

struct WaveMultiplierPolicy
{
    static double nl(double x) noexcept
    {
        static constexpr double d = 2.45;
        static constexpr double b = 1.0 - 2.0 * 0.02;
        return (2.0 / d) * std::tanh(x * d) - b * x;
    }

    static double ad1(double x) noexcept
    {
        static constexpr double d = 2.45;
        static constexpr double b = 1.0 - 2.0 * 0.02;
        return (2.0 / square(d)) * tanhAD1(x * d) - 0.5 * b * square(x);
    }

    static double ad2(double x) noexcept
    {
        static constexpr double d = 2.45;
        static constexpr double b = 1.0 - 2.0 * 0.02;
        return (2.0 / cube(d)) * tanhAD2(x * d) - (b / 6.0) * cube(x);
    }
};

template <std::size_t numStages>
struct CascadedWaveMultiplier
{
    std::array<ADAAShaper<WaveMultiplierPolicy>, numStages> stages {};

    void reset() noexcept
    {
        for (auto& stage : stages)
            stage.reset();
    }

    void sanitize() noexcept
    {
        for (auto& stage : stages)
            stage.sanitize();
    }

    double processSample(double input) noexcept
    {
        auto x = input;
        for (auto& stage : stages)
            x = stage.processSample(x);
        return x;
    }
};
} // namespace waveshapers
