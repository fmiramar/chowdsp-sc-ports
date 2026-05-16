#include "SC_PlugIn.h"

#include "../../shared/neural/lstm_model.hpp"
#include "../../shared/module_directory.hpp"
#include "../../FuzzMachineCh/src/dsp/filters.hpp"
#include "../../FuzzMachineCh/src/dsp/denormal.hpp"

#include <array>
#include <filesystem>
#include <mutex>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr int oversamplingRatio = 2;

struct SharedModels
{
    std::once_flag initFlag;
    sharedneural::LSTMModel<2, 24> model96 {};
    sharedneural::LSTMModel<2, 24> model88 {};
    bool loaded = false;
};

SharedModels& sharedModels() noexcept
{
    static SharedModels shared;
    return shared;
}

std::filesystem::path moduleDirectory()
{
    return sharedpaths::moduleDirectory((const void*) &moduleDirectory);
}

void ensureModelsLoaded()
{
    auto& shared = sharedModels();
    std::call_once(shared.initFlag, [&shared] {
        const auto dir = moduleDirectory() / "models";
        const auto ok96 = shared.model96.loadFromJsonFile(dir / "bass_face_model_96k.json");
        const auto ok88 = shared.model88.loadFromJsonFile(dir / "bass_face_model_88_2k.json");
        shared.loaded = ok96 && ok88;
    });
}

enum InputIndex
{
    InputIn = 0,
    InputGain
};

struct BassFaceCh : public Unit
{
    sharedneural::LSTMModel<2, 24> model;
    fuzzmachinech::Oversampling<oversamplingRatio> oversampling;
    fuzzmachinech::BiquadFilter dcBlocker;
    fuzzmachinech::LinearSmoother gainSmooth;
    bool useOversampling = true;
    bool use88Family = false;
    bool modelReady = false;
};

inline float inputAt(BassFaceCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clamp01(float x) noexcept
{
    return sc_clip(x, 0.0f, 1.0f);
}

inline bool choose88Family(float sampleRate) noexcept
{
    return std::fmod(sampleRate, 44100.0f) == 0.0f;
}

void BassFaceCh_next(BassFaceCh* unit, int inNumSamples)
{
    const auto* in = IN(InputIn);
    auto* out = OUT(0);
    const auto audioGain = INRATE(InputGain) == calc_FullRate;

    if (!audioGain)
        unit->gainSmooth.setTarget(clamp01(IN0(InputGain)));

    for (int i = 0; i < inNumSamples; ++i) {
        if (audioGain)
            unit->gainSmooth.setTarget(clamp01(inputAt(unit, InputGain, i)));

        const auto gainValue = unit->gainSmooth.process();
        float y = in[i];

        if (unit->modelReady) {
            if (unit->useOversampling) {
                unit->oversampling.upsample(in[i]);
                auto* os = unit->oversampling.getOSBuffer();
                for (int j = 0; j < oversamplingRatio; ++j) {
                    std::array<float, 2> input { os[j], gainValue };
                    os[j] = unit->model.process(input, true);
                }
                y = unit->oversampling.downsample();
            } else {
                std::array<float, 2> input { in[i], gainValue };
                y = unit->model.process(input, true);
            }
        }

        out[i] = fuzzmachinech::flushSubnormal(unit->dcBlocker.process(y));
    }
}

void BassFaceCh_Ctor(BassFaceCh* unit)
{
    new (unit) BassFaceCh;
    ensureModelsLoaded();

    const auto sr = static_cast<float>(SAMPLERATE);
    unit->useOversampling = sr <= 48000.0f;
    unit->use88Family = choose88Family(sr);
    unit->oversampling.reset(sr);
    unit->dcBlocker.setHighpass(15.0f / sr, 0.70710678f);
    unit->dcBlocker.reset();
    unit->gainSmooth.setRampTime(sr * (unit->useOversampling ? oversamplingRatio : 1.0f), 0.05f);
    unit->gainSmooth.reset(clamp01(IN0(InputGain)));

    auto& shared = sharedModels();
    if (shared.loaded) {
        unit->model = unit->use88Family ? shared.model88 : shared.model96;
        unit->model.reset();
        unit->modelReady = true;
    }

    SETCALC(BassFaceCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(BassFaceCh)
{
    ft = inTable;
    DefineSimpleUnit(BassFaceCh);
}
