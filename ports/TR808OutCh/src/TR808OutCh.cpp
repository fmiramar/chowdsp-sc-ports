#include "SC_PlugIn.h"

#include "dsp/TR808OutputFilter.h"
#include "dsp/denormal.hpp"

#include <new>

static InterfaceTable* ft;

namespace
{
enum InputIndex
{
    InputIn = 0,
    InputVolume,
    InputTone
};

struct TR808OutCh : public Unit
{
    TR808OutputFilter filter;
};

inline float inputAt(TR808OutCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clamp01(float x) noexcept
{
    return sc_clip(x, 0.0f, 1.0f);
}

void TR808OutCh_next(TR808OutCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);

    const auto audioVolume = INRATE(InputVolume) == calc_FullRate;
    const auto audioTone = INRATE(InputTone) == calc_FullRate;

    if (!audioVolume && !audioTone) {
        unit->filter.setVolume(clamp01(IN0(InputVolume)));
        unit->filter.setTone(clamp01(IN0(InputTone)));
    }

    for (int i = 0; i < inNumSamples; ++i) {
        if (audioVolume || audioTone) {
            const auto volume = audioVolume ? clamp01(inputAt(unit, InputVolume, i)) : clamp01(IN0(InputVolume));
            const auto tone = audioTone ? clamp01(inputAt(unit, InputTone, i)) : clamp01(IN0(InputTone));
            unit->filter.setVolume(volume);
            unit->filter.setTone(tone);
        }

        out[i] = delaybb::flushSubnormal(unit->filter.processSample(in[i]));
    }
}

void TR808OutCh_Ctor(TR808OutCh* unit)
{
    new (unit) TR808OutCh;

    unit->filter.prepare((float) SAMPLERATE);
    unit->filter.reset();
    unit->filter.setVolume(clamp01(IN0(InputVolume)));
    unit->filter.setTone(clamp01(IN0(InputTone)));

    SETCALC(TR808OutCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(TR808OutCh)
{
    ft = inTable;
    DefineSimpleUnit(TR808OutCh);
}
