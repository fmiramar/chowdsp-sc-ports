#include "SC_PlugIn.h"

#include "dsp/SallenKeyLPF.h"
#include "dsp/denormal.hpp"
#include "dsp/oversampling.hpp"
#include "dsp/smoothed_value.hpp"

#include <algorithm>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr int oversamplingRatio = 2;

enum InputIndex
{
    InputIn = 0,
    InputFreq,
    InputQ
};

struct SallenKeyCh : public Unit
{
    Oversampling<oversamplingRatio> oversampling;
    SallenKeyLPF filter;
    sallenkeych::SmoothedValue<float, sallenkeych::ValueSmoothingTypes::Multiplicative> freqSmooth { 2000.0f };
    sallenkeych::SmoothedValue<float, sallenkeych::ValueSmoothingTypes::Multiplicative> qSmooth { 0.7071f };
};

inline float inputAt(SallenKeyCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clampFreq(float freq) noexcept
{
    return sc_clip(freq, 20.0f, 20000.0f);
}

inline float clampQ(float q) noexcept
{
    return sc_clip(q, 0.2f, 10.0f);
}

void SallenKeyCh_next(SallenKeyCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);
    auto* osBuffer = unit->oversampling.getOSBuffer();

    const auto audioFreq = INRATE(InputFreq) == calc_FullRate;
    const auto audioQ = INRATE(InputQ) == calc_FullRate;

    if (!audioFreq)
        unit->freqSmooth.setTargetValue(clampFreq(IN0(InputFreq)));
    if (!audioQ)
        unit->qSmooth.setTargetValue(clampQ(IN0(InputQ)));

    const auto smoothFreq = !audioFreq && unit->freqSmooth.isSmoothing();
    const auto smoothQ = !audioQ && unit->qSmooth.isSmoothing();
    const auto needsPerSubsampleParams = audioFreq || audioQ || smoothFreq || smoothQ;

    if (!needsPerSubsampleParams) {
        unit->filter.setParams(unit->freqSmooth.getNextValue(), unit->qSmooth.getNextValue());

        for (int i = 0; i < inNumSamples; ++i) {
            unit->oversampling.upsample(in[i]);
            for (int j = 0; j < oversamplingRatio; ++j)
                osBuffer[j] = unit->filter.processSample(osBuffer[j]);

            out[i] = delaybb::flushSubnormal(unit->oversampling.downsample());
        }

        return;
    }

    for (int i = 0; i < inNumSamples; ++i) {
        const auto freqValue = audioFreq ? clampFreq(inputAt(unit, InputFreq, i)) : 0.0f;
        const auto qValue = audioQ ? clampQ(inputAt(unit, InputQ, i)) : 0.0f;

        unit->oversampling.upsample(in[i]);
        for (int j = 0; j < oversamplingRatio; ++j) {
            const auto freq = audioFreq ? freqValue : (smoothFreq ? unit->freqSmooth.getNextValue() : unit->freqSmooth.getCurrentValue());
            const auto q = audioQ ? qValue : (smoothQ ? unit->qSmooth.getNextValue() : unit->qSmooth.getCurrentValue());
            unit->filter.setParams(freq, q);
            osBuffer[j] = unit->filter.processSample(osBuffer[j]);
        }

        out[i] = delaybb::flushSubnormal(unit->oversampling.downsample());
    }
}

void SallenKeyCh_Ctor(SallenKeyCh* unit)
{
    new (unit) SallenKeyCh;

    const auto sampleRate = static_cast<float>(SAMPLERATE);
    const auto osSampleRate = sampleRate * (float) oversamplingRatio;

    unit->oversampling.reset(sampleRate);
    unit->filter.prepare(osSampleRate);
    unit->filter.reset();
    unit->freqSmooth.reset(osSampleRate, 0.02);
    unit->qSmooth.reset(osSampleRate, 0.02);
    unit->freqSmooth.setCurrentAndTargetValue(clampFreq(IN0(InputFreq)));
    unit->qSmooth.setCurrentAndTargetValue(clampQ(IN0(InputQ)));
    unit->filter.setParams(unit->freqSmooth.getCurrentValue(), unit->qSmooth.getCurrentValue());

    SETCALC(SallenKeyCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(SallenKeyCh)
{
    ft = inTable;
    DefineSimpleUnit(SallenKeyCh);
}
