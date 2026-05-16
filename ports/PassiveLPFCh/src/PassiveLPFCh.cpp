#include "SC_PlugIn.h"

#include "dsp/PassiveLPF.h"
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
    InputFreq
};

struct PassiveLPFCh : public Unit
{
    Oversampling<oversamplingRatio> oversampling;
    PassiveLPF filter;
};

inline float inputAt(PassiveLPFCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clampFreq(float freq) noexcept
{
    return sc_clip(freq, 20.0f, 20000.0f);
}

void PassiveLPFCh_next(PassiveLPFCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);
    auto* osBuffer = unit->oversampling.getOSBuffer();

    const auto audioFreq = INRATE(InputFreq) == calc_FullRate;

    if (!audioFreq)
        unit->filter.setCutoffFrequency(clampFreq(IN0(InputFreq)));

    for (int i = 0; i < inNumSamples; ++i) {
        if (audioFreq)
            unit->filter.setCutoffFrequency(clampFreq(inputAt(unit, InputFreq, i)));

        unit->oversampling.upsample(in[i]);
        for (int j = 0; j < oversamplingRatio; ++j)
            osBuffer[j] = unit->filter.processSample(osBuffer[j]);

        out[i] = delaybb::flushSubnormal(unit->oversampling.downsample());
    }
}

void PassiveLPFCh_Ctor(PassiveLPFCh* unit)
{
    new (unit) PassiveLPFCh;

    const auto sampleRate = static_cast<float>(SAMPLERATE);
    unit->oversampling.reset(sampleRate);
    unit->filter.prepare(sampleRate);
    unit->filter.reset();
    unit->filter.setCutoffFrequency(clampFreq(IN0(InputFreq)));

    SETCALC(PassiveLPFCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(PassiveLPFCh)
{
    ft = inTable;
    DefineSimpleUnit(PassiveLPFCh);
}
