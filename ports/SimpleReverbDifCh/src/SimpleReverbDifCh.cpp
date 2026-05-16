#include "SC_PlugIn.h"

#include "../../shared/reverb/reverb_core.hpp"

#include <cstring>
#include <new>

InterfaceTable* ft;

namespace
{
using chow_reverb::ConvolutionDiffuserCore;
using chow_reverb::LinearSmoother;
using chow_reverb::MultiplicativeSmoother;
using chow_reverb::SimpleReverbFDNCore;
using chow_reverb::SineWave;

constexpr int smallBlockSize = 16;
constexpr float smootherTimeSeconds = 0.05f;

enum InputIndex
{
    InputLeft = 0,
    InputRight,
    InputDiffusionTime,
    InputFDNDelay,
    InputFDNT60Low,
    InputFDNT60High,
    InputModAmount,
    InputDryWet
};

struct SimpleReverbDifCh : public Unit
{
    ConvolutionDiffuserCore diffuser;
    SimpleReverbFDNCore fdn;

    MultiplicativeSmoother fdnTimeSmoother;
    MultiplicativeSmoother fdnT60LowSmoother;
    MultiplicativeSmoother fdnT60HighSmoother;
    LinearSmoother modAmountSmoother;

    SineWave lfos[2];
    float lfoVals[2] {};

