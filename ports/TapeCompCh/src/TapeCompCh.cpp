#include "SC_PlugIn.h"

#include "dsp/level_detector.hpp"

#include <algorithm>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr float attackBase = 160000.0f / 9801.0f;
constexpr float attackMult = 9801.0f / 3010.0f;
constexpr float attackOff = -950.0f / 301.0f;

constexpr float releaseBase = 100.0f;
constexpr float releaseMult = 10.0f;

constexpr int paramDivide = 32;
constexpr float minAmplitude = 1.0e-15f;

enum InputIndex
{
    InputIn = 0,
    InputAmount,
    InputAttack,
    InputRelease
};

struct TapeCompCh : public Unit
{
    tapecomp::LevelDetector<float> slewLimiter;
    int samplesUntilCook = 0;
    float amount = 0.0f;
    float attack = 0.5f;
    float release = 0.5f;
};

inline float inputAt(TapeCompCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float attackTimeMs(float value) noexcept
{
    const auto clipped = std::clamp(value, 0.0f, 1.0f);
    return std::pow(attackBase, clipped) * attackMult + attackOff;
}

inline float releaseTimeMs(float value) noexcept
{
    const auto clipped = std::clamp(value, 0.0f, 1.0f);
    return std::pow(releaseBase, clipped) * releaseMult;
}

inline float ampToDb(float amplitude) noexcept
{
    return 20.0f * std::log10(std::max(amplitude, minAmplitude));
}

inline float dbToAmp(float db) noexcept
{
    return std::pow(10.0f, db / 20.0f);
}

inline float compressionDB(float xDB, float dbPlus) noexcept
{
    const auto window = 2.0f * dbPlus;
    if (dbPlus <= 0.0f || xDB < -window)
        return dbPlus;

    return std::log(xDB + window + 1.0f) - dbPlus - xDB;
}

inline void cookParams(TapeCompCh* unit, float amount, float attack, float release) noexcept
{
    unit->amount = std::clamp(amount, 0.0f, 9.0f);
    unit->attack = std::clamp(attack, 0.0f, 1.0f);
    unit->release = std::clamp(release, 0.0f, 1.0f);
    unit->slewLimiter.setParameters(attackTimeMs(unit->attack), releaseTimeMs(unit->release));
}

void TapeCompCh_next(TapeCompCh* unit, int inNumSamples)
{
    auto* out = OUT(0);
    const auto* in = IN(InputIn);

    for (int i = 0; i < inNumSamples; ++i) {
        if (unit->samplesUntilCook <= 0) {
            cookParams(unit, inputAt(unit, InputAmount, i), inputAt(unit, InputAttack, i), inputAt(unit, InputRelease, i));
            unit->samplesUntilCook = paramDivide;
        }

        const auto x = in[i] / 5.0f;
        const auto xDB = ampToDb(std::abs(x));
        const auto targetGain = dbToAmp(compressionDB(xDB, unit->amount));

        auto gain = std::min(targetGain, unit->slewLimiter.processSample(targetGain));
        if (!std::isfinite(gain)) {
            gain = 0.0f;
            unit->slewLimiter.reset();
        }

        out[i] = gain * x * 5.0f;
        --unit->samplesUntilCook;
    }
}

void TapeCompCh_Ctor(TapeCompCh* unit)
{
    new (unit) TapeCompCh;
    unit->slewLimiter.prepare(SAMPLERATE);
    cookParams(unit, IN0(InputAmount), IN0(InputAttack), IN0(InputRelease));
    unit->samplesUntilCook = paramDivide;
    SETCALC(TapeCompCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(TapeCompCh)
{
    ft = inTable;
    DefineSimpleUnit(TapeCompCh);
}
