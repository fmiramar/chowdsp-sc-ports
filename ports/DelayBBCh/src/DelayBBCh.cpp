#include "SC_PlugIn.h"

#include "dsp/bbd_delay_line.hpp"
#include "dsp/biquad.hpp"
#include "dsp/denormal.hpp"

#include <algorithm>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr int bbdStages = 4096;
constexpr float minDelayTime = 0.001f;
constexpr float maxDelayTime = 0.5f;

enum InputIndex
{
    InputIn = 0,
    InputDelayTime,
    InputFilterFreq,
    InputFeedback,
    InputDriveDB,
    InputMix
};

struct DelayBBCh : public Unit
{
    delaybb::BBDDelayLine<bbdStages> delay;
    delaybb::BiquadFilter dcBlocker;
    float feedbackState = 0.0f;
};

inline float inputAt(DelayBBCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clampDelayTime(float delayTime) noexcept
{
    return sc_clip(delayTime, minDelayTime, maxDelayTime);
}

inline float clampFilterFreq(float filterFreq, float sampleRate) noexcept
{
    return sc_clip(filterFreq, 200.0f, sampleRate * 0.49f);
}

inline float clampFeedback(float feedback) noexcept
{
    return sc_clip(feedback, -0.995f, 0.995f);
}

inline float clampDriveDB(float driveDB) noexcept
{
    return sc_clip(driveDB, -24.0f, 24.0f);
}

inline float clampMix(float mix) noexcept
{
    return sc_clip(mix, 0.0f, 1.0f);
}

inline float dbToGain(float db) noexcept
{
    return std::pow(10.0f, db * 0.05f);
}

inline void applyLineParams(DelayBBCh* unit, float delayTime, float filterFreq) noexcept
{
    const auto sampleRate = static_cast<float>(SAMPLERATE);
    unit->delay.setFilterFreq(clampFilterFreq(filterFreq, sampleRate));
    unit->delay.setDelayTime(clampDelayTime(delayTime));
}

inline float processSample(DelayBBCh* unit, float input, float feedback, float driveDB, float mix) noexcept
{
    const auto driveGain = dbToGain(clampDriveDB(driveDB));
    const auto feedbackAmt = clampFeedback(feedback);
    const auto wetMix = clampMix(mix);

    const auto lineInput = std::tanh(input * driveGain + unit->feedbackState);
    const auto wet = unit->delay.process(lineInput);

    unit->feedbackState = delaybb::flushSubnormal(unit->dcBlocker.process(wet * feedbackAmt));
    return delaybb::flushSubnormal(wet * wetMix + input * (1.0f - wetMix));
}

void DelayBBCh_next_k(DelayBBCh* unit, int inNumSamples);
void DelayBBCh_next_a(DelayBBCh* unit, int inNumSamples);

void DelayBBCh_Ctor(DelayBBCh* unit)
{
    new (unit) DelayBBCh;

    const auto sampleRate = static_cast<float>(SAMPLERATE);
    unit->delay.prepare(sampleRate);
    unit->dcBlocker.setParameters(delaybb::BiquadFilter::HIGHPASS, 20.0f / sampleRate, 0.7071f);
    unit->dcBlocker.reset();
    unit->feedbackState = 0.0f;

    applyLineParams(unit, IN0(InputDelayTime), IN0(InputFilterFreq));

    const auto hasAudioRateParams = INRATE(InputDelayTime) == calc_FullRate || INRATE(InputFilterFreq) == calc_FullRate
        || INRATE(InputFeedback) == calc_FullRate || INRATE(InputDriveDB) == calc_FullRate || INRATE(InputMix) == calc_FullRate;

    if (hasAudioRateParams)
        SETCALC(DelayBBCh_next_a);
    else
        SETCALC(DelayBBCh_next_k);

    OUT0(0) = 0.0f;
}

void DelayBBCh_next_k(DelayBBCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);

    const auto delayTime = IN0(InputDelayTime);
    const auto filterFreq = IN0(InputFilterFreq);
    const auto feedback = IN0(InputFeedback);
    const auto driveDB = IN0(InputDriveDB);
    const auto mix = IN0(InputMix);

    applyLineParams(unit, delayTime, filterFreq);

    for (int i = 0; i < inNumSamples; ++i)
        out[i] = processSample(unit, in[i], feedback, driveDB, mix);
}

void DelayBBCh_next_a(DelayBBCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);

    const auto audioRateDelayTime = INRATE(InputDelayTime) == calc_FullRate;
    const auto audioRateFilterFreq = INRATE(InputFilterFreq) == calc_FullRate;

    if (!audioRateDelayTime && !audioRateFilterFreq)
        applyLineParams(unit, IN0(InputDelayTime), IN0(InputFilterFreq));

    for (int i = 0; i < inNumSamples; ++i) {
        if (audioRateDelayTime || audioRateFilterFreq)
            applyLineParams(unit, inputAt(unit, InputDelayTime, i), inputAt(unit, InputFilterFreq, i));

        out[i] = processSample(
            unit,
            in[i],
            inputAt(unit, InputFeedback, i),
            inputAt(unit, InputDriveDB, i),
            inputAt(unit, InputMix, i));
    }
}
} // namespace

PluginLoad(DelayBBCh)
{
    ft = inTable;
    DefineSimpleUnit(DelayBBCh);
}
