#include "SC_PlugIn.h"

#include "dsp/bbd_delay_line.hpp"
#include "dsp/biquad.hpp"
#include "dsp/denormal.hpp"
#include "dsp/sine_wave.hpp"

#include <algorithm>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr float pi = 3.14159265358979323846f;
constexpr float rate1Low = 0.005f;
constexpr float rate1High = 5.0f;
constexpr float rate2Low = 0.5f;
constexpr float rate2High = 40.0f;
constexpr float delay1Ms = 0.6f;
constexpr float delay2Ms = 0.2f;
constexpr int paramDivide = 64;
constexpr int numChannels = 2;
constexpr int delaysPerChannel = 2;
constexpr float initialPhases[numChannels * delaysPerChannel] = { -pi / 3.0f, 0.0f, 0.0f, pi / 3.0f };

enum InputIndex
{
    InputIn = 0,
    InputRate,
    InputDepth,
    InputFeedback,
    InputMix
};

struct ChorusCh : public Unit
{
    chorus::BBDDelayLine<4096> delay[numChannels][delaysPerChannel];
    chorus::SineWave slowLFOs[numChannels][delaysPerChannel];
    chorus::SineWave fastLFOs[numChannels][delaysPerChannel];
    chorus::BiquadFilter aaFilter[numChannels];
    chorus::BiquadFilter dcBlocker[numChannels];

    float feedbackState[numChannels] = { 0.0f, 0.0f };
    float fbAmount = 0.0f;
    int samplesUntilCook = paramDivide;
};

inline float inputAt(ChorusCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline void prepareDSP(ChorusCh* unit) noexcept
{
    const auto sampleRate = static_cast<float>(SAMPLERATE);

    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 0; i < delaysPerChannel; ++i) {
            unit->delay[ch][i].prepare(sampleRate);
            unit->slowLFOs[ch][i].prepare(sampleRate);
            unit->fastLFOs[ch][i].prepare(sampleRate);

            const auto phase = initialPhases[ch * delaysPerChannel + i];
            unit->slowLFOs[ch][i].reset(phase);
            unit->fastLFOs[ch][i].reset(phase);
        }

        unit->aaFilter[ch].setParameters(chorus::BiquadFilter::LOWPASS, std::min(12000.0f, sampleRate * 0.49f) / sampleRate, 0.7071f);
        unit->dcBlocker[ch].setParameters(chorus::BiquadFilter::HIGHPASS, 240.0f / sampleRate, 0.7071f);
        unit->aaFilter[ch].reset();
        unit->dcBlocker[ch].reset();
        unit->feedbackState[ch] = 0.0f;
    }
}

inline void cookParams(ChorusCh* unit, float rateParam, float feedbackParam) noexcept
{
    rateParam = sc_clip(rateParam, 0.0f, 1.0f);
    feedbackParam = sc_clip(feedbackParam, 0.0f, 1.0f);

    const auto slowRate = rate1Low * std::pow(rate1High / rate1Low, rateParam);
    const auto fastRate = rate2Low * std::pow(rate2High / rate2Low, rateParam);
    unit->fbAmount = 0.39f * std::sqrt(feedbackParam);

    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 0; i < delaysPerChannel; ++i) {
            unit->slowLFOs[ch][i].setFrequency(slowRate);
            unit->fastLFOs[ch][i].setFrequency(fastRate);
            unit->delay[ch][i].setFilterFreq(10000.0f);
        }
    }
}

inline void processFrame(ChorusCh* unit, float input, float depthParam, float mixParam, float& left, float& right) noexcept
{
    const auto sampleRate = static_cast<float>(SAMPLERATE);
    const auto sampleTime = 1.0f / sampleRate;
    const auto depth = sc_clip(depthParam, 0.0f, 1.0f);
    const auto mix = sc_clip(mixParam, 0.0f, 1.0f);

    const auto slowDepth = 5.0f * delay1Ms * 0.001f * sampleRate * depth;
    const auto fastDepth = 5.0f * delay2Ms * 0.001f * sampleRate * depth;

    float outputs[numChannels] {};

    for (int ch = 0; ch < numChannels; ++ch) {
        auto x = std::tanh(input * 0.075f - unit->feedbackState[ch]);
        auto y = 0.0f;

        for (int i = 0; i < delaysPerChannel; ++i) {
            auto delayAmt = slowDepth * (1.0f + 0.95f * unit->slowLFOs[ch][i].processSample());
            delayAmt += fastDepth * (1.0f + 0.95f * unit->fastLFOs[ch][i].processSample());

            unit->delay[ch][i].setDelayTime((delayAmt + 25.0f) * sampleTime);
            y += unit->delay[ch][i].process(x);
        }

        y = unit->aaFilter[ch].process(y);
        unit->feedbackState[ch] = chorus::flushSubnormal(unit->dcBlocker[ch].process(y * unit->fbAmount));
        outputs[ch] = chorus::flushSubnormal(y * 5.0f * mix + input * (1.0f - mix));
    }

    left = outputs[0];
    right = outputs[1];
}

void ChorusCh_next_k(ChorusCh* unit, int inNumSamples);
void ChorusCh_next_a(ChorusCh* unit, int inNumSamples);

void ChorusCh_init(ChorusCh* unit)
{
    prepareDSP(unit);
    unit->samplesUntilCook = paramDivide;
    cookParams(unit, IN0(InputRate), IN0(InputFeedback));

    const auto hasAudioRateParams = INRATE(InputRate) == calc_FullRate || INRATE(InputDepth) == calc_FullRate
        || INRATE(InputFeedback) == calc_FullRate || INRATE(InputMix) == calc_FullRate;

    if (hasAudioRateParams)
        SETCALC(ChorusCh_next_a);
    else
        SETCALC(ChorusCh_next_k);

    OUT0(0) = 0.0f;
    OUT0(1) = 0.0f;
}

void ChorusCh_next_k(ChorusCh* unit, int inNumSamples)
{
    auto* outL = OUT(0);
    auto* outR = OUT(1);
    const auto* in = IN(InputIn);

    const auto rate = IN0(InputRate);
    const auto depth = IN0(InputDepth);
    const auto feedback = IN0(InputFeedback);
    const auto mix = IN0(InputMix);

    for (int i = 0; i < inNumSamples; ++i) {
        if (--unit->samplesUntilCook <= 0) {
            cookParams(unit, rate, feedback);
            unit->samplesUntilCook = paramDivide;
        }

        processFrame(unit, in[i], depth, mix, outL[i], outR[i]);
    }
}

void ChorusCh_next_a(ChorusCh* unit, int inNumSamples)
{
    auto* outL = OUT(0);
    auto* outR = OUT(1);
    const auto* in = IN(InputIn);

    for (int i = 0; i < inNumSamples; ++i) {
        if (--unit->samplesUntilCook <= 0) {
            cookParams(unit, inputAt(unit, InputRate, i), inputAt(unit, InputFeedback, i));
            unit->samplesUntilCook = paramDivide;
        }

        processFrame(unit, in[i], inputAt(unit, InputDepth, i), inputAt(unit, InputMix, i), outL[i], outR[i]);
    }
}

void ChorusCh_Ctor(ChorusCh* unit)
{
    new (unit) ChorusCh;
    ChorusCh_init(unit);
}
} // namespace

PluginLoad(ChorusCh)
{
    ft = inTable;
    DefineSimpleUnit(ChorusCh);
}
