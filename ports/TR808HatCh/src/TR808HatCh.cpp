#include "SC_PlugIn.h"

#include "dsp/HatResonatorWDF.h"
#include "dsp/denormal.hpp"
#include "dsp/oversampling.hpp"

#include <new>

static InterfaceTable* ft;

namespace
{
constexpr int oversamplingRatio = 2;

enum InputIndex
{
    InputIn = 0,
    InputFreq,
    InputRes
};

struct TR808HatCh : public Unit
{
    Oversampling<oversamplingRatio> oversampling;
    HatResonatorWDF resonator;
};

inline float inputAt(TR808HatCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clampFreq(float freq) noexcept
{
    return sc_clip(freq, 1000.0f, 20000.0f);
}

inline float clampRes(float res) noexcept
{
    return sc_clip(res, 0.0f, 1.0f);
}

void TR808HatCh_next(TR808HatCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);
    auto* osBuffer = unit->oversampling.getOSBuffer();

    const auto audioFreq = INRATE(InputFreq) == calc_FullRate;
    const auto audioRes = INRATE(InputRes) == calc_FullRate;

    if (!audioFreq && !audioRes)
        unit->resonator.setParameters(clampFreq(IN0(InputFreq)), 1.0f - clampRes(IN0(InputRes)));

    for (int i = 0; i < inNumSamples; ++i) {
        if (audioFreq || audioRes) {
            const auto freq = audioFreq ? clampFreq(inputAt(unit, InputFreq, i)) : clampFreq(IN0(InputFreq));
            const auto res = audioRes ? clampRes(inputAt(unit, InputRes, i)) : clampRes(IN0(InputRes));
            unit->resonator.setParameters(freq, 1.0f - res);
        }

        unit->oversampling.upsample(in[i]);
        for (int j = 0; j < oversamplingRatio; ++j)
            osBuffer[j] = unit->resonator.processSample(osBuffer[j]);

        out[i] = delaybb::flushSubnormal(unit->oversampling.downsample());
    }
}

void TR808HatCh_Ctor(TR808HatCh* unit)
{
    new (unit) TR808HatCh;

    const auto sampleRate = static_cast<float>(SAMPLERATE);
    const auto osSampleRate = sampleRate * (float) oversamplingRatio;
    unit->oversampling.reset(sampleRate);
    unit->resonator.prepare(osSampleRate);
    unit->resonator.reset();
    unit->resonator.setParameters(clampFreq(IN0(InputFreq)), 1.0f - clampRes(IN0(InputRes)));

    SETCALC(TR808HatCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(TR808HatCh)
{
    ft = inTable;
    DefineSimpleUnit(TR808HatCh);
}
