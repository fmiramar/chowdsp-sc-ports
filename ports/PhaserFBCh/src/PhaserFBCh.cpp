#include "SC_PlugIn.h"
#include "dsp/ldr.hpp"

#include <algorithm>
#include <cmath>

static InterfaceTable* ft;

namespace
{
enum InputIndex
{
    InputIn = 0,
    InputLfo,
    InputSkew,
    InputFeedback
};

struct PhaserFBCh : public Unit
{
    float a1 = 0.0f;
    float a2 = 0.0f;
    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float z1 = 0.0f;
    float z2 = 0.0f;
};

inline float inputAt(PhaserFBCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float calcPoleFreq(float a0s, float a1s, float a2s) noexcept
{
    const float radicand = a1s * a1s - 4.0f * a0s * a2s;
    return radicand >= 0.0f ? 0.0f : std::sqrt(-radicand) / (2.0f * a0s);
}

inline void calcCoefs(PhaserFBCh* unit, float resistance, float feedback, float sampleRate) noexcept
{
    constexpr float capacitor = 15.0e-9f;
    const float rc = resistance * capacitor;

    const float b0s = rc * rc;
    const float b1s = -2.0f * rc;
    const float b2s = 1.0f;
    const float a0s = b0s * (1.0f + feedback);
    const float a1s = -b1s * (1.0f - feedback);
    const float a2s = 1.0f + feedback;

    const float wc = calcPoleFreq(a0s, a1s, a2s);
    const float k = wc == 0.0f ? 2.0f * sampleRate : wc / std::tan(wc / (2.0f * sampleRate));
    const float kSq = k * k;

    const float a0 = a0s * kSq + a1s * k + a2s;
    unit->a1 = 2.0f * (a2s - a0s * kSq) / a0;
    unit->a2 = (a0s * kSq - a1s * k + a2s) / a0;
    unit->b0 = (b0s * kSq + b1s * k + b2s) / a0;
    unit->b1 = 2.0f * (b2s - b0s * kSq) / a0;
    unit->b2 = (b0s * kSq - b1s * k + b2s) / a0;
}

inline float processFilter(PhaserFBCh* unit, float x) noexcept
{
    const float y = unit->z1 + x * unit->b0;
    unit->z1 = unit->z2 + x * unit->b1 - y * unit->a1;
    unit->z2 = x * unit->b2 - y * unit->a2;
    return y;
}

void PhaserFBCh_next(PhaserFBCh* unit, int inNumSamples)
{
    float* out = OUT(0);

    for (int i = 0; i < inNumSamples; ++i) {
        const float x = inputAt(unit, InputIn, i);
        const float lfo = inputAt(unit, InputLfo, i) * 0.2f;
        const float skew = std::clamp(inputAt(unit, InputSkew, i), -1.0f, 1.0f);
        const float feedback = -std::clamp(inputAt(unit, InputFeedback, i), 0.0f, 0.95f);

        calcCoefs(unit, phaserfbch::ldr::getResistance(lfo, skew), feedback, static_cast<float>(SAMPLERATE));

        const float y = processFilter(unit, x);
        out[i] = std::tanh(y * 0.2f) * 5.0f;
    }

    unit->z1 = zapgremlins(unit->z1);
    unit->z2 = zapgremlins(unit->z2);
}

void PhaserFBCh_Ctor(PhaserFBCh* unit)
{
    unit->a1 = 0.0f;
    unit->a2 = 0.0f;
    unit->b0 = 1.0f;
    unit->b1 = 0.0f;
    unit->b2 = 0.0f;
    unit->z1 = 0.0f;
    unit->z2 = 0.0f;

    SETCALC(PhaserFBCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(PhaserFBCh)
{
    ft = inTable;
    DefineSimpleUnit(PhaserFBCh);
}
