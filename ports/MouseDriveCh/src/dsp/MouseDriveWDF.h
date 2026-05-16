#pragma once

#include "../../../shared/wdf/omega.h"
#include "../../../shared/wdf/r_type.h"
#include "../../../shared/wdf/wdf_t.h"

namespace mousedrivech
{
using namespace chowdsp::WDFT;

template <typename T>
class ResistorCapacitorSeriesT final : public BaseWDF
{
public:
    ResistorCapacitorSeriesT(T resistance, T capacitance)
        : resistor(resistance)
        , capacitor(capacitance)
        , series(resistor, capacitor)
    {
        series.connectToParent(this);
        calcImpedance();
    }

    void prepare(T sampleRate)
    {
        capacitor.prepare(sampleRate);
    }

    void reset()
    {
        capacitor.reset();
    }

    void setResistanceValue(T resistance)
    {
        resistor.setResistanceValue(resistance);
    }

    void setCapacitanceValue(T capacitance)
    {
        capacitor.setCapacitanceValue(capacitance);
    }

    void calcImpedance() override
    {
        wdf = series.wdf;
    }

    void incident(T x) noexcept
    {
        series.incident(x);
        wdf.a = series.wdf.a;
    }

    T reflected() noexcept
    {
        wdf.b = series.reflected();
        return wdf.b;
    }

    WDFMembers<T> wdf;

private:
    ResistorT<T> resistor;
    CapacitorT<T> capacitor;
    WDFSeriesT<T, ResistorT<T>, CapacitorT<T>> series;
};

template <typename T>
class ResistorCapacitorParallelT final : public BaseWDF
{
public:
    ResistorCapacitorParallelT(T resistance, T capacitance)
        : resistor(resistance)
        , capacitor(capacitance)
        , parallel(resistor, capacitor)
    {
        parallel.connectToParent(this);
        calcImpedance();
    }

    void prepare(T sampleRate)
    {
        capacitor.prepare(sampleRate);
    }

    void reset()
    {
        capacitor.reset();
    }

    void setResistanceValue(T resistance)
    {
        resistor.setResistanceValue(resistance);
    }

    void setCapacitanceValue(T capacitance)
    {
        capacitor.setCapacitanceValue(capacitance);
    }

    void calcImpedance() override
    {
        wdf = parallel.wdf;
    }

    void incident(T x) noexcept
    {
        parallel.incident(x);
        wdf.a = parallel.wdf.a;
    }

    T reflected() noexcept
    {
        wdf.b = parallel.reflected();
        return wdf.b;
    }

    WDFMembers<T> wdf;

private:
    ResistorT<T> resistor;
    CapacitorT<T> capacitor;
    WDFParallelT<T, ResistorT<T>, CapacitorT<T>> parallel;
};

template <typename T>
class CapacitiveVoltageSourceT final : public BaseWDF
{
public:
    explicit CapacitiveVoltageSourceT(T capacitance, T sampleRate = static_cast<T>(48000.0))
        : capacitor(capacitance, sampleRate)
    {
        calcImpedance();
    }

    void prepare(T sampleRate)
    {
        capacitor.prepare(sampleRate);
    }

    void reset()
    {
        capacitor.reset();
    }

    void setCapacitanceValue(T capacitance)
    {
        capacitor.setCapacitanceValue(capacitance);
    }

    void setVoltage(T newVoltage) noexcept
    {
        voltage = newVoltage;
    }

    void calcImpedance() override
    {
        wdf = capacitor.wdf;
    }

    void incident(T x) noexcept
    {
        capacitor.incident(x + voltage);
        wdf.a = capacitor.wdf.a - voltage;
    }

    T reflected() noexcept
    {
        wdf.b = capacitor.reflected() + voltage;
        return wdf.b;
    }

    WDFMembers<T> wdf;

private:
    CapacitorT<T> capacitor;
    T voltage = static_cast<T>(0);
};

struct OmegaProvider
{
    template <typename T>
    static T omega(T x)
    {
        return chowdsp::Omega::omega4(x);
    }
};

class MouseDriveWDF
{
public:
    static constexpr float Rdistortion = 100.0e3f;

    void prepare(double sampleRate)
    {
        Vin_C1.prepare((float) sampleRate);
        C2.prepare((float) sampleRate);
        Rd_C4.prepare((float) sampleRate);
        R4_C5.prepare((float) sampleRate);
        R5_C6.prepare((float) sampleRate);
        R6_C7.prepare((float) sampleRate);
        R2.setVoltage(4.5f);
    }

