#pragma once

#include <cmath>

namespace chorus
{

class SineWave
{
public:
    void setFrequency(float newFrequency) noexcept
    {
        constexpr float pi = 3.14159265358979323846f;
        freq = newFrequency;
        eps = 2.0f * std::sin(pi * newFrequency / fs);
    }

    void prepare(double sampleRate) noexcept
    {
        fs = static_cast<float>(sampleRate);
        reset();
    }

    void reset() noexcept
    {
        x1 = -1.0f;
        x2 = 0.0f;
    }

    void reset(float phase) noexcept
    {
        x1 = std::sin(phase);
        x2 = std::cos(phase);
    }

    float processSample() noexcept
    {
        const auto y = x2;
        x1 += eps * x2;
        x2 -= eps * x1;
        return y;
    }

private:
    float x1 = 0.0f;
    float x2 = 0.0f;
    float eps = 0.0f;
    float freq = 0.0f;
    float fs = 44100.0f;
};

} // namespace chorus
