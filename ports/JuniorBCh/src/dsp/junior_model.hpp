#pragma once

#include "../../../../ChowDSP-VCV/lib/chowdsp_utils/modules/chowdsp_plugin_utils/third_party/nlohmann/json.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <vector>

namespace juniorbch
{
class JuniorTriodeModel
{
public:
    static constexpr size_t inputSize = 2;
    static constexpr size_t hiddenSize = 8;
    static constexpr size_t numHiddenLayers = 5;
    static constexpr size_t outputSize = 2;

    bool loadFromJsonFile(const std::filesystem::path& path)
    {
        std::ifstream stream(path);
        if (!stream.is_open())
            return false;

        nlohmann::json modelJson;
        stream >> modelJson;

        const auto& layers = modelJson.at("layers");
        int denseIndex = 0;
        for (const auto& layer : layers) {
            if (layer.at("type").get<std::string>() != "dense")
                continue;

            const auto& weights = layer.at("weights");
            if (denseIndex < (int) numHiddenLayers) {
                loadDenseLayer(weights, hiddenWeights[(size_t) denseIndex], hiddenBiases[(size_t) denseIndex],
                               denseIndex == 0 ? inputSize : hiddenSize, hiddenSize);
            } else {
                loadOutputLayer(weights);
            }
            ++denseIndex;
        }

        loaded = denseIndex == 6;
        return loaded;
    }

    template <typename T>
    const T* compute(T* input) noexcept
    {
        std::array<T, hiddenSize> cur {};
        std::array<T, hiddenSize> next {};

        for (size_t i = 0; i < hiddenSize; ++i) {
            T sum = static_cast<T>(hiddenBiases[0][i]);
            for (size_t j = 0; j < inputSize; ++j)
                sum += static_cast<T>(hiddenWeights[0][j][i]) * input[j];
            cur[i] = elu(sum);
        }

        for (size_t layer = 1; layer < numHiddenLayers; ++layer) {
            for (size_t i = 0; i < hiddenSize; ++i) {
                T sum = static_cast<T>(hiddenBiases[layer][i]);
                for (size_t j = 0; j < hiddenSize; ++j)
                    sum += static_cast<T>(hiddenWeights[layer][j][i]) * cur[j];
                next[i] = elu(sum);
            }
            cur = next;
        }

        for (size_t i = 0; i < outputSize; ++i) {
            T sum = static_cast<T>(outputBias[i]);
            for (size_t j = 0; j < hiddenSize; ++j)
                sum += static_cast<T>(outputWeights[j][i]) * cur[j];
            outputCache[i] = sum;
        }

        return outputCache.data();
    }

    bool loaded = false;

private:
    using HiddenMatrix = std::array<std::array<float, hiddenSize>, hiddenSize>;
    using InputMatrix = std::array<std::array<float, hiddenSize>, inputSize>;
    using BiasVec = std::array<float, hiddenSize>;

    template <typename T>
    static T elu(T x) noexcept
    {
        return x > static_cast<T>(0) ? x : static_cast<T>(std::exp((double) x) - 1.0);
    }

    template <typename MatrixType, typename BiasType>
    static void loadDenseLayer(const nlohmann::json& weights,
                               MatrixType& matrix,
                               BiasType& bias,
                               size_t rows,
                               size_t cols)
    {
        const auto& w = weights.at(0);
        const auto& b = weights.at(1);
        for (size_t i = 0; i < rows; ++i)
            for (size_t j = 0; j < cols; ++j)
                matrix[i][j] = w.at(i).at(j).get<float>();
        for (size_t j = 0; j < cols; ++j)
            bias[j] = b.at(j).get<float>();
    }

    void loadOutputLayer(const nlohmann::json& weights)
    {
        const auto& w = weights.at(0);
        const auto& b = weights.at(1);
        for (size_t i = 0; i < hiddenSize; ++i)
            for (size_t j = 0; j < outputSize; ++j)
                outputWeights[i][j] = w.at(i).at(j).get<float>();
        for (size_t j = 0; j < outputSize; ++j)
            outputBias[j] = b.at(j).get<float>();
    }

    InputMatrix inputWeights {};
    std::array<HiddenMatrix, numHiddenLayers - 1> hiddenWeightsTail {};
    std::array<std::array<std::array<float, hiddenSize>, hiddenSize>, numHiddenLayers> hiddenWeights {};
    std::array<BiasVec, numHiddenLayers> hiddenBiases {};
    std::array<std::array<float, outputSize>, hiddenSize> outputWeights {};
    std::array<float, outputSize> outputBias {};
    std::array<float, outputSize> outputCache {};
};
} // namespace juniorbch
