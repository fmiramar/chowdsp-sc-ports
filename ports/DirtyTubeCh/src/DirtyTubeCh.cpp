#include "SC_PlugIn.h"

#include "dsp/TubeProc.h"
#include "dsp/denormal.hpp"

#include <algorithm>
#include <cmath>
#include <new>
#include <memory>

static InterfaceTable* ft;

namespace
{
enum InputIndex
{
    InputIn = 0,
    InputDrive
};

struct DirtyTubeCh : public Unit
{
    std::unique_ptr<dirtytubech::TubeProc> tube;
};

inline float inputAt(DirtyTubeCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clamp01(float x) noexcept
{
    return sc_clip(x, 0.0f, 1.0f);
}

void DirtyTubeCh_next(DirtyTubeCh* unit, int inNumSamples)
{
    const auto* in = IN(InputIn);
    auto* out = OUT(0);

    for (int i = 0; i < inNumSamples; ++i) {
        const auto drive = clamp01(inputAt(unit, InputDrive, i));
        const auto driveAmtSkew = std::pow(drive, 8.0f);
        const auto driveGain = 5.9f * driveAmtSkew + 0.1f;
        const auto invDriveGain = 1.0f / (5.0f * driveAmtSkew + 1.0f);
        const auto y = unit->tube->processSample(static_cast<double>(in[i] * driveGain));
        out[i] = dirtytubech::flushSubnormal(static_cast<float>(y * static_cast<double>(invDriveGain)));
    }
}

void DirtyTubeCh_Ctor(DirtyTubeCh* unit)
{
    new (unit) DirtyTubeCh;
    unit->tube = std::make_unique<dirtytubech::TubeProc>(static_cast<double>(SAMPLERATE));
    unit->tube->reset();

    SETCALC(DirtyTubeCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(DirtyTubeCh)
{
    ft = inTable;
    DefineSimpleUnit(DirtyTubeCh);
}
