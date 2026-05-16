#pragma once

#include "denormal.hpp"
#include "wdf_t.h"

#include <cmath>

namespace dirtytubech
{

class TubeProc
{
public:
    explicit TubeProc(double sampleRate)
        : Cpk(0.33e-12, sampleRate, alpha)
        , Cgp(1.7e-12, sampleRate, alpha)
        , Cgk(1.6e-12, sampleRate, alpha)
    {
        Vp = 0.0;
    }

    void reset() noexcept
    {
        Cpk.reset();
        Cgp.reset();
        Cgk.reset();
        Vp = 0.0;
    }

    double processSample(double Vg) noexcept
    {
        Vin.setVoltage(Vg);
        Is.setCurrent(getCurrent(Vg, Vp));

        D1.incident(S2.reflected());
        S2.incident(D1.reflected());
        Vp = chowdsp::WDFT::voltage<double>(Cpk);

        return flushSubnormal(Vp);
    }

private:
    template <typename T>
    static int sgn(T val) noexcept
    {
        return (T(0) < val) - (val < T(0));
    }

    template <typename T>
    static T pwrs(T x, T y) noexcept
    {
        return static_cast<T>(sgn(x)) * std::pow(std::abs(x), y);
    }

    static double getCurrent(double curVg, double curVp) noexcept
    {
        return 2.0e-9 * pwrs(0.1 * curVg - 0.01 * curVp, 0.33);
    }

    using ResIs = chowdsp::WDFT::ResistiveCurrentSourceT<double>;
    using ResVs = chowdsp::WDFT::ResistiveVoltageSourceT<double>;
    using Res = chowdsp::WDFT::ResistorT<double>;
    using Cap = chowdsp::WDFT::CapacitorAlphaT<double>;

    ResIs Is;
    ResVs Vin;
    Res Rgk { 2.7e3 };

    Cap Cpk;
    Cap Cgp;
    Cap Cgk;

    chowdsp::WDFT::WDFParallelT<double, Cap, ResIs> P1 { Cpk, Is };
    chowdsp::WDFT::WDFSeriesT<double, Cap, decltype(P1)> S1 { Cgp, P1 };
    chowdsp::WDFT::WDFParallelT<double, Cap, decltype(S1)> P2 { Cgk, S1 };
    chowdsp::WDFT::PolarityInverterT<double, ResVs> I1 { Vin };
    chowdsp::WDFT::WDFParallelT<double, decltype(I1), decltype(P2)> P3 { I1, P2 };
    chowdsp::WDFT::WDFSeriesT<double, Res, decltype(P3)> S2 { Rgk, P3 };

    chowdsp::WDFT::DiodeT<double, decltype(S2)> D1 { S2, 1.0e-10 };

    double Vp = 0.0;
    static constexpr double alpha = 0.4;
};

} // namespace dirtytubech
