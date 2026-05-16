#include "SC_PlugIn.h"

#include "dsp/biquad.hpp"
#include "dsp/rnn_model.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr float dcBlockFreq = 30.0f;

enum InputIndex
{
    Input1 = 0,
    Input2,
    Input3,
    Input4,
    InputRandomize,
    InputSeed,
    InputReset
};

struct RNNCh : public Unit
{
    rnnch::RNNModel model;
    rnnch::BiquadFilter dcBlocker;
    float prevRandomize = 0.0f;
    float prevReset = 0.0f;
};

inline float inputAt(RNNCh* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline bool risingEdge(float previous, float current) noexcept
{
    return current > 0.0f && previous <= 0.0f;
}

inline std::uint32_t seedFromValue(float value) noexcept
{
    if (!std::isfinite(value))
        return 0u;

    const auto clamped = std::clamp<double>(std::llround(static_cast<double>(value)), 0.0, static_cast<double>(std::numeric_limits<std::uint32_t>::max()));
    return static_cast<std::uint32_t>(clamped);
}

inline int activeInputCount(RNNCh* unit) noexcept
{
    int count = 0;
    for (int inputIndex = Input1; inputIndex <= Input4; ++inputIndex) {
        if (INRATE(inputIndex) != calc_ScalarRate || IN0(inputIndex) != 0.0f)
            ++count;
    }

    return std::max(count, 1);
}

inline float makeupGain(RNNCh* unit) noexcept
{
    return 4.0f / static_cast<float>(activeInputCount(unit));
}

inline void randomizeModel(RNNCh* unit, float seedValue) noexcept
{
    unit->model.randomize(seedFromValue(seedValue));
    unit->dcBlocker.reset();
}

inline void resetState(RNNCh* unit) noexcept
{
    unit->model.reset();
    unit->dcBlocker.reset();
}

void RNNCh_next(RNNCh* unit, int inNumSamples)
{
    auto* out = OUT(0);
    const auto gain = makeupGain(unit);

    for (int i = 0; i < inNumSamples; ++i) {
        const auto randomize = inputAt(unit, InputRandomize, i);
        const auto reset = inputAt(unit, InputReset, i);

        if (risingEdge(unit->prevRandomize, randomize))
            randomizeModel(unit, inputAt(unit, InputSeed, i));

        if (risingEdge(unit->prevReset, reset))
            resetState(unit);

        unit->prevRandomize = randomize;
        unit->prevReset = reset;

        std::array<float, rnnch::RNNModel::dims> input {
            inputAt(unit, Input1, i),
            inputAt(unit, Input2, i),
            inputAt(unit, Input3, i),
            inputAt(unit, Input4, i),
        };

        auto y = unit->model.forward(input);
        if (!std::isfinite(y)) {
            y = 0.0f;
            resetState(unit);
        }

        y = unit->dcBlocker.process(y);
        out[i] = gain * y;
    }
}

void RNNCh_Ctor(RNNCh* unit)
{
    new (unit) RNNCh;
    unit->dcBlocker.setParameters(rnnch::BiquadFilter::HIGHPASS, dcBlockFreq / static_cast<float>(SAMPLERATE), 0.70710678f);
    unit->dcBlocker.reset();
    randomizeModel(unit, IN0(InputSeed));
    unit->prevRandomize = IN0(InputRandomize);
    unit->prevReset = IN0(InputReset);
    SETCALC(RNNCh_next);
    OUT0(0) = 0.0f;
}
} // namespace

PluginLoad(RNNCh)
{
    ft = inTable;
    DefineSimpleUnit(RNNCh);
}
