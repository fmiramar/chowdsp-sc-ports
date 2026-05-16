#include "SC_PlugIn.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <new>
#include <numeric>
#include <random>

static InterfaceTable* ft;

namespace
{
constexpr int numOutputs = 2;
constexpr int fdnChannels = 8;
constexpr int diffuserStages = 3;
constexpr int smallBlockSize = 16;
constexpr float smootherTimeSeconds = 0.05f;
constexpr float fdnCrossoverHz = 1000.0f;
constexpr float lfoFreqs[2] = { 2.0f, 0.95f };
constexpr float sqrtHalf = 0.7071067811865476f;
constexpr float pi = 3.14159265358979323846f;

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

inline float inputAt(Unit* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

struct LinearSmoother
{
    float current = 0.0f;
    float target = 0.0f;
    float step = 0.0f;
    float sampleRate = 48000.0f;
    int samplesRemaining = 0;

    void reset(float newSampleRate, float initialValue) noexcept
    {
        sampleRate = newSampleRate;
        current = initialValue;
        target = initialValue;
        step = 0.0f;
        samplesRemaining = 0;
    }

    void setTarget(float newTarget) noexcept
    {
        if (newTarget == target)
            return;

        target = newTarget;
        samplesRemaining = std::max(1, static_cast<int>(std::round(smootherTimeSeconds * sampleRate)));
        step = (target - current) / static_cast<float>(samplesRemaining);
    }

    float skip(int numSamples) noexcept
    {
        if (samplesRemaining <= 0)
            return current;

        const int stepCount = std::min(numSamples, samplesRemaining);
        current += step * static_cast<float>(stepCount);
        samplesRemaining -= stepCount;

        if (samplesRemaining <= 0) {
            current = target;
            samplesRemaining = 0;
            step = 0.0f;
        }

        return current;
    }
};

struct SineWave
{
    float sampleRate = 48000.0f;
    float frequency = 1.0f;
    float phase = 0.0f;
    float phaseInc = 0.0f;

    void prepare(float newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        setFrequency(frequency);
        phase = 0.0f;
    }

    void setFrequency(float newFrequency) noexcept
    {
        frequency = newFrequency;
        phaseInc = 2.0f * pi * frequency / sampleRate;
    }

    float processSample() noexcept
    {
        const auto y = std::sin(phase);
        phase += phaseInc;
        if (phase >= 2.0f * pi)
            phase -= 2.0f * pi;
        return y;
    }
};

struct OnePoleFilter
{
    float b0 = 1.0f;
    float b1 = 0.0f;
    float a1 = 0.0f;
    float z1 = 0.0f;

    void reset() noexcept
    {
        z1 = 0.0f;
    }

    float process(float x) noexcept
    {
        const auto y = b0 * x + z1;
        z1 = b1 * x - a1 * y;
        return y;
    }
};

struct ShelfFilter
{
    OnePoleFilter filter;

    void reset() noexcept
    {
        filter.reset();
    }

    void calcCoefs(float lowGain, float highGain, float fc, float sampleRate) noexcept
    {
        if (std::abs(lowGain - highGain) < 1.0e-9f) {
            filter.b0 = lowGain;
            filter.b1 = 0.0f;
            filter.a1 = 0.0f;
            return;
        }

        const float wc = 2.0f * pi * fc;
        const float numerator = wc * wc * (highGain * highGain - lowGain * highGain);
        const float denominator = lowGain * highGain - lowGain * lowGain;
        const float p = std::sqrt(std::max(numerator / std::max(denominator, 1.0e-12f), 1.0e-12f));
        const float k = p / std::tan(p / (2.0f * sampleRate));

        const float b0 = highGain / p;
        const float b1 = lowGain;
        const float a0 = 1.0f / p;
        const float a1 = 1.0f;
        const float a0z = a0 * k + a1;

        filter.b0 = (b0 * k + b1) / a0z;
        filter.b1 = (-b0 * k + b1) / a0z;
        filter.a1 = (-a0 * k + a1) / a0z;
    }

    float process(float x) noexcept
    {
        return filter.process(x);
    }
};

class DynamicDelayLine
{
public:
    bool allocate(World* world, int maxDelaySamples) noexcept
    {
        free(world);

        size = std::max(maxDelaySamples + 8, 8);
        buffer = static_cast<float*>(RTAlloc(world, (size_t) size * sizeof(float)));
        if (buffer == nullptr) {
            size = 0;
            return false;
        }

        reset();
        return true;
    }

    void free(World* world) noexcept
    {
        if (buffer != nullptr) {
            RTFree(world, buffer);
            buffer = nullptr;
        }

        size = 0;
        writePos = 0;
        readPos = 0;
        delayInt = 0;
        delayFrac = 0.0f;
    }

    void reset() noexcept
    {
        writePos = 0;
        readPos = 0;
        delayInt = 1;
        delayFrac = 0.0f;

        if (buffer != nullptr) {
            for (int i = 0; i < size; ++i)
                buffer[i] = 0.0f;
        }
    }

    void setDelay(float newDelayInSamples) noexcept
    {
        if (buffer == nullptr || size == 0)
            return;

        const float upperLimit = static_cast<float>(size - 1);
        const float delay = sc_clip(newDelayInSamples, 1.0f, upperLimit);
        delayInt = static_cast<int>(std::floor(delay));
        delayFrac = delay - static_cast<float>(delayInt);

        if (delayInt >= 1) {
            delayFrac += 1.0f;
            --delayInt;
        }
    }

    float popSample() const noexcept
    {
        if (buffer == nullptr || size == 0)
            return 0.0f;

        int index1 = readPos + delayInt;
        int index2 = index1 + 1;
        int index3 = index2 + 1;
        int index4 = index3 + 1;

        if (index4 >= size) {
            index1 %= size;
            index2 %= size;
            index3 %= size;
            index4 %= size;
        }

        const float value1 = buffer[index1];
        const float value2 = buffer[index2];
        const float value3 = buffer[index3];
        const float value4 = buffer[index4];

        const float d1 = delayFrac - 1.0f;
        const float d2 = delayFrac - 2.0f;
        const float d3 = delayFrac - 3.0f;

        const float c1 = -d1 * d2 * d3 / 6.0f;
        const float c2 = d2 * d3 * 0.5f;
        const float c3 = -d1 * d3 * 0.5f;
        const float c4 = d1 * d2 / 6.0f;

        return value1 * c1 + delayFrac * (value2 * c2 + value3 * c3 + value4 * c4);
    }

    void pushSample(float sample) noexcept
    {
        if (buffer == nullptr || size == 0)
            return;

        buffer[writePos] = sample;
        writePos = (writePos + size - 1) % size;
    }

    void updateReadPointer() noexcept
    {
        if (size != 0)
            readPos = (readPos + size - 1) % size;
    }

private:
    float* buffer = nullptr;
    int size = 0;
    int writePos = 0;
    int readPos = 0;
    int delayInt = 1;
    float delayFrac = 0.0f;
};

struct StereoDiffuserStage
{
    DynamicDelayLine delays[2];
    float delayMults[2] { 1.0f, 1.0f };
    float polarity[2] { 1.0f, -1.0f };
    int channelSwap[2] { 0, 1 };

    bool allocate(World* world, float sampleRate, float maxStageMs, uint32 seed) noexcept
    {
        std::mt19937 mt(seed);
        std::uniform_real_distribution<float> dist0(1.0f / 3.0f, 2.0f / 3.0f);
        std::uniform_real_distribution<float> dist1(2.0f / 3.0f, 1.0f);
        delayMults[0] = dist0(mt);
        delayMults[1] = dist1(mt);
        polarity[0] = (mt() & 1u) == 0u ? 1.0f : -1.0f;
        polarity[1] = (mt() & 1u) == 0u ? 1.0f : -1.0f;
        channelSwap[0] = (mt() & 1u) == 0u ? 0 : 1;
        channelSwap[1] = channelSwap[0] == 0 ? 1 : 0;

        const int maxDelaySamples = static_cast<int>(std::ceil(sampleRate * maxStageMs / 1000.0f));
        return delays[0].allocate(world, maxDelaySamples) && delays[1].allocate(world, maxDelaySamples);
    }

    void free(World* world) noexcept
    {
        delays[0].free(world);
        delays[1].free(world);
    }

    void reset() noexcept
    {
        delays[0].reset();
        delays[1].reset();
    }

    void setDiffusionTimeMs(float diffusionMs, float stageMult, float sampleRate) noexcept
    {
        const auto delay0 = delayMults[0] * diffusionMs * stageMult * sampleRate / 1000.0f;
        const auto delay1 = delayMults[1] * diffusionMs * stageMult * sampleRate / 1000.0f;
        delays[0].setDelay(delay0);
        delays[1].setDelay(delay1);
    }

    inline void process(float& left, float& right) noexcept
    {
        delays[0].pushSample(left);
        delays[1].pushSample(right);

        const float delayed[2] {
            delays[channelSwap[0]].popSample(),
            delays[channelSwap[1]].popSample()
        };

        delays[0].updateReadPointer();
        delays[1].updateReadPointer();

        const auto mixed0 = sqrtHalf * (delayed[0] + delayed[1]) * polarity[0];
        const auto mixed1 = sqrtHalf * (delayed[0] - delayed[1]) * polarity[1];
        left = mixed0;
        right = mixed1;
    }
};

struct StereoDiffuser
{
    std::array<StereoDiffuserStage, diffuserStages> stages {};
    std::array<float, diffuserStages> stageMults { 0.25f, 0.5f, 1.0f };

    bool allocate(World* world, float sampleRate) noexcept
    {
        for (int i = 0; i < diffuserStages; ++i) {
            if (!stages[(size_t) i].allocate(world, sampleRate, 1000.0f * stageMults[(size_t) i], 0x53445256u + (uint32) i))
                return false;
        }

        reset();
        return true;
    }

    void free(World* world) noexcept
    {
        for (auto& stage : stages)
            stage.free(world);
    }

    void reset() noexcept
    {
        for (auto& stage : stages)
            stage.reset();
    }

    void setDiffusionTimeMs(float diffusionMs, float sampleRate) noexcept
    {
        for (int i = 0; i < diffuserStages; ++i)
            stages[(size_t) i].setDiffusionTimeMs(diffusionMs, stageMults[(size_t) i], sampleRate);
    }

    inline void process(float& left, float& right) noexcept
    {
        for (auto& stage : stages)
            stage.process(left, right);
    }
};

struct FDNCore8
{
    std::array<DynamicDelayLine, fdnChannels> delays {};
    std::array<float, fdnChannels> delayMults {};
    std::array<float, fdnChannels> delaySamples {};
    std::array<float, fdnChannels> reads {};
    std::array<ShelfFilter, fdnChannels> shelves {};

    bool allocate(World* world, float sampleRate) noexcept
    {
        std::mt19937 mt(0x53465256u);

        for (int i = 0; i < fdnChannels; ++i) {
            const float rangeLow = float(i + 1) / float(fdnChannels + 1);
            const float rangeHigh = float(i + 2) / float(fdnChannels + 1);
            std::uniform_real_distribution<float> dist(rangeLow, rangeHigh);
            delayMults[(size_t) i] = dist(mt);

            const int maxDelaySamples = static_cast<int>(std::ceil(sampleRate * 500.0f * delayMults[(size_t) i] / 1000.0f));
            if (!delays[(size_t) i].allocate(world, maxDelaySamples))
                return false;
        }

        reset();
        return true;
    }

    void free(World* world) noexcept
    {
        for (auto& delay : delays)
            delay.free(world);
    }

    void reset() noexcept
    {
        for (auto& delay : delays)
            delay.reset();
        for (auto& shelf : shelves)
            shelf.reset();
        reads.fill(0.0f);
        delaySamples.fill(1.0f);
    }

    void setDecay(float sampleRate, float t60LowMs, float t60HighMs) noexcept
    {
        for (int i = 0; i < fdnChannels; ++i) {
            const auto delayMs = delaySamples[(size_t) i] * 1000.0f / sampleRate;
            const auto lowGain = calcGainForT60(t60LowMs, delayMs);
            const auto highGain = calcGainForT60(t60HighMs, delayMs);
            shelves[(size_t) i].calcCoefs(sc_clip(lowGain, -1.0f, 1.0f), sc_clip(highGain, -1.0f, 1.0f), fdnCrossoverHz, sampleRate);
        }
    }

    void setDelay(float sampleRate, float baseDelayMs, float mod1, float mod2) noexcept
    {
        for (int i = 0; i < fdnChannels; ++i) {
            auto mod = 1.0f;
            if (i == 0)
                mod = mod1;
            else if (i == 1)
                mod = mod2;

            delaySamples[(size_t) i] = delayMults[(size_t) i] * baseDelayMs * mod * sampleRate / 1000.0f;
            delays[(size_t) i].setDelay(delaySamples[(size_t) i]);
        }
    }

    inline void process(float inL, float inR, float& outL, float& outR) noexcept
    {
        float inputVec[fdnChannels] { inL, inR, inL, inR, inL, inR, inL, inR };

        for (int i = 0; i < fdnChannels; ++i)
            reads[(size_t) i] = delays[(size_t) i].popSample();

        applyHouseholder(reads.data());

        outL = 0.0f;
        outR = 0.0f;
        for (int i = 0; i < fdnChannels; ++i) {
            const auto mixed = reads[(size_t) i];
            if ((i & 1) == 0)
                outL += mixed;
            else
                outR += mixed;

            const auto fb = shelves[(size_t) i].process(mixed);
            delays[(size_t) i].pushSample(inputVec[i] + fb);
            delays[(size_t) i].updateReadPointer();
        }

        outL *= 0.125f;
        outR *= 0.125f;
    }

private:
    static float calcGainForT60(float decayTimeMs, float delayTimeMs) noexcept
    {
        const auto nTimes = decayTimeMs / std::max(delayTimeMs, 1.0e-6f);
        return std::pow(0.001f, 1.0f / nTimes);
    }

    static void applyHouseholder(float* data) noexcept
    {
        float sum = 0.0f;
        for (int i = 0; i < fdnChannels; ++i)
            sum += data[i];

        const auto correction = 2.0f * sum / static_cast<float>(fdnChannels);
        for (int i = 0; i < fdnChannels; ++i)
            data[i] -= correction;
    }
};

struct SimpleReverbCh : public Unit
{
    StereoDiffuser diffuser;
    FDNCore8 fdn;
    SineWave lfos[2];

    LinearSmoother diffusionTime;
    LinearSmoother fdnDelay;
    LinearSmoother fdnT60Low;
    LinearSmoother fdnT60High;
    LinearSmoother modAmount;
    LinearSmoother dryWet;

    float sampleRate = 48000.0f;
};

void SimpleReverbCh_next(SimpleReverbCh* unit, int inNumSamples)
{
    auto* outL = OUT(0);
    auto* outR = OUT(1);
    const auto* inL = IN(InputLeft);
    const auto* inR = IN(InputRight);

    for (int sampleIndex = 0; sampleIndex < inNumSamples; sampleIndex += smallBlockSize) {
        const int samplesToProcess = std::min(smallBlockSize, inNumSamples - sampleIndex);

        unit->diffusionTime.setTarget(clampDiffusion(inputAt(unit, InputDiffusionTime, sampleIndex)));
        unit->fdnDelay.setTarget(clampDelay(inputAt(unit, InputFDNDelay, sampleIndex)));
        unit->fdnT60Low.setTarget(clampT60(inputAt(unit, InputFDNT60Low, sampleIndex)));
        unit->fdnT60High.setTarget(clampT60(inputAt(unit, InputFDNT60High, sampleIndex)));
        unit->modAmount.setTarget(clampPercent(inputAt(unit, InputModAmount, sampleIndex)));
        unit->dryWet.setTarget(clampPercent(inputAt(unit, InputDryWet, sampleIndex)));

        const auto diffusionMs = unit->diffusionTime.skip(samplesToProcess);
        const auto baseDelayMs = unit->fdnDelay.skip(samplesToProcess);
        const auto t60LowMs = unit->fdnT60Low.skip(samplesToProcess);
        const auto t60HighMs = unit->fdnT60High.skip(samplesToProcess);
        const auto modAmount = unit->modAmount.skip(samplesToProcess);
        const auto dryWet = unit->dryWet.skip(samplesToProcess);

        unit->diffuser.setDiffusionTimeMs(diffusionMs, unit->sampleRate);
        unit->fdn.setDecay(unit->sampleRate, t60LowMs, t60HighMs);

        for (int n = sampleIndex; n < sampleIndex + samplesToProcess; ++n) {
            auto left = inL[n];
            auto right = inR[n];

            unit->diffuser.process(left, right);

            const auto mod1 = 1.0f + unit->lfos[0].processSample() * modAmount * 0.25f;
            const auto mod2 = 1.0f + unit->lfos[1].processSample() * modAmount * 0.25f;
            unit->fdn.setDelay(unit->sampleRate, baseDelayMs, mod1, mod2);

            float wetL = 0.0f;
            float wetR = 0.0f;
            unit->fdn.process(left, right, wetL, wetR);

            const auto dry = 1.0f - dryWet;
            outL[n] = dry * inL[n] + dryWet * wetL;
            outR[n] = dry * inR[n] + dryWet * wetR;
        }
    }
}

void SimpleReverbCh_Ctor(SimpleReverbCh* unit)
{
    new (unit) SimpleReverbCh;
    unit->sampleRate = static_cast<float>(SAMPLERATE);

    const bool diffuserOK = unit->diffuser.allocate(unit->mWorld, unit->sampleRate);
    const bool fdnOK = diffuserOK && unit->fdn.allocate(unit->mWorld, unit->sampleRate);
    if (!(diffuserOK && fdnOK)) {
        unit->diffuser.free(unit->mWorld);
        unit->fdn.free(unit->mWorld);
        ClearUnitOnMemFailed
    }

    unit->diffuser.reset();
    unit->fdn.reset();

    unit->lfos[0].prepare(unit->sampleRate);
    unit->lfos[1].prepare(unit->sampleRate);
    unit->lfos[0].setFrequency(lfoFreqs[0]);
    unit->lfos[1].setFrequency(lfoFreqs[1]);

    unit->diffusionTime.reset(unit->sampleRate, clampDiffusion(IN0(InputDiffusionTime)));
    unit->fdnDelay.reset(unit->sampleRate, clampDelay(IN0(InputFDNDelay)));
    unit->fdnT60Low.reset(unit->sampleRate, clampT60(IN0(InputFDNT60Low)));
    unit->fdnT60High.reset(unit->sampleRate, clampT60(IN0(InputFDNT60High)));
    unit->modAmount.reset(unit->sampleRate, clampPercent(IN0(InputModAmount)));
    unit->dryWet.reset(unit->sampleRate, clampPercent(IN0(InputDryWet)));

    unit->diffuser.setDiffusionTimeMs(unit->diffusionTime.current, unit->sampleRate);
    unit->fdn.setDelay(unit->sampleRate, unit->fdnDelay.current, 1.0f, 1.0f);
    unit->fdn.setDecay(unit->sampleRate, unit->fdnT60Low.current, unit->fdnT60High.current);

    SETCALC(SimpleReverbCh_next);
    OUT0(0) = 0.0f;
    OUT0(1) = 0.0f;
}

void SimpleReverbCh_Dtor(SimpleReverbCh* unit)
{
    unit->diffuser.free(unit->mWorld);
    unit->fdn.free(unit->mWorld);
}
} // namespace

PluginLoad(SimpleReverbCh)
{
    ft = inTable;
    DefineDtorUnit(SimpleReverbCh);
}
