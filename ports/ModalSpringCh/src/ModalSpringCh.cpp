#include "SC_PlugIn.h"

#include "dsp/mode_params.hpp"

#include <array>
#include <cmath>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr double twoPi = 6.28318530717958647692;
constexpr double log1000 = 6.90775527898213705205;
constexpr double log001Value = -6.90775527898213705205;
constexpr double wetGain = 1.0e-6;
constexpr int maxModesToMod = 200;
constexpr int modModesStep = 20;

enum InputIndex
{
    InputIn = 0,
    InputPitch,
    InputDecay,
    InputMix,
    InputModModes,
    InputModFreq,
    InputModDepth
};

struct ModeState
{
    double stateReal = 0.0;
    double stateImag = 0.0;
    double ampReal = 0.0;
    double ampImag = 0.0;
    double baseFreq = 0.0;
    double decayFactor = 0.0;
    double coefReal = 0.0;
    double coefImag = 0.0;
};

struct ModalSpringCh : public Unit
{
    std::array<ModeState, ModeParams::numModes> modes {};
    double modPhase = 0.0;
    double maxFreq = 0.0;
};

inline double clampFrequency(const ModalSpringCh* unit, double frequency) noexcept
{
    return frequency > unit->maxFreq ? 0.0 : frequency;
}

inline double calcDecayFactor(double t60, double sampleRate) noexcept
{
    if (t60 <= 0.0)
        return 0.0;

    return std::exp(log001Value / (t60 * sampleRate));
}

inline double tauToT60(double tau) noexcept
{
    return tau * log1000 / static_cast<double>(ModeParams::analysisFs);
}

inline double calcAmplitudeNormalization() noexcept
{
    const auto firstModeReal = static_cast<double>(ModeParams::amps_real[0]);
    const auto firstModeImag = static_cast<double>(ModeParams::amps_imag[0]);
    const auto magnitude = std::hypot(firstModeReal, firstModeImag);
    return magnitude > 0.0 ? 1.0 / magnitude : 1.0;
}

inline double wrapPhase(double phase) noexcept
{
    while (phase >= twoPi)
        phase -= twoPi;

    while (phase < 0.0)
        phase += twoPi;

    return phase;
}

inline int quantizeModModes(float modModesParam) noexcept
{
    modModesParam = sc_clip(modModesParam, 0.0f, static_cast<float>(maxModesToMod));
    const auto quantizedSteps = static_cast<int>(std::lround(modModesParam / static_cast<float>(modModesStep)));
    return sc_clip(quantizedSteps * modModesStep, 0, maxModesToMod);
}

inline void setModeCoefficients(ModalSpringCh* unit, ModeState& mode, double frequency) noexcept
{
    const auto clampedFrequency = clampFrequency(unit, frequency);
    const auto angle = twoPi * clampedFrequency / static_cast<double>(SAMPLERATE);
    mode.coefReal = mode.decayFactor * std::cos(angle);
    mode.coefImag = mode.decayFactor * std::sin(angle);
}

inline void updateStaticBank(ModalSpringCh* unit, float pitchParam, float decayParam) noexcept
{
    const auto pitch = sc_clip(pitchParam, -1.0f, 1.0f);
    const auto decay = sc_clip(decayParam, 0.0f, 1.0f);

    const auto freqMultiplier = std::pow(2.0, static_cast<double>(pitch));
    const auto decayScale = std::pow(4.0, static_cast<double>(2.0f * decay - 1.0f));
    const auto sampleRate = static_cast<double>(SAMPLERATE);

    for (int i = 0; i < ModeParams::numModes; ++i) {
        auto& mode = unit->modes[static_cast<std::size_t>(i)];
        mode.baseFreq = static_cast<double>(ModeParams::freqs[i]) * freqMultiplier;
        mode.decayFactor = calcDecayFactor(tauToT60(static_cast<double>(ModeParams::taus[i])) * decayScale, sampleRate);
        setModeCoefficients(unit, mode, mode.baseFreq);
    }
}

