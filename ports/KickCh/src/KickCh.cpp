#include "SC_PlugIn.h"

#include "dsp/biquad.hpp"
#include "dsp/denormal.hpp"
#include "dsp/pulse_shaper.hpp"
#include "dsp/smoothed_value.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr int maxVoices = 4;
constexpr float triggerHigh = 1.0f;
constexpr float triggerLow = 0.1f;
constexpr float pulseCapVal = 0.015e-6f;
constexpr float lowDamp = 0.0001f;
constexpr float highDamp = 0.5f;
constexpr float pi = 3.14159265358979323846f;
constexpr float sqrtHalf = 0.7071067811865476f;

enum InputIndex
{
    InputTrig = 0,
    InputFreq,
    InputPulseWidthMs,
    InputPulseAmp,
    InputVelocity,
    InputVoices,
    InputSustain,
    InputDecay,
    InputQ,
    InputDamping,
    InputTight,
    InputBounce,
    InputMode,
    InputPortamentoMs,
    InputNoiseAmt,
    InputNoiseDecay,
    InputNoiseCutoff,
    InputNoiseType,
    InputTone,
    InputLevelDB
};

enum ResonantMode
{
    ModeLinear = 0,
    ModeBasic = 1,
    ModeBouncy = 2
};

enum NoiseType
{
    NoiseUniform = 0,
    NoiseNormal = 1,
    NoisePink = 2
};

struct XorShift32
{
    explicit XorShift32(uint32_t seed = 0x12345678u) noexcept : state(seed == 0u ? 0x12345678u : seed) {}

    uint32_t nextU32() noexcept
    {
        auto x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return x;
    }

    float next01() noexcept
    {
        return static_cast<float>(nextU32() >> 8) * (1.0f / 16777216.0f);
    }

    float nextSigned() noexcept
    {
        return next01() * 2.0f - 1.0f;
    }

    uint32_t state;
};

struct OutputFilter
{
    kickch::SmoothedValue<float, kickch::ValueSmoothingTypes::Multiplicative> freqSmooth;
    kickch::SmoothedValue<float, kickch::ValueSmoothingTypes::Multiplicative> gainSmooth;
    float sampleRate = 44100.0f;
    float b0 = 1.0f;
    float b1 = 0.0f;
    float a1 = 0.0f;
    float z1 = 0.0f;

    void reset(float newSampleRate, float toneHz, float gain) noexcept
    {
        sampleRate = newSampleRate;
        z1 = 0.0f;
        freqSmooth.reset(sampleRate, 0.05);
        gainSmooth.reset(sampleRate, 0.05);
        freqSmooth.setCurrentAndTargetValue(toneHz);
        gainSmooth.setCurrentAndTargetValue(gain);
        calcCoefs(toneHz, gain);
    }

    void calcCoefs(float freqHz, float gain) noexcept
    {
        const float freq = std::clamp(freqHz, 30.0f, 0.45f * sampleRate);
        const float wc = 2.0f * pi * freq / sampleRate;
        const float c = 1.0f / std::tan(wc * 0.5f);
        const float a0 = c + 1.0f;

        b0 = gain / a0;
        b1 = b0;
        a1 = (1.0f - c) / a0;
    }

    float process(float x, float toneHz, float gain) noexcept
    {
        freqSmooth.setTargetValue(std::clamp(toneHz, 30.0f, 0.45f * sampleRate));
        gainSmooth.setTargetValue(std::max(gain, 1.0e-6f));
        calcCoefs(freqSmooth.getNextValue(), gainSmooth.getNextValue());

        const float y = z1 + x * b0;
        z1 = kickch::flushSubnormal(x * b1 - y * a1);
        return kickch::flushSubnormal(y);
    }
};

struct PinkNoiseState
{
    int frame = -1;
    std::array<float, 8> values {};

    void reset() noexcept
    {
        frame = -1;
        values.fill(0.0f);
    }

    float process(XorShift32& rng) noexcept
    {
        const int lastFrame = frame;
        frame += 1;
        if (frame >= (1 << 8))
            frame = 0;

        const int diff = lastFrame ^ frame;
        float sum = 0.0f;
        for (size_t i = 0; i < values.size(); ++i) {
            if ((diff & (1 << static_cast<int>(i))) != 0)
                values[i] = rng.next01() - 0.5f;
            sum += values[i];
        }

        return sum * (1.0f / 8.0f);
    }
};

