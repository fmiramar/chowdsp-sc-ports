#include "SC_PlugIn.h"

#include "dsp/denormal.hpp"
#include "dsp/phase90_dsp.hpp"

#include <algorithm>
#include <new>

static InterfaceTable* ft;

namespace
{
enum InputIndex
{
    InputIn = 0,
    InputRate,
    InputDepth,
    InputFeedback,
    InputMix,
    InputFbStage,
    InputStereo
};

struct Phase90Ch : public Unit
{
    phase90ch::TriangleLFO lfo;
    phase90ch::Phase90FB2 fb2[2];
    phase90ch::Phase90FB3 fb3[2];
    phase90ch::Phase90FB4 fb4[2];
};

inline float inputAt(Phase90Ch* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clamp01(float x) noexcept
{
    return sc_clip(x, 0.0f, 1.0f);
}

void Phase90Ch_next(Phase90Ch* unit, int inNumSamples)
{
    const float* in = IN(InputIn);
    float* outL = OUT(0);
    const bool stereoMode = IN0(InputStereo) >= 0.5f && unit->mNumOutputs > 1;
    float* outR = stereoMode ? OUT(1) : nullptr;

    for (int i = 0; i < inNumSamples; ++i) {
        const auto rate = sc_clip(inputAt(unit, InputRate, i), 0.1f, 10.0f);
        const auto depth = 0.45f * clamp01(inputAt(unit, InputDepth, i));
        const auto feedback = 0.95f * sc_clip(inputAt(unit, InputFeedback, i), -1.0f, 1.0f);
        const auto wet = clamp01(inputAt(unit, InputMix, i));
        const auto dry = 1.0f - wet;
        const int fbStage = sc_clip((int) std::lround(inputAt(unit, InputFbStage, i)), 2, 4);

        unit->lfo.setFrequency(rate);
        const auto mod = phase90ch::shapeLFO(unit->lfo.processSample());
        const auto mod01L = mod * depth + 0.5f;
        const auto mod01R = stereoMode ? ((-mod) * depth + 0.5f) : mod01L;

        auto processChannel = [&] (int ch, float mod01, float x) noexcept
        {
            float y = 0.0f;
            if (fbStage == 2)
                y = unit->fb2[ch].processSample(x, mod01, feedback);
            else if (fbStage == 3)
                y = unit->fb3[ch].processSample(x, mod01, feedback);
            else
                y = unit->fb4[ch].processSample(x, mod01, feedback);
            return phase90ch::flushSubnormal(wet * y + dry * x);
        };

        const auto left = processChannel(0, mod01L, in[i]);
        outL[i] = left;
        if (stereoMode)
            outR[i] = processChannel(1, mod01R, in[i]);
    }
}

void Phase90Ch_Ctor(Phase90Ch* unit)
{
    new (unit) Phase90Ch;

    const auto sampleRate = (float) SAMPLERATE;
    unit->lfo.reset(sampleRate);
    for (int ch = 0; ch < 2; ++ch) {
        unit->fb2[ch].prepare(sampleRate);
        unit->fb3[ch].prepare(sampleRate);
        unit->fb4[ch].prepare(sampleRate);
    }

    SETCALC(Phase90Ch_next);
    OUT0(0) = 0.0f;
    if (unit->mNumOutputs > 1)
        OUT0(1) = 0.0f;
}
} // namespace

PluginLoad(Phase90Ch)
{
    ft = inTable;
    DefineSimpleUnit(Phase90Ch);
}
