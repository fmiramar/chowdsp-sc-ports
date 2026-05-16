#include "SC_PlugIn.h"

#include "dsp/denormal.hpp"
#include "dsp/diode_pair.hpp"
#include "dsp/filters.hpp"
#include "dsp/oversampling.hpp"
#include "dsp/smoothed_value.hpp"
#include "dsp/wdf_t.h"

#include <algorithm>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
using namespace chowdsp::WDFT;

constexpr float sqrtHalf = 0.7071067811865476f;
constexpr int oversamplingRatio = 2;

enum InputIndex
{
    InputIn = 0,
    InputGain,
    InputTreble,
    InputLevel
};

struct InputBufferProcessor
{
    centaurch::OnePoleFilter filter;
    float sampleRate = 44100.0f;

    void prepare(float newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        filter.reset();
        calcCoefs();
    }

    void calcCoefs() noexcept
    {
        constexpr float R1 = 10000.0f;
        constexpr float R2 = 1000000.0f;
        constexpr float C1 = 0.1e-6f;

        const auto b0s = C1 * R2;
        const auto b1s = 0.0f;
        const auto a0s = C1 * (R1 + R2);
        const auto a1s = 1.0f;

        centaurch::bilinearTransformOrder1(filter, b0s, b1s, a0s, a1s, 2.0f * sampleRate);
    }

    float process(float x) noexcept
    {
        return filter.process(x);
    }
};

struct ToneFilterProcessor
{
    centaurch::OnePoleFilter filter;
    centaurch::SmoothedValue<float, centaurch::ValueSmoothingTypes::Linear> trebleSmooth { 0.287394f };
    float sampleRate = 44100.0f;

    void prepare(float newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        filter.reset();
        trebleSmooth.setCurrentAndTargetValue(trebleSmooth.getTargetValue());
        trebleSmooth.reset(sampleRate, 0.05);
        calcCoefs(trebleSmooth.getTargetValue());
    }

    void setTreble(float treble) noexcept
    {
        trebleSmooth.setTargetValue(std::clamp(treble, 0.0f, 1.0f));
    }

    void calcCoefs(float curTreble) noexcept
    {
        constexpr float Rpot = 10e3f;
        constexpr float C = 3.9e-9f;
        constexpr float G1 = 1.0f / 100e3f;
        const float G2 = 1.0f / (1.8e3f + (1.0f - curTreble) * Rpot);
        const float G3 = 1.0f / (4.7e3f + curTreble * Rpot);
        constexpr float G4 = 1.0f / 100e3f;

        constexpr float wc = G1 / C;
        const auto K = wc / std::tan(wc / (2.0f * sampleRate));

        const auto b0s = C * (G1 + G2);
        const auto b1s = G1 * (G2 + G3);
        const auto a0s = C * (G3 - G4);
        const auto a1s = -G4 * (G2 + G3);

        centaurch::OnePoleFilter unstable {};
        centaurch::bilinearTransformOrder1(unstable, b0s, b1s, a0s, a1s, K);

        filter.b0 = unstable.b0 / unstable.a1;
        filter.b1 = unstable.b1 / unstable.a1;
        filter.a1 = 1.0f / unstable.a1;
    }

    float process(float x) noexcept
    {
        if (trebleSmooth.isSmoothing())
            calcCoefs(trebleSmooth.getNextValue());

        return filter.process(x);
    }
};

struct OutputStageProcessor
{
    centaurch::OnePoleFilter filter;
    centaurch::SmoothedValue<float, centaurch::ValueSmoothingTypes::Multiplicative> levelSmooth { 1.0f };
    float sampleRate = 44100.0f;

    void prepare(float newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        filter.reset();
        levelSmooth.setCurrentAndTargetValue(levelSmooth.getTargetValue());
        levelSmooth.reset(sampleRate, 0.05);
        calcCoefs(levelSmooth.getTargetValue());
    }

    void setLevel(float level) noexcept
    {
        levelSmooth.setTargetValue(std::clamp(level, 1.0e-5f, 1.0f));
    }

    void calcCoefs(float curLevel) noexcept
    {
        const float R1 = 560.0f + (1.0f - curLevel) * 10000.0f;
        const float R2 = curLevel * 10000.0f + 1.0f;
        constexpr float C1 = 4.7e-6f;

        const auto b0s = C1 * R2;
        const auto b1s = 0.0f;
        const auto a0s = C1 * (R1 + R2);
        const auto a1s = 1.0f;

        centaurch::bilinearTransformOrder1(filter, b0s, b1s, a0s, a1s, 2.0f * sampleRate);
    }

