#pragma once

#include "wdf_t.h"

#include <array>
#include <initializer_list>
#include <tuple>
#include <type_traits>
#include <utility>

namespace chowdsp::WDFT
{
namespace rtype_detail
{
template <typename Fn, typename Tuple, size_t... Ix>
constexpr void forEachInTuple(Fn&& fn, Tuple&& tuple, std::index_sequence<Ix...>)
{
    (void) std::initializer_list<int> { ((void) fn(std::get<Ix>(tuple), Ix), 0)... };
}

template <typename T>
using TupleIndexSequence = std::make_index_sequence<
    std::tuple_size<std::remove_cv_t<std::remove_reference_t<T>>>::value>;

template <typename Fn, typename Tuple>
constexpr void forEachInTuple(Fn&& fn, Tuple&& tuple)
{
    forEachInTuple(std::forward<Fn>(fn), std::forward<Tuple>(tuple), TupleIndexSequence<Tuple> {});
}

template <typename T, int numPorts>
inline void RtypeScatter(const std::array<std::array<T, (size_t) numPorts>, (size_t) numPorts>& S,
    const std::array<T, (size_t) numPorts>& a,
    std::array<T, (size_t) numPorts>& b)
{
    for (int c = 0; c < numPorts; ++c) {
        b[(size_t) c] = static_cast<T>(0);
        for (int r = 0; r < numPorts; ++r)
            b[(size_t) c] += S[(size_t) c][(size_t) r] * a[(size_t) r];
    }
}
} // namespace rtype_detail

template <typename T, typename ImpedanceCalculator, typename... PortTypes>
class RootRtypeAdaptor : public RootWDF
{
public:
    static constexpr auto numPorts = sizeof...(PortTypes);

    explicit RootRtypeAdaptor(PortTypes&... ports) : RootRtypeAdaptor(std::tuple<PortTypes&...>(ports...)) {}

    explicit RootRtypeAdaptor(std::tuple<PortTypes&...> ports) : downPorts(ports)
    {
        a_vec.fill(static_cast<T>(0));
        b_vec.fill(static_cast<T>(0));
        rtype_detail::forEachInTuple([&](auto& port, size_t) { port.connectToParent(this); }, downPorts);
    }

    void calcImpedance() override
    {
        ImpedanceCalculator::calcImpedance(*this);
    }

    constexpr auto getPortImpedances()
    {
        std::array<T, numPorts> portImpedances {};
        rtype_detail::forEachInTuple([&](auto& port, size_t i) { portImpedances[i] = port.wdf.R; }, downPorts);
        return portImpedances;
    }

    void setSMatrixData(const T (&mat)[numPorts][numPorts])
    {
        for (int i = 0; i < (int) numPorts; ++i)
            for (int j = 0; j < (int) numPorts; ++j)
                S_matrix[(size_t) i][(size_t) j] = mat[i][j];
    }

    void compute() noexcept
    {
        rtype_detail::RtypeScatter<T, (int) numPorts>(S_matrix, a_vec, b_vec);
        rtype_detail::forEachInTuple(
            [&](auto& port, size_t i) {
                port.incident(b_vec[i]);
                a_vec[i] = port.reflected();
            },
            downPorts);
    }

private:
    std::tuple<PortTypes&...> downPorts;
    std::array<std::array<T, (size_t) numPorts>, (size_t) numPorts> S_matrix {};
    std::array<T, (size_t) numPorts> a_vec {};
    std::array<T, (size_t) numPorts> b_vec {};
};

template <typename T, int upPortIndex, typename ImpedanceCalculator, typename... PortTypes>
class RtypeAdaptor : public BaseWDF
{
public:
    static constexpr auto numPorts = sizeof...(PortTypes) + 1;

    explicit RtypeAdaptor(PortTypes&... ports) : RtypeAdaptor(std::tuple<PortTypes&...>(ports...)) {}

    explicit RtypeAdaptor(std::tuple<PortTypes&...> ports) : downPorts(ports)
    {
        a_vec.fill(static_cast<T>(0));
        b_vec.fill(static_cast<T>(0));
        rtype_detail::forEachInTuple([&](auto& port, size_t) { port.connectToParent(this); }, downPorts);
    }

    void calcImpedance() override
    {
        wdf.R = ImpedanceCalculator::calcImpedance(*this);
        wdf.G = static_cast<T>(1) / wdf.R;
    }

    constexpr auto getPortImpedances()
    {
        std::array<T, numPorts - 1> portImpedances {};
        rtype_detail::forEachInTuple([&](auto& port, size_t i) { portImpedances[i] = port.wdf.R; }, downPorts);
        return portImpedances;
    }

    void setSMatrixData(const T (&mat)[numPorts][numPorts])
    {
        for (int i = 0; i < (int) numPorts; ++i)
            for (int j = 0; j < (int) numPorts; ++j)
                S_matrix[(size_t) i][(size_t) j] = mat[i][j];
    }

    void incident(T downWave) noexcept
    {
        wdf.a = downWave;
        a_vec[(size_t) upPortIndex] = wdf.a;

        rtype_detail::RtypeScatter<T, numPorts>(S_matrix, a_vec, b_vec);
        rtype_detail::forEachInTuple(
            [&](auto& port, size_t i) {
                const auto portIndex = getPortIndex((int) i);
                port.incident(b_vec[(size_t) portIndex]);
            },
            downPorts);
    }

    T reflected() noexcept
    {
        rtype_detail::forEachInTuple(
            [&](auto& port, size_t i) {
                const auto portIndex = getPortIndex((int) i);
                a_vec[(size_t) portIndex] = port.reflected();
            },
            downPorts);

        wdf.b = b_vec[(size_t) upPortIndex];
        return wdf.b;
    }

    WDFMembers<T> wdf;

private:
    static constexpr int getPortIndex(int tupleIndex) noexcept
    {
        return tupleIndex < upPortIndex ? tupleIndex : tupleIndex + 1;
    }

    std::tuple<PortTypes&...> downPorts;
    std::array<std::array<T, (size_t) numPorts>, (size_t) numPorts> S_matrix {};
    std::array<T, (size_t) numPorts> a_vec {};
    std::array<T, (size_t) numPorts> b_vec {};
};
} // namespace chowdsp::WDFT
