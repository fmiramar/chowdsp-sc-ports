#pragma once

#include "bbd_filter_bank.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace chorus
{

template <size_t STAGES>
class BBDDelayLine
{
public:
    void prepare(double sampleRate) noexcept
    {
        FS = static_cast<float>(sampleRate);
        Ts = 1.0f / FS;
        Ts_bbd = Ts;

        buffer.fill(0.0f);
        bufferPtr = 0;
        yBBDOld = 0.0f;
        tn = 0.0f;
        evenOn = true;

        inputFilter.reset(Ts);
        outputFilter.reset(Ts);
        H0 = outputFilter.calcH0();
    }

    void setFilterFreq(float freqHz)
    {
        inputFilter.setFreq(freqHz);
        inputFilter.setTime(tn);

        outputFilter.setFreq(freqHz);
        outputFilter.setTime(tn);
    }

    void setDelayTime(float delaySec) noexcept
    {
        const auto safeDelay = std::max(delaySec, 1.0e-6f);
        const auto clockRateHz = (2.0f * static_cast<float>(STAGES)) / safeDelay;
        Ts_bbd = 1.0f / clockRateHz;

        const auto doubleTs = 2.0f * Ts_bbd;
        inputFilter.setDelta(doubleTs);
        outputFilter.setDelta(doubleTs);
    }

    float process(float input) noexcept
    {
        ComplexVec xOutAccum;
        lastStepCount = 0;

        while (tn < Ts) {
            ++lastStepCount;
            if (evenOn) {
                inputFilter.calcG();
                const auto sum = (inputFilter.Gcalc * inputFilter.x).sumReal();
                buffer[bufferPtr++] = flushSubnormal(sum);
                bufferPtr = (bufferPtr < STAGES) ? bufferPtr : 0;
            } else {
                const auto yBBD = flushSubnormal(buffer[bufferPtr]);
                const auto delta = flushSubnormal(yBBD - yBBDOld);
                yBBDOld = yBBD;
                outputFilter.calcG();
                xOutAccum += outputFilter.Gcalc * delta;
            }

            evenOn = !evenOn;
            tn += Ts_bbd;
        }

        tn = flushSubnormal(tn - Ts);
        yBBDOld = flushSubnormal(yBBDOld);

        inputFilter.process(input);
        outputFilter.process(xOutAccum);
        return flushSubnormal(H0 * yBBDOld + xOutAccum.sumReal());
    }

    size_t debugLastStepCount() const noexcept { return lastStepCount; }

    int debugSubnormalCount() const noexcept
    {
        int count = inputFilter.debugSubnormalCount() + outputFilter.debugSubnormalCount();
        if (std::abs(yBBDOld) > 0.0f && std::abs(yBBDOld) < std::numeric_limits<float>::min())
            ++count;
        if (std::abs(tn) > 0.0f && std::abs(tn) < std::numeric_limits<float>::min())
            ++count;
        if (std::abs(Ts_bbd) > 0.0f && std::abs(Ts_bbd) < std::numeric_limits<float>::min())
            ++count;
        return count;
    }

    float debugMaxAbs() const noexcept
    {
        return std::max({ inputFilter.debugMaxAbs(), outputFilter.debugMaxAbs(), std::abs(yBBDOld), std::abs(tn), std::abs(Ts_bbd) });
    }

private:
    float FS = 48000.0f;
    float Ts = 1.0f / FS;
    float Ts_bbd = Ts;

    InputFilterBank inputFilter;
    OutputFilterBank outputFilter;
    float H0 = 1.0f;

    std::array<float, STAGES> buffer {};
    size_t bufferPtr = 0;

    float yBBDOld = 0.0f;
    float tn = 0.0f;
    bool evenOn = true;
    size_t lastStepCount = 0;
};

} // namespace chorus
