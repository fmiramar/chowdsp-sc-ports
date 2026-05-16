#include "SC_PlugIn.h"

#include <cmath>

static InterfaceTable* ft;

namespace
{
constexpr double twoPi = 6.28318530717958647692;

enum InputIndex
{
    InputIn = 0,
    InputFreq,
    InputDecay,
    InputAmp,
    InputPhase
};

struct ModalCh : public Unit
{
    double stateReal = 0.0;
    double stateImag = 0.0;

    float freq = 440.0f;
    float decay = 1.4f;
    float amp = 0.25f;
    float phase = 0.0f;

    double ampReal = 0.0;
    double ampImag = 0.0;
    double coefReal = 0.0;
    double coefImag = 0.0;
};

inline double calcDecayFactor(float decayTime, double sampleRate) noexcept
{
    if (decayTime <= 0.0f)
        return 0.0;

    return std::exp(log001 / (static_cast<double>(decayTime) * sampleRate));
}

inline void setParams(ModalCh* unit, float freq, float decay, float amp, float phase) noexcept
{
    const double decayFactor = calcDecayFactor(decay, SAMPLERATE);
    const double angle = twoPi * static_cast<double>(freq) / SAMPLERATE;

    unit->freq = freq;
    unit->decay = decay;
    unit->amp = amp;
    unit->phase = phase;

    unit->ampReal = static_cast<double>(amp) * std::cos(static_cast<double>(phase));
    unit->ampImag = static_cast<double>(amp) * std::sin(static_cast<double>(phase));
    unit->coefReal = decayFactor * std::cos(angle);
    unit->coefImag = decayFactor * std::sin(angle);
}

inline float inputAt(ModalCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline void processSample(ModalCh* unit, double input, double ampReal, double ampImag, double coefReal, double coefImag,
                          float& output) noexcept
{
    const double yReal = ampReal * input + coefReal * unit->stateReal - coefImag * unit->stateImag;
    const double yImag = ampImag * input + coefImag * unit->stateReal + coefReal * unit->stateImag;

    unit->stateReal = yReal;
    unit->stateImag = yImag;
    output = static_cast<float>(yImag);
}

void ModalCh_next_k(ModalCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);

    const float nextFreq = IN0(InputFreq);
    const float nextDecay = IN0(InputDecay);
    const float nextAmp = IN0(InputAmp);
    const float nextPhase = IN0(InputPhase);

    if (nextFreq != unit->freq || nextDecay != unit->decay || nextAmp != unit->amp || nextPhase != unit->phase) {
        double freq = unit->freq;
        double decay = unit->decay;
        double amp = unit->amp;
        double phase = unit->phase;

        const double freqSlope = CALCSLOPE(static_cast<double>(nextFreq), freq);
        const double decaySlope = CALCSLOPE(static_cast<double>(nextDecay), decay);
        const double ampSlope = CALCSLOPE(static_cast<double>(nextAmp), amp);
        const double phaseSlope = CALCSLOPE(static_cast<double>(nextPhase), phase);

        for (int i = 0; i < inNumSamples; ++i) {
            const double decayFactor = decay <= 0.0 ? 0.0 : std::exp(log001 / (decay * SAMPLERATE));
            const double angle = twoPi * freq / SAMPLERATE;
            const double ampReal = amp * std::cos(phase);
            const double ampImag = amp * std::sin(phase);
            const double coefReal = decayFactor * std::cos(angle);
            const double coefImag = decayFactor * std::sin(angle);

            processSample(unit, static_cast<double>(in[i]), ampReal, ampImag, coefReal, coefImag, out[i]);

            freq += freqSlope;
            decay += decaySlope;
            amp += ampSlope;
            phase += phaseSlope;
        }

        setParams(unit, nextFreq, nextDecay, nextAmp, nextPhase);
    } else {
        const double ampReal = unit->ampReal;
        const double ampImag = unit->ampImag;
        const double coefReal = unit->coefReal;
        const double coefImag = unit->coefImag;

        for (int i = 0; i < inNumSamples; ++i)
            processSample(unit, static_cast<double>(in[i]), ampReal, ampImag, coefReal, coefImag, out[i]);
    }

    unit->stateReal = zapgremlins(unit->stateReal);
    unit->stateImag = zapgremlins(unit->stateImag);
}

void ModalCh_next_a(ModalCh* unit, int inNumSamples)
{
    float* out = OUT(0);
    const float* in = IN(InputIn);

    float lastFreq = unit->freq;
    float lastDecay = unit->decay;
    float lastAmp = unit->amp;
    float lastPhase = unit->phase;

    for (int i = 0; i < inNumSamples; ++i) {
        const float freq = inputAt(unit, InputFreq, i);
        const float decay = inputAt(unit, InputDecay, i);
        const float amp = inputAt(unit, InputAmp, i);
        const float phase = inputAt(unit, InputPhase, i);

        const double decayFactor = calcDecayFactor(decay, SAMPLERATE);
        const double angle = twoPi * static_cast<double>(freq) / SAMPLERATE;
        const double ampReal = static_cast<double>(amp) * std::cos(static_cast<double>(phase));
        const double ampImag = static_cast<double>(amp) * std::sin(static_cast<double>(phase));
        const double coefReal = decayFactor * std::cos(angle);
        const double coefImag = decayFactor * std::sin(angle);

        processSample(unit, static_cast<double>(in[i]), ampReal, ampImag, coefReal, coefImag, out[i]);

        lastFreq = freq;
        lastDecay = decay;
        lastAmp = amp;
        lastPhase = phase;
    }

    setParams(unit, lastFreq, lastDecay, lastAmp, lastPhase);
    unit->stateReal = zapgremlins(unit->stateReal);
    unit->stateImag = zapgremlins(unit->stateImag);
}

void ModalCh_Ctor(ModalCh* unit)
{
    unit->stateReal = 0.0;
    unit->stateImag = 0.0;

    setParams(unit, IN0(InputFreq), IN0(InputDecay), IN0(InputAmp), IN0(InputPhase));

    const bool hasAudioRateParams = INRATE(InputFreq) == calc_FullRate || INRATE(InputDecay) == calc_FullRate
        || INRATE(InputAmp) == calc_FullRate || INRATE(InputPhase) == calc_FullRate;

    if (hasAudioRateParams)
        SETCALC(ModalCh_next_a);
    else
        SETCALC(ModalCh_next_k);

    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(ModalCh)
{
    ft = inTable;
    DefineSimpleUnit(ModalCh);
}
