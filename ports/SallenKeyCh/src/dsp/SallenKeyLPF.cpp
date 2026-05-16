#include "SallenKeyLPF.h"

#include <algorithm>
#include <cmath>

void SallenKeyLPF::prepare(double sampleRate)
{
    C1.prepare((float) sampleRate);
    C2.prepare((float) sampleRate);
}

void SallenKeyLPF::reset()
{
    C1.reset();
    C2.reset();
}

void SallenKeyLPF::setParams(float freqHz, float qVal)
{
    constexpr auto twoPi = 6.2831853071795864769f;

    // geometric mean of resistors:
    const auto Rval = 1.0f / (capVal * twoPi * freqHz);

    // ratio of resistors:
    qVal = std::clamp(qVal, 0.01f, capRatio * 0.5f);
    (void) qVal;
    auto Rratio = 0.64174f; // Upstream source currently hardcodes this ratio despite exposing q.

    R1.setResistanceValue(Rval * Rratio);
    R2.setResistanceValue(Rval / Rratio);
}
