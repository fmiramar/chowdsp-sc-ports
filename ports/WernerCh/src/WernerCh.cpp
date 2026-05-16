#include "SC_PlugIn.h"

#include "dsp/gen_svf.hpp"
#include "dsp/oversampling.hpp"

#include <algorithm>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr float highFreq = 20000.0f;
constexpr float lowFreq = 20.0f;
constexpr float lowDrive = 0.1f;
constexpr float highDrive = 10.0f;
constexpr int oversamplingRatio = 2;
constexpr int paramDivide = 16;

enum InputIndex
{
    InputIn = 0,
    InputFreq,
    InputFeedback,
    InputDamping,
    InputDrive
};

struct WernerCh : public Unit
{
    Oversampling<oversamplingRatio> oversampling;
    GeneralSVF svf;
    int samplesUntilCook = 0;

    float freq = 632.4555f;
    float feedback = 0.5f;
    float damping = 0.5f;
    float drive = 0.1f;
};

void WernerCh_next(WernerCh* unit, int inNumSamples);

inline float inputAt(WernerCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline void cookParams(WernerCh* unit, float freq, float feedback, float damping, float drive) noexcept
{
    unit->freq = std::clamp(freq, lowFreq, highFreq);
    unit->feedback = std::clamp(feedback, 0.0f, 1.0f);
    unit->damping = std::clamp(damping, 0.25f, 1.25f);
    unit->drive = std::clamp(drive, lowDrive, highDrive);

    const float wc = (unit->freq / ((float) SAMPLERATE * oversamplingRatio)) * (float) M_PI;
    unit->svf.calcCoefs(unit->damping, unit->feedback, wc);
    unit->svf.setDrive(unit->drive);
}

inline float processSample(WernerCh* unit, float x) noexcept
{
    unit->oversampling.upsample(x);
    float* osBuffer = unit->oversampling.getOSBuffer();

    for (int k = 0; k < oversamplingRatio; ++k)
        osBuffer[k] = unit->svf.process(osBuffer[k]);

    return unit->oversampling.downsample();
}

void WernerCh_init(WernerCh* unit)
{
    unit->oversampling.reset((float) SAMPLERATE);
    unit->svf.reset();
    unit->samplesUntilCook = paramDivide;
    cookParams(unit, IN0(InputFreq), IN0(InputFeedback), IN0(InputDamping), IN0(InputDrive));

    SETCALC(WernerCh_next);
    OUT0(0) = 0.0f;
}

void WernerCh_next(WernerCh* unit, int inNumSamples)
{
    float* out = OUT(0);

    for (int i = 0; i < inNumSamples; ++i) {
        if (--unit->samplesUntilCook <= 0) {
            cookParams(unit, inputAt(unit, InputFreq, i), inputAt(unit, InputFeedback, i), inputAt(unit, InputDamping, i),
                       inputAt(unit, InputDrive, i));
            unit->samplesUntilCook = paramDivide;
        }

        out[i] = processSample(unit, inputAt(unit, InputIn, i));
    }
}

void WernerCh_Ctor(WernerCh* unit)
{
    // Default-initialize to preserve the server-populated Unit base while constructing C++ members.
    new (unit) WernerCh;
    WernerCh_init(unit);
}
} // namespace

PluginLoad(WernerCh)
{
    ft = inTable;
    DefineSimpleUnit(WernerCh);
}
