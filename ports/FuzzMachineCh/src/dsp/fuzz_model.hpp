#pragma once

#include "../../../../ChowDSP-VCV/lib/chowdsp_utils/modules/chowdsp_plugin_utils/third_party/nlohmann/json.hpp"
#include "denormal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

namespace fuzzmachinech
{

struct FuzzModel
{
    static constexpr size_t inputSize = 2;
    static constexpr size_t hiddenSize = 24;
    static constexpr size_t gateCount = hiddenSize * 4;

    using InputWeights = std::array<std::array<float, inputSize>, gateCount>;
    using RecurrentWeights = std::array<std::array<float, hiddenSize>, gateCount>;
    using GateBias = std::array<float, gateCount>;
    using DenseWeights = std::array<float, hiddenSize>;
    using HiddenState = std::array<float, hiddenSize>;

    bool loadFromJsonFile(const std::filesystem::path& path)
    {
        std::ifstream stream(path);
        if (!stream.is_open())
            return false;

        nlohmann::json modelJson;
        stream >> modelJson;

        const auto& stateDict = modelJson.at("state_dict");
        load2D(stateDict.at("rec.weight_ih_l0"), inputWeights);
        load2D(stateDict.at("rec.weight_hh_l0"), recurrentWeights);

        const auto biasIH = load1D<gateCount>(stateDict.at("rec.bias_ih_l0"));
        const auto biasHH = load1D<gateCount>(stateDict.at("rec.bias_hh_l0"));
        for (size_t i = 0; i < gateCount; ++i)
            gateBias[i] = biasIH[i] + biasHH[i];

        const auto denseMatrix = stateDict.at("lin.weight");
        for (size_t i = 0; i < hiddenSize; ++i)
            denseWeights[i] = denseMatrix.at(0).at(i).get<float>();
        denseBias = stateDict.at("lin.bias").at(0).get<float>();

        loaded = true;
        reset();
        return true;
    }

    void reset() noexcept
    {
        hidden.fill(0.0f);
        cell.fill(0.0f);
    }

    float process(float x, float condition, bool residual = true) noexcept
    {
        if (!loaded)
            return x;

        HiddenState newHidden {};
        HiddenState newCell {};

        for (size_t i = 0; i < hiddenSize; ++i) {
            const auto inputGate = sigmoid(gateDot(i, x, condition));
            const auto forgetGate = sigmoid(gateDot(hiddenSize + i, x, condition));
            const auto cellGate = std::tanh(gateDot(2 * hiddenSize + i, x, condition));
            const auto outputGate = sigmoid(gateDot(3 * hiddenSize + i, x, condition));

            newCell[i] = flushSubnormal(forgetGate * cell[i] + inputGate * cellGate);
            newHidden[i] = flushSubnormal(outputGate * std::tanh(newCell[i]));
        }

        hidden = newHidden;
        cell = newCell;

        float y = denseBias;
        for (size_t i = 0; i < hiddenSize; ++i)
            y += denseWeights[i] * hidden[i];

        y = flushSubnormal(y);
        return residual ? flushSubnormal(x + y) : y;
    }

    bool loaded = false;
    InputWeights inputWeights {};
    RecurrentWeights recurrentWeights {};
    GateBias gateBias {};
    DenseWeights denseWeights {};
    float denseBias = 0.0f;
    HiddenState hidden {};
    HiddenState cell {};

private:
    template <size_t Size>
    static std::array<float, Size> load1D(const nlohmann::json& data)
    {
        std::array<float, Size> values {};
        for (size_t i = 0; i < Size; ++i)
            values[i] = data.at(i).get<float>();
        return values;
    }

    template <typename MatrixType>
    static void load2D(const nlohmann::json& data, MatrixType& matrix)
    {
        for (size_t row = 0; row < matrix.size(); ++row) {
            for (size_t col = 0; col < matrix[row].size(); ++col)
                matrix[row][col] = data.at(row).at(col).get<float>();
        }
    }

    float gateDot(size_t gateIndex, float x, float condition) const noexcept
    {
        float sum = gateBias[gateIndex] + inputWeights[gateIndex][0] * x + inputWeights[gateIndex][1] * condition;
        for (size_t i = 0; i < hiddenSize; ++i)
            sum += recurrentWeights[gateIndex][i] * hidden[i];
        return flushSubnormal(sum);
    }

    static float sigmoid(float x) noexcept
    {
        if (x >= 0.0f) {
            const auto e = std::exp(-x);
            return 1.0f / (1.0f + e);
        }

        const auto e = std::exp(x);
        return e / (1.0f + e);
    }
};

struct FuzzModelSet
{
    bool loadFromDirectory(const std::filesystem::path& dir)
    {
        const auto ok15 = model15.loadFromJsonFile(dir / "fuzz_15.json");
        const auto ok2 = model2.loadFromJsonFile(dir / "fuzz_2.json");
        const auto ok1588 = model15_88.loadFromJsonFile(dir / "fuzz_15_88.json");
        const auto ok288 = model2_88.loadFromJsonFile(dir / "fuzz_2_88.json");
        loaded = ok15 && ok2 && ok1588 && ok288;
        return loaded;
    }

    void reset() noexcept
    {
        model15.reset();
        model2.reset();
        model15_88.reset();
        model2_88.reset();
    }

    FuzzModel& select(bool mode15, bool use88Family) noexcept
    {
        if (mode15)
            return use88Family ? model15_88 : model15;
        return use88Family ? model2_88 : model2;
    }

    bool loaded = false;
    FuzzModel model15 {};
    FuzzModel model2 {};
    FuzzModel model15_88 {};
    FuzzModel model2_88 {};
};

} // namespace fuzzmachinech
