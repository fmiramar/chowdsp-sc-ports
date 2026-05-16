#pragma once

#include "omega.h"
#include "signum.h"

#include <cmath>

namespace chowdsp::WDFT
{

class BaseWDF
{
public:
    virtual ~BaseWDF() = default;
    void connectToParent(BaseWDF* p) { parent = p; }
    virtual void calcImpedance() = 0;

    virtual void propagateImpedanceChange()
    {
        calcImpedance();
        if (parent != nullptr)
            parent->propagateImpedanceChange();
    }

protected:
    BaseWDF* parent = nullptr;
};

class RootWDF : public BaseWDF
{
public:
    void propagateImpedanceChange() override
    {
        calcImpedance();
    }
};

template <typename T>
struct WDFMembers
{
    T R = static_cast<T>(1.0e-9);
    T G = static_cast<T>(1) / R;
    T a = static_cast<T>(0);
    T b = static_cast<T>(0);
};

template <typename T>
class ResistorT final : public BaseWDF
{
public:
    explicit ResistorT(T value) : R_value(value) { calcImpedance(); }

    void setResistanceValue(T newR)
    {
        if (newR == R_value)
            return;
        R_value = newR;
        propagateImpedanceChange();
    }

    void calcImpedance() override
    {
        wdf.R = R_value;
        wdf.G = static_cast<T>(1) / wdf.R;
    }

    void incident(T x) noexcept { wdf.a = x; }
    T reflected() noexcept
    {
        wdf.b = static_cast<T>(0);
        return wdf.b;
    }

    WDFMembers<T> wdf;

private:
    T R_value = static_cast<T>(1.0e-9);
};

template <typename T>
class CapacitorT final : public BaseWDF
{
public:
    explicit CapacitorT(T value, T fs = static_cast<T>(48000.0)) : C_value(value), fs(fs)
    {
        calcImpedance();
    }

    void prepare(T sampleRate)
    {
        fs = sampleRate;
        propagateImpedanceChange();
        reset();
    }

    void reset()
    {
        z = static_cast<T>(0);
    }

    void calcImpedance() override
    {
        wdf.R = static_cast<T>(1) / (static_cast<T>(2) * C_value * fs);
        wdf.G = static_cast<T>(1) / wdf.R;
    }

    void incident(T x) noexcept
    {
        wdf.a = x;
        z = wdf.a;
    }

    T reflected() noexcept
    {
        wdf.b = z;
        return wdf.b;
    }

    WDFMembers<T> wdf;

private:
    T C_value = static_cast<T>(1.0e-6);
    T z = static_cast<T>(0);
    T fs = static_cast<T>(48000.0);
};

template <typename T, typename Port1Type, typename Port2Type>
class WDFParallelT final : public BaseWDF
{
public:
    WDFParallelT(Port1Type& p1, Port2Type& p2) : port1(p1), port2(p2)
    {
        port1.connectToParent(this);
        port2.connectToParent(this);
        calcImpedance();
    }

    void calcImpedance() override
    {
        wdf.G = port1.wdf.G + port2.wdf.G;
        wdf.R = static_cast<T>(1) / wdf.G;
        port1Reflect = port1.wdf.G / wdf.G;
    }

    void incident(T x) noexcept
    {
        const auto b2 = x + bTemp;
        port1.incident(bDiff + b2);
        port2.incident(b2);
        wdf.a = x;
    }

    T reflected() noexcept
    {
        port1.reflected();
        port2.reflected();
        bDiff = port2.wdf.b - port1.wdf.b;
        bTemp = static_cast<T>(0) - port1Reflect * bDiff;
        wdf.b = port2.wdf.b + bTemp;
        return wdf.b;
    }