    float process(float x) noexcept
    {
        if (levelSmooth.isSmoothing())
            calcCoefs(levelSmooth.getNextValue());

        return filter.process(x);
    }
};

struct AmpStage
{
    centaurch::BiquadFilter filter;
    centaurch::SmoothedValue<float, centaurch::ValueSmoothingTypes::Multiplicative> r10bSmooth { 2000.0f };
    float sampleRate = 44100.0f;

    void prepare(float newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        filter.reset();
        r10bSmooth.setCurrentAndTargetValue(r10bSmooth.getTargetValue());
        r10bSmooth.reset(sampleRate, 0.05);
        calcCoefs(r10bSmooth.getTargetValue());
    }

    void setGain(float gain) noexcept
    {
        const float newR10b = (1.0f - std::clamp(gain, 0.0f, 1.0f)) * 100000.0f + 2000.0f;
        r10bSmooth.setTargetValue(std::clamp(newR10b, 2000.0f, 102000.0f));
    }

    void calcCoefs(float curR10b) noexcept
    {
        constexpr float C7 = 82e-9f;
        constexpr float C8 = 390e-12f;
        constexpr float R11 = 15e3f;
        constexpr float R12 = 422e3f;

        const float a0s = C7 * C8 * curR10b * R11 * R12;
        const float a1s = C7 * curR10b * R11 + C8 * R12 * (curR10b + R11);
        const float a2s = curR10b + R11;
        const float b0s = a0s;
        const float b1s = C7 * R11 * R12 + a1s;
        const float b2s = R12 + a2s;

        const float wc = centaurch::calcPoleFreq(a0s, a1s, a2s);
        const auto K = wc == 0.0f ? 2.0f * sampleRate : wc / std::tan(wc / (2.0f * sampleRate));
        centaurch::bilinearTransformOrder2(filter, b0s, b1s, b2s, a0s, a1s, a2s, K);
    }

    float process(float x) noexcept
    {
        if (r10bSmooth.isSmoothing())
            calcCoefs(r10bSmooth.getNextValue());

        return filter.process(x);
    }
};

struct SummingAmp
{
    centaurch::OnePoleFilter filter;
    float sampleRate = 44100.0f;

    void prepare(float newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        filter.reset();
        calcCoefs();
    }

    void calcCoefs() noexcept
    {
        constexpr float R20 = 392e3f;
        constexpr float C13 = 820e-12f;

        const auto b0s = 0.0f;
        const auto b1s = R20;
        const auto a0s = C13 * R20;
        const auto a1s = 1.0f;

        centaurch::bilinearTransformOrder1(filter, b0s, b1s, a0s, a1s, 2.0f * sampleRate);
    }

    float process(float x) noexcept
    {
        return filter.process(x);
    }
};

class PreAmpWDF
{
public:
    explicit PreAmpWDF(double sampleRate)
        : C3(0.1e-6, sampleRate)
        , C5(68.0e-9, sampleRate)
        , C16(1.0e-6, sampleRate)
    {
        reset();
    }

    void prepare(double sampleRate)
    {
        C3.prepare(sampleRate);
        C5.prepare(sampleRate);
        C16.prepare(sampleRate);
        reset();
    }

    void setGain(float gain)
    {
        Vbias.setResistanceValue(static_cast<double>(std::clamp(gain, 0.0f, 1.0f)) * 100.0e3);
    }

    void reset()
    {
        C3.reset();
        C5.reset();
        C16.reset();
        Vbias.setVoltage(0.0);
        Vbias2.setVoltage(0.0);
    }

    float getFF1() noexcept
    {
        return static_cast<float>(current<double>(Vbias2));
    }

    float processSample(float x) noexcept
    {
        Vin.setVoltage(static_cast<double>(x));
        Vin.incident(I1.reflected());
        const auto y = voltage<double>(Vbias) + voltage<double>(R6);
        I1.incident(Vin.reflected());
        return static_cast<float>(y);
    }

private:
    using Capacitor = CapacitorT<double>;
    using Resistor = ResistorT<double>;
    using ResVs = ResistiveVoltageSourceT<double>;

    Capacitor C3;
    Capacitor C5;
    Capacitor C16;

    Resistor R6 { 10000.0 };
    Resistor R7 { 1500.0 };

    ResVs Vbias2 { 15000.0 };
    ResVs Vbias;

    WDFParallelT<double, Capacitor, Resistor> P1 { C5, R6 };
    WDFSeriesT<double, decltype(P1), ResVs> S1 { P1, Vbias };

