#pragma once

#include "omega.h"
#include "signum.h"
#include "wdf_t.h"

#include <cmath>

namespace centaurch
{
using namespace chowdsp::WDFT;

template <typename T, typename Next>
class CustomDiodePairT final : public RootWDF
{
public:
    CustomDiodePairT(T Is, T Vt, Next& next)
        : Is(Is)
        , Vt(Vt)
        , oneOverVt(static_cast<T>(1) / Vt)
        , next(next)
    {
        next.connectToParent(this);
        calcImpedance();
    }

    void calcImpedance() override
    {
        R_Is = next.wdf.R * Is;
        R_Is_overVt = R_Is * oneOverVt;
        logR_Is_overVt = std::log(R_Is_overVt);
    }

    void incident(T x) noexcept
    {
        a = x;
    }

    T reflected() noexcept
    {
        const T lambda = static_cast<T>(chowdsp::signum(a));
        const T wrightIn = logR_Is_overVt + lambda * a * oneOverVt + R_Is_overVt;
        b = a + static_cast<T>(2) * lambda * (R_Is - Vt * chowdsp::Omega::omega4(wrightIn));
        return b;
    }

    T a = static_cast<T>(0);
    T b = static_cast<T>(0);

private:
    const T Is;
    const T Vt;
    const T oneOverVt;
    T R_Is = static_cast<T>(0);
    T R_Is_overVt = static_cast<T>(0);
    T logR_Is_overVt = static_cast<T>(0);

    Next& next;
};

} // namespace centaurch
