#include "SC_PlugIn.h"

#include "dsp/biquad.hpp"
#include "dsp/fir_filter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr float speedBase = 25.0f / 4.0f;
constexpr float speedMult = 28.0f / 3.0f;
constexpr float speedOff = -25.0f / 3.0f;

constexpr float spaceBase = 10000.0f / 9801.0f;
constexpr float spaceMult = 9801.0f / 10.0f;
constexpr float spaceOff = -980.0f;

constexpr float thickBase = 122500.0f / 22201.0f;
constexpr float thickMult = 22201.0f / 2010.0f;
constexpr float thickOff = -2200.0f / 201.0f;

constexpr float gapBase = 1600.0f / 81.0f;
constexpr float gapMult = 81.0f / 31.0f;
constexpr float gapOff = -50.0f / 31.0f;

constexpr int paramDivide = 128;
constexpr int baseOrder = 64;
constexpr int maxOrder = 512;
constexpr float referenceSampleRate = 44100.0f;
constexpr float micronsToMeters = 1.0e-6f;
constexpr float twoPi = 6.28318530717958647692f;

enum InputIndex
{
    InputIn = 0,
    InputGap,
    InputThickness,
    InputSpacing,
    InputSpeed
};

struct TapeLossCh : public Unit
{
    tapeloss::FIRFilter<maxOrder> fir;
    tapeloss::BiquadFilter headBumpFilter;
    std::array<float, maxOrder> currentCoefs {};
    std::array<float, maxOrder> hCoefs {};

    int curOrder = baseOrder;
    int samplesUntilCook = 0;

    float gapParam = 0.5f;
    float thicknessParam = 0.5f;
    float spacingParam = 0.5f;
    float speedParam = 0.5f;
    float sampleRate = referenceSampleRate;
    float binWidth = referenceSampleRate / static_cast<float>(baseOrder);
};

