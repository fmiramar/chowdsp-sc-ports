#include "SC_PlugIn.h"

#include "dsp/HighPassLadder.h"
#include "dsp/LowPassLadder.h"
#include "dsp/denormal.hpp"
#include "dsp/utility.h"

#include <new>

static InterfaceTable* ft;

namespace
{
constexpr float resonanceLimitNonOscillating = 0.96f;

enum InputIndex
{
    InputIn = 0,
    InputHpCutoff,
    InputHpResonance,
    InputLpCutoff,
    InputLpResonance,
    InputDrive,
    InputOscillating
};

struct VogueLadderCh : public Unit
{
    HighPassLadder hp[2];
    LowPassLadder lp[2];
};

inline float inputAt(VogueLadderCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clamp01(float x) noexcept
{
    return sc_clip(x, 0.0f, 1.0f);
}

inline float mapDrive(float norm) noexcept
{
    const auto decibels = vogue_ladder_utility::mapLinearNormalized((double) clamp01(norm), -24.0, 24.0);
    return (float) vogue_ladder_utility::decibelToRawGain(decibels);
}

inline float mapCutoff(float norm) noexcept
{
    const auto controlVoltage = vogue_ladder_utility::mapLinearNormalized((double) clamp01(norm), -5.0, 5.0);
    const auto cutoff = vogue_ladder_utility::voltToFreq(controlVoltage);
    return (float) sc_clip((float) cutoff,
                           (float) vogue_ladder_utility::MIN_FILTER_FREQ,
                           (float) vogue_ladder_utility::MAX_FILTER_FREQ);
}

inline float mapResonance(float norm, bool oscillating) noexcept
{
    auto clipped = clamp01(norm);
    if (!oscillating)
        clipped = (float) vogue_ladder_utility::limitUpper((double) clipped, (double) resonanceLimitNonOscillating);

    auto resonance = vogue_ladder_utility::skewNormalized((double) clipped, 0.33);
    resonance = vogue_ladder_utility::mapLinearNormalized(resonance, 0.0, 4.0);
    return (float) resonance;
}

void VogueLadderCh_next(VogueLadderCh* unit, int inNumSamples)
{
    const float* in = IN(InputIn);
    const bool stereo = unit->mNumOutputs > 1;
    float* outL = OUT(0);
    float* outR = stereo ? OUT(1) : nullptr;

    const auto hpCutAudio = INRATE(InputHpCutoff) == calc_FullRate;
    const auto hpResAudio = INRATE(InputHpResonance) == calc_FullRate;
    const auto lpCutAudio = INRATE(InputLpCutoff) == calc_FullRate;
    const auto lpResAudio = INRATE(InputLpResonance) == calc_FullRate;
    const auto driveAudio = INRATE(InputDrive) == calc_FullRate;
    const auto oscAudio = INRATE(InputOscillating) == calc_FullRate;

    if (!hpCutAudio && !hpResAudio && !lpCutAudio && !lpResAudio && !driveAudio && !oscAudio) {
        const bool oscillating = IN0(InputOscillating) >= 0.5f;
        const auto hpCutoff = mapCutoff(IN0(InputHpCutoff));
        const auto hpResonance = mapResonance(IN0(InputHpResonance), oscillating);
        const auto lpCutoff = mapCutoff(IN0(InputLpCutoff));
        const auto lpResonance = mapResonance(IN0(InputLpResonance), oscillating);
        const auto drive = mapDrive(IN0(InputDrive));
        const auto driveNorm = clamp01(IN0(InputDrive));

        for (int ch = 0; ch < (stereo ? 2 : 1); ++ch) {
            unit->hp[ch].setCutoff(hpCutoff);
            unit->hp[ch].setResonance(hpResonance);
            unit->lp[ch].setCutoff(lpCutoff);
            unit->lp[ch].setResonance(lpResonance);
        }

        for (int i = 0; i < inNumSamples; ++i) {
            for (int ch = 0; ch < (stereo ? 2 : 1); ++ch) {
                float x = in[i];
                x = (float) unit->hp[ch].process(x);
                x = (float) unit->lp[ch].process(x);
                x *= drive;
                x = driveNorm * vogue_ladder_utility::fastTanh2(x) + (1.0f - driveNorm) * x;
                x = delaybb::flushSubnormal(x);

                if (ch == 0)
                    outL[i] = x;
                else
                    outR[i] = x;
            }
        }
        return;
    }

    for (int i = 0; i < inNumSamples; ++i) {
        const bool oscillating = (oscAudio ? inputAt(unit, InputOscillating, i) : IN0(InputOscillating)) >= 0.5f;
        const auto hpCutoff = mapCutoff(hpCutAudio ? inputAt(unit, InputHpCutoff, i) : IN0(InputHpCutoff));
        const auto hpResonance = mapResonance(hpResAudio ? inputAt(unit, InputHpResonance, i) : IN0(InputHpResonance), oscillating);
        const auto lpCutoff = mapCutoff(lpCutAudio ? inputAt(unit, InputLpCutoff, i) : IN0(InputLpCutoff));
        const auto lpResonance = mapResonance(lpResAudio ? inputAt(unit, InputLpResonance, i) : IN0(InputLpResonance), oscillating);
        const auto driveInput = driveAudio ? inputAt(unit, InputDrive, i) : IN0(InputDrive);
        const auto drive = mapDrive(driveInput);
        const auto driveNorm = clamp01(driveInput);

        for (int ch = 0; ch < (stereo ? 2 : 1); ++ch) {
            unit->hp[ch].setCutoff(hpCutoff);
            unit->hp[ch].setResonance(hpResonance);
            unit->lp[ch].setCutoff(lpCutoff);
            unit->lp[ch].setResonance(lpResonance);

            float x = in[i];
            x = (float) unit->hp[ch].process(x);
            x = (float) unit->lp[ch].process(x);
            x *= drive;
            x = driveNorm * vogue_ladder_utility::fastTanh2(x) + (1.0f - driveNorm) * x;
            x = delaybb::flushSubnormal(x);

            if (ch == 0)
                outL[i] = x;
            else
                outR[i] = x;
        }
    }
}

void VogueLadderCh_Ctor(VogueLadderCh* unit)
{
    new (unit) VogueLadderCh;

    const auto sampleRate = (double) SAMPLERATE;
    for (auto& filter : unit->hp)
        filter.reset(sampleRate);
    for (auto& filter : unit->lp)
        filter.reset(sampleRate);

    SETCALC(VogueLadderCh_next);

    OUT0(0) = 0.0f;
    if (unit->mNumOutputs > 1)
        OUT0(1) = 0.0f;
}
} // namespace

PluginLoad(VogueLadderCh)
{
    ft = inTable;
    DefineSimpleUnit(VogueLadderCh);
}
