#include "SC_PlugIn.h"

#include "dsp/denormal.hpp"
#include "dsp/filters.hpp"
#include "dsp/fuzz_model.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <mutex>
#include <new>

#if defined(__APPLE__) || defined(__linux__)
#include <dlfcn.h>
#endif

static InterfaceTable* ft;

namespace
{
constexpr int oversamplingRatio = 2;

enum InputIndex
{
    InputIn = 0,
    InputFuzz,
    InputModel,
    InputVolume
};

struct SharedModels
{
    std::once_flag initFlag;
    fuzzmachinech::FuzzModelSet models;
    bool initialized = false;
};

SharedModels& sharedModels() noexcept
{
    static SharedModels shared;
    return shared;
}

std::filesystem::path moduleDirectory()
{
#if defined(__APPLE__) || defined(__linux__)
    Dl_info info {};
    if (dladdr((const void*) &moduleDirectory, &info) != 0 && info.dli_fname != nullptr)
        return std::filesystem::path(info.dli_fname).parent_path();
#endif
    return std::filesystem::current_path();
}

void ensureModelsLoaded()
{
    auto& shared = sharedModels();
    std::call_once(shared.initFlag, [&shared] {
        shared.initialized = shared.models.loadFromDirectory(moduleDirectory() / "models");
    });
}

struct FuzzMachineCh : public Unit
{
    fuzzmachinech::FuzzModelSet models;
    fuzzmachinech::Oversampling<oversamplingRatio> oversampling;
    fuzzmachinech::BiquadFilter dcBlocker;
    fuzzmachinech::LinearSmoother fuzzSmooth;
    fuzzmachinech::LinearSmoother volumeSmooth;
    float sampleRate = 48000.0f;
    bool useOversampling = true;
    bool use88Family = false;
    bool modelsReady = false;
};

inline float inputAt(FuzzMachineCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clamp01(float x) noexcept
{
    return sc_clip(x, 0.0f, 1.0f);
}

inline int modelSelection(float model) noexcept
{
    return sc_clip(static_cast<int>(std::lround(model)), 1, 2);
}

inline bool choose88Family(float sampleRate) noexcept
{
    return std::fmod(sampleRate, 44100.0f) == 0.0f;
}

void processSample(FuzzMachineCh* unit, float x, float fuzzValue, int modelValue, float& y) noexcept
{
    if (!unit->modelsReady || !unit->models.loaded) {
        y = x;
        return;
    }

    auto& model = unit->models.select(modelValue == 1, unit->use88Family);

    if (unit->useOversampling) {
        unit->oversampling.upsample(x);
        auto* os = unit->oversampling.getOSBuffer();
        for (int j = 0; j < oversamplingRatio; ++j)
            os[j] = model.process(os[j], fuzzValue, true);
        y = unit->oversampling.downsample();
    } else {
        y = model.process(x, fuzzValue, true);
    }
}

void FuzzMachineCh_next(FuzzMachineCh* unit, int inNumSamples)
{
    const auto* in = IN(InputIn);
    auto* out = OUT(0);
    const auto audioFuzz = INRATE(InputFuzz) == calc_FullRate;
    const auto audioModel = INRATE(InputModel) == calc_FullRate;
    const auto audioVolume = INRATE(InputVolume) == calc_FullRate;

    if (!audioFuzz)
        unit->fuzzSmooth.setTarget(clamp01(IN0(InputFuzz)));
    if (!audioVolume)
        unit->volumeSmooth.setTarget(1.5f * clamp01(IN0(InputVolume)));

    for (int i = 0; i < inNumSamples; ++i) {
        if (audioFuzz)
            unit->fuzzSmooth.setTarget(clamp01(inputAt(unit, InputFuzz, i)));
        if (audioVolume)
            unit->volumeSmooth.setTarget(1.5f * clamp01(inputAt(unit, InputVolume, i)));

        const auto fuzzValue = unit->fuzzSmooth.process();
        const auto modelValue = modelSelection(audioModel ? inputAt(unit, InputModel, i) : IN0(InputModel));
        float y = 0.0f;
        processSample(unit, in[i], fuzzValue, modelValue, y);
        y *= unit->volumeSmooth.process();
        y = unit->dcBlocker.process(y);
        out[i] = fuzzmachinech::flushSubnormal(y);
    }
}

void FuzzMachineCh_Ctor(FuzzMachineCh* unit)
{
    new (unit) FuzzMachineCh;

    ensureModelsLoaded();
    unit->models = sharedModels().models;
    unit->models.reset();
    unit->modelsReady = sharedModels().initialized && sharedModels().models.loaded;

    unit->sampleRate = static_cast<float>(SAMPLERATE);
    unit->use88Family = choose88Family(unit->sampleRate);
    unit->useOversampling = unit->sampleRate < 80000.0f;
    unit->oversampling.reset(unit->sampleRate);
    unit->dcBlocker.setHighpass(30.0f / unit->sampleRate, 0.70710678f);
    unit->dcBlocker.reset();
    unit->fuzzSmooth.setRampTime(unit->sampleRate * (unit->useOversampling ? oversamplingRatio : 1.0f), 0.025f);
    unit->volumeSmooth.setRampTime(unit->sampleRate, 0.05f);
    unit->fuzzSmooth.reset(clamp01(IN0(InputFuzz)));
    unit->volumeSmooth.reset(1.5f * clamp01(IN0(InputVolume)));

    SETCALC(FuzzMachineCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(FuzzMachineCh)
{
    ft = inTable;
    DefineSimpleUnit(FuzzMachineCh);
}
