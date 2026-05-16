#pragma once

#include <algorithm>
#include <cmath>

namespace arpfilter
{
constexpr float pi = 3.14159265358979323846f;

enum class Mode
{
    Lowpass = 0,
    Bandpass = 1,
    Highpass = 2,
    Notch = 3
};

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

class StateVariableFilter
{
public:
    void prepare(float newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        reset();
        setCutoffFrequency(cutoffFrequency);
    }

    void reset() noexcept
    {
        ic1eq = 0.0f;
        ic2eq = 0.0f;
    }

    void setCutoffFrequency(float newCutoffFrequencyHz) noexcept
    {
        cutoffFrequency = std::clamp(newCutoffFrequencyHz, 0.0f, sampleRate * 0.499f);
        const float w = pi * cutoffFrequency / sampleRate;
        g0 = std::tan(w);
        update();
    }

    void setQValue(float newResonance) noexcept
    {
        resonance = std::max(newResonance, 1.0e-6f);
        k0 = 1.0f / resonance;
        update();
    }

    void update() noexcept
    {
        const float g = g0;
        const float k = k0;
        const float gk = g + k;
        a1 = 1.0f / (1.0f + g * gk);
        a2 = g * a1;
        a3 = g * a2;
        ak = gk * a1;
    }

    void processCore(float x, float& v0, float& v1, float& v2) noexcept
    {
        const float v3 = x - ic2eq;
        v0 = a1 * v3 - ak * ic1eq;
        v1 = a2 * v3 + a1 * ic1eq;
        v2 = a3 * v3 + a2 * ic1eq + ic2eq;

        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;
    }

    float k0 = 1.4142135623730951f;

private:
    float sampleRate = 48000.0f;
    float cutoffFrequency = 1000.0f;
    float resonance = 0.7071067811865476f;
    float g0 = 0.0f;
    float a1 = 1.0f;
    float a2 = 0.0f;
    float a3 = 0.0f;
    float ak = 0.0f;
    float ic1eq = 0.0f;
    float ic2eq = 0.0f;
};

class ARPFilter
{
public:
    void prepare(float newSampleRate) noexcept
    {
        filter.prepare(newSampleRate);
    }

    void reset() noexcept
    {
        filter.reset();
    }

    void setCutoffFrequency(float hz) noexcept
    {
        filter.setCutoffFrequency(hz);
    }

    void setQValue(float q) noexcept
    {
        filter.setQValue(q);
    }

    void setLimitMode(bool shouldLimitModeBeOn) noexcept
    {
        useLimitMode = shouldLimitModeBeOn;
    }

    float process(float x, Mode mode, float notchMix) noexcept
    {
        const float inputMult = useLimitMode ? filter.k0 : 1.0f;
        float v0 {};
        float v1 {};
        float v2 {};
        filter.processCore(inputMult * x, v0, v1, v2);

        switch (mode) {
            case Mode::Lowpass:
                return v2;
            case Mode::Bandpass:
                return v1;
            case Mode::Highpass:
                return v0;
            case Mode::Notch:
            default: {
                const float lowMix = notchMix * 0.5f + 0.5f;
                const float highMix = 1.0f - lowMix;
                const float makeup = 2.0f - std::abs(notchMix);
                return makeup * (lowMix * v2 + highMix * v0);
            }
        }
    }

private:
    bool useLimitMode = false;
    StateVariableFilter filter;
};
} // namespace arpfilter