    float* dryL = nullptr;
    float* dryR = nullptr;
    float* wetL = nullptr;
    float* wetR = nullptr;
    int scratchSize = 0;
};

inline float clampDiffusion(float value) noexcept
{
    return sc_clip(value, 10.0f, 1000.0f);
}

inline float clampDelay(float value) noexcept
{
    return sc_clip(value, 50.0f, 500.0f);
}

inline float clampT60(float value) noexcept
{
    return sc_clip(value, 100.0f, 5000.0f);
}

inline float clampPercent(float value) noexcept
{
    return sc_clip(value, 0.0f, 1.0f);
}

inline float inputAt(SimpleReverbDifCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

bool allocateScratch(SimpleReverbDifCh* unit) noexcept
{
    unit->scratchSize = BUFLENGTH;
    unit->dryL = static_cast<float*>(RTAlloc(unit->mWorld, static_cast<size_t>(unit->scratchSize) * sizeof(float)));
    unit->dryR = static_cast<float*>(RTAlloc(unit->mWorld, static_cast<size_t>(unit->scratchSize) * sizeof(float)));
    unit->wetL = static_cast<float*>(RTAlloc(unit->mWorld, static_cast<size_t>(unit->scratchSize) * sizeof(float)));
    unit->wetR = static_cast<float*>(RTAlloc(unit->mWorld, static_cast<size_t>(unit->scratchSize) * sizeof(float)));
    return unit->dryL != nullptr && unit->dryR != nullptr && unit->wetL != nullptr && unit->wetR != nullptr;
}

void freeScratch(SimpleReverbDifCh* unit) noexcept
{
    if (unit->dryL != nullptr) {
        RTFree(unit->mWorld, unit->dryL);
        unit->dryL = nullptr;
    }

    if (unit->dryR != nullptr) {
        RTFree(unit->mWorld, unit->dryR);
        unit->dryR = nullptr;
    }

    if (unit->wetL != nullptr) {
        RTFree(unit->mWorld, unit->wetL);
        unit->wetL = nullptr;
    }

    if (unit->wetR != nullptr) {
        RTFree(unit->mWorld, unit->wetR);
        unit->wetR = nullptr;
    }

    unit->scratchSize = 0;
}

void SimpleReverbDifCh_next(SimpleReverbDifCh* unit, int inNumSamples)
{
    std::memcpy(unit->dryL, IN(InputLeft), static_cast<size_t>(inNumSamples) * sizeof(float));
    std::memcpy(unit->dryR, IN(InputRight), static_cast<size_t>(inNumSamples) * sizeof(float));

    unit->diffuser.setDiffusionTimeMs(clampDiffusion(inputAt(unit, InputDiffusionTime, inNumSamples - 1)));
    unit->diffuser.processBlock(unit->dryL, unit->dryR, unit->wetL, unit->wetR, inNumSamples);

    for (int sampleIndex = 0; sampleIndex < inNumSamples; sampleIndex += smallBlockSize) {
        const int samplesToProcess = std::min(smallBlockSize, inNumSamples - sampleIndex);
        const int targetIndex = sampleIndex + samplesToProcess - 1;

        unit->fdnTimeSmoother.setTarget(clampDelay(inputAt(unit, InputFDNDelay, targetIndex)), smootherTimeSeconds);
        unit->fdnT60LowSmoother.setTarget(clampT60(inputAt(unit, InputFDNT60Low, targetIndex)), smootherTimeSeconds);
        unit->fdnT60HighSmoother.setTarget(clampT60(inputAt(unit, InputFDNT60High, targetIndex)), smootherTimeSeconds);
        unit->modAmountSmoother.setTarget(clampPercent(inputAt(unit, InputModAmount, targetIndex)), smootherTimeSeconds);

        const float modAmountMult = unit->modAmountSmoother.skip(samplesToProcess);
        float modulators[2] {
            unit->lfoVals[0] * modAmountMult * 0.25f + 1.0f,
            unit->lfoVals[1] * modAmountMult * 0.25f + 1.0f
        };

        unit->fdn.setDelayTimeMsWithModulators(unit->fdnTimeSmoother.skip(samplesToProcess), modulators, 2);
        unit->fdn.setDecayTimeMs(unit->fdnT60LowSmoother.skip(samplesToProcess),
                                 unit->fdnT60HighSmoother.skip(samplesToProcess),
                                 1000.0f);

        for (int n = sampleIndex; n < sampleIndex + samplesToProcess; ++n) {
            unit->lfoVals[0] = unit->lfos[0].processSample();
            unit->lfoVals[1] = unit->lfos[1].processSample();

            float wetL = unit->wetL[n];
            float wetR = unit->wetR[n];
            unit->fdn.processStereo(wetL, wetR);

            const float mix = clampPercent(inputAt(unit, InputDryWet, n));
            OUT(0)[n] = mix * wetL + (1.0f - mix) * unit->dryL[n];
            OUT(1)[n] = mix * wetR + (1.0f - mix) * unit->dryR[n];
        }
    }
}

void SimpleReverbDifCh_Ctor(SimpleReverbDifCh* unit)
{
    new (unit) SimpleReverbDifCh;

    if (!allocateScratch(unit)
        || !unit->diffuser.allocate(ft, unit->mWorld, static_cast<float>(SAMPLERATE), BUFLENGTH, 2, 1.0f)
        || !unit->fdn.allocate(unit->mWorld, static_cast<float>(SAMPLERATE))) {
        freeScratch(unit);
        unit->diffuser.free(ft, unit->mWorld);
        unit->fdn.free(unit->mWorld);
        ClearUnitOnMemFailed
    }

    unit->fdnTimeSmoother.reset(static_cast<float>(SAMPLERATE), clampDelay(IN0(InputFDNDelay)));
    unit->fdnT60LowSmoother.reset(static_cast<float>(SAMPLERATE), clampT60(IN0(InputFDNT60Low)));
    unit->fdnT60HighSmoother.reset(static_cast<float>(SAMPLERATE), clampT60(IN0(InputFDNT60High)));
    unit->modAmountSmoother.reset(static_cast<float>(SAMPLERATE), clampPercent(IN0(InputModAmount)));

    unit->lfos[0].prepare(static_cast<float>(SAMPLERATE));
    unit->lfos[0].setFrequency(2.0f);
    unit->lfos[1].prepare(static_cast<float>(SAMPLERATE));
    unit->lfos[1].setFrequency(0.95f);
    unit->lfoVals[0] = 0.0f;
    unit->lfoVals[1] = 0.0f;

    unit->diffuser.setDiffusionTimeMs(clampDiffusion(IN0(InputDiffusionTime)));

    const float modulators[2] { 1.0f, 1.0f };
    unit->fdn.setDelayTimeMsWithModulators(clampDelay(IN0(InputFDNDelay)), modulators, 2);
    unit->fdn.setDecayTimeMs(clampT60(IN0(InputFDNT60Low)), clampT60(IN0(InputFDNT60High)), 1000.0f);

    SETCALC(SimpleReverbDifCh_next);
    OUT0(0) = 0.0f;
    OUT0(1) = 0.0f;
}

void SimpleReverbDifCh_Dtor(SimpleReverbDifCh* unit)
{
    freeScratch(unit);
    unit->diffuser.free(ft, unit->mWorld);
    unit->fdn.free(unit->mWorld);
}
} // namespace

PluginLoad(SimpleReverbDifCh)
{
    ft = inTable;
    DefineDtorUnit(SimpleReverbDifCh);
}