struct NoiseVoice
{
    XorShift32 rng;
    PinkNoiseState pink;
    kickch::BiquadFilter filter;
    kickch::SmoothedValue<float, kickch::ValueSmoothingTypes::Multiplicative> decaySmooth;
    float sampleRate = 44100.0f;

    explicit NoiseVoice(uint32_t seed = 0x12345678u) noexcept : rng(seed) {}

    void reset(float newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        pink.reset();
        filter.reset();
        filter.setParameters(kickch::BiquadFilter::LOWPASS, 2000.0f / sampleRate, sqrtHalf);
        decaySmooth.reset(sampleRate, 0.05);
        decaySmooth.setCurrentAndTargetValue(1.0f);
    }

    float nextNoiseSample(int type) noexcept
    {
        switch (type) {
            case NoiseNormal: {
                const float u1 = std::max(rng.next01(), 1.0e-12f);
                const float u2 = rng.next01();
                const float radius = std::sqrt(-2.0f * std::log(u1));
                const float theta = 2.0f * pi * u2;
                return (radius * std::sin(theta)) * sqrtHalf;
            }
            case NoisePink:
                return pink.process(rng);
            case NoiseUniform:
            default:
                return rng.nextSigned();
        }
    }

    float process(float x, float amount, float decay, float cutoffHz, int type) noexcept
    {
        const float amt = std::clamp(amount, 0.0f, 1.0f);
        const float gain = amt * amt;
        const float decayExp = std::pow(1.0f - std::clamp(decay, 0.0f, 1.0f), 2.5f) * 2.0f + 1.0f;
        decaySmooth.setTargetValue(std::max(decayExp, 1.0e-6f));

        filter.setParameters(kickch::BiquadFilter::LOWPASS,
                             std::clamp(cutoffHz, 20.0f, 0.45f * sampleRate) / sampleRate,
                             sqrtHalf);

        const float noiseSample = filter.process(nextNoiseSample(type) * gain);
        const float noiseGain = std::pow(std::abs(x), decaySmooth.getNextValue());
        return kickch::flushSubnormal(x + noiseGain * noiseSample);
    }
};

struct ResonantFilterVoice
{
    kickch::SmoothedValue<float, kickch::ValueSmoothingTypes::Multiplicative> freqSmooth;
    kickch::SmoothedValue<float, kickch::ValueSmoothingTypes::Multiplicative> qSmooth;
    kickch::SmoothedValue<float, kickch::ValueSmoothingTypes::Multiplicative> gSmooth;
    kickch::SmoothedValue<float, kickch::ValueSmoothingTypes::Multiplicative> d1Smooth;
    kickch::SmoothedValue<float, kickch::ValueSmoothingTypes::Multiplicative> d2Smooth;
    kickch::SmoothedValue<float, kickch::ValueSmoothingTypes::Multiplicative> d3Smooth;
    float sampleRate = 44100.0f;
    float prevPortamentoMs = 50.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float z1 = 0.0f;
    float z2 = 0.0f;

    static float drive(float x, float driveAmount) noexcept
    {
        return std::tanh(x * driveAmount) / driveAmount;
    }

    static void getDriveValues(int mode, float tight, float bounce, float& d1, float& d2, float& d3) noexcept
    {
        tight = std::clamp(tight, 0.0f, 1.0f);
        bounce = std::clamp(bounce, 0.0f, 1.0f);

        if (mode == ModeLinear) {
            d1 = 1.0f;
            d2 = 1.0f;
            d3 = 1.0f;
            return;
        }

        if (mode == ModeBasic) {
            d1 = 4.9f * std::pow(tight, 4.0f) + 0.1f;
            d2 = 4.9f * std::pow(tight, 6.0f) + 0.1f;
            d3 = 4.75f * std::pow(bounce, 3.0f) + 0.25f;
            return;
        }

        d1 = 4.9f * std::pow(tight, 4.0f) + 0.1f;
        const float bounceScale = 0.7f * tight + 0.3f;
        d3 = std::pow(bounceScale * bounce, 2.0f) + 0.1f;
        d2 = 0.4f * std::pow(tight, 0.8f) + 0.4f * std::pow(1.0f - bounce, 0.8f) + 0.1f;
    }

