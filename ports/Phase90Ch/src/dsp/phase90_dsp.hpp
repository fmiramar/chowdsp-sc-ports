#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace phase90ch
{
inline float algebraicSigmoid(float x) noexcept
{
    return x / std::sqrt(1.0f + x * x);
}

template <typename T>
inline T fastSigmoid(T x) noexcept
{
    return (T) 2 * algebraicSigmoid((T) 0.5 * x);
}

namespace conformal_maps
{
template <typename T>
inline T computeKValueAngular(T wc, T fs) noexcept
{
    return wc / std::tan(wc / ((T) 2 * fs));
}

template <typename T>
inline void bilinear1(T (&b)[2], T (&a)[2], const T (&bs)[2], const T (&as)[2], T K) noexcept
{
    const auto a0Inv = (T) 1 / (as[0] * K + as[1]);
    b[0] = (bs[0] * K + bs[1]) * a0Inv;
    b[1] = (-bs[0] * K + bs[1]) * a0Inv;
    a[0] = (T) 1;
    a[1] = (-as[0] * K + as[1]) * a0Inv;
}

template <typename T>
inline void bilinear2(T (&b)[3], T (&a)[3], const T (&bs)[3], const T (&as)[3], T K) noexcept
{
    const auto KSq = K * K;
    const auto as0KSq = as[0] * KSq;
    const auto as1K = as[1] * K;
    const auto bs0KSq = bs[0] * KSq;
    const auto bs1K = bs[1] * K;
    const auto a0Inv = (T) 1 / (as0KSq + as1K + as[2]);

    a[0] = (T) 1;
    a[1] = (T) 2 * (as[2] - as0KSq) * a0Inv;
    a[2] = (as0KSq - as1K + as[2]) * a0Inv;
    b[0] = (bs0KSq + bs1K + bs[2]) * a0Inv;
    b[1] = (T) 2 * (bs[2] - bs0KSq) * a0Inv;
    b[2] = (bs0KSq - bs1K + bs[2]) * a0Inv;
}

template <typename T>
inline void bilinear3(T (&b)[4], T (&a)[4], const T (&bs)[4], const T (&as)[4], T K) noexcept
{
    const auto KSq = K * K;
    const auto KCb = K * KSq;
    const auto as0KCb = as[0] * KCb;
    const auto as1KSq = as[1] * KSq;
    const auto as2K = as[2] * K;
    const auto bs0KCb = bs[0] * KCb;
    const auto bs1KSq = bs[1] * KSq;
    const auto bs2K = bs[2] * K;
    const auto a0Inv = (T) 1 / (as0KCb + as1KSq + as2K + as[3]);

    a[0] = (T) 1;
    a[1] = ((T) -3 * as0KCb - as1KSq + as2K + (T) 3 * as[3]) * a0Inv;
    a[2] = ((T) 3 * as0KCb - as1KSq - as2K + (T) 3 * as[3]) * a0Inv;
    a[3] = (-as0KCb + as1KSq - as2K + as[3]) * a0Inv;
    b[0] = (bs0KCb + bs1KSq + bs2K + bs[3]) * a0Inv;
    b[1] = ((T) -3 * bs0KCb - bs1KSq + bs2K + (T) 3 * bs[3]) * a0Inv;
    b[2] = ((T) 3 * bs0KCb - bs1KSq - bs2K + (T) 3 * bs[3]) * a0Inv;
    b[3] = (-bs0KCb + bs1KSq - bs2K + bs[3]) * a0Inv;
}
} // namespace conformal_maps

namespace linear_transforms
{
template <int Order, typename T>
inline void transformFeedback(T (&b)[Order + 1], T (&a)[Order + 1], T G) noexcept
{
    const auto a0 = a[0] - G * b[0];
    const auto a0Inv = (T) 1 / a0;
    a[0] = (T) 1;
    for (int i = 1; i <= Order; ++i)
        a[i] = (a[i] - G * b[i]) * a0Inv;
    for (int i = 0; i <= Order; ++i)
        b[i] *= a0Inv;
}
} // namespace linear_transforms

template <int Order>
struct IIRFilter;

template <>
struct IIRFilter<1>
{
    float a[2] { 1.0f, 0.0f };
    float b[2] { 1.0f, 0.0f };
    float z1 = 0.0f;

    void reset() noexcept { z1 = 0.0f; }

    inline float processSample(float x) noexcept
    {
        const float y = z1 + x * b[0];
        z1 = x * b[1] - y * a[1];
        return y;
    }
};

template <>
struct IIRFilter<2>
{
    float a[3] { 1.0f, 0.0f, 0.0f };
    float b[3] { 1.0f, 0.0f, 0.0f };
    float z1 = 0.0f;
    float z2 = 0.0f;

    void reset() noexcept { z1 = z2 = 0.0f; }

    inline float processSample(float x) noexcept
    {
        const float y = z1 + x * b[0];
        z1 = z2 + x * b[1] - y * a[1];
        z2 = x * b[2] - y * a[2];
        return y;
    }
};

template <>
struct IIRFilter<3>
{
    float a[4] { 1.0f, 0.0f, 0.0f, 0.0f };
    float b[4] { 1.0f, 0.0f, 0.0f, 0.0f };
    float z1 = 0.0f;
    float z2 = 0.0f;
    float z3 = 0.0f;

    void reset() noexcept { z1 = z2 = z3 = 0.0f; }

    inline float processSample(float x) noexcept
    {
        const float y = z1 + x * b[0];
        z1 = z2 + x * b[1] - y * a[1];
        z2 = z3 + x * b[2] - y * a[2];
        z3 = x * b[3] - y * a[3];
        return y;
    }
};

template <bool withFeedback = false>
struct Phase90Stage1 : IIRFilter<1>
{
    void prepare(float sampleRate)
    {
        fs = sampleRate;
        K = conformal_maps::computeKValueAngular(1.0f / (C1 * R6), fs);
        this->reset();
    }

    void setResistor(float newR6) noexcept
    {
        R6 = newR6;
        K = conformal_maps::computeKValueAngular(1.0f / (C1 * R6), fs);
    }

    void setCapacitor(float newC1) noexcept
    {
        C1 = newC1;
        K = conformal_maps::computeKValueAngular(1.0f / (C1 * R6), fs);
    }

    inline float processSample(float input, float modulation01, float feedback) noexcept
    {
        const auto k = C1 * R6 * modulation01;
        float bs[2] { k, -1.0f };
        float as[2] { k, 1.0f };
        conformal_maps::bilinear1(this->b, this->a, bs, as, K);
        if constexpr (withFeedback)
            linear_transforms::transformFeedback<1>(this->b, this->a, feedback);
        return fastSigmoid(IIRFilter<1>::processSample(input));
    }

    float fs = 48000.0f;
    float K = 1.0f;
    float C1 = 47.0e-9f;
    float R6 = 24.0e3f;
};

template <bool withFeedback = false>
struct Phase90Stage2 : IIRFilter<2>
{
    void prepare(float sampleRate)
    {
        fs = sampleRate;
        K = conformal_maps::computeKValueAngular(1.0f / (C1 * R6), fs);
        this->reset();
    }

    void setResistor(float newR6) noexcept
    {
        R6 = newR6;
        K = conformal_maps::computeKValueAngular(1.0f / (C1 * R6), fs);
    }

    void setCapacitor(float newC1) noexcept
    {
        C1 = newC1;
        K = conformal_maps::computeKValueAngular(1.0f / (C1 * R6), fs);
    }

    inline float processSample(float input, float modulation01, float feedback) noexcept
    {
        const auto k = C1 * R6 * modulation01;
        float bs[3] { k * k, -2.0f * k, 1.0f };
        float as[3] { k * k, 2.0f * k, 1.0f };
        conformal_maps::bilinear2(this->b, this->a, bs, as, K);
        if constexpr (withFeedback)
            linear_transforms::transformFeedback<2>(this->b, this->a, feedback);
        return fastSigmoid(IIRFilter<2>::processSample(input));
    }

    float fs = 48000.0f;
    float K = 1.0f;
    float C1 = 47.0e-9f;
    float R6 = 24.0e3f;
};

template <bool withFeedback = false>
struct Phase90Stage3 : IIRFilter<3>
{
    void prepare(float sampleRate)
    {
        fs = sampleRate;
        K = conformal_maps::computeKValueAngular(1.0f / (C1 * R6), fs);
        this->reset();
    }

    void setResistor(float newR6) noexcept
    {
        R6 = newR6;
        K = conformal_maps::computeKValueAngular(1.0f / (C1 * R6), fs);
    }

    void setCapacitor(float newC1) noexcept
    {
        C1 = newC1;
        K = conformal_maps::computeKValueAngular(1.0f / (C1 * R6), fs);
    }

    inline float processSample(float input, float modulation01, float feedback) noexcept
    {
        const auto k = C1 * R6 * modulation01;
        float bs[4] { k * k * k, -3.0f * k * k, 3.0f * k, -1.0f };
        float as[4] { k * k * k, 3.0f * k * k, 3.0f * k, 1.0f };
        conformal_maps::bilinear3(this->b, this->a, bs, as, K);
        if constexpr (withFeedback)
            linear_transforms::transformFeedback<3>(this->b, this->a, feedback);
        return fastSigmoid(IIRFilter<3>::processSample(input));
    }

    float fs = 48000.0f;
    float K = 1.0f;
    float C1 = 47.0e-9f;
    float R6 = 24.0e3f;
};

struct Phase90FB2
{
    void prepare(float sampleRate) { stage1.prepare(sampleRate); stage2_4.prepare(sampleRate); }
    void setResistor(float r) noexcept { stage1.setResistor(r); stage2_4.setResistor(r); }
    void setCapacitor(float c) noexcept { stage1.setCapacitor(c); stage2_4.setCapacitor(c); }
    inline float processSample(float x, float mod01, float fb) noexcept
    {
        return stage2_4.processSample(stage1.processSample(x, mod01, fb), mod01, fb);
    }
    Phase90Stage1<> stage1;
    Phase90Stage3<true> stage2_4;
};

struct Phase90FB3
{
    void prepare(float sampleRate) { stage1_2.prepare(sampleRate); stage3_4.prepare(sampleRate); }
    void setResistor(float r) noexcept { stage1_2.setResistor(r); stage3_4.setResistor(r); }
    void setCapacitor(float c) noexcept { stage1_2.setCapacitor(c); stage3_4.setCapacitor(c); }
    inline float processSample(float x, float mod01, float fb) noexcept
    {
        return stage3_4.processSample(stage1_2.processSample(x, mod01, fb), mod01, fb);
    }
    Phase90Stage2<> stage1_2;
    Phase90Stage2<true> stage3_4;
};

struct Phase90FB4
{
    void prepare(float sampleRate) { stage1_3.prepare(sampleRate); stage4.prepare(sampleRate); }
    void setResistor(float r) noexcept { stage1_3.setResistor(r); stage4.setResistor(r); }
    void setCapacitor(float c) noexcept { stage1_3.setCapacitor(c); stage4.setCapacitor(c); }
    inline float processSample(float x, float mod01, float fb) noexcept
    {
        return stage4.processSample(stage1_3.processSample(x, mod01, fb), mod01, fb);
    }
    Phase90Stage3<> stage1_3;
    Phase90Stage1<true> stage4;
};

struct TriangleLFO
{
    float sampleRate = 48000.0f;
    float phase = 0.0f;
    float freq = 1.0f;

    void reset(float fs, float initialPhase = 0.0f) noexcept
    {
        sampleRate = fs;
        phase = initialPhase;
    }

    void setFrequency(float newFreq) noexcept
    {
        freq = std::max(newFreq, 0.0f);
    }

    inline float processSample() noexcept
    {
        phase += freq / sampleRate;
        if (phase >= 1.0f)
            phase -= std::floor(phase);
        return 1.0f - 4.0f * std::abs(phase - 0.5f);
    }
};

inline float shapeLFO(float x) noexcept
{
    constexpr auto skewFactor = 0.7071067811865476f;
    return 2.0f * std::pow((x + 1.0f) * 0.5f, skewFactor) - 1.0f;
}
} // namespace phase90ch
