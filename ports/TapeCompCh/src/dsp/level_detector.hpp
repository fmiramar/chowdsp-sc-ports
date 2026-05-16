#pragma once

#include <cmath>

namespace tapecomp
{
template <typename SampleType>
class LevelDetector
{
public:
    LevelDetector() = default;

    void setParameters(float attackTimeMs, float releaseTimeMs)
    {
        tauAtt = static_cast<SampleType>(calcTimeConstant(attackTimeMs, expFactor));
        tauRel = static_cast<SampleType>(calcTimeConstant(releaseTimeMs, expFactor));
    }

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

    inline SampleType processSample(SampleType x) noexcept
    {
        const auto tau = increasing ? tauAtt : tauRel;
        x = yOld + tau * (x - yOld);

        increasing = x > yOld;
        yOld = x;

        return x;
    }

private:
    static inline float calcTimeConstant(float timeMs, float exponentFactor)
    {
        return timeMs < 1.0e-3f ? 0.0f : 1.0f - std::exp(exponentFactor / timeMs);
    }

    float expFactor = 0.0f;
    SampleType yOld = static_cast<SampleType>(0);
    bool increasing = true;

    SampleType tauAtt = static_cast<SampleType>(1);
    SampleType tauRel = static_cast<SampleType>(1);
};
} // namespace tapecomp
