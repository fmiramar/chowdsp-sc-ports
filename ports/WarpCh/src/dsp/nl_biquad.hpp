#pragma once

#include "filters.hpp"

#include <algorithm>
#include <cmath>

struct NLBiquad : public BiquadFilter
{
    float drive = 1.0f;

    void setDrive(float newDrive) noexcept
    {
        drive = std::max(newDrive, 1.0e-4f);
    }

    float process(float x) noexcept
    {
        const float y = z[1] + x * b[0];
        const float yDrive = std::tanh(y * drive) / drive;
        z[1] = z[2] + x * b[1] - yDrive * a[1];
        z[2] = x * b[2] - yDrive * a[2];
        return y;
    }
};
