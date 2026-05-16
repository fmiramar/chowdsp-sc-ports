#include "SC_PlugIn.h"

#include "dsp/hysteresis_processing.hpp"
#include "dsp/oversampling.hpp"

#include <algorithm>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr int oversamplingRatio = 4;
constexpr float outputGain = 4.18f;

enum InputIndex
{
    InputIn = 0,
    InputBias,
    InputSaturation,
    InputDrive
};

struct TapeCh : public Unit
{
    tapech::Oversampling<oversamplingRatio> oversampling;
    tapech::HysteresisProcessing hysteresis;

    float sampleRate = 44100.0f;
    float bias = 0.5f;
    float saturation = 0.5f;
    float drive = 0.5f;
};

inline float inputAt(TapeCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline void resetDSP(TapeCh* unit) noexcept
{
    unit->oversampling.reset(unit->sampleRate);
    unit->hysteresis.reset();
    unit->hysteresis.setSolver(tapech::SolverType::NR4);
    unit->hysteresis.setSampleRate(static_cast<double>(unit->sampleRate * oversamplingRatio));
}

inline void cookParams(TapeCh* unit, float bias, float saturation, float drive) noexcept
{
    unit->bias = std::clamp(bias, 0.0f, 1.0f);
    unit->saturation = std::clamp(saturation, 0.0f, 1.0f);
    unit->drive = std::clamp(drive, 0.0f, 1.0f);

    const auto width = 1.0f - unit->bias;
    unit->hysteresis.cook(unit->drive, width, unit->saturation, false);
}

void TapeCh_next(TapeCh* unit, int inNumSamples)
{
    auto* out = OUT(0);
    const auto* in = IN(InputIn);

    for (int i = 0; i < inNumSamples; ++i) {
        cookParams(unit, inputAt(unit, InputBias, i), inputAt(unit, InputSaturation, i), inputAt(unit, InputDrive, i));

        const auto x = std::clamp(in[i] / 5.0f, -1.0f, 1.0f);

        unit->oversampling.upsample(x);
        auto* osBuffer = unit->oversampling.getOSBuffer();
        for (int k = 0; k < oversamplingRatio; ++k)
            osBuffer[k] = static_cast<float>(unit->hysteresis.process(static_cast<double>(osBuffer[k])));

        auto y = unit->oversampling.downsample();
        if (!std::isfinite(y)) {
            y = 0.0f;
            resetDSP(unit);
            cookParams(unit, unit->bias, unit->saturation, unit->drive);
        }

        out[i] = y * outputGain;
    }
}

void TapeCh_Ctor(TapeCh* unit)
{
    new (unit) TapeCh;
    unit->sampleRate = static_cast<float>(SAMPLERATE);
    resetDSP(unit);
    cookParams(unit, IN0(InputBias), IN0(InputSaturation), IN0(InputDrive));
    SETCALC(TapeCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(TapeCh)
{
    ft = inTable;
    DefineSimpleUnit(TapeCh);
}
