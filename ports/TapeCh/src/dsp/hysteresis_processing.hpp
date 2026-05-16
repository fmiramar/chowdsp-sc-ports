#pragma once

#include <cmath>

namespace tapech
{
enum class SolverType
{
    RK2 = 0,
    RK4,
    NR4,
    NR8,
};

class HysteresisProcessing
{
public:
    HysteresisProcessing()
    {
        solver = &HysteresisProcessing::NR;
    }

    void reset()
    {
        M_n1 = 0.0;
        H_n1 = 0.0;
        H_d_n1 = 0.0;
        coth = 0.0;
        nearZero = false;
    }

    void setSampleRate(double newSampleRate)
    {
        fs = newSampleRate;
        T = 1.0 / fs;
        Talpha = T / 1.9;
    }

    void cook(float drive, float width, float sat, bool v1)
    {
        M_s = 0.5 + (1.0 - static_cast<double>(sat));
        a = M_s / (1.0e-6 + 6.0 * static_cast<double>(drive));
        c = std::sqrt(1.0f - static_cast<double>(width)) - 0.01;
        k = 0.47875;
        upperLim = 1.2;

        if (v1) {
            k = 27.0e3;
            c = 1.7e-1;
            M_s *= 50000.0;
            a = M_s / (0.01 + 40.0 * static_cast<double>(drive));
            upperLim = 100000.0;
        }

        nc = 1.0 - c;
        M_s_oa = M_s / a;
        M_s_oa_talpha = alpha * M_s_oa;
        M_s_oa_tc = c * M_s_oa;
        M_s_oa_tc_talpha = alpha * M_s_oa_tc;
        M_s_oaSq_tc_talpha = M_s_oa_tc_talpha / a;
        M_s_oaSq_tc_talphaSq = alpha * M_s_oaSq_tc_talpha;
    }

    void setSolver(SolverType solverType)
    {
        numIter = 0;
        solver = &HysteresisProcessing::NR;

        switch (solverType) {
        case SolverType::RK4:
            solver = &HysteresisProcessing::RK4;
            return;
        case SolverType::NR4:
            numIter = 4;
            return;
        case SolverType::NR8:
            numIter = 8;
            return;
        default:
            solver = &HysteresisProcessing::RK2;
            return;
        }
    }

    double process(double H) noexcept
    {
        const auto H_d = deriv(H, H_n1, H_d_n1);
        auto M = (this->*solver)(H, H_d);

        if (!std::isfinite(M) || M > upperLim) {
            M = 0.0;
            H = 0.0;
            H_d_n1 = 0.0;
        }

        M_n1 = M;
        H_n1 = H;
        H_d_n1 = H_d;

        return M;
    }

private:
    static constexpr double oneThird = 1.0 / 3.0;
    static constexpr double negTwoOver15 = -2.0 / 15.0;

    static int sign(double x) noexcept
    {
        return (x > 0.0) - (x < 0.0);
    }

    double langevin(double x) const noexcept
    {
        if (!nearZero)
            return coth - (1.0 / x);
        return x / 3.0;
    }

    double langevinD(double x) const noexcept
    {
        if (!nearZero)
            return (1.0 / (x * x)) - (coth * coth) + 1.0;
        return oneThird;
    }

    double langevinD2(double x) const noexcept
    {
        if (!nearZero)
            return 2.0 * coth * (coth * coth - 1.0) - (2.0 / (x * x * x));
        return negTwoOver15 * x;
    }

    double deriv(double x_n, double x_n1, double x_d_n1) const noexcept
    {
        constexpr double dAlpha = 0.25;
        return (((1.0 + dAlpha) / T) * (x_n - x_n1)) - dAlpha * x_d_n1;
    }

    double hysteresisFunc(double M, double H, double H_d) noexcept
    {
        Q = (H + alpha * M) / a;
        coth = 1.0 / std::tanh(Q);
        nearZero = Q < 0.001 && Q > -0.001;

        M_diff = M_s * langevin(Q) - M;

        delta = static_cast<double>((H_d >= 0.0) - (H_d < 0.0));
        delta_M = static_cast<double>(sign(delta) == sign(M_diff));

        L_prime = langevinD(Q);

        kap1 = nc * delta_M;
        f1Denom = nc * delta * k - alpha * M_diff;
        f1 = kap1 * M_diff / f1Denom;
        f2 = M_s_oa_tc * L_prime;
        f3 = 1.0 - (M_s_oa_tc_talpha * L_prime);

        return H_d * (f1 + f2) / f3;
    }

