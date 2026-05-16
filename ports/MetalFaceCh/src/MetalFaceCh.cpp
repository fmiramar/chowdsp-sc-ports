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
struct SharedModels
{
    std::once_flag initFlag;
    sharedneural::LSTMModel<1, 28> model {};
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
        shared.loaded = shared.model.loadFromJsonFile(moduleDirectory() / "models" / "metal_face_model.json");
    });
}

enum InputIndex
{
    InputIn = 0,
    InputGainDB
};

struct MetalFaceCh : public Unit
{
    sharedneural::LSTMModel<1, 28> model;
    fuzzmachinech::BiquadFilter dcBlocker;
    bool modelReady = false;
};

inline float inputAt(MetalFaceCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clampGainDB(float x) noexcept
{
    return sc_clip(x, -48.0f, 6.0f);
}

inline float dbToGain(float db) noexcept
{
    return std::pow(10.0f, db * 0.05f);
}

void MetalFaceCh_next(MetalFaceCh* unit, int inNumSamples)
{
    const auto* in = IN(InputIn);
    auto* out = OUT(0);
    for (int i = 0; i < inNumSamples; ++i) {
        const auto gainDB = clampGainDB(inputAt(unit, InputGainDB, i));
        const auto preGain = dbToGain(gainDB);
        float y = in[i] * preGain;
        if (unit->modelReady) {
            std::array<float, 1> input { y };
            y = unit->model.process(input, false);
        }
        const auto makeupDB = (-48.0f - gainDB) / 10.0f;
        y *= dbToGain(makeupDB);
        out[i] = fuzzmachinech::flushSubnormal(unit->dcBlocker.process(y));
    }
}

void MetalFaceCh_Ctor(MetalFaceCh* unit)
{
    new (unit) MetalFaceCh;
    ensureModelsLoaded();
    unit->dcBlocker.setHighpass(25.0f / static_cast<float>(SAMPLERATE), 0.70710678f);
    unit->dcBlocker.reset();
    auto& shared = sharedModels();
    if (shared.loaded) {
        unit->model = shared.model;
        unit->model.reset();
        unit->modelReady = true;
    }
    SETCALC(MetalFaceCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(MetalFaceCh)
{
    ft = inTable;
    DefineSimpleUnit(MetalFaceCh);
}
