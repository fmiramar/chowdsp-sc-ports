#pragma once

#include "wdf_t.h"

#include <array>
#include <initializer_list>
#include <tuple>
#include <utility>

namespace juniorbwdft
{
namespace detail
{
template <typename Fn, typename Tuple, size_t... Ix>
constexpr void forEachInTuple(Fn&& fn, Tuple&& tuple, std::index_sequence<Ix...>)
{
    (void) std::initializer_list<int> { ((void) fn(std::get<Ix>(tuple), Ix), 0)... };
}

template <typename Fn, typename Tuple>
constexpr void forEachInTuple(Fn&& fn, Tuple&& tuple)
{
    forEachInTuple(std::forward<Fn>(fn),
                   std::forward<Tuple>(tuple),
                   std::make_index_sequence<std::tuple_size<std::remove_cv_t<std::remove_reference_t<Tuple>>>::value> {});
}

template <typename T, int nRows, int nCols>
inline void vecMatMult(const std::array<std::array<T, nCols>, nRows>& S,
                       const std::array<T, nCols>& a,
                       std::array<T, nRows>& b)
{
    for (int r = 0; r < nRows; ++r)
        b[(size_t) r] = static_cast<T>(0);

    for (int c = 0; c < nCols; ++c)
        for (int r = 0; r < nRows; ++r)
            b[(size_t) r] += S[(size_t) r][(size_t) c] * a[(size_t) c];
}
} // namespace detail

template <typename T, int numUpPorts, typename ImpedanceCalculator, typename... PortTypes>
class RtypeAdaptorMultN : public chowdsp::WDFT::BaseWDF
{
public:
    static constexpr auto numDownPorts = int(sizeof...(PortTypes));

    explicit RtypeAdaptorMultN(PortTypes&... dps) : downPorts(std::tie(dps...))
    {
        detail::forEachInTuple([&](auto& port, size_t) { port.connectToParent(this); }, downPorts);
    }

    void prepare(T sampleRate) { fs = sampleRate; }
    T getSampleRate() const noexcept { return fs; }

    void calcImpedance() override
    {
        ImpedanceCalculator::calcImpedance(*this);
    }

    constexpr auto getPortImpedances()
    {
        std::array<T, numDownPorts> portImpedances {};
        detail::forEachInTuple([&](auto& port, size_t i) { portImpedances[i] = port.wdf.R; }, downPorts);
        return portImpedances;
    }

    void setSMatrixData(const T (&mat)[numUpPorts + numDownPorts][numUpPorts + numDownPorts])
    {
        for (int i = 0; i < numUpPorts; ++i)
            for (int j = 0; j < numDownPorts; ++j)
                S12[(size_t) i][(size_t) j] = mat[i][j + numUpPorts];

        for (int i = 0; i < numDownPorts; ++i)
            for (int j = 0; j < numUpPorts + numDownPorts; ++j)
                S21S22[(size_t) i][(size_t) j] = mat[i + numUpPorts][j];
    }

    void incident(const T* downWave) noexcept
    {
        std::array<T, numUpPorts + numDownPorts> matMulVec {};
        for (int i = 0; i < numUpPorts; ++i)
            matMulVec[(size_t) i] = downWave[i];
        for (int i = 0; i < numDownPorts; ++i)
            matMulVec[(size_t) (numUpPorts + i)] = aDown[(size_t) i];

        detail::vecMatMult<T, numDownPorts, numUpPorts + numDownPorts>(S21S22, matMulVec, bDown);
        detail::forEachInTuple([&](auto& port, size_t i) { port.incident(bDown[i]); }, downPorts);
    }

    T* reflected() noexcept
    {
        detail::forEachInTuple([&](auto& port, size_t i) { aDown[i] = port.reflected(); }, downPorts);
        detail::vecMatMult<T, numUpPorts, numDownPorts>(S12, aDown, bUp);
        return bUp.data();
    }

private:
    std::tuple<PortTypes&...> downPorts;
    std::array<std::array<T, numDownPorts>, numUpPorts> S12 {};
    std::array<std::array<T, numUpPorts + numDownPorts>, numDownPorts> S21S22 {};
    std::array<T, numDownPorts> aDown {};
    std::array<T, numDownPorts> bDown {};
    std::array<T, numUpPorts> bUp {};
    T fs = static_cast<T>(48000);
};
} // namespace juniorbwdft
