#pragma once

#include <cmath>
#include <utility>

template <size_t MaxIter = 4>
struct DFLFilter
{
    float driveParam = 1.0f;
    float fbParam = 0.0f;
    float y1 = 0.0f;

    void reset() noexcept
    {
        y1 = 0.0f;
    }

    std::pair<float, float> process(float x, float b0, float z1) noexcept
    {
        for (size_t k = 0; k < MaxIter; ++k) {
            const float yDrive = y1 * driveParam;
            const float delta = -func(x, y1, yDrive, z1, b0, fbParam) / funcDeriv(yDrive, b0, fbParam);
            y1 += delta;
        }

        const float y0 = x + fbParam * nonlinear(y1 * driveParam);
        return { y0, y1 };
    }

private:
    float func(float x, float y, float yDrive, float state, float h0, float feedback) const noexcept
    {
        return y - h0 * (x + feedback * nonlinear(yDrive)) - state;
    }

    float funcDeriv(float yDrive, float h0, float feedback) const noexcept
    {
        return 1.0f - h0 * feedback * nonlinearDeriv(yDrive);
    }

    float nonlinear(float x) const noexcept
    {
        return std::tanh(x) / driveParam;
    }

    static float nonlinearDeriv(float x) noexcept
    {
        const float coshX = std::cosh(x);
        return 1.0f / (coshX * coshX);
    }
};
