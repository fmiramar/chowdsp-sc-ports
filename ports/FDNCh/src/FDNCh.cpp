#include "SC_PlugIn.h"

#include "dsp/dynamic_delay_line.hpp"
#include "dsp/fdn.hpp"

#include <algorithm>
#include <cmath>
#include <new>

InterfaceTable* ft;

namespace
{
constexpr float maxPreDelayMs = 500.0f;

enum InputIndex
{
    InputIn = 0,
    InputPreDelay,
    InputSize,
    InputT60High,
    InputT60Low,
    InputNumDelays,
    InputMix
};

struct FDNCh : public Unit
{
    DynamicDelayLine preDelay;
    FDNCore fdn;

    float preDelayParam = 0.5f;
    float size = 0.5f;
    float t60High = 0.5f;
    float t60Low = 1.0f;
    float mix = 1.0f;
    int numDelays = 4;
};

inline float inputAt(FDNCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline int numDelaysFromValue(float value) noexcept
{
    int count = static_cast<int>(value);
    count = sc_clip(count, 1, FDNCore::maxDelays);
    return count;
}

inline void setParams(FDNCh* unit, float preDelayParam, float size, float t60High, float t60Low, float numDelays, float mix) noexcept
{
    unit->preDelayParam = sc_clip(preDelayParam, 0.0f, 1.0f);
    unit->size = sc_clip(size, 0.1f, 1.0f);
    unit->t60High = sc_clip(t60High, 0.5f, 10.0f);
    unit->t60Low = sc_clip(t60Low, 0.5f, 10.0f);
    unit->numDelays = numDelaysFromValue(numDelays);
    unit->mix = sc_clip(mix, 0.0f, 1.0f);

    const float delayMs = std::pow(maxPreDelayMs, unit->preDelayParam);
    unit->preDelay.setDelay(SAMPLERATE * delayMs / 1000.0f);
    unit->fdn.prepare((float) SAMPLERATE, unit->size, unit->t60Low, unit->t60High, unit->numDelays);
}

inline float processSample(FDNCh* unit, float x) noexcept
{
    const float delayed = unit->preDelay.process(x);
    const float wet = unit->fdn.process(delayed, unit->numDelays);
    return unit->mix * wet + (1.0f - unit->mix) * x;
}

void FDNCh_next_k(FDNCh* unit, int inNumSamples);
void FDNCh_next_a(FDNCh* unit, int inNumSamples);

void FDNCh_init(FDNCh* unit)
{
    const int preDelaySamples = static_cast<int>(std::ceil(SAMPLERATE * maxPreDelayMs / 1000.0f));
    const bool preDelayOK = unit->preDelay.allocate(unit->mWorld, preDelaySamples);
    const bool fdnOK = preDelayOK && unit->fdn.allocate(unit->mWorld, (float) SAMPLERATE);
    if (!(preDelayOK && fdnOK)) {
        unit->preDelay.free(unit->mWorld);
        unit->fdn.free(unit->mWorld);
        ClearUnitOnMemFailed
    }

    unit->preDelay.reset();
    unit->fdn.reset();
    setParams(unit, IN0(InputPreDelay), IN0(InputSize), IN0(InputT60High), IN0(InputT60Low), IN0(InputNumDelays), IN0(InputMix));

    const bool hasAudioRateParams = INRATE(InputPreDelay) == calc_FullRate || INRATE(InputSize) == calc_FullRate
        || INRATE(InputT60High) == calc_FullRate || INRATE(InputT60Low) == calc_FullRate || INRATE(InputNumDelays) == calc_FullRate
        || INRATE(InputMix) == calc_FullRate;

    if (hasAudioRateParams)
        SETCALC(FDNCh_next_a);
    else
        SETCALC(FDNCh_next_k);

    OUT0(0) = 0.0f;
}

void FDNCh_next_k(FDNCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);

    const float preDelay = IN0(InputPreDelay);
    const float size = IN0(InputSize);
    const float t60High = IN0(InputT60High);
    const float t60Low = IN0(InputT60Low);
    const float numDelays = IN0(InputNumDelays);
    const float mix = IN0(InputMix);

    setParams(unit, preDelay, size, t60High, t60Low, numDelays, mix);

    for (int i = 0; i < inNumSamples; ++i)
        out[i] = processSample(unit, in[i]);
}

void FDNCh_next_a(FDNCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);

    for (int i = 0; i < inNumSamples; ++i) {
        setParams(unit,
                  inputAt(unit, InputPreDelay, i),
                  inputAt(unit, InputSize, i),
                  inputAt(unit, InputT60High, i),
                  inputAt(unit, InputT60Low, i),
                  inputAt(unit, InputNumDelays, i),
                  inputAt(unit, InputMix, i));

        out[i] = processSample(unit, in[i]);
    }
}

void FDNCh_Ctor(FDNCh* unit)
{
    // Default-initialize to preserve the server-populated Unit base while constructing C++ members.
    new (unit) FDNCh;
    FDNCh_init(unit);
}

void FDNCh_Dtor(FDNCh* unit)
{
    unit->preDelay.free(unit->mWorld);
    unit->fdn.free(unit->mWorld);
}
} // namespace

PluginLoad(FDNCh)
{
    ft = inTable;
    DefineDtorUnit(FDNCh);
}