    WDFParallelT<double, ResVs, Capacitor> P2 { Vbias2, C16 };
    WDFSeriesT<double, decltype(P2), Resistor> S2 { P2, R7 };
    WDFParallelT<double, decltype(S1), decltype(S2)> P3 { S1, S2 };

    WDFSeriesT<double, decltype(P3), Capacitor> S3 { P3, C3 };
    PolarityInverterT<double, decltype(S3)> I1 { S3 };

    IdealVoltageSourceT<double, decltype(I1)> Vin { I1 };
};

class ClippingWDF
{
public:
    explicit ClippingWDF(double sampleRate)
        : C9(1.0e-6, sampleRate)
        , C10(1.0e-6, sampleRate)
    {
        reset();
    }

    void prepare(double sampleRate)
    {
        C9.prepare(sampleRate);
        C10.prepare(sampleRate);
        reset();
    }

    void reset()
    {
        C9.reset();
        C10.reset();
        Vbias.setVoltage(0.0);
    }

    float processSample(float x) noexcept
    {
        Vin.setVoltage(static_cast<double>(x));

        D23.incident(P1.reflected());
        P1.incident(D23.reflected());
        const auto y = current<double>(C10);
        return static_cast<float>(y);
    }

private:
    using Capacitor = CapacitorT<double>;
    using Resistor = ResistorT<double>;
    using ResVs = ResistiveVoltageSourceT<double>;

    ResVs Vin;
    Capacitor C9;
    Resistor R13 { 1000.0 };

    Capacitor C10;
    ResVs Vbias { 47000.0 };

    PolarityInverterT<double, ResVs> I1 { Vin };
    WDFSeriesT<double, decltype(I1), Capacitor> S1 { I1, C9 };
    WDFSeriesT<double, decltype(S1), Resistor> S2 { S1, R13 };

    WDFSeriesT<double, Capacitor, ResVs> S3 { C10, Vbias };
    WDFParallelT<double, decltype(S2), decltype(S3)> P1 { S2, S3 };

    centaurch::CustomDiodePairT<double, decltype(P1)> D23 { 15e-6, 0.02585, P1 };
};

class FeedForward2WDF
{
public:
    explicit FeedForward2WDF(double sampleRate)
        : C4(68e-9, sampleRate)
        , C6(390e-9, sampleRate)
        , C11(2.2e-9, sampleRate)
        , C12(27e-9, sampleRate)
    {
        reset();
    }

    void prepare(double sampleRate)
    {
        C4.prepare(sampleRate);
        C6.prepare(sampleRate);
        C11.prepare(sampleRate);
        C12.prepare(sampleRate);
        reset();
    }

    void reset()
    {
        C4.reset();
        C6.reset();
        C11.reset();
        C12.reset();
        Vbias.setVoltage(0.0);
    }

    void setGain(float gain)
    {
        const auto g = static_cast<double>(std::clamp(gain, 0.0f, 1.0f));
        RVTop.setResistanceValue(std::max(g * 100e3, 1.0));
        RVBot.setResistanceValue(std::max((1.0 - g) * 100e3, 1.0));
    }

    float processSample(float x) noexcept
    {
        Vin.setVoltage(static_cast<double>(x));
        Vin.incident(I1.reflected());
        I1.incident(Vin.reflected());
        const auto y = current<double>(R16);
        return static_cast<float>(y);
    }

private:
    using Capacitor = CapacitorT<double>;
    using Resistor = ResistorT<double>;
    using ResVs = ResistiveVoltageSourceT<double>;

    Resistor R5 { 5100.0 };
    Resistor R8 { 1500.0 };
    Resistor R9 { 1000.0 };
    Resistor RVTop { 50000.0 };
    Resistor RVBot { 50000.0 };
    Resistor R15 { 22000.0 };
    Resistor R16 { 47000.0 };
    Resistor R17 { 27000.0 };
    Resistor R18 { 12000.0 };
    ResVs Vbias;

    Capacitor C4;
    Capacitor C6;
    Capacitor C11;
    Capacitor C12;

    WDFSeriesT<double, Capacitor, Resistor> S1 { C12, R18 };
    WDFParallelT<double, decltype(S1), Resistor> P1 { S1, R17 };

    WDFSeriesT<double, Capacitor, Resistor> S2 { C11, R15 };
    WDFSeriesT<double, decltype(S2), Resistor> S3 { S2, R16 };
    WDFParallelT<double, decltype(S3), decltype(P1)> P2 { S3, P1 };
    WDFParallelT<double, decltype(P2), Resistor> P3 { P2, RVBot };
    WDFSeriesT<double, decltype(P3), Resistor> S4 { P3, RVTop };

