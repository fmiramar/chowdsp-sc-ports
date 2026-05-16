#include "SC_PlugIn.h"

#include "dsp/polygonal_oscillator.hpp"

#include <new>

static InterfaceTable* ft;

namespace
{
constexpr float gainSmoothSeconds = 0.05f;

enum InputIndex
{
    InputFreq = 0,
    InputOrder,
    InputTeeth,
    InputGainDB
};

struct PolygonalOscCh : public Unit
{
    polygonal::PolygonalOscillator oscillator;
    polygonal::LinearSmoother gainSmoother;
};

inline float inputAt(PolygonalOscCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clampFreq(float value, float sampleRate) noexcept
{
    return sc_clip(value, 20.0f, sampleRate * 0.499f);
}

inline float clampOrder(float value) noexcept
{
    return sc_clip(value, 2.01f, 10.0f);
}

inline float clampTeeth(float value) noexcept
{
    return sc_clip(value, 0.0f, 1.0f);
}

inline float clampGainDB(float value) noexcept
{
    return sc_clip(value, -30.0f, 0.0f);
}

void PolygonalOscCh_next(PolygonalOscCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float sampleRate = static_cast<float>(SAMPLERATE);
    const bool audioRateGain = INRATE(InputGainDB) == calc_FullRate;

    if (!audioRateGain)
        unit->gainSmoother.setTarget(polygonal::dbToGain(clampGainDB(IN0(InputGainDB))), gainSmoothSeconds);

    for (int i = 0; i < inNumSamples; ++i) {
        unit->oscillator.setFrequency(clampFreq(inputAt(unit, InputFreq, i), sampleRate));
        unit->oscillator.setOrder(clampOrder(inputAt(unit, InputOrder, i)));
        unit->oscillator.setTeeth(clampTeeth(inputAt(unit, InputTeeth, i)));

        const float gain = audioRateGain ? polygonal::dbToGain(clampGainDB(IN(InputGainDB)[i])) : unit->gainSmoother.process();
        out[i] = gain * unit->oscillator.processSample();
    }
}

void PolygonalOscCh_Ctor(PolygonalOscCh* unit)
{
    new (unit) PolygonalOscCh;

    unit->oscillator.prepare(static_cast<float>(SAMPLERATE));
    unit->gainSmoother.reset(static_cast<float>(SAMPLERATE), polygonal::dbToGain(clampGainDB(IN0(InputGainDB))));

    SETCALC(PolygonalOscCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(PolygonalOscCh)
{
    ft = inTable;
    DefineSimpleUnit(PolygonalOscCh);
}