    void reset()
    {
        Vin_C1.reset();
        C2.reset();
        Rd_C4.reset();
        R4_C5.reset();
        R5_C6.reset();
        R6_C7.reset();
    }

    void setDistortionResistance(float resistance) noexcept
    {
        Rd_C4.setResistanceValue(resistance);
    }

    float process(float x) noexcept
    {
        Vin_C1.setVoltage(x);
        diodes.incident(Sd.reflected());
        const auto y = voltage<float>(diodes);
        Sd.incident(diodes.reflected());
        return y;
    }

private:
    CapacitiveVoltageSourceT<float> Vin_C1 { 22.0e-9f };
    ResistiveVoltageSourceT<float> R2 { 1.0e6f };
    WDFParallelT<float, decltype(Vin_C1), decltype(R2)> P1 { Vin_C1, R2 };

    ResistorT<float> R3 { 1.0e3f };
    WDFSeriesT<float, decltype(P1), decltype(R3)> S2 { P1, R3 };

    CapacitorT<float> C2 { 1.0e-9f };
    WDFParallelT<float, decltype(S2), decltype(C2)> Pa { S2, C2 };

    ResistorCapacitorSeriesT<float> R4_C5 { 47.0f, 2.2e-6f };
    ResistorCapacitorSeriesT<float> R5_C6 { 560.0f, 4.7e-6f };
    WDFParallelT<float, decltype(R4_C5), decltype(R5_C6)> Pb { R4_C5, R5_C6 };

    ResistorCapacitorParallelT<float> Rd_C4 { 0.5f * Rdistortion, 100.0e-12f };

    struct ImpedanceCalc
    {
        template <typename RType>
        static float calcImpedance(RType& R)
        {
            constexpr float Ag = 100.0f;
            constexpr float Ri = 10.0e6f;
            constexpr float Ro = 1.0e-1f;

            const auto [Ra, Rb, Rc] = R.getPortImpedances();
            const auto Rd = -(((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro)
                              / (Ra * Rb + Ra * Rc + Rb * Rc + Rb * Ri + Ag * Rb * Ri + Rc * Ri - (Ra + Rb + Ri) * Ro));

            R.setSMatrixData({ { (Ra * Rd * (Rb + Rc - Ro) + Rc * Ri * Ro + Rb * (Rc + Ri) * Ro) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), (Ra * (-(Rc * Rd) + (Rc + Rd) * Ro)) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), (Ra * Rb * (Rd - Ro)) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), -((Ra * Rb) / (Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri))) },
                                { (-(Rb * Rd * (Rc + Ag * Ri)) + Rb * (Rc + Rd) * Ro) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), -((Rc * Rd * (Ra + Ri) + (Ra * (Rb - Rd) - Rd * Ri + Rb * (Rc + Ri)) * Ro) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro)), (Rb * (Ra + Ri) * (Rd - Ro)) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), -((Rb * (Ra + Ri)) / (Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri))) },
                                { -((Rc * (Ag * Rd * Ri + Rb * (-Rd + Ro))) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro)), (Rc * Rd * (Ra + Ri + Ag * Ri) - Rc * (Ra + Ri) * Ro) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), (Rc * Rd * (Ra + Rb + Ri) + Rb * (Ra + Ri) * Ro) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), -((Rc * (Ra + Rb + Ri)) / (Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri))) },
                                { (Rd * (Ag * (Rb + Rc) * Ri - Rb * Ro)) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro), -((Rd * (Ag * Rc * Ri + (Ra + Ri) * Ro)) / ((Ra * (Rb + Rc) + Rc * Ri + Rb * (Rc + Ri)) * Ro)), -((Rd + Ro) / Ro), 0 } });

            return Rd;
        }
    };

    RtypeAdaptor<float, 3, ImpedanceCalc, decltype(Pa), decltype(Pb), decltype(Rd_C4)> R { Pa, Pb, Rd_C4 };
    ResistorCapacitorSeriesT<float> R6_C7 { 1.0e3f, 4.7e-6f };
    WDFSeriesT<float, decltype(R), decltype(R6_C7)> Sd { R, R6_C7 };
    DiodePairT<float, decltype(Sd), DiodeQuality::Best> diodes { Sd, 5.0e-9f, 25.85e-3f, 2.0f };
};

} // namespace mousedrivech
