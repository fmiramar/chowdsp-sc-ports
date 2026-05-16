#pragma once

#include <algorithm>
#include <cmath>

namespace autowah
{
class ModPeakingFilter
{
public:
    void prepare(float newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
    }

    void reset() noexcept
    {
        s1 = 0.0f;
        s2 = 0.0f;
    }

    void calcCoefs(float freqHz, float qValue, float gainLinear) noexcept
    {
        float b[3] {};
        float a[3] {};
        calcPeakingFilter(b, a, freqHz, qValue, gainLinear, sampleRate);
        updateFromBiquad(b, a);
    }

    float process(float x) noexcept
    {
        const auto hp = (x - (g + R2) * s1 - s2) * h;
        const auto y = c0 * hp + c1 * s1 + c2 * s2;
        s2 = two_gSqr * hp + g2 * s1 + s2;
        s1 = g2 * hp + s1;
        return y;
    }

private:
    static void bilinearSecondOrder(float (&b)[3], float (&a)[3], const float (&bs)[3], const float (&as)[3], float K) noexcept
    {
        const auto kSq = K * K;
        const auto as0KSq = as[0] * kSq;
        const auto as1K = as[1] * K;
        const auto bs0KSq = bs[0] * kSq;
        const auto bs1K = bs[1] * K;
        const auto a0Inv = 1.0f / (as0KSq + as1K + as[2]);

        a[0] = 1.0f;
        a[1] = 2.0f * (as[2] - as0KSq) * a0Inv;
        a[2] = (as0KSq - as1K + as[2]) * a0Inv;
        b[0] = (bs0KSq + bs1K + bs[2]) * a0Inv;
        b[1] = 2.0f * (bs[2] - bs0KSq) * a0Inv;
        b[2] = (bs0KSq - bs1K + bs[2]) * a0Inv;
    }

    static void calcPeakingFilter(float (&b)[3], float (&a)[3], float freqHz, float qValue, float gainLinear, float sampleRate) noexcept
    {
        constexpr auto twoPi = 6.28318530717958647692f;
        const auto fc = std::clamp(freqHz, 10.0f, 0.48f * sampleRate);
        const auto q = std::max(qValue, 1.0e-4f);
        const auto gain = std::max(gainLinear, 1.0e-6f);
        const auto wc = twoPi * fc;
        const auto K = wc / std::tan(wc / (2.0f * sampleRate));
        const auto kSqTerm = 1.0f / (wc * wc);
        const auto kTerm = 1.0f / (q * wc);
        const auto kNum = gain > 1.0f ? kTerm * gain : kTerm;
        const auto kDen = gain < 1.0f ? kTerm / gain : kTerm;

        const float bs[3] { kSqTerm, kNum, 1.0f };
        const float asAnalog[3] { kSqTerm, kDen, 1.0f };
        bilinearSecondOrder(b, a, bs, asAnalog, K);
    }

    void updateFromBiquad(const float (&b)[3], const float (&a)[3]) noexcept
    {
        const auto tau = std::max(1.0f - a[1] + a[2], 1.0e-8f);
        const auto fourGSq = std::max((4.0f / tau) * (1.0f + a[1] + a[2]), 1.0e-8f);

        g2 = std::sqrt(fourGSq);
        R2 = 4.0f * (1.0f - a[2]) / (g2 * tau);
        c0 = (b[0] - b[1] + b[2]) / tau;
        c1 = 4.0f * (b[0] - b[2]) / (g2 * tau);
        c2 = 4.0f * (b[0] + b[1] + b[2]) / (fourGSq * tau);

        g = 0.5f * g2;
        two_gSqr = 0.5f * fourGSq;
        const auto gSqr = 0.25f * fourGSq;
        h = 1.0f / (1.0f + R2 * g + gSqr);

        c1 = c1 + c2 * g;
        c0 = c0 + c1 * g;
    }

    float sampleRate = 44100.0f;
    float s1 = 0.0f;
    float s2 = 0.0f;
    float g = 0.0f;
    float g2 = 0.0f;
    float two_gSqr = 0.0f;
    float h = 1.0f;
    float R2 = 0.0f;
    float c0 = 1.0f;
    float c1 = 0.0f;
    float c2 = 0.0f;
};
} // namespace autowah
