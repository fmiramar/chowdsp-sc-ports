#include "SC_PlugIn.h"

#include "../../shared/waveshapers/waveshapers.hpp"

#include <new>

static InterfaceTable* ft;

namespace
{
struct HardClipCh : public Unit
{
    waveshapers::ADAAShaper<waveshapers::HardClipPolicy> shaper;
};

void HardClipCh_next(HardClipCh* unit, int inNumSamples)
{
    const auto* in = IN(0);
    auto* out = OUT(0);

    for (int i = 0; i < inNumSamples; ++i)
        out[i] = static_cast<float>(unit->shaper.processSample(static_cast<double>(in[i])));

    unit->shaper.sanitize();
}

void HardClipCh_Ctor(HardClipCh* unit)
{
    new (unit) HardClipCh;
    unit->shaper.reset();
    SETCALC(HardClipCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(HardClipCh)
{
    ft = inTable;
    DefineSimpleUnit(HardClipCh);
}
