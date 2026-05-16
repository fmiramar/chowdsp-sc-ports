#include "SC_PlugIn.h"

#include "../../shared/reverb/reverb_core.hpp"

#include <new>

InterfaceTable* ft;

namespace
{
using chow_reverb::ConvolutionDiffuserCore;

enum InputIndex
{
    InputLeft = 0,
    InputRight,
    InputDiffusionTime
};

struct ConvDifusserCh : public Unit
{
    ConvolutionDiffuserCore diffuser;
};

inline float diffusionInput(ConvDifusserCh* unit, int sampleIndex) noexcept
{
    return INRATE(InputDiffusionTime) == calc_FullRate ? IN(InputDiffusionTime)[sampleIndex] : IN0(InputDiffusionTime);
}

void ConvDifusserCh_next(ConvDifusserCh* unit, int inNumSamples)
{
    unit->diffuser.setDiffusionTimeMs(sc_clip(diffusionInput(unit, inNumSamples - 1), 1.0f, 1000.0f));
    unit->diffuser.processBlock(IN(InputLeft), IN(InputRight), OUT(0), OUT(1), inNumSamples);
}

void ConvDifusserCh_Ctor(ConvDifusserCh* unit)
{
    new (unit) ConvDifusserCh;

    if (!unit->diffuser.allocate(ft, unit->mWorld, static_cast<float>(SAMPLERATE), BUFLENGTH, 2, 1.0f)) {
        unit->diffuser.free(ft, unit->mWorld);
        ClearUnitOnMemFailed
    }

    SETCALC(ConvDifusserCh_next);
    OUT0(0) = 0.0f;
    OUT0(1) = 0.0f;
}

void ConvDifusserCh_Dtor(ConvDifusserCh* unit)
{
    unit->diffuser.free(ft, unit->mWorld);
}
} // namespace

PluginLoad(ConvDifusserCh)
{
    ft = inTable;
    DefineDtorUnit(ConvDifusserCh);
}
