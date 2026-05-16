#include "SC_PlugIn.h"

#include "dsp/degrade_filter.hpp"
#include "dsp/degrade_noise.hpp"
#include "dsp/level_detector.hpp"
#include "dsp/random.hpp"
#include "dsp/smoothed_value.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr int paramDivide = 64;

enum InputIndex
{
    InputIn = 0,
    InputDepth,
    InputAmount,
    InputVariance,
    InputEnvelope
};

struct TapeDegradeCh : public Unit
{
    tapedegrade::DegradeFilter filterProc;
    tapedegrade::DegradeNoise noiseProc;
    tapedegrade::LevelDetector<float> levelDetector;
    tapedegrade::SmoothedValue<float, tapedegrade::ValueSmoothingTypes::Multiplicative> gainProc;
    tapedegrade::Random rng;

    int samplesUntilCook = 0;
    float sampleRate = 44100.0f;

    float depth = 0.0f;
    float amount = 0.0f;
    float variance = 0.0f;
    float envelope = 0.0f;
};

inline float inputAt(TapeDegradeCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float dbToAmp(float db) noexcept
{
    return std::pow(10.0f, db / 20.0f);
}

inline void cookParams(TapeDegradeCh* unit, float depthParam, float amountParam, float varianceParam, float envelopeParam) noexcept
{
    unit->depth = std::clamp(depthParam, 0.0f, 1.0f);
    unit->amount = std::clamp(amountParam, 0.0f, 1.0f);
    unit->variance = std::clamp(varianceParam, 0.0f, 1.0f);
    unit->envelope = std::clamp(envelopeParam, 0.0f, 1.0f);

    const auto freqHz = 200.0f * std::pow(20000.0f / 200.0f, 1.0f - unit->amount);
    const auto gainDB = -24.0f * unit->depth;

    unit->noiseProc.setGain(0.33f * unit->depth * unit->amount);
    const auto variedFreq = freqHz + (unit->variance * (freqHz / 0.6f) * (unit->rng.uniform() - 0.5f));
    unit->filterProc.setFreq(std::min(variedFreq, 0.49f * unit->sampleRate));

    const auto envSkew = 1.0f - std::pow(unit->envelope, 0.8f);
    unit->levelDetector.setParameters(10.0f, 20.0f * std::pow(5000.0f / 20.0f, envSkew));

    const auto variedGainDB = std::min(gainDB + (unit->variance * 36.0f * (unit->rng.uniform() - 0.5f)), 3.0f);
    unit->gainProc.setTargetValue(dbToAmp(variedGainDB));
}

void TapeDegradeCh_next(TapeDegradeCh* unit, int inNumSamples)
{
    auto* out = OUT(0);
    const auto* in = IN(InputIn);

    for (int i = 0; i < inNumSamples; ++i) {
        if (unit->samplesUntilCook <= 0) {
            cookParams(unit,
                       inputAt(unit, InputDepth, i),
                       inputAt(unit, InputAmount, i),
                       inputAt(unit, InputVariance, i),
                       inputAt(unit, InputEnvelope, i));
            unit->samplesUntilCook = paramDivide;
        }

        auto x = in[i] / 10.0f;
        auto level = unit->levelDetector.processSample(x);
        auto noise = unit->noiseProc.processSample(0.0f);

        if (unit->envelope > 0.0f)
            noise *= level;

        x += noise;
        x = unit->filterProc.processSample(x);
        x *= unit->gainProc.getNextValue();

        if (!std::isfinite(x)) {
            x = 0.0f;
            unit->filterProc.reset(unit->sampleRate, static_cast<int>(unit->sampleRate * 0.05f));
            unit->levelDetector.reset();
            unit->gainProc.setCurrentAndTargetValue(1.0f);
            cookParams(unit, unit->depth, unit->amount, unit->variance, unit->envelope);
        }

        out[i] = x * 10.0f;
        --unit->samplesUntilCook;
    }
}

void TapeDegradeCh_Ctor(TapeDegradeCh* unit)
{
    new (unit) TapeDegradeCh;
    unit->sampleRate = static_cast<float>(SAMPLERATE);
    unit->noiseProc.prepare(unit->sampleRate);
    unit->filterProc.reset(unit->sampleRate, static_cast<int>(unit->sampleRate * 0.05f));
    unit->levelDetector.prepare(unit->sampleRate);
    unit->gainProc.reset(unit->sampleRate, 0.05);
    cookParams(unit, IN0(InputDepth), IN0(InputAmount), IN0(InputVariance), IN0(InputEnvelope));
    unit->samplesUntilCook = paramDivide;
    SETCALC(TapeDegradeCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(TapeDegradeCh)
{
    ft = inTable;
    DefineSimpleUnit(TapeDegradeCh);
}