inline double processMode(ModeState& mode, double input, double coefReal, double coefImag) noexcept
{
    const auto yReal = mode.ampReal * input + coefReal * mode.stateReal - coefImag * mode.stateImag;
    const auto yImag = mode.ampImag * input + coefImag * mode.stateReal + coefReal * mode.stateImag;

    mode.stateReal = yReal;
    mode.stateImag = yImag;
    return yImag;
}

inline void sanitizeState(ModalSpringCh* unit) noexcept
{
    for (auto& mode : unit->modes) {
        mode.stateReal = zapgremlins(mode.stateReal);
        mode.stateImag = zapgremlins(mode.stateImag);
    }
}

void ModalSpringCh_next(ModalSpringCh* unit, int inNumSamples)
{
    auto* out = OUT(0);
    const auto* in = IN(InputIn);

    updateStaticBank(unit, IN0(InputPitch), IN0(InputDecay));

    const auto mix = sc_clip(IN0(InputMix), 0.0f, 1.0f);
    const auto dryMix = static_cast<double>(1.0f - mix);
    const auto wetMix = static_cast<double>(mix) * wetGain;
    const auto numModesToMod = quantizeModModes(IN0(InputModModes));
    const auto modFreq = sc_clip(IN0(InputModFreq), 0.5f, 10.0f);
    const auto modDepth = sc_clip(IN0(InputModDepth), 0.0f, 1.0f);
    const auto phaseInc = twoPi * static_cast<double>(modFreq) / static_cast<double>(SAMPLERATE);

    auto phase = unit->modPhase;

    if (numModesToMod == 0) {
        for (int i = 0; i < inNumSamples; ++i) {
            const auto input = static_cast<double>(in[i]);
            auto wet = 0.0;

            for (auto& mode : unit->modes)
                wet += processMode(mode, input, mode.coefReal, mode.coefImag);

            out[i] = static_cast<float>(dryMix * input + wetMix * wet);
        }
    } else {
        for (int i = 0; i < inNumSamples; ++i) {
            const auto input = static_cast<double>(in[i]);
            auto wet = 0.0;

            const auto modSample = std::sin(phase);
            phase = wrapPhase(phase + phaseInc);
            const auto freqOffset = std::pow(2.0, modSample) * static_cast<double>(modDepth);

            for (int modeIndex = 0; modeIndex < numModesToMod; ++modeIndex) {
                auto& mode = unit->modes[static_cast<std::size_t>(modeIndex)];
                const auto frequency = clampFrequency(unit, mode.baseFreq * freqOffset);
                const auto angle = twoPi * frequency / static_cast<double>(SAMPLERATE);
                const auto coefReal = mode.decayFactor * std::cos(angle);
                const auto coefImag = mode.decayFactor * std::sin(angle);
                wet += processMode(mode, input, coefReal, coefImag);
            }

            for (int modeIndex = numModesToMod; modeIndex < ModeParams::numModes; ++modeIndex) {
                auto& mode = unit->modes[static_cast<std::size_t>(modeIndex)];
                wet += processMode(mode, input, mode.coefReal, mode.coefImag);
            }

            out[i] = static_cast<float>(dryMix * input + wetMix * wet);
        }
    }

    unit->modPhase = phase;
    sanitizeState(unit);
}

void ModalSpringCh_Ctor(ModalSpringCh* unit)
{
    new (unit) ModalSpringCh;

    unit->maxFreq = 0.495 * static_cast<double>(SAMPLERATE);
    unit->modPhase = 0.0;

    const auto amplitudeNormalization = calcAmplitudeNormalization();
    for (int i = 0; i < ModeParams::numModes; ++i) {
        auto& mode = unit->modes[static_cast<std::size_t>(i)];
        mode.ampReal = static_cast<double>(ModeParams::amps_real[i]) * amplitudeNormalization;
        mode.ampImag = static_cast<double>(ModeParams::amps_imag[i]) * amplitudeNormalization;
        mode.stateReal = 0.0;
        mode.stateImag = 0.0;
    }

    updateStaticBank(unit, IN0(InputPitch), IN0(InputDecay));

    SETCALC(ModalSpringCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(ModalSpringCh)
{
    ft = inTable;
    DefineSimpleUnit(ModalSpringCh);
}
