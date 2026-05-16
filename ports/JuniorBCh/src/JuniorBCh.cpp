#include "SC_PlugIn.h"

#include "../../FuzzMachineCh/src/dsp/filters.hpp"
#include "../../shared/module_directory.hpp"
#include "dsp/JuniorBWDF.h"
#include "dsp/junior_model.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <mutex>
#include <new>

static InterfaceTable* ft;

namespace
{
struct SharedModel
{
    std::once_flag initFlag;
    juniorbch::JuniorTriodeModel model {};
    bool loaded = false;
};

SharedModel& sharedModel() noexcept
{
    static SharedModel shared;
    return shared;
}

std::filesystem::path moduleDirectory()
{
    return sharedpaths::moduleDirectory((const void*) &moduleDirectory);
}

void ensureModelLoaded()
{
    auto& shared = sharedModel();
    std::call_once(shared.initFlag, [&shared] {
        shared.loaded = shared.model.loadFromJsonFile(moduleDirectory() / "models" / "junior_1_stage.json");
    });
}

enum InputIndex
{
    InputIn = 0,
    InputDrive,
    InputBlend,
    InputStages
};

struct JuniorBCh : public Unit
{
    juniorbch::JuniorTriodeModel triodeModel;
    using Stage = juniorbch::JuniorBWDF<float, juniorbch::JuniorTriodeModel>;
    std::array<Stage, 4> stages { Stage { triodeModel }, Stage { triodeModel }, Stage { triodeModel }, Stage { triodeModel } };
    fuzzmachinech::BiquadFilter dcBlocker;
    bool modelReady = false;
};

inline float inputAt(JuniorBCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}
inline float clamp01(float x) noexcept { return sc_clip(x, 0.0f, 1.0f); }
inline int clampStages(float x) noexcept { return sc_clip((int) std::lround(x), 1, 4); }
inline float dbToGain(float db) noexcept { return std::pow(10.0f, db * 0.05f); }

void JuniorBCh_next(JuniorBCh* unit, int inNumSamples)
{
    const auto* in = IN(InputIn);
    auto* out = OUT(0);
    for (int i = 0; i < inNumSamples; ++i) {
        const auto drivePercent = clamp01(inputAt(unit, InputDrive, i));
        const auto blendPercent = clamp01(inputAt(unit, InputBlend, i));
        const auto numStages = clampStages(inputAt(unit, InputStages, i));
        const auto driveGain = dbToGain(drivePercent * 12.0f);

        auto y = in[i] * driveGain;
        if (unit->modelReady) {
            for (int stageIndex = 0; stageIndex < numStages; ++stageIndex)
                y = unit->stages[(size_t) stageIndex].process(y);
        }

        y = unit->dcBlocker.process(y);

        const auto dryGain = std::sin(0.5f * 3.14159265358979323846f * (1.0f - blendPercent));
        const auto wetGain = std::sin(0.5f * 3.14159265358979323846f * blendPercent);
        const auto polarityGain = numStages % 2 == 1 ? -1.0f : 1.0f;
        const auto makeupGainDB = (-15.0f + 9.0f * (1.0f - drivePercent)) * std::pow((float) numStages, 0.2f);
        const auto makeupGain = polarityGain * dbToGain(makeupGainDB);
        out[i] = fuzzmachinech::flushSubnormal(dryGain * in[i] + wetGain * makeupGain * y);
    }
}

void JuniorBCh_Ctor(JuniorBCh* unit)
{
    new (unit) JuniorBCh;
    ensureModelLoaded();
    if (sharedModel().loaded) {
        unit->triodeModel = sharedModel().model;
        unit->modelReady = true;
    }
    const auto sr = static_cast<float>(SAMPLERATE);
    for (auto& stage : unit->stages)
        stage.prepare(sr);
    unit->dcBlocker.setHighpass(25.0f / sr, 0.70710678f);
    unit->dcBlocker.reset();
    SETCALC(JuniorBCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(JuniorBCh)
{
    ft = inTable;
    DefineSimpleUnit(JuniorBCh);
}
