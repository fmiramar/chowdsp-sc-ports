#pragma once

#include "../../../shared/wdf/wdf_t.h"

#include <cmath>

using namespace chowdsp::WDFT;

class LCOscillator
{
public:
    LCOscillator() = default;

    void prepare(double sampleRate)
    {
        C1.prepare((float) sampleRate);
        I1.prepare((float) sampleRate);
    }

    void reset()
    {
        C1.reset();
        I1.reset();
    }

    void setCircuitParams(float freq, bool switchClosed)
    {
        constexpr auto cap = 1.0e-6f;
        constexpr auto twoPi = 6.2831853071795864769f;
        const auto ind = 1.0f / std::pow(twoPi * freq, 2.0f) / cap;

        C1.setCapacitanceValue(cap);
        I1.setInductanceValue(ind);
        Sw1.setClosed(switchClosed);
    }

    inline float processSample(float x)
    {
        Vs.setVoltage(x);

        Sw1.incident(S1.reflected());
        S1.incident(Sw1.reflected());

        return voltage<float>(C1);
    }

private:
    CapacitorT<float> C1 { 1.0e-3f };
    InductorT<float> I1 { 1.0e3f };
    WDFSeriesT<float, decltype(I1), decltype(C1)> S2 { I1, C1 };

    ResistiveVoltageSourceT<float> Vs;
    WDFSeriesT<float, decltype(Vs), decltype(S2)> S1 { Vs, S2 };

    SwitchT<float, decltype(S1)> Sw1 { S1 };
};