    void reset(float newSampleRate, float initialFreq, float initialQ, float initialG, float tight, float bounce, int mode, float portamentoMs) noexcept
    {
        sampleRate = newSampleRate;
        prevPortamentoMs = std::max(portamentoMs, 0.1f);
        z1 = 0.0f;
        z2 = 0.0f;

        freqSmooth.reset(sampleRate, prevPortamentoMs * 0.001f);
        qSmooth.reset(sampleRate, 0.05);
        gSmooth.reset(sampleRate, 0.05);
        d1Smooth.reset(sampleRate, 0.05);
        d2Smooth.reset(sampleRate, 0.05);
        d3Smooth.reset(sampleRate, 0.05);

        freqSmooth.setCurrentAndTargetValue(initialFreq);
        qSmooth.setCurrentAndTargetValue(initialQ);
        gSmooth.setCurrentAndTargetValue(initialG);

        float d1 = 1.0f;
        float d2 = 1.0f;
        float d3 = 1.0f;
        getDriveValues(mode, tight, bounce, d1, d2, d3);
        d1Smooth.setCurrentAndTargetValue(d1);
        d2Smooth.setCurrentAndTargetValue(d2);
        d3Smooth.setCurrentAndTargetValue(d3);

        calcCoefs(initialFreq, initialQ, initialG);
    }

    void calcCoefs(float freqHz, float qValue, float G) noexcept
    {
        const float freq = std::clamp(freqHz, 30.0f, 0.45f * sampleRate);
        const float wc = freq * 2.0f * pi / sampleRate;
        const float wS = std::sin(wc);
        const float wC = std::cos(wc);
        const float alpha = wS / (2.0f * std::max(qValue, 0.1f));
        const float a0 = (G + 1.0f) + alpha * G;

        b0 = (alpha + 1.0f) / a0;
        b1 = wC * -2.0f / a0;
        b2 = (1.0f - alpha) / a0;
        a1 = wC * -2.0f * (G + 1.0f) / a0;
        a2 = ((G + 1.0f) - alpha * G) / a0;
    }

    float process(float x, float targetFreq, float qValue, float damping, float tight, float bounce, int mode, float portamentoMs) noexcept
    {
        const float portMs = std::clamp(portamentoMs, 0.1f, 200.0f);
        if (portMs != prevPortamentoMs) {
            prevPortamentoMs = portMs;
            freqSmooth.reset(sampleRate, prevPortamentoMs * 0.001f);
            freqSmooth.setCurrentAndTargetValue(freqSmooth.getTargetValue());
        }

        const float G = lowDamp * std::pow(highDamp / lowDamp, std::clamp(damping, 0.0f, 1.0f));

        freqSmooth.setTargetValue(std::clamp(targetFreq, 30.0f, 500.0f));
        qSmooth.setTargetValue(std::clamp(qValue, 0.1f, 2.0f));
        gSmooth.setTargetValue(G);

        float d1 = 1.0f;
        float d2 = 1.0f;
        float d3 = 1.0f;
        getDriveValues(mode, tight, bounce, d1, d2, d3);
        d1Smooth.setTargetValue(d1);
        d2Smooth.setTargetValue(d2);
        d3Smooth.setTargetValue(d3);

        calcCoefs(freqSmooth.getNextValue(), qSmooth.getNextValue(), gSmooth.getNextValue());

        const float s1 = d1Smooth.getNextValue();
        const float s2 = d2Smooth.getNextValue();
        const float s3 = d3Smooth.getNextValue();

        if (mode == ModeLinear) {
            const float y = (z1 + x * b0) * 0.999999f;
            z1 = kickch::flushSubnormal(z2 + x * b1 - y * a1);
            z2 = kickch::flushSubnormal(x * b2 - y * a2);
            return kickch::flushSubnormal(y * 0.15f);
        }

        if (mode == ModeBasic) {
            const float y = z1 + x * b0;
            const float yDrive = drive(y, s3);
            z1 = kickch::flushSubnormal(drive(z2 + x * b1 - yDrive * a1, s1));
            z2 = kickch::flushSubnormal(drive(x * b2 - yDrive * a2, s2));
            return kickch::flushSubnormal(y);
        }

        const float y = z1 + x * b0;
        const float yDrive = drive(y, s3);
        z1 = kickch::flushSubnormal(drive(z2 + x * b1 - yDrive * a1, s1));
        z2 = kickch::flushSubnormal(drive(x * b2 - y * a2, s1));
        return kickch::flushSubnormal(y * s2);
    }
};

struct KickVoice
{
    kickch::PulseShaper pulseShaper;
    ResonantFilterVoice resonantFilter;
    NoiseVoice noise;
    int pulseSamplesRemaining = 0;
    float pulseLevel = 0.0f;
    float latchedFreq = 100.0f;

