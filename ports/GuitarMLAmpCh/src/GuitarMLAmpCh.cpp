#include "SC_PlugIn.h"

#include "../../shared/neural/lstm_model.hpp"
#include "../../FuzzMachineCh/src/dsp/filters.hpp"
#include "../../FuzzMachineCh/src/dsp/denormal.hpp"

#include <array>
#include <filesystem>
#include <mutex>
#include <new>

#if defined(__APPLE__) || defined(__linux__)
#include <dlfcn.h>
#endif

static InterfaceTable* ft;

namespace
{
enum ModelIndex
{
    BluesJr = 0,
    TS9 = 1,
    Mesa = 2
};

struct HighShelfFilter
{
    void reset() noexcept
    {
        z1 = 0.0f;
        z2 = 0.0f;
    }

    void setParameters(float sampleRate, float freq, float gainDB, float slope = 1.0f) noexcept
    {
        const auto A = std::pow(10.0f, gainDB / 40.0f);
        const auto w0 = 2.0f * 3.14159265358979323846f * freq / sampleRate;
        const auto cs = std::cos(w0);
        const auto sn = std::sin(w0);
        const auto alpha = sn / 2.0f * std::sqrt((A + 1.0f / A) * (1.0f / slope - 1.0f) + 2.0f);
        const auto beta = 2.0f * std::sqrt(A) * alpha;

        const auto b0n = A * ((A + 1.0f) + (A - 1.0f) * cs + beta);
        const auto b1n = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cs);
        const auto b2n = A * ((A + 1.0f) + (A - 1.0f) * cs - beta);
        const auto a0n = (A + 1.0f) - (A - 1.0f) * cs + beta;
        const auto a1n = 2.0f * ((A - 1.0f) - (A + 1.0f) * cs);
        const auto a2n = (A + 1.0f) - (A - 1.0f) * cs - beta;

        b0 = b0n / a0n;
        b1 = b1n / a0n;
        b2 = b2n / a0n;
        a1 = a1n / a0n;
        a2 = a2n / a0n;
    }

    float process(float x) noexcept
    {
        const auto y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;
};

struct SharedModels
{
    std::once_flag initFlag;
    sharedneural::LSTMModel<2, 40> bluesJr {};
    sharedneural::LSTMModel<2, 40> ts9 {};
    sharedneural::LSTMModel<2, 40> mesa {};
    bool loaded = false;
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
        const auto dir = moduleDirectory() / "models";
        const auto ok1 = shared.bluesJr.loadFromJsonFile(dir / "BluesJrAmp_VolKnob.json");
        const auto ok2 = shared.ts9.loadFromJsonFile(dir / "TS9_DriveKnob.json");
        const auto ok3 = shared.mesa.loadFromJsonFile(dir / "MesaRecMini_ModernChannel_GainKnob.json");
        shared.loaded = ok1 && ok2 && ok3;
    });
}

enum InputIndex
{
    InputIn = 0,
    InputModel,
    InputCondition,
    InputCorrection
};

struct GuitarMLAmpCh : public Unit
{
    sharedneural::LSTMModel<2, 40> bluesJr;
    sharedneural::LSTMModel<2, 40> ts9;
    sharedneural::LSTMModel<2, 40> mesa;
    fuzzmachinech::LinearSmoother conditionSmooth;
    HighShelfFilter correctionFilter;
    fuzzmachinech::BiquadFilter dcBlocker;
    bool modelsReady = false;
};

inline float inputAt(GuitarMLAmpCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}
inline float clamp01(float x) noexcept { return sc_clip(x, 0.0f, 1.0f); }
inline int clampModel(float x) noexcept { return sc_clip((int) std::lround(x), 0, 2); }

sharedneural::LSTMModel<2, 40>& selectModel(GuitarMLAmpCh* unit, int model) noexcept
{
    if (model == TS9)
        return unit->ts9;
    if (model == Mesa)
        return unit->mesa;
    return unit->bluesJr;
}

void GuitarMLAmpCh_next(GuitarMLAmpCh* unit, int inNumSamples)
{
    const auto* in = IN(InputIn);
    auto* out = OUT(0);
    const auto sampleRateCorrectionOn = IN0(InputCorrection) > 0.5f;

    if (INRATE(InputCondition) != calc_FullRate)
        unit->conditionSmooth.setTarget(clamp01(IN0(InputCondition)));

    for (int i = 0; i < inNumSamples; ++i) {
        if (INRATE(InputCondition) == calc_FullRate)
            unit->conditionSmooth.setTarget(clamp01(inputAt(unit, InputCondition, i)));

        const auto modelIndex = clampModel(inputAt(unit, InputModel, i));
        const auto condition = unit->conditionSmooth.process();
        auto y = in[i];

        if (unit->modelsReady) {
            std::array<float, 2> modelInput { y, condition };
            y = selectModel(unit, modelIndex).process(modelInput, true);
        }

        if (sampleRateCorrectionOn)
            y = unit->correctionFilter.process(y);

        if (modelIndex == Mesa)
            y *= 0.5f;

        out[i] = fuzzmachinech::flushSubnormal(unit->dcBlocker.process(y));
    }
}

void GuitarMLAmpCh_Ctor(GuitarMLAmpCh* unit)
{
    new (unit) GuitarMLAmpCh;
    ensureModelsLoaded();

    const auto sr = static_cast<float>(SAMPLERATE);
    unit->conditionSmooth.setRampTime(sr, 0.05f);
    unit->conditionSmooth.reset(clamp01(IN0(InputCondition)));
    unit->correctionFilter.setParameters(sr, 8100.0f, 6.0f, sr < 50000.0f ? 1.0f : 0.25f);
    unit->correctionFilter.reset();
    unit->dcBlocker.setHighpass(25.0f / sr, 0.70710678f);
    unit->dcBlocker.reset();

    auto& shared = sharedModels();
    if (shared.loaded) {
        unit->bluesJr = shared.bluesJr;
        unit->ts9 = shared.ts9;
        unit->mesa = shared.mesa;
        unit->bluesJr.reset();
        unit->ts9.reset();
        unit->mesa.reset();
        unit->modelsReady = true;
    }

    SETCALC(GuitarMLAmpCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(GuitarMLAmpCh)
{
    ft = inTable;
    DefineSimpleUnit(GuitarMLAmpCh);
}