    WDFSeriesT<double, Capacitor, Resistor> S5 { C6, R9 };
    WDFParallelT<double, decltype(S4), decltype(S5)> P4 { S4, S5 };
    WDFParallelT<double, decltype(P4), Resistor> P5 { P4, R8 };
    WDFSeriesT<double, decltype(P5), ResVs> S6 { P5, Vbias };

    WDFParallelT<double, Resistor, Capacitor> P6 { R5, C4 };
    WDFSeriesT<double, decltype(P6), decltype(S6)> S7 { P6, S6 };
    PolarityInverterT<double, decltype(S7)> I1 { S7 };

    IdealVoltageSourceT<double, decltype(I1)> Vin { I1 };
};

struct GainStageProcessor
{
    centaurch::Oversampling<oversamplingRatio> oversampling;
    PreAmpWDF preAmp { 44100.0 };
    ClippingWDF clip { 44100.0 * oversamplingRatio };
    FeedForward2WDF ff2 { 44100.0 };
    AmpStage amp;
    SummingAmp sumAmp;

    void reset(float sampleRate) noexcept
    {
        oversampling.reset(sampleRate);
        preAmp.prepare(sampleRate);
        clip.prepare(sampleRate * oversamplingRatio);
        ff2.prepare(sampleRate);
        amp.prepare(sampleRate);
        sumAmp.prepare(sampleRate);
    }

    float process(float x, float gain) noexcept
    {
        float x2 = x;

        preAmp.setGain(gain);
        x = preAmp.processSample(x);
        const float x1 = preAmp.getFF1();

        amp.setGain(gain);
        x = std::clamp(amp.process(x), -4.5f, 4.5f);

        oversampling.upsample(x);
        auto* osBuffer = oversampling.getOSBuffer();
        for (int k = 0; k < oversamplingRatio; ++k)
            osBuffer[k] = clip.processSample(osBuffer[k]);
        x = oversampling.downsample();

        ff2.setGain(gain);
        x2 = ff2.processSample(x2);

        x = x + x1 + x2;
        x = sumAmp.process(x);
        return std::clamp(x, -13.1f, 11.7f);
    }
};

struct CentaurCh : public Unit
{
    InputBufferProcessor inputBuffer;
    GainStageProcessor gainStage;
    ToneFilterProcessor tone;
    OutputStageProcessor outputStage;
    centaurch::BiquadFilter dcBlocker;
    float sampleRate = 44100.0f;
};

inline float inputAt(CentaurCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline void resetDSP(CentaurCh* unit) noexcept
{
    unit->inputBuffer.prepare(unit->sampleRate);
    unit->gainStage.reset(unit->sampleRate);
    unit->tone.prepare(unit->sampleRate);
    unit->outputStage.prepare(unit->sampleRate);
    unit->dcBlocker.reset();
    unit->dcBlocker.setHighpass(35.0f / unit->sampleRate, sqrtHalf);
}

void CentaurCh_next(CentaurCh* unit, int inNumSamples)
{
    const auto* in = IN(InputIn);
    auto* out = OUT(0);

    for (int i = 0; i < inNumSamples; ++i) {
        const auto gain = std::clamp(inputAt(unit, InputGain, i), 0.0f, 1.0f);
        const auto treble = std::clamp(inputAt(unit, InputTreble, i), 0.0f, 1.0f);
        const auto level = std::clamp(inputAt(unit, InputLevel, i), 0.0f, 1.0f);

        unit->tone.setTreble(treble);
        unit->outputStage.setLevel(level);

        auto x = in[i] * 0.5f;
        x = unit->inputBuffer.process(x);
        x = std::clamp(x, -4.5f, 4.5f);
        x = unit->gainStage.process(x, gain);
        x = unit->tone.process(x);
        x = std::clamp(-x, -13.1f, 11.7f);
        x = unit->outputStage.process(x);
        x = unit->dcBlocker.process(x);

        if (!std::isfinite(x)) {
            x = 0.0f;
            resetDSP(unit);
        }

        out[i] = x;
    }
}

void CentaurCh_Ctor(CentaurCh* unit)
{
    new (unit) CentaurCh;
    unit->sampleRate = static_cast<float>(SAMPLERATE);
    resetDSP(unit);
    unit->tone.setTreble(IN0(InputTreble));
    unit->outputStage.setLevel(IN0(InputLevel));
    SETCALC(CentaurCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(CentaurCh)
{
    ft = inTable;
    DefineSimpleUnit(CentaurCh);
}