inline float inputAt(TapeLossCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float getSpeed(float param) noexcept
{
    return std::pow(speedBase, std::clamp(param, 0.0f, 1.0f)) * speedMult + speedOff;
}

inline float getSpacing(float param) noexcept
{
    return std::pow(spaceBase, std::clamp(param, 0.0f, 1.0f)) * spaceMult + spaceOff;
}

inline float getThickness(float param) noexcept
{
    return std::pow(thickBase, std::clamp(param, 0.0f, 1.0f)) * thickMult + thickOff;
}

inline float getGap(float param) noexcept
{
    return std::pow(gapBase, std::clamp(param, 0.0f, 1.0f)) * gapMult + gapOff;
}

inline float oneMinusExpOverX(float x) noexcept
{
    if (std::abs(x) < 1.0e-6f)
        return 1.0f;

    return (1.0f - std::exp(-x)) / x;
}

inline float sinc(float x) noexcept
{
    if (std::abs(x) < 1.0e-6f)
        return 1.0f;

    return std::sin(x) / x;
}

inline void calcHeadBumpFilter(TapeLossCh* unit, float speedIps, float gapMeters) noexcept
{
    const auto bumpFreq = speedIps * 0.0254f / (gapMeters * 500.0f);
    const auto gain = std::max(1.5f * (1000.0f - std::abs(bumpFreq - 100.0f)) / 1000.0f, 1.0f);
    unit->headBumpFilter.setParameters(tapeloss::BiquadFilter::PEAK, bumpFreq / unit->sampleRate, 2.0f, gain);
}

inline void rebuildFilters(TapeLossCh* unit, float gapParam, float thicknessParam, float spacingParam, float speedParam) noexcept
{
    unit->gapParam = std::clamp(gapParam, 0.0f, 1.0f);
    unit->thicknessParam = std::clamp(thicknessParam, 0.0f, 1.0f);
    unit->spacingParam = std::clamp(spacingParam, 0.0f, 1.0f);
    unit->speedParam = std::clamp(speedParam, 0.0f, 1.0f);

    const auto speed = getSpeed(unit->speedParam);
    const auto thickness = getThickness(unit->thicknessParam);
    const auto gap = getGap(unit->gapParam);
    const auto spacing = getSpacing(unit->spacingParam);

    unit->binWidth = unit->sampleRate / static_cast<float>(unit->curOrder);

    // Clear the working buffers before each rebuild so odd-order paths don't retain stale taps.
    std::fill(unit->currentCoefs.begin(), unit->currentCoefs.end(), 0.0f);
    std::fill(unit->hCoefs.begin(), unit->hCoefs.end(), 0.0f);

    for (int k = 0; k < unit->curOrder / 2; ++k) {
        const auto freq = static_cast<float>(k) * unit->binWidth;
        const auto waveNumber = twoPi * std::max(freq, 20.0f) / (speed * 0.0254f);
        const auto thickTimesK = waveNumber * (thickness * micronsToMeters);
        const auto kGapOverTwo = waveNumber * (gap * micronsToMeters) * 0.5f;

        auto h = std::exp(-waveNumber * (spacing * micronsToMeters));
        h *= oneMinusExpOverX(thickTimesK);
        h *= sinc(kGapOverTwo);

        unit->hCoefs[static_cast<size_t>(k)] = h;
        unit->hCoefs[static_cast<size_t>(unit->curOrder - k - 1)] = h;
    }

    for (int n = 0; n < unit->curOrder / 2; ++n) {
        const auto idx = unit->curOrder / 2 + n;
        auto value = 0.0f;
        for (int k = 0; k < unit->curOrder; ++k)
            value += unit->hCoefs[static_cast<size_t>(k)] * std::cos(twoPi * static_cast<float>(k * n) / static_cast<float>(unit->curOrder));

        value /= static_cast<float>(unit->curOrder);
        unit->currentCoefs[static_cast<size_t>(idx)] = value;
        unit->currentCoefs[static_cast<size_t>(unit->curOrder / 2 - n)] = value;
    }

    unit->fir.setCoefs(unit->currentCoefs.data());
    calcHeadBumpFilter(unit, speed, gap * micronsToMeters);
}

inline void cookParams(TapeLossCh* unit, float gapParam, float thicknessParam, float spacingParam, float speedParam) noexcept
{
    const auto nextGap = std::clamp(gapParam, 0.0f, 1.0f);
    const auto nextThickness = std::clamp(thicknessParam, 0.0f, 1.0f);
    const auto nextSpacing = std::clamp(spacingParam, 0.0f, 1.0f);
    const auto nextSpeed = std::clamp(speedParam, 0.0f, 1.0f);

    if (nextGap == unit->gapParam && nextThickness == unit->thicknessParam && nextSpacing == unit->spacingParam && nextSpeed == unit->speedParam)
        return;

    rebuildFilters(unit, nextGap, nextThickness, nextSpacing, nextSpeed);
}

void TapeLossCh_next(TapeLossCh* unit, int inNumSamples)
{
    auto* out = OUT(0);
    const auto* in = IN(InputIn);

    for (int i = 0; i < inNumSamples; ++i) {
        if (unit->samplesUntilCook <= 0) {
            cookParams(unit,
                       inputAt(unit, InputGap, i),
                       inputAt(unit, InputThickness, i),
                       inputAt(unit, InputSpacing, i),
                       inputAt(unit, InputSpeed, i));
            unit->samplesUntilCook = paramDivide;
        }

        auto y = unit->fir.process(in[i]);
        y = unit->headBumpFilter.process(y);
        if (!std::isfinite(y)) {
            y = 0.0f;
            unit->fir.reset();
            unit->headBumpFilter.reset();
            rebuildFilters(unit, unit->gapParam, unit->thicknessParam, unit->spacingParam, unit->speedParam);
        }

        out[i] = y;
        --unit->samplesUntilCook;
    }
}

void TapeLossCh_Ctor(TapeLossCh* unit)
{
    new (unit) TapeLossCh;
    unit->sampleRate = static_cast<float>(SAMPLERATE);
    unit->curOrder = std::clamp(static_cast<int>(baseOrder * (unit->sampleRate / referenceSampleRate)), 1, maxOrder);
    unit->fir.setOrder(unit->curOrder);
    unit->headBumpFilter.reset();
    rebuildFilters(unit, IN0(InputGap), IN0(InputThickness), IN0(InputSpacing), IN0(InputSpeed));
    unit->samplesUntilCook = paramDivide;
    SETCALC(TapeLossCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(TapeLossCh)
{
    ft = inTable;
    DefineSimpleUnit(TapeLossCh);
}