    explicit KickVoice(uint32_t seed = 0x12345678u) : noise(seed) {}

    void reset(float sampleRate, float initialFreq) noexcept
    {
        pulseShaper.prepare(sampleRate);
        pulseShaper.reset();
        noise.reset(sampleRate);
        resonantFilter.reset(sampleRate, initialFreq, 0.5f, lowDamp * std::pow(highDamp / lowDamp, 0.5f), 0.5f, 0.0f, ModeBasic, 50.0f);
        pulseSamplesRemaining = 0;
        pulseLevel = 0.0f;
        latchedFreq = initialFreq;
    }

    void trigger(int pulseWidthSamples, float newPulseLevel, float freqHz) noexcept
    {
        pulseSamplesRemaining = std::max(1, pulseWidthSamples);
        pulseLevel = newPulseLevel;
        latchedFreq = std::clamp(freqHz, 30.0f, 500.0f);
    }

    float nextPulseSample() noexcept
    {
        const float pulse = pulseSamplesRemaining > 0 ? pulseLevel : 0.0f;
        if (pulseSamplesRemaining > 0)
            --pulseSamplesRemaining;
        return pulseShaper.processSample(pulse);
    }

    float process(float qValue,
                  float damping,
                  float tight,
                  float bounce,
                  int mode,
                  float portamentoMs,
                  float noiseAmt,
                  float noiseDecay,
                  float noiseCutoff,
                  int noiseType) noexcept
    {
        float x = nextPulseSample();
        x = resonantFilter.process(x, latchedFreq, qValue, damping, tight, bounce, mode, portamentoMs);
        x = noise.process(x, noiseAmt, noiseDecay, noiseCutoff, noiseType);
        return kickch::flushSubnormal(x);
    }
};

struct KickCh : public Unit
{
    std::array<KickVoice, maxVoices> voices {
        KickVoice { 0x13572468u },
        KickVoice { 0x24681357u },
        KickVoice { 0xdeadbeefu },
        KickVoice { 0x0badc0deu },
    };
    OutputFilter outputFilter;
    kickch::BiquadFilter dcBlocker;
    float sampleRate = 44100.0f;
    bool triggerArmed = true;
    int voiceIndex = 0;
    int numVoices = 1;
};

