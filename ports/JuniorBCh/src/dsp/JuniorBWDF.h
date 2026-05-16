#pragma once

#include "modified_rtype.hpp"

namespace juniorbch
{
using namespace chowdsp::WDFT;

template <typename T>
class ResistorCapacitorParallelT final : public BaseWDF
{
public:
    ResistorCapacitorParallelT(T resistance, T capacitance)
        : resistor(resistance), capacitor(capacitance), parallel(resistor, capacitor)
    {
        parallel.connectToParent(this);
        calcImpedance();
    }
    void prepare(T sampleRate) { capacitor.prepare(sampleRate); }
    void setResistanceValue(T resistance) { resistor.setResistanceValue(resistance); }
    void setCapacitanceValue(T capacitance) { capacitor.setCapacitanceValue(capacitance); }
    void calcImpedance() override { wdf = parallel.wdf; }
    void incident(T x) noexcept { parallel.incident(x); wdf.a = parallel.wdf.a; }
    T reflected() noexcept { wdf.b = parallel.reflected(); return wdf.b; }
    WDFMembers<T> wdf;
private:
    ResistorT<T> resistor;
    CapacitorT<T> capacitor;
    WDFParallelT<T, ResistorT<T>, CapacitorT<T>> parallel;
};

template <typename Float, typename TriodeModelType>
struct JuniorBWDF
{
    explicit JuniorBWDF(TriodeModelType& model) : triode_model(model) {}

    void prepare(Float sampleRate)
    {
        R.prepare(sampleRate);
        C24.prepare(sampleRate);
        R4_C22.prepare(sampleRate);
        R5_C2.prepare(sampleRate);
        C1.prepare(sampleRate);
        Vbb.setVoltage((Float) 232.0);
    }

    inline Float process(Float x) noexcept
    {
        Vsig.setVoltage(x);
        const auto* up = R.reflected();
        Float triodeIn[2] { up[0], up[1] };
        R.incident(triode_model.compute(triodeIn));
        return voltage<Float>(R6);
    }

    ResistiveVoltageSourceT<Float> Vsig { (Float) 1.0e3 };
    PolarityInverterT<Float, decltype(Vsig)> Isig { Vsig };
    CapacitorT<Float> C24 { (Float) 220.0e-12 };
    WDFParallelT<Float, decltype(Isig), decltype(C24)> P0 { Isig, C24 };
    ResistorT<Float> R01 { (Float) 10.0e3 };
    WDFSeriesT<Float, decltype(P0), decltype(R01)> S0 { P0, R01 };
    ResistorT<Float> R2 { (Float) 1.0e6 };
    WDFParallelT<Float, decltype(S0), decltype(R2)> P1 { S0, R2 };
    ResistorCapacitorParallelT<Float> R4_C22 { (Float) 1.5e3, (Float) 47.0e-6 };
    ResistorCapacitorParallelT<Float> R5_C2 { (Float) 470.0e3, (Float) 22.0e-12 };
    ResistorT<Float> R6 { (Float) 22.0e3 };
    WDFSeriesT<Float, decltype(R5_C2), decltype(R6)> S2 { R5_C2, R6 };
    CapacitorT<Float> C1 { (Float) 0.022e-6 };
    WDFSeriesT<Float, decltype(C1), decltype(S2)> S1 { C1, S2 };
    ResistiveVoltageSourceT<Float> Vbb { (Float) 125.0e3 };
    WDFParallelT<Float, decltype(Vbb), decltype(S1)> P3 { Vbb, S1 };

    struct ImpedanceCalc
    {
        template <typename RType>
        static void calcImpedance(RType& R)
        {
            const auto fs = R.getSampleRate();
            const auto sampleRateFactor = fs / (Float) 96000;
            const auto Rcomp = (Float) 1000 / std::pow(sampleRateFactor, sampleRateFactor <= (Float) 1 ? (Float) 0.2 : (Float) 0.29);
            const auto Rg = Rcomp;
            const auto Rp = Rcomp;
            const auto [Ri, Rk, Ro] = R.getPortImpedances();
            R.setSMatrixData({ { -((Rg - Ri) * Rk + (Rg - Ri - Rk) * Ro + (Rg - Ri - Rk) * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * Rg * Rk / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * (Rg * Rk + Rg * Ro + Rg * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), -2 * (Rg * Ro + Rg * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), -2 * Rg * Rk / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp) },
                                { 2 * Rk * Rp / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro - (Rg + Ri + Rk) * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), -2 * Rk * Rp / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), -2 * (Rg + Ri) * Rp / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * (Rg + Ri + Rk) * Rp / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp) },
                                { 2 * (Ri * Rk + Ri * Ro + Ri * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), -2 * Ri * Rk / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), ((Rg - Ri) * Rk + (Rg - Ri + Rk) * Ro + (Rg - Ri + Rk) * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * (Ri * Ro + Ri * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * Ri * Rk / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp) },
                                { -2 * (Rk * Ro + Rk * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), -2 * (Rg + Ri) * Rk / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * (Rk * Ro + Rk * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), -((Rg + Ri) * Rk - (Rg + Ri - Rk) * Ro - (Rg + Ri - Rk) * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * (Rg + Ri) * Rk / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp) },
                                { -2 * Rk * Ro / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * (Rg + Ri + Rk) * Ro / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * Rk * Ro / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), 2 * (Rg + Ri) * Ro / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp), ((Rg + Ri) * Rk - (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp) / ((Rg + Ri) * Rk + (Rg + Ri + Rk) * Ro + (Rg + Ri + Rk) * Rp) } });
        }
    };

    using RType = juniorbwdft::RtypeAdaptorMultN<Float, 2, ImpedanceCalc, decltype(P1), decltype(R4_C22), decltype(P3)>;
    RType R { P1, R4_C22, P3 };
    TriodeModelType& triode_model;
};
} // namespace juniorbch
