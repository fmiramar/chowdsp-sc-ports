#include "SC_PlugIn.h"

#include "dsp/level_detector.hpp"
#include "dsp/mod_peaking_filter.hpp"

#include <algorithm>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr float minFreqHz = 200.0f;
constexpr float maxFreqHz = 2000.0f;
constexpr float minQ = 1.0f;
constexpr float maxQ = 15.0f;
constexpr float minGainDB = 0.0f;
constexpr float maxGainDB = 12.0f;
constexpr float minAttackMs = 0.1f;
constexpr float maxAttackMs = 10.0f;
constexpr float minReleaseMs = 2.5f;
constexpr float maxReleaseMs = 250.0f;
constexpr float minFreqMod = 0.0f;
constexpr float maxFreqMod = 1.0f;
constexpr float freqModScale = 9.0f;

enum InputIndex
{
    InputIn = 0,
    InputFreq,
    InputQ,
    InputGainDB,
    InputAttackMs,
    InputReleaseMs,
    InputFreqMod
};

struct AutoWahCh : public Unit
{
    autowah::LevelDetector<float> levelDetector;
    autowah::ModPeakingFilter wahFilter;
    float sampleRate = 44100.0f;
};

inline float inputAt(AutoWahCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float dbToGain(float gainDB) noexcept
{
    return std::pow(10.0f, gainDB / 20.0f);
}

inline void resetDSP(AutoWahCh* unit) noexcept
{
    unit->levelDetector.prepare(unit->sampleRate);
    unit->wahFilter.prepare(unit->sampleRate);
}

void AutoWahCh_next(AutoWahCh* unit, int inNumSamples)
{
    const auto* in = IN(InputIn);
    auto* out = OUT(0);

    for (int i = 0; i < inNumSamples; ++i) {
        const auto attackMs = std::clamp(inputAt(unit, InputAttackMs, i), minAttackMs, maxAttackMs);
        const auto releaseMs = std::clamp(inputAt(unit, InputReleaseMs, i), minReleaseMs, maxReleaseMs);
        unit->levelDetector.setParameters(attackMs, releaseMs);

        const auto x = in[i];
        const auto level = unit->levelDetector.processSample(std::abs(x));

        const auto baseFreqHz = std::clamp(inputAt(unit, InputFreq, i), minFreqHz, maxFreqHz);
        const auto qValue = std::clamp(inputAt(unit, InputQ, i), minQ, maxQ);
        const auto gainDB = std::clamp(inputAt(unit, InputGainDB, i), minGainDB, maxGainDB);
        const auto freqMod = freqModScale * std::clamp(inputAt(unit, InputFreqMod, i), minFreqMod, maxFreqMod);
        const auto freqHz = std::clamp(baseFreqHz + baseFreqHz * freqMod * level, 10.0f, 0.48f * unit->sampleRate);

        unit->wahFilter.calcCoefs(freqHz, qValue, dbToGain(gainDB));

        auto y = unit->wahFilter.process(x);
        if (!std::isfinite(y)) {
            y = 0.0f;
            resetDSP(unit);
            unit->levelDetector.setParameters(attackMs, releaseMs);
            unit->wahFilter.calcCoefs(baseFreqHz, qValue, dbToGain(gainDB));
        }

        out[i] = y;
    }
}

void AutoWahCh_Ctor(AutoWahCh* unit)
{
    new (unit) AutoWahCh;
    unit->sampleRate = static_cast<float>(SAMPLERATE);
    resetDSP(unit);
    unit->levelDetector.setParameters(IN0(InputAttackMs), IN0(InputReleaseMs));
    unit->wahFilter.calcCoefs(std::clamp(IN0(InputFreq), minFreqHz, maxFreqHz),
                              std::clamp(IN0(InputQ), minQ, maxQ),
                              dbToGain(std::clamp(IN0(InputGainDB), minGainDB, maxGainDB)));
    SETCALC(AutoWahCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(AutoWahCh)
{
    ft = inTable;
    DefineSimpleUnit(AutoWahCh);
}
