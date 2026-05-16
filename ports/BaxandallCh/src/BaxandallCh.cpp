#include "SC_PlugIn.h"

#include "dsp/BaxandallWDF.h"
#include "dsp/BaxandallWDFAdapt.h"
#include "dsp/denormal.hpp"
#include "dsp/oversampling.hpp"
#include "dsp/smoothed_value.hpp"

#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr int oversamplingRatio = 2;
constexpr float outputGain = 11.220184543019636f; // +21 dB

enum InputIndex
{
    InputIn = 0,
    InputBass,
    InputTreble,
    InputMode
};

struct BaxandallCh : public Unit
{
    Oversampling<oversamplingRatio> oversampling;
    BaxandallWDF wdfUnadapted;
    BaxandallWDFAdapt wdfAdapted;
    centaurch::SmoothedValue<float, centaurch::ValueSmoothingTypes::Linear> bassSmooth { 0.5f };
    centaurch::SmoothedValue<float, centaurch::ValueSmoothingTypes::Linear> trebleSmooth { 0.5f };
    int prevMode = 0;
};

inline float inputAt(BaxandallCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clampNormalParam(float x) noexcept
{
    return sc_clip(x, 0.0f, 1.0f);
}

inline int clampMode(float x) noexcept
{
    return sc_clip((int) std::lround(x), 0, 1);
}

inline float skewParam(float val) noexcept
{
    val = std::pow(clampNormalParam(val), 3.333f);
    return sc_clip(1.0f - val, 0.01f, 0.99f);
}

inline void resetForModeChange(BaxandallCh* unit, int mode) noexcept
{
    if (mode == unit->prevMode)
        return;

    unit->wdfUnadapted.reset();
    unit->wdfAdapted.reset();
    unit->prevMode = mode;
}

template <typename Fn>
inline void withWdf(BaxandallCh* unit, int mode, Fn&& fn) noexcept
{
    if (mode == 0)
        fn(unit->wdfUnadapted);
    else
        fn(unit->wdfAdapted);
}

void BaxandallCh_next(BaxandallCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);
    auto* osBuffer = unit->oversampling.getOSBuffer();

    const auto audioBass = INRATE(InputBass) == calc_FullRate;
    const auto audioTreble = INRATE(InputTreble) == calc_FullRate;
    const auto audioMode = INRATE(InputMode) == calc_FullRate;

    if (!audioBass)
        unit->bassSmooth.setTargetValue(skewParam(IN0(InputBass)));
    if (!audioTreble)
        unit->trebleSmooth.setTargetValue(skewParam(IN0(InputTreble)));

    if (!audioMode)
        resetForModeChange(unit, clampMode(IN0(InputMode)));

    const auto smoothBass = !audioBass && unit->bassSmooth.isSmoothing();
    const auto smoothTreble = !audioTreble && unit->trebleSmooth.isSmoothing();
    const auto needsPerSampleParams = audioBass || audioTreble || smoothBass || smoothTreble;

    if (!audioMode && !needsPerSampleParams) {
        const auto bass = unit->bassSmooth.getNextValue();
        const auto treble = unit->trebleSmooth.getNextValue();

        withWdf(unit, unit->prevMode, [&](auto& wdf) {
            wdf.setParams(bass, treble);

            for (int i = 0; i < inNumSamples; ++i) {
                unit->oversampling.upsample(in[i]);
                for (int j = 0; j < oversamplingRatio; ++j)
                    osBuffer[j] = wdf.processSample(osBuffer[j]);

                out[i] = delaybb::flushSubnormal(unit->oversampling.downsample() * outputGain);
            }
        });

        return;
    }

    for (int i = 0; i < inNumSamples; ++i) {
        const auto mode = audioMode ? clampMode(inputAt(unit, InputMode, i)) : unit->prevMode;
        resetForModeChange(unit, mode);

        unit->oversampling.upsample(in[i]);
        withWdf(unit, mode, [&](auto& wdf) {
            const auto audioBassValue = audioBass ? skewParam(inputAt(unit, InputBass, i)) : 0.0f;
            const auto audioTrebleValue = audioTreble ? skewParam(inputAt(unit, InputTreble, i)) : 0.0f;

            for (int j = 0; j < oversamplingRatio; ++j) {
                const auto bass = audioBass ? audioBassValue : (smoothBass ? unit->bassSmooth.getNextValue() : unit->bassSmooth.getCurrentValue());
                const auto treble = audioTreble ? audioTrebleValue : (smoothTreble ? unit->trebleSmooth.getNextValue() : unit->trebleSmooth.getCurrentValue());
                wdf.setParams(bass, treble);
                osBuffer[j] = wdf.processSample(osBuffer[j]);
            }
        });

        out[i] = delaybb::flushSubnormal(unit->oversampling.downsample() * outputGain);
    }
}

void BaxandallCh_Ctor(BaxandallCh* unit)
{
    new (unit) BaxandallCh;

    const auto sampleRate = static_cast<float>(SAMPLERATE);
    const auto osSampleRate = sampleRate * (float) oversamplingRatio;

    unit->oversampling.reset(sampleRate);
    unit->wdfUnadapted.prepare(osSampleRate);
    unit->wdfAdapted.prepare(osSampleRate);
    unit->bassSmooth.reset(osSampleRate, 0.05);
    unit->trebleSmooth.reset(osSampleRate, 0.05);
    unit->bassSmooth.setCurrentAndTargetValue(skewParam(IN0(InputBass)));
    unit->trebleSmooth.setCurrentAndTargetValue(skewParam(IN0(InputTreble)));
    unit->prevMode = clampMode(IN0(InputMode));

    SETCALC(BaxandallCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(BaxandallCh)
{
    ft = inTable;
    DefineSimpleUnit(BaxandallCh);
}
