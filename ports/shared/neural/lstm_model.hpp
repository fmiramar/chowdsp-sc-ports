#pragma once

#include "../third_party/nlohmann/json.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <limits>

namespace sharedneural
{

template <size_t InputSize, size_t HiddenSize>
struct LSTMModel
{
    static constexpr size_t gateCount = HiddenSize * 4;

    using InputWeights = std::array<std::array<float, InputSize>, gateCount>;
    using RecurrentWeights = std::array<std::array<float, HiddenSize>, gateCount>;
    using GateBias = std::array<float, gateCount>;
    using DenseWeights = std::array<float, HiddenSize>;
    using HiddenState = std::array<float, HiddenSize>;

    bool loadFromJsonFile(const std::filesystem::path& path)
    {
        std::ifstream stream(path);
        if (!stream.is_open())
            return false;

        nlohmann::json modelJson;
        stream >> modelJson;
        return loadFromJson(modelJson);
    }

    bool loadFromJson(const nlohmann::json& modelJson)
    {
        const auto& stateDict = modelJson.at("state_dict");
        load2D(stateDict.at("rec.weight_ih_l0"), inputWeights);
        load2D(stateDict.at("rec.weight_hh_l0"), recurrentWeights);

        const auto biasIH = load1D<gateCount>(stateDict.at("rec.bias_ih_l0"));
        const auto biasHH = load1D<gateCount>(stateDict.at("rec.bias_hh_l0"));
        for (size_t i = 0; i < gateCount; ++i)
            gateBias[i] = biasIH[i] + biasHH[i];

        const auto& denseMatrix = stateDict.at("lin.weight");
        for (size_t i = 0; i < HiddenSize; ++i)
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

    float process(const std::array<float, InputSize>& input, bool residual = false) noexcept
    {
        if (!loaded)
            return input[0];

        HiddenState newHidden {};
        HiddenState newCell {};

        for (size_t i = 0; i < HiddenSize; ++i) {
            const auto inputGate = sigmoid(gateDot(i, input));
            const auto forgetGate = sigmoid(gateDot(HiddenSize + i, input));
            const auto cellGate = std::tanh(gateDot(2 * HiddenSize + i, input));
            const auto outputGate = sigmoid(gateDot(3 * HiddenSize + i, input));

            newCell[i] = flushSubnormal(forgetGate * cell[i] + inputGate * cellGate);
            newHidden[i] = flushSubnormal(outputGate * std::tanh(newCell[i]));
        }

        hidden = newHidden;
        cell = newCell;

        float y = denseBias;
        for (size_t i = 0; i < HiddenSize; ++i)
            y += denseWeights[i] * hidden[i];

        y = flushSubnormal(y);
        return residual ? flushSubnormal(input[0] + y) : y;
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
        for (size_t row = 0; row < matrix.size(); ++row)
            for (size_t col = 0; col < matrix[row].size(); ++col)
                matrix[row][col] = data.at(row).at(col).get<float>();
    }

    float gateDot(size_t gateIndex, const std::array<float, InputSize>& input) const noexcept
    {
        float sum = gateBias[gateIndex];
        for (size_t i = 0; i < InputSize; ++i)
            sum += inputWeights[gateIndex][i] * input[i];
        for (size_t i = 0; i < HiddenSize; ++i)
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

    static float flushSubnormal(float value) noexcept
    {
        const auto absValue = std::abs(value);
        return (absValue > 0.0f && absValue < std::numeric_limits<float>::min()) ? 0.0f : value;
    }
};

} // namespace sharedneural
