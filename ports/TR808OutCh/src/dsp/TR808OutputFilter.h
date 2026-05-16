#pragma once

#include "wdf_t.h"

#include <cmath>

namespace wdft = chowdsp::WDFT;

class TR808OutputFilter
{
public:
    TR808OutputFilter() = default;

    void prepare(double sampleRate);
    void reset();

    void setVolume(float vol);
    void setTone(float tone);

    inline float processSample(float x)
    {
        Vbtr.setVoltage(x);

        I2.incident(inv4.reflected());
        const auto voltageOut = wdft::voltage<float>(r174);
        inv4.incident(I2.reflected());

        return voltageOut;
    }

private:
    wdft::ResistorT<float> r172 { 10000.0f };
    wdft::ResistorT<float> r174 { 100000.0f };
    wdft::ResistorT<float> r177 { 82000.0f };

    wdft::ResistorT<float> vr4 { 100000.0f };
    wdft::ResistorT<float> vr4neg { 0.1f };
    wdft::ResistorT<float> vr5 { 10000.0f };

    wdft::ResistiveVoltageSourceT<float> Vbtr { 220.0f };
    wdft::ResistiveCurrentSourceT<float> I1 { 100000.0f };
    wdft::ResistiveVoltageSourceT<float> VnegB2 { 6800.0f };

    wdft::CapacitorT<float> c45 { 0.1e-6f };
    wdft::CapacitorT<float> c47 { 0.47e-6f };
    wdft::CapacitorT<float> c48 { 220.0e-9f };
    wdft::CapacitorT<float> c49 { 0.47e-6f };
    wdft::CapacitorT<float> c50 { 1.0e-6f };

    wdft::PolarityInverterT<float, decltype(Vbtr)> inv1 { Vbtr };
    wdft::WDFParallelT<float, decltype(vr5), decltype(r172)> p1 { vr5, r172 };
    wdft::WDFSeriesT<float, decltype(inv1), decltype(p1)> s1 { inv1, p1 };

    wdft::WDFParallelT<float, decltype(s1), decltype(c45)> p2 { s1, c45 };
    wdft::WDFSeriesT<float, decltype(c47), decltype(vr4)> s3 { c47, vr4 };
    wdft::WDFSeriesT<float, decltype(s3), decltype(p2)> s2 { s3, p2 };

    wdft::WDFParallelT<float, decltype(vr4neg), decltype(s2)> p3 { vr4neg, s2 };
    wdft::WDFSeriesT<float, decltype(c49), decltype(r177)> s5 { c49, r177 };
    wdft::WDFSeriesT<float, decltype(s5), decltype(p3)> s4 { s5, p3 };

    wdft::PolarityInverterT<float, decltype(I1)> inv2 { I1 };
    wdft::WDFParallelT<float, decltype(s4), decltype(inv2)> p4 { s4, inv2 };

    wdft::YParameterT<float, decltype(p4)> y { p4, 0.261273e-3f, -2.61273e-4f, -7.86431e-2f, 78.6431e-3f };

    wdft::PolarityInverterT<float, decltype(r174)> inv3 { r174 };
    wdft::WDFSeriesT<float, decltype(inv3), decltype(c50)> s6 { inv3, c50 };
    wdft::WDFParallelT<float, decltype(s6), decltype(y)> p5 { s6, y };

    wdft::WDFParallelT<float, decltype(c48), decltype(p5)> p6 { c48, p5 };
    wdft::WDFParallelT<float, decltype(p6), decltype(VnegB2)> p7 { p6, VnegB2 };

    wdft::PolarityInverterT<float, decltype(p7)> inv4 { p7 };
    wdft::IdealCurrentSourceT<float, decltype(inv4)> I2 { inv4 };
};
