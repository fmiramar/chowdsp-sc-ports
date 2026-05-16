#pragma once

#include "denormal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

namespace rnnch
{

struct RNNModel
{
    static constexpr size_t dims = 4;

    void reset() noexcept
    {
        hiddenState.fill(0.0f);
    }

    void randomize(std::uint32_t seed)
    {
        std::mt19937 engine(seed);
        std::uniform_real_distribution<float> denseWeightDist(-1.0f, 1.0f);
        std::uniform_real_distribution<float> biasDist(-0.5f, 0.5f);
        std::uniform_real_distribution<float> kernelDist(-1.0f, 1.0f);
        std::uniform_real_distribution<float> recurrentDist(-0.5f, 0.5f);

        for (auto& row : denseInWeights)
            for (auto& weight : row)
                weight = denseWeightDist(engine);

        for (auto& bias : denseInBias)
            bias = biasDist(engine);

        for (auto& row : gruKernelZ)
            for (auto& weight : row)
                weight = kernelDist(engine);
        for (auto& row : gruKernelR)
            for (auto& weight : row)
                weight = kernelDist(engine);
        for (auto& row : gruKernelC)
            for (auto& weight : row)
                weight = kernelDist(engine);

        for (auto& row : gruRecurrentZ)
            for (auto& weight : row)
                weight = recurrentDist(engine);
        for (auto& row : gruRecurrentR)
            for (auto& weight : row)
                weight = recurrentDist(engine);
        for (auto& row : gruRecurrentC)
            for (auto& weight : row)
                weight = recurrentDist(engine);

        for (auto& biases : gruBiasZ)
            for (auto& bias : biases)
                bias = biasDist(engine);
        for (auto& biases : gruBiasR)
            for (auto& bias : biases)
                bias = biasDist(engine);
        for (auto& biases : gruBiasC)
            for (auto& bias : biases)
                bias = biasDist(engine);

        for (auto& weight : denseOutWeights)
            weight = denseWeightDist(engine);

        reset();
    }

    float forward(const std::array<float, dims>& input) noexcept
    {
        std::array<float, dims> denseOut {};
        std::array<float, dims> gruOut {};

        denseForward(input, denseOut);
        for (auto& sample : denseOut)
            sample = flushSubnormal(std::tanh(sample));

        gruForward(denseOut, gruOut);

        float output = 0.0f;
        for (size_t i = 0; i < dims; ++i)
            output += denseOutWeights[i] * gruOut[i];

        return flushSubnormal(output);
    }

private:
    using Vec4 = std::array<float, dims>;
    using Mat4 = std::array<Vec4, dims>;
    using BiasPair = std::array<Vec4, 2>;

    static float dot(const Vec4& weights, const Vec4& values) noexcept
    {
        float sum = 0.0f;
        for (size_t i = 0; i < dims; ++i)
            sum += weights[i] * values[i];
        return flushSubnormal(sum);
    }

    static float sigmoid(float value) noexcept
    {
        return flushSubnormal(1.0f / (1.0f + std::exp(-value)));
    }

    void denseForward(const Vec4& input, Vec4& output) const noexcept
    {
        for (size_t i = 0; i < dims; ++i)
            output[i] = flushSubnormal(dot(denseInWeights[i], input) + denseInBias[i]);
    }

    void gruForward(const Vec4& input, Vec4& output) noexcept
    {
        for (size_t i = 0; i < dims; ++i) {
            const auto z = sigmoid(dot(gruKernelZ[i], input) + dot(gruRecurrentZ[i], hiddenState) + gruBiasZ[0][i] + gruBiasZ[1][i]);
            const auto r = sigmoid(dot(gruKernelR[i], input) + dot(gruRecurrentR[i], hiddenState) + gruBiasR[0][i] + gruBiasR[1][i]);
            const auto candidate = std::tanh(dot(gruKernelC[i], input) + r * (dot(gruRecurrentC[i], hiddenState) + gruBiasC[1][i]) + gruBiasC[0][i]);
            output[i] = flushSubnormal((1.0f - z) * candidate + z * hiddenState[i]);
        }

        hiddenState = output;
    }

    Mat4 denseInWeights {};
    Vec4 denseInBias {};

    Mat4 gruKernelZ {};
    Mat4 gruKernelR {};
    Mat4 gruKernelC {};
    Mat4 gruRecurrentZ {};
    Mat4 gruRecurrentR {};
    Mat4 gruRecurrentC {};
    BiasPair gruBiasZ {};
    BiasPair gruBiasR {};
    BiasPair gruBiasC {};

    Vec4 denseOutWeights {};
    Vec4 hiddenState {};
};

} // namespace rnnch
