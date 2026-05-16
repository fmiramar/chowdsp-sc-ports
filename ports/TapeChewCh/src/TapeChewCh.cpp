#include "SC_PlugIn.h"

#include "dsp/degrade_filter.hpp"
#include "dsp/dropout.hpp"
#include "dsp/random.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr int paramDivide = 64;
constexpr int initialSamplesUntilChange = 1000;

enum InputIndex
{
    InputIn = 0,
    InputDepth,
    InputFrequency,
    InputVariance
};

struct TapeChewCh : public Unit
{
    tapechew::Dropout dropout;
    tapechew::DegradeFilter filter;
    tapechew::Random rng;

    float mix = 0.0f;
    float power = 0.0f;
    float depth = 0.0f;
    float frequency = 0.0f;
    float variance = 0.0f;
    float sampleRate = 44100.0f;
    int samplesUntilChange = initialSamplesUntilChange;
    bool isCrinkled = false;
    int sampleCounter = 0;
    int samplesUntilCook = 0;
};

inline float inputAt(TapeChewCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float randomDurationScale(TapeChewCh* unit, float variance) noexcept
{
    const auto base = std::max(unit->rng.uniform() * 2.0f, 1.0e-6f);
    return std::pow(base, variance);
}

inline int boundedRandomInt(TapeChewCh* unit, int minValue, int maxValue) noexcept
{
    if (maxValue <= minValue)
        return std::max(minValue, 1);

    const auto span = static_cast<std::uint32_t>(maxValue - minValue);
    return static_cast<int>(unit->rng.u32() % span) + minValue;
}

inline int getDryTime(TapeChewCh* unit, float frequency, float variance) noexcept
{
    const auto tScale = std::pow(frequency, 0.1f);
    const auto varScale = randomDurationScale(unit, variance);

    const auto minValue = static_cast<int>((1.0f - tScale) * unit->sampleRate * varScale);
    const auto maxValue = static_cast<int>((2.0f - 1.99f * tScale) * unit->sampleRate * varScale);
    return boundedRandomInt(unit, minValue, maxValue);
}

inline int getWetTime(TapeChewCh* unit, float frequency, float depth, float variance) noexcept
{
    const auto tScale = std::pow(frequency, 0.1f);
    const auto start = 0.2f + 0.8f * depth;
    const auto end = start - (0.001f + 0.01f * depth);
    const auto varScale = randomDurationScale(unit, variance);

    const auto minValue = static_cast<int>((1.0f - tScale) * unit->sampleRate * varScale);
    const auto maxValue = static_cast<int>(((1.0f - tScale) + start - end * tScale) * unit->sampleRate * varScale);
    return boundedRandomInt(unit, minValue, maxValue);
}

inline void cookParams(TapeChewCh* unit, float depthParam, float frequencyParam, float varianceParam) noexcept
{
    const auto highFreq = std::min(22000.0f, 0.49f * unit->sampleRate);
    const auto freqChange = highFreq - 5000.0f;

    unit->depth = std::clamp(depthParam, 0.0f, 1.0f);
    unit->frequency = std::clamp(frequencyParam, 0.0f, 1.0f);
    unit->variance = std::clamp(varianceParam, 0.0f, 1.0f);

    if (unit->frequency <= 0.0f) {
        unit->mix = 0.0f;
        unit->filter.setFreq(highFreq);
    } else if (unit->frequency >= 1.0f) {
        unit->mix = 1.0f;
        unit->power = 3.0f * unit->depth;
        unit->filter.setFreq(highFreq - freqChange * unit->depth);
    } else if (unit->sampleCounter >= unit->samplesUntilChange) {
        unit->sampleCounter = 0;
        unit->isCrinkled = !unit->isCrinkled;

        if (unit->isCrinkled) {
            unit->mix = 1.0f;
            unit->power = (1.0f + 2.0f * unit->rng.uniform()) * unit->depth;
            unit->filter.setFreq(highFreq - freqChange * unit->depth);
            unit->samplesUntilChange = getWetTime(unit, unit->frequency, unit->depth, unit->variance);
        } else {
            unit->mix = 0.0f;
            unit->filter.setFreq(highFreq);
            unit->samplesUntilChange = getDryTime(unit, unit->frequency, unit->variance);
        }
    } else {
        unit->power = (1.0f + 2.0f * unit->rng.uniform()) * unit->depth;
        if (unit->isCrinkled)
            unit->filter.setFreq(highFreq - freqChange * unit->depth);
    }

    unit->dropout.setMix(unit->mix);
    unit->dropout.setPower(1.0f + unit->power);
}

void TapeChewCh_next(TapeChewCh* unit, int inNumSamples)
{
    auto* out = OUT(0);
    const auto* in = IN(InputIn);

    for (int i = 0; i < inNumSamples; ++i) {
        if (unit->samplesUntilCook <= 0) {
            cookParams(unit,
                       inputAt(unit, InputDepth, i),
                       inputAt(unit, InputFrequency, i),
                       inputAt(unit, InputVariance, i));
            unit->samplesUntilCook = paramDivide;
        }

        auto x = in[i] / 10.0f;
        x = unit->dropout.processSample(x);
        x = unit->filter.processSample(x);

        if (!std::isfinite(x)) {
            x = 0.0f;
            unit->dropout.prepare(unit->sampleRate);
            unit->filter.reset(unit->sampleRate, static_cast<int>(unit->sampleRate * 0.02f));
            unit->mix = 0.0f;
            unit->power = 0.0f;
            unit->samplesUntilChange = initialSamplesUntilChange;
            unit->isCrinkled = false;
            unit->sampleCounter = 0;
            cookParams(unit, unit->depth, unit->frequency, unit->variance);
        }

        out[i] = x * 10.0f;
        ++unit->sampleCounter;
        --unit->samplesUntilCook;
    }
}

void TapeChewCh_Ctor(TapeChewCh* unit)
{
    new (unit) TapeChewCh;
    unit->sampleRate = static_cast<float>(SAMPLERATE);
    unit->filter.reset(unit->sampleRate, static_cast<int>(unit->sampleRate * 0.02f));
    unit->dropout.prepare(unit->sampleRate);
    cookParams(unit, IN0(InputDepth), IN0(InputFrequency), IN0(InputVariance));
    unit->samplesUntilCook = paramDivide;
    SETCALC(TapeChewCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(TapeChewCh)
{
    ft = inTable;
    DefineSimpleUnit(TapeChewCh);
}
