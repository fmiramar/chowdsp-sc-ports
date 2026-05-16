#include "SC_PlugIn.h"
#include "dsp/ldr.hpp"

#include <algorithm>
#include <cmath>

static InterfaceTable* ft;

namespace
{
constexpr int maxNumStages = 52;

enum InputIndex
{
    InputIn = 0,
    InputLfo,
    InputSkew,
    InputMod,
    InputStages
};

struct PhaserModCh : public Unit
{
    float a1 = 0.0f;
    float b0 = 1.0f;
    float b1 = 0.0f;
    float z[maxNumStages];
};

inline float inputAt(PhaserModCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float processStage(PhaserModCh* unit, float x, int stage) noexcept
{
    const float y = unit->z[stage] + x * unit->b0;
    unit->z[stage] = x * unit->b1 - y * unit->a1;
    return y;
}

inline void calcCoefs(PhaserModCh* unit, float resistance, float sampleRate) noexcept
{
    constexpr float capacitor = 25.0e-9f;
    const float rc = resistance * capacitor;
    const float k = 2.0f * sampleRate;
    const float a0 = rc * k + 1.0f;

    unit->b0 = (rc * k - 1.0f) / a0;
    unit->b1 = (-rc * k - 1.0f) / a0;
    unit->a1 = (-rc * k + 1.0f) / a0;
}

void PhaserModCh_next(PhaserModCh* unit, int inNumSamples)
{
    float* out = OUT(0);

    for (int i = 0; i < inNumSamples; ++i) {
        const float x = inputAt(unit, InputIn, i);
        const float lfo = inputAt(unit, InputLfo, i) * 0.2f;
        const float skew = std::clamp(inputAt(unit, InputSkew, i), -1.0f, 1.0f);
        const float mod = std::clamp(inputAt(unit, InputMod, i), 0.0f, 1.0f);
        const float stages = std::clamp(inputAt(unit, InputStages, i), 1.0f, 50.0f);

        calcCoefs(unit, phasermodch::ldr::getResistance(lfo, skew), static_cast<float>(SAMPLERATE));

        float y = x;
        const int wholeStages = static_cast<int>(stages);
        for (int stage = 0; stage < wholeStages; ++stage)
            y = processStage(unit, y, stage);

        // Keep the VCV interpolation expression semantics, which always advances
        // the next stage state even when the stage count is an integer.
        const float stageFrac = stages - static_cast<float>(wholeStages);
        const float interpStage = processStage(unit, y, wholeStages);
        y = stageFrac * interpStage + (1.0f - stageFrac) * y;

        out[i] = mod * y + (1.0f - mod) * x;
    }

    for (float& stageState : unit->z)
        stageState = zapgremlins(stageState);
}

void PhaserModCh_Ctor(PhaserModCh* unit)
{
    unit->a1 = 0.0f;
    unit->b0 = 1.0f;
    unit->b1 = 0.0f;
    std::fill(unit->z, unit->z + maxNumStages, 0.0f);

    SETCALC(PhaserModCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(PhaserModCh)
{
    ft = inTable;
    DefineSimpleUnit(PhaserModCh);
}
