#pragma once

#include "wdf_t.h"

class ClippingStage
{
public:
    explicit ClippingStage(float sampleRate = 96000.0f) : C9(1.0e-6f, sampleRate) {}

    void prepare(float sampleRate)
    {
        C9.prepare(sampleRate);
    }

    float processSample(float x) noexcept
    {
        Vin.setVoltage(x);

        D23.incident(S2.reflected());
        S2.incident(D23.reflected());
        return chowdsp::WDFT::voltage<float>(D23) * 10.0f;
    }

private:
    chowdsp::WDFT::ResistiveVoltageSourceT<float> Vin;
    chowdsp::WDFT::PolarityInverterT<float, decltype(Vin)> I1 { Vin };

    chowdsp::WDFT::ResistorT<float> R13 { 1000.0f };
    chowdsp::WDFT::WDFSeriesT<float, decltype(I1), decltype(R13)> S2 { I1, R13 };

    chowdsp::WDFT::CapacitorT<float> C9;
    chowdsp::WDFT::WDFParallelT<float, decltype(S2), decltype(C9)> P1 { S2, C9 };

    chowdsp::WDFT::DiodePairT<float, decltype(P1)> D23 { P1, 15.0e-6f };
};