    double hysteresisFuncPrime(double H_d, double dMdt) noexcept
    {
        const auto L_prime2 = langevinD2(Q);
        const auto M_diff2 = M_s_oa_talpha * L_prime - 1.0;

        const auto f1_p = kap1 * ((M_diff2 / f1Denom) + M_diff * alpha * M_diff2 / (f1Denom * f1Denom));
        const auto f2_p = M_s_oaSq_tc_talpha * L_prime2;
        const auto f3_p = -M_s_oaSq_tc_talphaSq * L_prime2;

        return H_d * (f1_p + f2_p) / f3 - dMdt * f3_p / f3;
    }

    double RK2(double H, double H_d) noexcept
    {
        const auto k1 = T * hysteresisFunc(M_n1, H_n1, H_d_n1);
        const auto k2 = T * hysteresisFunc(M_n1 + (k1 / 2.0), (H + H_n1) / 2.0, (H_d + H_d_n1) / 2.0);
        return M_n1 + k2;
    }

    double RK4(double H, double H_d) noexcept
    {
        const auto H_1_2 = (H + H_n1) / 2.0;
        const auto H_d_1_2 = (H_d + H_d_n1) / 2.0;

        const auto k1 = T * hysteresisFunc(M_n1, H_n1, H_d_n1);
        const auto k2 = T * hysteresisFunc(M_n1 + (k1 / 2.0), H_1_2, H_d_1_2);
        const auto k3 = T * hysteresisFunc(M_n1 + (k2 / 2.0), H_1_2, H_d_1_2);
        const auto k4 = T * hysteresisFunc(M_n1 + k3, H, H_d);

        return M_n1 + k1 / 6.0 + k2 / 3.0 + k3 / 3.0 + k4 / 6.0;
    }

    double NR(double H, double H_d) noexcept
    {
        auto M = M_n1;
        const auto last_dMdt = hysteresisFunc(M_n1, H_n1, H_d_n1);

        double dMdt = 0.0;
        double dMdtPrime = 0.0;
        double deltaNR = 0.0;
        for (int n = 0; n < numIter; n += 4) {
            dMdt = hysteresisFunc(M, H, H_d);
            dMdtPrime = hysteresisFuncPrime(H_d, dMdt);
            deltaNR = (M - M_n1 - Talpha * (dMdt + last_dMdt)) / (1.0 - Talpha * dMdtPrime);
            M -= deltaNR;

            dMdt = hysteresisFunc(M, H, H_d);
            dMdtPrime = hysteresisFuncPrime(H_d, dMdt);
            deltaNR = (M - M_n1 - Talpha * (dMdt + last_dMdt)) / (1.0 - Talpha * dMdtPrime);
            M -= deltaNR;

            dMdt = hysteresisFunc(M, H, H_d);
            dMdtPrime = hysteresisFuncPrime(H_d, dMdt);
            deltaNR = (M - M_n1 - Talpha * (dMdt + last_dMdt)) / (1.0 - Talpha * dMdtPrime);
            M -= deltaNR;

            dMdt = hysteresisFunc(M, H, H_d);
            dMdtPrime = hysteresisFuncPrime(H_d, dMdt);
            deltaNR = (M - M_n1 - Talpha * (dMdt + last_dMdt)) / (1.0 - Talpha * dMdtPrime);
            M -= deltaNR;
        }

        return M;
    }

    using Solver = double (HysteresisProcessing::*)(double, double);
    Solver solver = nullptr;

    int numIter = 0;

    double fs = 48000.0;
    double T = 1.0 / fs;
    double Talpha = T / 1.9;
    double M_s = 1.0;
    double a = M_s / 4.0;
    const double alpha = 1.6e-3;
    double k = 0.47875;
    double c = 1.7e-1;
    double upperLim = 20.0;

    double nc = 1.0 - c;
    double M_s_oa = M_s / a;
    double M_s_oa_talpha = alpha * M_s / a;
    double M_s_oa_tc = c * M_s / a;
    double M_s_oa_tc_talpha = alpha * c * M_s / a;
    double M_s_oaSq_tc_talpha = alpha * c * M_s / (a * a);
    double M_s_oaSq_tc_talphaSq = alpha * alpha * c * M_s / (a * a);

    double M_n1 = 0.0;
    double H_n1 = 0.0;
    double H_d_n1 = 0.0;

    double Q = 0.0;
    double M_diff = 0.0;
    double delta = 0.0;
    double delta_M = 0.0;
    double L_prime = 0.0;
    double kap1 = 0.0;
    double f1Denom = 0.0;
    double f1 = 0.0;
    double f2 = 0.0;
    double f3 = 0.0;
    double coth = 0.0;
    bool nearZero = false;
};
} // namespace tapech