    Port1Type& port1;
    Port2Type& port2;
    WDFMembers<T> wdf;

private:
    T port1Reflect = static_cast<T>(1);
    T bTemp = static_cast<T>(0);
    T bDiff = static_cast<T>(0);
};

template <typename T, typename Port1Type, typename Port2Type>
class WDFSeriesT final : public BaseWDF
{
public:
    WDFSeriesT(Port1Type& p1, Port2Type& p2) : port1(p1), port2(p2)
    {
        port1.connectToParent(this);
        port2.connectToParent(this);
        calcImpedance();
    }

    void calcImpedance() override
    {
        wdf.R = port1.wdf.R + port2.wdf.R;
        wdf.G = static_cast<T>(1) / wdf.R;
        port1Reflect = port1.wdf.R / wdf.R;
    }

    void incident(T x) noexcept
    {
        const auto b1 = port1.wdf.b - port1Reflect * (x + port1.wdf.b + port2.wdf.b);
        port1.incident(b1);
        port2.incident(static_cast<T>(0) - (x + b1));
        wdf.a = x;
    }

    T reflected() noexcept
    {
        wdf.b = static_cast<T>(0) - (port1.reflected() + port2.reflected());
        return wdf.b;
    }

    Port1Type& port1;
    Port2Type& port2;
    WDFMembers<T> wdf;

private:
    T port1Reflect = static_cast<T>(1);
};

template <typename T, typename PortType>
class PolarityInverterT final : public BaseWDF
{
public:
    explicit PolarityInverterT(PortType& p) : port1(p)
    {
        port1.connectToParent(this);
        calcImpedance();
    }

    void calcImpedance() override
    {
        wdf.R = port1.wdf.R;
        wdf.G = static_cast<T>(1) / wdf.R;
    }

    void incident(T x) noexcept
    {
        wdf.a = x;
        port1.incident(static_cast<T>(0) - x);
    }

    T reflected() noexcept
    {
        wdf.b = static_cast<T>(0) - port1.reflected();
        return wdf.b;
    }

    WDFMembers<T> wdf;

private:
    PortType& port1;
};

template <typename T>
class ResistiveVoltageSourceT final : public BaseWDF
{
public:
    explicit ResistiveVoltageSourceT(T value = static_cast<T>(1.0e-9)) : R_value(value)
    {
        calcImpedance();
    }

    void setResistanceValue(T newR)
    {
        if (newR == R_value)
            return;
        R_value = newR;
        propagateImpedanceChange();
    }

    void calcImpedance() override
    {
        wdf.R = R_value;
        wdf.G = static_cast<T>(1) / wdf.R;
    }

    void setVoltage(T newV) noexcept { Vs = newV; }
    void incident(T x) noexcept { wdf.a = x; }
    T reflected() noexcept
    {
        wdf.b = Vs;
        return wdf.b;
    }

    WDFMembers<T> wdf;

private:
    T Vs = static_cast<T>(0);
    T R_value = static_cast<T>(1.0e-9);
};

template <typename T, typename Next>
class IdealVoltageSourceT final : public RootWDF
{
public:
    explicit IdealVoltageSourceT(Next& next) : next(next)
    {
        next.connectToParent(this);
        calcImpedance();
    }

    void calcImpedance() override {}

    void setVoltage(T newV) noexcept { Vs = newV; }
    void incident(T x) noexcept { wdf.a = x; }
    T reflected() noexcept
    {
        wdf.b = static_cast<T>(0) - wdf.a + static_cast<T>(2) * Vs;
        return wdf.b;
    }

    WDFMembers<T> wdf;

private:
    Next& next;
    T Vs = static_cast<T>(0);
};

template <typename T, typename WDFType>
inline T voltage(const WDFType& wdf) noexcept
{
    return (wdf.wdf.a + wdf.wdf.b) * static_cast<T>(0.5);
}

template <typename T, typename WDFType>
inline T current(const WDFType& wdf) noexcept
{
    return (wdf.wdf.a - wdf.wdf.b) * ((static_cast<T>(0.5)) * wdf.wdf.G);
}

} // namespace chowdsp::WDFT
