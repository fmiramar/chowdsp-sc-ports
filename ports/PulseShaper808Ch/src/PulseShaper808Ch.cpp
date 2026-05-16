#include "SC_PlugIn.h"

#include "dsp/pulse_shaper.hpp"

#include <algorithm>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr float widthBase = 300.0f;
constexpr float widthMult = 1.0f;

constexpr float decayBase = 30.0f;
constexpr float decayMult = 10.0f;

constexpr float capVal = 0.015e-6f;
constexpr float triggerHigh = 1.0f;
constexpr float triggerLow = 0.1f;

enum InputIndex
{
    InputTrig = 0,
    InputWidth,
    InputDecay,
    InputDoubleTap
};

struct PulseShaper808Ch : public Unit
{
    PulseShaper pulseShaper;

    bool triggerArmed = true;
    int pulseWidthSamples = 0;
    int sampleCount = 0;

    float width = 0.5f;
    float decay = 0.5f;
    float doubleTap = 0.0f;
    float doubleTapGain = 0.0f;
};

inline void setParams(PulseShaper808Ch* unit, float width, float decay, float doubleTap) noexcept;
void PulseShaper808Ch_next_k(PulseShaper808Ch* unit, int inNumSamples);
void PulseShaper808Ch_next_a(PulseShaper808Ch* unit, int inNumSamples);

void PulseShaper808Ch_init(PulseShaper808Ch* unit)
{
    unit->pulseShaper.prepare((float) SAMPLERATE);
    unit->triggerArmed = true;
    unit->pulseWidthSamples = 0;
    unit->sampleCount = 0;

    setParams(unit, IN0(InputWidth), IN0(InputDecay), IN0(InputDoubleTap));

    const bool hasAudioRateParams = INRATE(InputWidth) == calc_FullRate || INRATE(InputDecay) == calc_FullRate
        || INRATE(InputDoubleTap) == calc_FullRate;

    if (hasAudioRateParams)
        SETCALC(PulseShaper808Ch_next_a);
    else
        SETCALC(PulseShaper808Ch_next_k);

    OUT0(0) = 0.0f;
}

inline float inputAt(PulseShaper808Ch* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline void setParams(PulseShaper808Ch* unit, float width, float decay, float doubleTap) noexcept
{
    unit->width = std::clamp(width, 0.0f, 1.0f);
    unit->decay = std::clamp(decay, 0.0f, 1.0f);
    unit->doubleTap = std::clamp(doubleTap, -1.0f, 1.0f);

    const float pulseWidthMs = std::pow(widthBase, unit->width) * widthMult;
    unit->pulseWidthSamples = std::max(1, (int) std::lround((pulseWidthMs / 1000.0f) * SAMPLERATE));

    const float decayTimeMs = std::pow(decayBase, unit->decay) * decayMult;
    const float r162 = (decayTimeMs / 1000.0f) / capVal;
    const float r163 = r162 * 200.0f;
    unit->pulseShaper.setResistors(r162, r163);

    unit->doubleTapGain = -2.0f * unit->doubleTap;
}

inline float nextPulse(PulseShaper808Ch* unit, float trig) noexcept
{
    if (unit->triggerArmed) {
        if (trig >= triggerHigh) {
            unit->sampleCount = unit->pulseWidthSamples;
            unit->triggerArmed = false;
        }
    } else if (trig <= triggerLow) {
        unit->triggerArmed = true;
    }

    const float pulse = unit->sampleCount > 0 ? 1.0f : 0.0f;
    if (unit->sampleCount > 0)
        --unit->sampleCount;

    return pulse;
}

inline float processSample(PulseShaper808Ch* unit, float trig) noexcept
{
    const float pulse = nextPulse(unit, trig);
    float env = unit->pulseShaper.processSample(pulse);
    env = env > 0.0f ? env : env * unit->doubleTapGain;
    return env * 10.0f;
}

void PulseShaper808Ch_next_k(PulseShaper808Ch* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* trig = IN(InputTrig);

    const float nextWidth = IN0(InputWidth);
    const float nextDecay = IN0(InputDecay);
    const float nextDoubleTap = IN0(InputDoubleTap);

    if (nextWidth != unit->width || nextDecay != unit->decay || nextDoubleTap != unit->doubleTap)
        setParams(unit, nextWidth, nextDecay, nextDoubleTap);

    for (int i = 0; i < inNumSamples; ++i)
        out[i] = processSample(unit, trig[i]);
}

void PulseShaper808Ch_next_a(PulseShaper808Ch* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* trig = IN(InputTrig);

    float lastWidth = unit->width;
    float lastDecay = unit->decay;
    float lastDoubleTap = unit->doubleTap;

    for (int i = 0; i < inNumSamples; ++i) {
        lastWidth = inputAt(unit, InputWidth, i);
        lastDecay = inputAt(unit, InputDecay, i);
        lastDoubleTap = inputAt(unit, InputDoubleTap, i);

        setParams(unit, lastWidth, lastDecay, lastDoubleTap);
        out[i] = processSample(unit, trig[i]);
    }

    setParams(unit, lastWidth, lastDecay, lastDoubleTap);
}

void PulseShaper808Ch_Ctor(PulseShaper808Ch* unit)
{
    // Default-initialize to preserve the server-populated Unit base while constructing C++ members.
    new (unit) PulseShaper808Ch;
    PulseShaper808Ch_init(unit);
}
} // namespace

PluginLoad(PulseShaper808Ch)
{
    ft = inTable;
    DefineSimpleUnit(PulseShaper808Ch);
}