inline float inputAt(KickCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float dbToGain(float gainDB) noexcept
{
    return std::pow(10.0f, gainDB / 20.0f);
}

inline int clampInt(float value, int lo, int hi) noexcept
{
    return std::clamp(static_cast<int>(std::lround(value)), lo, hi);
}

inline float toneMakeupDB(float toneHz) noexcept
{
    const float norm = (std::clamp(toneHz, 300.0f, 7000.0f) - 300.0f) / (7000.0f - 300.0f);
    return (norm - 0.5f) * -6.0f;
}

inline float outputGain(float levelDB, float toneHz, float bounce) noexcept
{
    const float bounceMakeupDB = 14.0f * std::pow(std::clamp(bounce, 0.0f, 1.0f), 2.5f);
    return dbToGain(std::clamp(levelDB, -30.0f, 30.0f) + bounceMakeupDB + toneMakeupDB(toneHz) + 3.5f);
}

inline int pulseWidthSamples(float sampleRate, float pulseWidthMs) noexcept
{
    const float widthMs = std::clamp(pulseWidthMs, 0.025f, 2.5f);
    return std::max(1, static_cast<int>(std::lround((widthMs / 1000.0f) * sampleRate)));
}

inline void cookPulseShaper(KickVoice& voice, float sustain, float decay) noexcept
{
    constexpr float r1Off = 5000.0f;
    constexpr float r1Scale = 500000.0f;
    constexpr float r2Off = 500.0f;
    constexpr float r2Scale = 100000.0f;

    const float sustainVal = 1.0f - std::pow(std::clamp(sustain, 0.0f, 1.0f), 0.05f);
    const float r163 = r1Off + (sustainVal * (r1Scale - r1Off));

    const float decayVal = std::pow(std::clamp(decay, 0.0f, 1.0f), 2.0f);
    const float r162 = r2Off + (decayVal * (r2Scale - r2Off));

    voice.pulseShaper.setResistors(r162, r163);
}

inline void resetDSP(KickCh* unit) noexcept
{
    const float initialFreq = std::clamp(IN0(InputFreq), 30.0f, 500.0f);
    for (auto& voice : unit->voices)
        voice.reset(unit->sampleRate, initialFreq);

    unit->outputFilter.reset(unit->sampleRate,
                             std::clamp(IN0(InputTone), 300.0f, 7000.0f),
                             outputGain(IN0(InputLevelDB), IN0(InputTone), IN0(InputBounce)));

    unit->dcBlocker.reset();
    unit->dcBlocker.setParameters(kickch::BiquadFilter::HIGHPASS, 10.0f / unit->sampleRate, sqrtHalf);
    unit->triggerArmed = true;
    unit->voiceIndex = 0;
    unit->numVoices = clampInt(IN0(InputVoices), 1, maxVoices);
}

void KickCh_next(KickCh* unit, int inNumSamples)
{
    auto* out = OUT(0);

    for (int i = 0; i < inNumSamples; ++i) {
        const float trig = inputAt(unit, InputTrig, i);
        const float freq = std::clamp(inputAt(unit, InputFreq, i), 30.0f, 500.0f);
        const float pulseWidthMsValue = inputAt(unit, InputPulseWidthMs, i);
        const float pulseAmp = std::clamp(inputAt(unit, InputPulseAmp, i), 0.0f, 1.0f);
        const float velocity = std::clamp(inputAt(unit, InputVelocity, i), 0.0f, 2.0f);
        const int voiceCount = clampInt(inputAt(unit, InputVoices, i), 1, maxVoices);
        const float sustain = std::clamp(inputAt(unit, InputSustain, i), 0.0f, 1.0f);
        const float decay = std::clamp(inputAt(unit, InputDecay, i), 0.0f, 1.0f);
        const float qValue = std::clamp(inputAt(unit, InputQ, i), 0.1f, 2.0f);
        const float damping = std::clamp(inputAt(unit, InputDamping, i), 0.0f, 1.0f);
        const float tight = std::clamp(inputAt(unit, InputTight, i), 0.0f, 1.0f);
        const float bounce = std::clamp(inputAt(unit, InputBounce, i), 0.0f, 1.0f);
        const int mode = clampInt(inputAt(unit, InputMode, i), ModeLinear, ModeBouncy);
        const float portamentoMs = std::clamp(inputAt(unit, InputPortamentoMs, i), 0.1f, 200.0f);
        const float noiseAmt = std::clamp(inputAt(unit, InputNoiseAmt, i), 0.0f, 1.0f);
        const float noiseDecay = std::clamp(inputAt(unit, InputNoiseDecay, i), 0.0f, 1.0f);
        const float noiseCutoff = std::clamp(inputAt(unit, InputNoiseCutoff, i), 20.0f, 0.45f * unit->sampleRate);
        const int noiseType = clampInt(inputAt(unit, InputNoiseType, i), NoiseUniform, NoisePink);
        const float toneHz = std::clamp(inputAt(unit, InputTone, i), 300.0f, 7000.0f);
        const float levelDB = std::clamp(inputAt(unit, InputLevelDB, i), -30.0f, 30.0f);

        unit->numVoices = voiceCount;

        if (unit->triggerArmed) {
            if (trig >= triggerHigh) {
                unit->voiceIndex = (unit->voiceIndex + 1) % unit->numVoices;
                auto& voice = unit->voices[unit->voiceIndex];
                voice.trigger(pulseWidthSamples(unit->sampleRate, pulseWidthMsValue), pulseAmp * velocity, freq);
                unit->triggerArmed = false;
            }
        } else if (trig <= triggerLow) {
            unit->triggerArmed = true;
        }

        float y = 0.0f;
        for (int voice = 0; voice < unit->numVoices; ++voice) {
            auto& v = unit->voices[voice];
            cookPulseShaper(v, sustain, decay);
            y += v.process(qValue, damping, tight, bounce, mode, portamentoMs, noiseAmt, noiseDecay, noiseCutoff, noiseType);
        }

        y = unit->outputFilter.process(y, toneHz, outputGain(levelDB, toneHz, bounce));
        y = unit->dcBlocker.process(y);

        if (!std::isfinite(y)) {
            y = 0.0f;
            resetDSP(unit);
        }

        out[i] = y;
    }
}

void KickCh_Ctor(KickCh* unit)
{
    new (unit) KickCh;
    unit->sampleRate = static_cast<float>(SAMPLERATE);
    resetDSP(unit);
    SETCALC(KickCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(KickCh)
{
    ft = inTable;
    DefineSimpleUnit(KickCh);
}
