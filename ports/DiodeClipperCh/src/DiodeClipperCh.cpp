#include "SC_PlugIn.h"

#include "dsp/DiodeClipper.h"
#include "dsp/denormal.hpp"
#include "dsp/oversampling.hpp"

#include <algorithm>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr int oversamplingRatio = 2;

enum InputIndex
{
    InputIn = 0,
    InputCutoff,
    InputGainDB,
    InputOutDB
};

struct DiodeClipperCh : public Unit
{
    Oversampling<oversamplingRatio> oversampling;
    DiodeClipper circuit;
    float currentGain = 1.0f;
    float currentOutGain = 1.0f;
};

inline float inputAt(DiodeClipperCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float dbToGain(float db) noexcept
{
    return std::pow(10.0f, db * 0.05f);
}

inline float clampGainDB(float db) noexcept
{
    return sc_clip(db, 0.0f, 30.0f);
}

inline float clampOutDB(float db) noexcept
{
    return sc_clip(db, -30.0f, 30.0f);
}

inline float clampCutoff(float cutoff, float sampleRate) noexcept
{
    return sc_clip(cutoff, 20.0f, std::min(20000.0f, sampleRate * 0.49f));
}

void DiodeClipperCh_next(DiodeClipperCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);
    auto* osBuffer = unit->oversampling.getOSBuffer();

    const auto sampleRate = static_cast<float>(SAMPLERATE);
    const auto audioCutoff = INRATE(InputCutoff) == calc_FullRate;
    const auto audioGain = INRATE(InputGainDB) == calc_FullRate;
    const auto audioOut = INRATE(InputOutDB) == calc_FullRate;

    if (!audioCutoff)
        unit->circuit.setCircuitParams(clampCutoff(IN0(InputCutoff), sampleRate));

    const auto targetGain = dbToGain(clampGainDB(IN0(InputGainDB)));
    const auto targetOutGain = dbToGain(clampOutDB(IN0(InputOutDB)));
    float gain = unit->currentGain;
    float outGain = unit->currentOutGain;
    const auto gainStep = audioGain ? 0.0f : (targetGain - gain) / static_cast<float>(std::max(inNumSamples, 1));
    const auto outGainStep = audioOut ? 0.0f : (targetOutGain - outGain) / static_cast<float>(std::max(inNumSamples, 1));

    for (int i = 0; i < inNumSamples; ++i) {
        if (audioCutoff)
            unit->circuit.setCircuitParams(clampCutoff(inputAt(unit, InputCutoff, i), sampleRate));
        if (audioGain)
            gain = dbToGain(clampGainDB(inputAt(unit, InputGainDB, i)));
        if (audioOut)
            outGain = dbToGain(clampOutDB(inputAt(unit, InputOutDB, i)));

        unit->oversampling.upsample(in[i] * gain);
        for (int j = 0; j < oversamplingRatio; ++j)
            osBuffer[j] = unit->circuit.processSample(osBuffer[j]);

        out[i] = delaybb::flushSubnormal(unit->oversampling.downsample() * outGain);

        if (!audioGain)
            gain += gainStep;
        if (!audioOut)
            outGain += outGainStep;
    }

    unit->currentGain = audioGain ? gain : targetGain;
    unit->currentOutGain = audioOut ? outGain : targetOutGain;
}

void DiodeClipperCh_Ctor(DiodeClipperCh* unit)
{
    new (unit) DiodeClipperCh;

    const auto sampleRate = static_cast<float>(SAMPLERATE);
    unit->oversampling.reset(sampleRate);
    unit->circuit.prepare(sampleRate);
    unit->circuit.reset();
    unit->currentGain = dbToGain(clampGainDB(IN0(InputGainDB)));
    unit->currentOutGain = dbToGain(clampOutDB(IN0(InputOutDB)));

    SETCALC(DiodeClipperCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(DiodeClipperCh)
{
    ft = inTable;
    DefineSimpleUnit(DiodeClipperCh);
}
