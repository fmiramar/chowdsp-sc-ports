#include "SC_PlugIn.h"

#include "dsp/LCOscillator.h"
#include "dsp/denormal.hpp"
#include "dsp/oversampling.hpp"

#include <algorithm>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr int oversamplingRatio = 2;
constexpr float outputGain = 0.06309573444801933f; // -24 dB

enum InputIndex
{
    InputIn = 0,
    InputFreq,
    InputClosed
};

struct LCOscCh : public Unit
{
    Oversampling<oversamplingRatio> oversampling;
    LCOscillator oscillator;
};

inline float inputAt(LCOscCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clampFreq(float freq) noexcept
{
    return sc_clip(freq, 20.0f, 2000.0f);
}

inline bool isClosed(float x) noexcept
{
    return x >= 0.5f;
}

void LCOscCh_next(LCOscCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);
    auto* osBuffer = unit->oversampling.getOSBuffer();

    const auto audioFreq = INRATE(InputFreq) == calc_FullRate;
    const auto audioClosed = INRATE(InputClosed) == calc_FullRate;

    if (!audioFreq && !audioClosed)
        unit->oscillator.setCircuitParams(clampFreq(IN0(InputFreq)), isClosed(IN0(InputClosed)));

    for (int i = 0; i < inNumSamples; ++i) {
        if (audioFreq || audioClosed)
            unit->oscillator.setCircuitParams(
                clampFreq(inputAt(unit, InputFreq, i)),
                isClosed(inputAt(unit, InputClosed, i)));

        unit->oversampling.upsample(in[i]);
        for (int j = 0; j < oversamplingRatio; ++j)
            osBuffer[j] = unit->oscillator.processSample(osBuffer[j]);

        out[i] = delaybb::flushSubnormal(unit->oversampling.downsample() * outputGain);
    }
}

void LCOscCh_Ctor(LCOscCh* unit)
{
    new (unit) LCOscCh;

    const auto sampleRate = static_cast<float>(SAMPLERATE);
    unit->oversampling.reset(sampleRate);
    unit->oscillator.prepare(sampleRate);
    unit->oscillator.reset();
    unit->oscillator.setCircuitParams(clampFreq(IN0(InputFreq)), isClosed(IN0(InputClosed)));

    SETCALC(LCOscCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(LCOscCh)
{
    ft = inTable;
    DefineSimpleUnit(LCOscCh);
}
