#pragma once

#include <algorithm>
#include <cmath>

namespace polygonal
{
constexpr float pi = 3.14159265358979323846f;
constexpr float twoPi = 6.28318530717958647692f;

struct LinearSmoother
{
    float current = 1.0f;
    float target = 1.0f;
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

    void setTarget(float newTarget, float timeSeconds) noexcept
    {
        if (newTarget == target)
            return;

        target = newTarget;
        samplesRemaining = std::max(1, static_cast<int>(std::round(timeSeconds * sampleRate)));
        step = (target - current) / static_cast<float>(samplesRemaining);
    }

    float process() noexcept
    {
        if (samplesRemaining > 0) {
            current += step;
            --samplesRemaining;
            if (samplesRemaining <= 0) {
                current = target;
                samplesRemaining = 0;
                step = 0.0f;
            }
        }

        return current;
    }
};

inline float dbToGain(float db) noexcept
{
    return std::pow(10.0f, 0.05f * db);
}

class PolygonalOscillator
{
public:
    void prepare(float newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        reset();
        setFrequency(frequencyHz);
        setOrder(order);
    }

    void reset() noexcept
    {
        phase = 0.0f;
    }

    void reset(float newPhase) noexcept
    {
        phase = std::fmod(newPhase, twoPi);
        if (phase < 0.0f)
            phase += twoPi;
    }

    void setFrequency(float newFrequencyHz) noexcept
    {
        frequencyHz = newFrequencyHz;
        deltaPhase = twoPi * frequencyHz / sampleRate;
    }

    void setOrder(float newOrder) noexcept
    {
        order = newOrder;
        reciprocalOrder = 1.0f / order;
        cosPiOverOrder = std::cos(pi * reciprocalOrder);
    }

    void setTeeth(float newTeeth) noexcept
    {
        teeth = newTeeth;
    }

    float processSample() noexcept
    {
        const float numerator = cosPiOverOrder * std::sin(phase);
        const float foldedPhase = std::fmod(phase * order, twoPi);
        float denominator = std::cos(reciprocalOrder * foldedPhase - pi * reciprocalOrder + teeth);

        // Guard against exact singularities so the server never emits inf/nan samples.
        if (std::abs(denominator) < 1.0e-6f)
            denominator = denominator >= 0.0f ? 1.0e-6f : -1.0e-6f;

        const float y = numerator / denominator;

        phase += deltaPhase;
        if (phase >= twoPi)
            phase -= twoPi;
        else if (phase < 0.0f)
            phase += twoPi;

        return y;
    }

private:
    float sampleRate = 48000.0f;
    float frequencyHz = 1000.0f;
    float order = 3.5f;
    float reciprocalOrder = 1.0f / 3.5f;
    float cosPiOverOrder = 0.0f;
    float teeth = 0.0f;
    float phase = 0.0f;
    float deltaPhase = 0.0f;
};
} // namespace polygonal
