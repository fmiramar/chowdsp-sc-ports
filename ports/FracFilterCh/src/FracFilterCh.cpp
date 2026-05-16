#include "SC_PlugIn.h"

#include "dsp/frac_filter.hpp"

#include <new>

static InterfaceTable* ft;

namespace
{
enum InputIndex
{
    InputIn = 0,
    InputFreq,
    InputAlpha
};

struct FracFilterCh : public Unit
{
    fracfilter::FractionalOrderLowpass filter;
};

inline float inputAt(FracFilterCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

void FracFilterCh_next(FracFilterCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);

    const bool audioRateFreq = INRATE(InputFreq) == calc_FullRate;
    const bool audioRateAlpha = INRATE(InputAlpha) == calc_FullRate;

    if (!audioRateFreq && !audioRateAlpha)
        unit->filter.updateCoefs(IN0(InputFreq), IN0(InputAlpha));

    for (int i = 0; i < inNumSamples; ++i) {
        if (audioRateFreq || audioRateAlpha)
            unit->filter.updateCoefs(inputAt(unit, InputFreq, i), inputAt(unit, InputAlpha, i));

        out[i] = unit->filter.process(in[i]);
    }
}

void FracFilterCh_Ctor(FracFilterCh* unit)
{
    new (unit) FracFilterCh;

    unit->filter.prepare(static_cast<float>(SAMPLERATE));
    unit->filter.updateCoefs(IN0(InputFreq), IN0(InputAlpha));

    SETCALC(FracFilterCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(FracFilterCh)
{
    ft = inTable;
    DefineSimpleUnit(FracFilterCh);
}
