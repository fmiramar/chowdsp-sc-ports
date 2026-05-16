#pragma once

#include <cmath>

namespace autowah
{
template <typename SampleType>
class LevelDetector
{
public:
    void prepare(float sampleRate)
    {
        expFactor = -1000.0f / sampleRate;
        reset();
    }

    void reset()
    {
        yOld = static_cast<SampleType>(0);
        increasing = true;
    }

    void setParameters(float attackTimeMs, float releaseTimeMs)
    {
        tauAtt = static_cast<SampleType>(calcTimeConstant(attackTimeMs, expFactor));
        tauRel = static_cast<SampleType>(calcTimeConstant(releaseTimeMs, expFactor));
    }

    SampleType processSample(SampleType x) noexcept
    {
        const auto tau = increasing ? tauAtt : tauRel;
        x = yOld + tau * (x - yOld);
        increasing = x > yOld;
        yOld = x;
        return x;
    }

private:
    static float calcTimeConstant(float timeMs, float exponentFactor)
    {
        return timeMs < 1.0e-3f ? 0.0f : 1.0f - std::exp(exponentFactor / timeMs);
    }

    float expFactor = 0.0f;
    SampleType yOld = static_cast<SampleType>(0);
    bool increasing = true;
    SampleType tauAtt = static_cast<SampleType>(1);
    SampleType tauRel = static_cast<SampleType>(1);
};
} // namespace autowah
