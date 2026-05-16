#pragma once

#include "complex_vec.hpp"

namespace delaybb
{
namespace FilterSpec
{
inline const std::array<Complex, count> inputRoots {
    Complex { -10329.2715f, -329.848f },
    Complex { -10329.2715f, +329.848f },
    Complex { 366.990557f, -1811.4318f },
    Complex { 366.990557f, +1811.4318f },
};

inline const std::array<Complex, count> inputPoles {
    Complex { -55482.0f, -25082.0f },
    Complex { -55482.0f, +25082.0f },
    Complex { -26292.0f, -59437.0f },
    Complex { -26292.0f, +59437.0f },
};

inline const std::array<Complex, count> outputRoots {
    Complex { -11256.0f, -99566.0f },
    Complex { -11256.0f, +99566.0f },
    Complex { -13802.0f, -24606.0f },
    Complex { -13802.0f, +24606.0f },
};

inline const std::array<Complex, count> outputPoles {
    Complex { -51468.0f, -21437.0f },
    Complex { -51468.0f, +21437.0f },
    Complex { -26276.0f, -59699.0f },
    Complex { -26276.0f, +59699.0f },
};
} // namespace FilterSpec

struct InputFilterBank
{
    InputFilterBank() : roots(FilterSpec::inputRoots), poles(FilterSpec::inputPoles)
    {
        reset(1.0f / 48000.0f);
    }

    void reset(float sampleTime) noexcept
    {
        Ts = sampleTime;
        x.clear();
        Gcalc = ComplexVec::filled(Complex { 1.0f, 0.0f });
    }

    void setFreq(float freq)
    {
        constexpr float originalCutoff = 9900.0f;
        const float freqFactor = freq / originalCutoff;
        rootCorr = roots * freqFactor;
        poleCorr = poles.map([freqFactor, this](const Complex& value) { return std::exp(value * freqFactor * Ts); });
        poleCorrAngle = poleCorr.mapFloat([](const Complex& value) { return std::arg(value); });
        gCoef = rootCorr * Ts;
    }

    void setTime(float tn)
    {
        Gcalc = gCoef * poleCorr.map([tn](const Complex& value) { return std::pow(value, tn); });
        Gcalc.flushSubnormals();
    }

    void setDelta(float delta) noexcept
    {
        Aplus = expFromAngles(poleCorrAngle, delta);
    }

    void calcG() noexcept
    {
        Gcalc = Aplus * Gcalc;
        Gcalc.flushSubnormals();
    }

    void process(float input) noexcept
    {
        x = poleCorr * x + ComplexVec::filled(Complex { input, 0.0f });
        x.flushSubnormals();
    }

    ComplexVec x;
    ComplexVec Gcalc = ComplexVec::filled(Complex { 1.0f, 0.0f });

private:
    ComplexVec roots;
    ComplexVec poles;
    ComplexVec rootCorr;
    ComplexVec poleCorr;
    std::array<float, FilterSpec::count> poleCorrAngle {};
    ComplexVec Aplus;
    float Ts = 1.0f / 48000.0f;
    ComplexVec gCoef;
};

struct OutputFilterBank
{
    OutputFilterBank()
    {
        std::array<Complex, FilterSpec::count> gains {};
        for (size_t i = 0; i < FilterSpec::count; ++i)
            gains[i] = FilterSpec::outputRoots[i] / FilterSpec::outputPoles[i];

        gCoef = ComplexVec(gains);
        poles = ComplexVec(FilterSpec::outputPoles);
        reset(1.0f / 48000.0f);
    }

    void reset(float sampleTime) noexcept
    {
        Ts = sampleTime;
        x.clear();
        Gcalc = ComplexVec::filled(Complex { 1.0f, 0.0f });
    }

    float calcH0() const noexcept
    {
        return -1.0f * gCoef.sumReal();
    }

    void setFreq(float freq)
    {
        constexpr float originalCutoff = 9500.0f;
        const float freqFactor = freq / originalCutoff;
        poleCorr = poles.map([freqFactor, this](const Complex& value) { return std::exp(value * freqFactor * Ts); });
        poleCorrAngle = poleCorr.mapFloat([](const Complex& value) { return std::arg(value); });
        Amult = gCoef * poleCorr;
    }

    void setTime(float tn)
    {
        Gcalc = Amult * poleCorr.map([tn](const Complex& value) { return std::pow(value, 1.0f - tn); });
        Gcalc.flushSubnormals();
    }

    void setDelta(float delta) noexcept
    {
        Aplus = expFromAngles(poleCorrAngle, -delta);
    }

    void calcG() noexcept
    {
        Gcalc = Aplus * Gcalc;
        Gcalc.flushSubnormals();
    }

    void process(const ComplexVec& input) noexcept
    {
        x = poleCorr * x + input;
        x.flushSubnormals();
    }

private:
    ComplexVec gCoef;
    ComplexVec poles;
    ComplexVec poleCorr;
    std::array<float, FilterSpec::count> poleCorrAngle {};
    ComplexVec Aplus;
    float Ts = 1.0f / 48000.0f;
    ComplexVec Amult;

public:
    ComplexVec x;
    ComplexVec Gcalc = ComplexVec::filled(Complex { 1.0f, 0.0f });
};

} // namespace delaybb
