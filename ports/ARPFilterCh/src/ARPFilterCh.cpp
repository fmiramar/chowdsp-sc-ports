#include "SC_PlugIn.h"

#include "dsp/arp_filter.hpp"

#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr float notchSmoothSeconds = 0.02f;

enum InputIndex
{
    InputIn = 0,
    InputFreq,
    InputQ,
    InputLimitMode,
    InputNotchOffset,
    InputMode
};

struct ARPFilterCh : public Unit
{
    arpfilter::ARPFilter filter;
    arpfilter::LinearSmoother notchSmoother;
};

inline float inputAt(ARPFilterCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clampFreq(float freq, float sampleRate) noexcept
{
    return sc_clip(freq, 20.0f, sampleRate * 0.499f);
}

inline float clampQ(float q) noexcept
{
    return sc_clip(q, 0.5f, 2.5f);
}

inline float clampNotchOffset(float notchOffset) noexcept
{
    return sc_clip(notchOffset, 0.0f, 1.0f);
}

inline arpfilter::Mode modeFromValue(float value) noexcept
{
    const int modeIndex = sc_clip(static_cast<int>(std::lround(value)), 0, 3);
    switch (modeIndex) {
        case 0:
            return arpfilter::Mode::Lowpass;
        case 1:
            return arpfilter::Mode::Bandpass;
        case 2:
            return arpfilter::Mode::Highpass;
        case 3:
        default:
            return arpfilter::Mode::Notch;
    }
}

void ARPFilterCh_next(ARPFilterCh* unit, int inNumSamples)
{
    const float sampleRate = static_cast<float>(SAMPLERATE);
    const bool audioRateNotch = INRATE(InputNotchOffset) == calc_FullRate;
    float* out = OUT(0);
    const float* in = IN(InputIn);

    if (!audioRateNotch)
        unit->notchSmoother.setTarget(clampNotchOffset(IN0(InputNotchOffset)), notchSmoothSeconds);

    for (int i = 0; i < inNumSamples; ++i) {
        unit->filter.setCutoffFrequency(clampFreq(inputAt(unit, InputFreq, i), sampleRate));
        unit->filter.setQValue(clampQ(inputAt(unit, InputQ, i)));
        unit->filter.setLimitMode(inputAt(unit, InputLimitMode, i) > 0.5f);

        const float notchOffset = audioRateNotch ? clampNotchOffset(IN(InputNotchOffset)[i]) : unit->notchSmoother.process();
        out[i] = unit->filter.process(in[i], modeFromValue(inputAt(unit, InputMode, i)), notchOffset);
    }
}

void ARPFilterCh_Ctor(ARPFilterCh* unit)
{
    new (unit) ARPFilterCh;

    unit->filter.prepare(static_cast<float>(SAMPLERATE));
    unit->filter.reset();
    unit->notchSmoother.reset(static_cast<float>(SAMPLERATE), clampNotchOffset(IN0(InputNotchOffset)));

    SETCALC(ARPFilterCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(ARPFilterCh)
{
    ft = inTable;
    DefineSimpleUnit(ARPFilterCh);
}
