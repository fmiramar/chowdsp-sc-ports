#include "SC_PlugIn.h"

#include "dsp/clipping_stage.hpp"
#include "dsp/oversampling.hpp"
#include "dsp/shelf_filter.hpp"

#include <algorithm>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
enum InputIndex
{
    InputIn = 0,
    InputBass,
    InputTreble,
    InputDrive,
    InputBias
};

struct DistortCh : public Unit
{
    Oversampling<2> oversampling;
    ShelfFilter shelfFilter;
    ClippingStage clipper;

    float bass = 0.0f;
    float treble = 0.0f;
    float drive = 0.5f;
    float bias = 0.0f;
    float driveGain = 1.0f;
    float biasOffset = 0.0f;
};

inline void setParams(DistortCh* unit, float bass, float treble, float drive, float bias) noexcept;
void DistortCh_next_k(DistortCh* unit, int inNumSamples);
void DistortCh_next_a(DistortCh* unit, int inNumSamples);

void DistortCh_init(DistortCh* unit)
{
    unit->oversampling.reset(SAMPLERATE);
    unit->shelfFilter.reset();
    unit->clipper.prepare((float) (SAMPLERATE * 2.0));

    setParams(unit, IN0(InputBass), IN0(InputTreble), IN0(InputDrive), IN0(InputBias));

    const bool hasAudioRateParams = INRATE(InputBass) == calc_FullRate || INRATE(InputTreble) == calc_FullRate
        || INRATE(InputDrive) == calc_FullRate || INRATE(InputBias) == calc_FullRate;

    if (hasAudioRateParams)
        SETCALC(DistortCh_next_a);
    else
        SETCALC(DistortCh_next_k);

    OUT0(0) = 0.0f;
}

inline float dbToAmp(float db) noexcept
{
    return std::pow(10.0f, db / 20.0f);
}

inline float inputAt(DistortCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline void setParams(DistortCh* unit, float bass, float treble, float drive, float bias) noexcept
{
    unit->bass = std::clamp(bass, -1.0f, 1.0f);
    unit->treble = std::clamp(treble, -1.0f, 1.0f);
    unit->drive = std::clamp(drive, 0.0f, 1.0f);
    unit->bias = std::clamp(bias, 0.0f, 1.0f);

    const float lowGain = dbToAmp(unit->bass * 9.0f - 20.0f);
    const float highGain = dbToAmp(unit->treble * 9.0f - 20.0f);
    unit->shelfFilter.calcCoefs(lowGain, highGain, 600.0f, SAMPLERATE);

    unit->driveGain = dbToAmp(unit->drive * 30.0f);
    unit->biasOffset = unit->bias * 2.5f;
}

inline float processSample(DistortCh* unit, float x) noexcept
{
    x = unit->driveGain * unit->shelfFilter.process(x) + unit->biasOffset;

    unit->oversampling.upsample(x);
    auto* osBuffer = unit->oversampling.getOSBuffer();

    for (int k = 0; k < 2; ++k)
        osBuffer[k] = unit->clipper.processSample(osBuffer[k]);

    return unit->oversampling.downsample();
}

void DistortCh_next_k(DistortCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);

    const float nextBass = IN0(InputBass);
    const float nextTreble = IN0(InputTreble);
    const float nextDrive = IN0(InputDrive);
    const float nextBias = IN0(InputBias);

    if (nextBass != unit->bass || nextTreble != unit->treble || nextDrive != unit->drive || nextBias != unit->bias)
        setParams(unit, nextBass, nextTreble, nextDrive, nextBias);

    for (int i = 0; i < inNumSamples; ++i)
        out[i] = processSample(unit, in[i]);
}

void DistortCh_next_a(DistortCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);

    float lastBass = unit->bass;
    float lastTreble = unit->treble;
    float lastDrive = unit->drive;
    float lastBias = unit->bias;

    for (int i = 0; i < inNumSamples; ++i) {
        lastBass = inputAt(unit, InputBass, i);
        lastTreble = inputAt(unit, InputTreble, i);
        lastDrive = inputAt(unit, InputDrive, i);
        lastBias = inputAt(unit, InputBias, i);

        setParams(unit, lastBass, lastTreble, lastDrive, lastBias);
        out[i] = processSample(unit, in[i]);
    }

    setParams(unit, lastBass, lastTreble, lastDrive, lastBias);
}

void DistortCh_Ctor(DistortCh* unit)
{
    // Default-initialize to preserve the server-populated Unit base while constructing C++ members.
    new (unit) DistortCh;
    DistortCh_init(unit);
}
} // namespace

PluginLoad(DistortCh)
{
    ft = inTable;
    DefineSimpleUnit(DistortCh);
}
