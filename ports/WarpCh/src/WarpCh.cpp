#include "SC_PlugIn.h"

#include "dsp/newton_raphson.hpp"
#include "dsp/nl_biquad.hpp"
#include "dsp/oversampling.hpp"

#include <algorithm>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr float highFreq = 1000.0f;
constexpr float lowFreq = 5.0f;
constexpr float qMult = 0.25f;
constexpr float qBase = 19.75f;
constexpr float qOff = 0.05f;
constexpr int oversamplingRatio = 2;
constexpr int paramDivide = 16;
constexpr size_t fadeCount = 2048;
constexpr float outputGain = 10.0f;

enum InputIndex
{
    InputIn = 0,
    InputCutoff,
    InputHeat,
    InputWidth,
    InputDrive,
    InputMode
};

struct MappedParams
{
    float freqParam = 0.5f;
    float qParam = 0.324f;
    float gainDb = -6.0f;
    float driveParam = 1.0f;
    float fbDriveParam = 1.0f;
    float fbParam = 0.0f;
};

struct WarpCh : public Unit
{
    Oversampling<oversamplingRatio> oversampling;
    NLBiquad filter;
    DFLFilter<4> nrSolver;
    int samplesUntilCook = 0;
    size_t fadeCounter = 0;
    int prevMode = 0;

    float cutoff = 0.5f;
    float heat = 0.5f;
    float width = 0.5f;
    float drive = 0.0f;
};

inline float inputAt(WarpCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline int modeFromValue(float value) noexcept
{
    int choice = static_cast<int>(value);
    choice = sc_clip(choice, 0, 2);
    return choice;
}

inline void mapParams(int mode, float cutoff, float heat, float width, float drive, MappedParams& mapped) noexcept
{
    mapped.freqParam = cutoff;

    switch (mode) {
    case 0:
        mapped.qParam = heat * 0.85f + 0.15f;
        mapped.driveParam = heat * -9.0f + 10.0f;
        mapped.gainDb = width * 48.0f - 24.0f;
        mapped.fbParam = width * 0.9f;
        mapped.fbDriveParam = drive * 9.0f + 1.0f;
        break;
    case 1:
        mapped.qParam = width * -0.85f + 1.0f;
        mapped.driveParam = heat * 9.0f + 1.0f;
        mapped.gainDb = width * 28.0f - 14.0f;
        mapped.fbParam = heat * 0.9f;
        mapped.fbDriveParam = drive * 9.0f + 1.0f;
        break;
    case 2:
    default:
        mapped.qParam = width * -0.85f + 1.0f;
        mapped.driveParam = drive * 9.0f + 1.0f;
        mapped.gainDb = drive * 24.0f;
        mapped.fbParam = heat * 0.9f;
        mapped.fbDriveParam = heat * 9.0f + 1.0f;
        break;
    }
}

inline void setParams(WarpCh* unit, float cutoff, float heat, float width, float drive, int mode) noexcept
{
    unit->cutoff = std::clamp(cutoff, 0.0f, 1.0f);
    unit->heat = std::clamp(heat, 0.0f, 1.0f);
    unit->width = std::clamp(width, 0.0f, 1.0f);
    unit->drive = std::clamp(drive, 0.0f, 1.0f);

    if (mode != unit->prevMode && unit->fadeCounter == 0) {
        unit->fadeCounter = fadeCount;
        unit->prevMode = mode;
    }

    MappedParams mapped;
    mapParams(mode, unit->cutoff, unit->heat, unit->width, unit->drive, mapped);

    const float freq = std::pow(highFreq / lowFreq, mapped.freqParam) * lowFreq;
    const float q = std::pow(qBase, mapped.qParam) * qMult + qOff;
    const float gain = std::pow(10.0f, mapped.gainDb / 20.0f);

    unit->filter.setPeak(freq / (SAMPLERATE * oversamplingRatio), q, gain);
    unit->filter.setDrive(mapped.driveParam);

    unit->nrSolver.driveParam = mapped.fbDriveParam;
    unit->nrSolver.fbParam = mapped.fbParam;
}

inline float processOS(WarpCh* unit, float x) noexcept
{
    const auto y0y1 = unit->nrSolver.process(x, unit->filter.b[0], unit->filter.z[1]);
    unit->filter.process(y0y1.first);
    return y0y1.second;
}

inline float processSample(WarpCh* unit, float x) noexcept
{
    unit->oversampling.upsample(x);
    float* osBuffer = unit->oversampling.getOSBuffer();

    for (int k = 0; k < oversamplingRatio; ++k)
        osBuffer[k] = processOS(unit, osBuffer[k]);

    float y = unit->oversampling.downsample();

    float gain = outputGain;
    if (unit->fadeCounter > 0) {
        --unit->fadeCounter;
        gain *= 1.0f - ((float) unit->fadeCounter / (float) fadeCount);
    }

    return gain * y;
}

void WarpCh_next_k(WarpCh* unit, int inNumSamples);
void WarpCh_next_a(WarpCh* unit, int inNumSamples);

void WarpCh_init(WarpCh* unit)
{
    unit->oversampling.reset((float) SAMPLERATE);
    unit->filter.reset();
    unit->nrSolver.reset();
    unit->samplesUntilCook = paramDivide;
    unit->prevMode = modeFromValue(IN0(InputMode));

    setParams(unit, IN0(InputCutoff), IN0(InputHeat), IN0(InputWidth), IN0(InputDrive), unit->prevMode);

    const bool hasAudioRateParams = INRATE(InputCutoff) == calc_FullRate || INRATE(InputHeat) == calc_FullRate
        || INRATE(InputWidth) == calc_FullRate || INRATE(InputDrive) == calc_FullRate || INRATE(InputMode) == calc_FullRate;

    if (hasAudioRateParams)
        SETCALC(WarpCh_next_a);
    else
        SETCALC(WarpCh_next_k);

    OUT0(0) = 0.0f;
}

void WarpCh_next_k(WarpCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);

    const float cutoff = IN0(InputCutoff);
    const float heat = IN0(InputHeat);
    const float width = IN0(InputWidth);
    const float drive = IN0(InputDrive);
    const int mode = modeFromValue(IN0(InputMode));

    for (int i = 0; i < inNumSamples; ++i) {
        if (--unit->samplesUntilCook <= 0) {
            setParams(unit, cutoff, heat, width, drive, mode);
            unit->samplesUntilCook = paramDivide;
        }

        out[i] = processSample(unit, in[i]);
    }
}

void WarpCh_next_a(WarpCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);

    for (int i = 0; i < inNumSamples; ++i) {
        if (--unit->samplesUntilCook <= 0) {
            setParams(unit,
                      inputAt(unit, InputCutoff, i),
                      inputAt(unit, InputHeat, i),
                      inputAt(unit, InputWidth, i),
                      inputAt(unit, InputDrive, i),
                      modeFromValue(inputAt(unit, InputMode, i)));
            unit->samplesUntilCook = paramDivide;
        }

        out[i] = processSample(unit, in[i]);
    }
}

void WarpCh_Ctor(WarpCh* unit)
{
    // Default-initialize to preserve the server-populated Unit base while constructing C++ members.
    new (unit) WarpCh;
    WarpCh_init(unit);
}
} // namespace

PluginLoad(WarpCh)
{
    ft = inTable;
    DefineSimpleUnit(WarpCh);
}
