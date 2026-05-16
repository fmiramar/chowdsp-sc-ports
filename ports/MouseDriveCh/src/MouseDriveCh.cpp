#include "SC_PlugIn.h"

#include "dsp/MouseDriveWDF.h"
#include "dsp/denormal.hpp"
#include "dsp/filters.hpp"

#include <algorithm>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
enum InputIndex
{
    InputIn = 0,
    InputDistortion,
    InputVolume
};

struct MouseDriveCh : public Unit
{
    mousedrivech::MouseDriveWDF circuit;
    mousedrivech::BiquadFilter dcBlocker;
    mousedrivech::LinearSmoother distortionSmooth;
    mousedrivech::LinearSmoother volumeSmooth;
};

inline float inputAt(MouseDriveCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clamp01(float x) noexcept
{
    return sc_clip(x, 0.0f, 1.0f);
}

inline float distortionToResistance(float distortion) noexcept
{
    const auto clipped = clamp01(distortion);
    return 1.0f + mousedrivech::MouseDriveWDF::Rdistortion * std::pow(clipped, 5.0f);
}

inline float volumeToGain(float volume) noexcept
{
    const auto clipped = clamp01(volume);
    const auto db = -24.0f * (1.0f - clipped) - 12.0f;
    return std::pow(10.0f, db * 0.05f);
}

void MouseDriveCh_next(MouseDriveCh* unit, int inNumSamples)
{
    const auto* in = IN(InputIn);
    auto* out = OUT(0);
    const auto audioDistortion = INRATE(InputDistortion) == calc_FullRate;
    const auto audioVolume = INRATE(InputVolume) == calc_FullRate;

    if (!audioDistortion)
        unit->distortionSmooth.setTarget(distortionToResistance(IN0(InputDistortion)));
    if (!audioVolume)
        unit->volumeSmooth.setTarget(volumeToGain(IN0(InputVolume)));

    for (int i = 0; i < inNumSamples; ++i) {
        if (audioDistortion)
            unit->distortionSmooth.setTarget(distortionToResistance(inputAt(unit, InputDistortion, i)));
        if (audioVolume)
            unit->volumeSmooth.setTarget(volumeToGain(inputAt(unit, InputVolume, i)));

        unit->circuit.setDistortionResistance(unit->distortionSmooth.process());
        auto y = unit->circuit.process(in[i]);
        y *= unit->volumeSmooth.process();
        y = unit->dcBlocker.process(y);
        out[i] = mousedrivech::flushSubnormal(y);
    }
}

void MouseDriveCh_Ctor(MouseDriveCh* unit)
{
    new (unit) MouseDriveCh;

    const auto sampleRate = static_cast<float>(SAMPLERATE);
    unit->circuit.prepare(sampleRate);
    unit->circuit.reset();
    unit->dcBlocker.setHighpass(15.0f / sampleRate, 0.70710678f);
    unit->dcBlocker.reset();
    unit->distortionSmooth.setRampTime(sampleRate, 0.025f);
    unit->volumeSmooth.setRampTime(sampleRate, 0.05f);
    unit->distortionSmooth.reset(distortionToResistance(IN0(InputDistortion)));
    unit->volumeSmooth.reset(volumeToGain(IN0(InputVolume)));
    unit->circuit.setDistortionResistance(unit->distortionSmooth.current);

    SETCALC(MouseDriveCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(MouseDriveCh)
{
    ft = inTable;
    DefineSimpleUnit(MouseDriveCh);
}
