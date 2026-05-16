#include "SC_PlugIn.h"

#include "../../FuzzMachineCh/src/dsp/filters.hpp"
#include "../../FuzzMachineCh/src/dsp/denormal.hpp"

#include <algorithm>
#include <cmath>
#include <new>
#include <tuple>

static InterfaceTable* ft;

namespace
{
namespace Zener
{
constexpr double alpha = 1.0168177;
constexpr double beta = 9.03240196;
constexpr double c = 0.222161;
constexpr double biasOffset = 8.2;
constexpr double maxVal = 7.5;
constexpr double mult = 10.0;
constexpr double oneOverMult = 0.99 / mult;
const double betaExpOverMult = beta * std::log(alpha) / mult;

inline double expApprox(double x) noexcept
{
    const auto x2 = x * x;
    const auto x3 = x2 * x;
    const auto x4 = x2 * x2;
    const auto num = x4 + 20.0 * x3 + 190.0 * x2 + 640.0 * x + 1680.0;
    const auto den = x4 - 15.0 * x3 + 180.0 * x2 - 840.0 * x + 2180.0;
    return num / den;
}

inline std::tuple<double, double> clipAndDerivative(double x) noexcept
{
    x *= mult;
    const auto xAbs = std::abs(x);
    if (xAbs < maxVal)
        return { x * oneOverMult, 0.99 };

    const auto expOut = expApprox(beta * std::log(alpha) * -std::abs(x + c));
    const auto sign = x >= 0.0 ? 1.0 : -1.0;
    return { sign * (-expOut + biasOffset) * oneOverMult, expOut + betaExpOverMult };
}
} // namespace Zener

struct PeakFilter
{
    void reset() noexcept
    {
        z1 = 0.0f;
        z2 = 0.0f;
    }

    void setParameters(float sampleRate, float freq, float q, float gainDB) noexcept
    {
        const auto a = std::pow(10.0f, gainDB / 40.0f);
        const auto w0 = 2.0f * 3.14159265358979323846f * freq / sampleRate;
        const auto alpha = std::sin(w0) / (2.0f * q);
        const auto cosw0 = std::cos(w0);

        const auto b0n = 1.0f + alpha * a;
        const auto b1n = -2.0f * cosw0;
        const auto b2n = 1.0f - alpha * a;
        const auto a0n = 1.0f + alpha / a;
        const auto a1n = -2.0f * cosw0;
        const auto a2n = 1.0f - alpha / a;

        b0 = b0n / a0n;
        b1 = b1n / a0n;
        b2 = b2n / a0n;
        a1 = a1n / a0n;
        a2 = a2n / a0n;
    }

    float process(float x) noexcept
    {
        const auto y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;
};

enum InputIndex
{
    InputIn = 0,
    InputDrive,
    InputCharacter,
    InputBias,
    InputHighQuality
};

struct BlondeDriveCh : public Unit
{
    fuzzmachinech::LinearSmoother driveSmooth;
    fuzzmachinech::LinearSmoother biasSmooth;
    PeakFilter characterFilter;
    fuzzmachinech::BiquadFilter dcBlocker;
    double state = 0.0;
};

inline float inputAt(BlondeDriveCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clamp01(float x) noexcept { return sc_clip(x, 0.0f, 1.0f); }
inline float clampBipolar(float x) noexcept { return sc_clip(x, -1.0f, 1.0f); }

template <int maxIter>
inline double processDriveSample(double x, double A, double& y0) noexcept
{
    for (int i = 0; i < maxIter; ++i) {
        const auto [f, fp] = Zener::clipAndDerivative(x + A * y0);
        const auto F = f + y0;
        const auto Fp = fp * A + 1.0;
        y0 -= F / Fp;
    }
    return -y0;
}

void BlondeDriveCh_next(BlondeDriveCh* unit, int inNumSamples)
{
    const auto* in = IN(InputIn);
    auto* out = OUT(0);
    const auto sampleRate = static_cast<float>(SAMPLERATE);
    const auto hq = IN0(InputHighQuality) > 0.5f;

    unit->driveSmooth.setTarget(1.0f - clamp01(IN0(InputDrive)));
    unit->biasSmooth.setTarget(0.5f * clamp01(IN0(InputBias)));
    unit->characterFilter.setParameters(sampleRate, 685.0f, 0.45f, clampBipolar(IN0(InputCharacter)) * 12.0f);

    for (int i = 0; i < inNumSamples; ++i) {
        const auto x1 = unit->characterFilter.process(in[i]) * 4.0f;
        const auto bias = unit->biasSmooth.process();
        const auto x2 = static_cast<double>(x1 + bias);
        const auto A = static_cast<double>(unit->driveSmooth.process());
        const auto y = hq ? processDriveSample<12>(x2, A, unit->state) : processDriveSample<8>(x2, A, unit->state);
        const auto unbias = static_cast<float>(y) - bias;
        out[i] = fuzzmachinech::flushSubnormal(unit->dcBlocker.process(unbias * 0.7071f));
    }
}

void BlondeDriveCh_Ctor(BlondeDriveCh* unit)
{
    new (unit) BlondeDriveCh;
    const auto sr = static_cast<float>(SAMPLERATE);
    unit->driveSmooth.setRampTime(sr, 0.05f);
    unit->biasSmooth.setRampTime(sr, 0.05f);
    unit->driveSmooth.reset(1.0f - clamp01(IN0(InputDrive)));
    unit->biasSmooth.reset(0.5f * clamp01(IN0(InputBias)));
    unit->characterFilter.setParameters(sr, 685.0f, 0.45f, clampBipolar(IN0(InputCharacter)) * 12.0f);
    unit->dcBlocker.setHighpass(30.0f / sr, 0.70710678f);
    unit->dcBlocker.reset();
    SETCALC(BlondeDriveCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(BlondeDriveCh)
{
    ft = inTable;
    DefineSimpleUnit(BlondeDriveCh);
}
