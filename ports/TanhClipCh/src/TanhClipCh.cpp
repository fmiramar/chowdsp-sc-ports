#include "SC_PlugIn.h"

#include "../../shared/waveshapers/waveshapers.hpp"

#include <new>

static InterfaceTable* ft;

namespace
{
struct TanhClipCh : public Unit
{
    waveshapers::ADAAShaper<waveshapers::TanhClipPolicy> shaper;
};

void TanhClipCh_next(TanhClipCh* unit, int inNumSamples)
{
    const auto* in = IN(0);
    auto* out = OUT(0);

    for (int i = 0; i < inNumSamples; ++i)
        out[i] = static_cast<float>(unit->shaper.processSample(static_cast<double>(in[i])));

    unit->shaper.sanitize();
}

void TanhClipCh_Ctor(TanhClipCh* unit)
{
    new (unit) TanhClipCh;
    unit->shaper.reset();
    SETCALC(TanhClipCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(TanhClipCh)
{
    ft = inTable;
    DefineSimpleUnit(TanhClipCh);
}
