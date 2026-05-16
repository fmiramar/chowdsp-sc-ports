#include "SC_PlugIn.h"

#include "../../shared/waveshapers/waveshapers.hpp"

#include <new>

static InterfaceTable* ft;

namespace
{
struct WestCoastFoldCh : public Unit
{
    waveshapers::ADAAShaper<waveshapers::WestCoastFoldPolicy> shaper;
};

void WestCoastFoldCh_next(WestCoastFoldCh* unit, int inNumSamples)
{
    const auto* in = IN(0);
    auto* out = OUT(0);

    for (int i = 0; i < inNumSamples; ++i)
        out[i] = static_cast<float>(unit->shaper.processSample(static_cast<double>(in[i])));

    unit->shaper.sanitize();
}

void WestCoastFoldCh_Ctor(WestCoastFoldCh* unit)
{
    new (unit) WestCoastFoldCh;
    unit->shaper.reset();
    SETCALC(WestCoastFoldCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(WestCoastFoldCh)
{
    ft = inTable;
    DefineSimpleUnit(WestCoastFoldCh);
}
