#include "SC_PlugIn.h"

#include "../../shared/waveshapers/waveshapers.hpp"

#include <new>

static InterfaceTable* ft;

namespace
{
struct WaveMultiplierCh : public Unit
{
    waveshapers::CascadedWaveMultiplier<6> shaper;
};

void WaveMultiplierCh_next(WaveMultiplierCh* unit, int inNumSamples)
{
    const auto* in = IN(0);
    auto* out = OUT(0);

    for (int i = 0; i < inNumSamples; ++i)
        out[i] = static_cast<float>(unit->shaper.processSample(static_cast<double>(in[i])));

    unit->shaper.sanitize();
}

void WaveMultiplierCh_Ctor(WaveMultiplierCh* unit)
{
    new (unit) WaveMultiplierCh;
    unit->shaper.reset();
    SETCALC(WaveMultiplierCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(WaveMultiplierCh)
{
    ft = inTable;
    DefineSimpleUnit(WaveMultiplierCh);
}
