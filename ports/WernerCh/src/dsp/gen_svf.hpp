#pragma once

#include <algorithm>
#include <array>
#include <cmath>

class GeneralSVF
{
public:
    GeneralSVF()
    {
        reset();
        setDrive(0.1f);
    }

    void reset() noexcept
    {
        v1.fill(0.0f);
    }

    void setDrive(float newDrive) noexcept
    {
        drive = std::max(newDrive, 1.0e-4f);
        invDrive = 1.0f / drive;
        makeup = std::max(1.0f, std::pow(drive, 0.75f));
    }

    void calcCoefs(float r, float k, float wc) noexcept
    {
        const double g = std::tan(static_cast<double>(wc));

        std::array<double, 16> a = {
            -2.0 * r, 1.0, 0.0, 4.0 * k * r * r,
            -1.0, 0.0, 0.0, 0.0,
            0.0, -1.0, -2.0 * r, 1.0,
            0.0, 0.0, -1.0, 0.0,
        };

        std::array<double, 16> m {};
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                const double identity = row == col ? 1.0 : 0.0;
                m[(size_t) (row * 4 + col)] = identity - g * a[(size_t) (row * 4 + col)];
            }
        }

        std::array<double, 16> inv {};
        if (!invert4x4(m, inv)) {
            setIdentity(aTilde);
            bTilde.fill(0.0f);
            cTilde.fill(0.0f);
            return;
        }

        std::array<double, 16> h {};
        for (size_t i = 0; i < h.size(); ++i)
            h[i] = g * inv[i];

        const auto hA = multiply4x4(h, a);
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                const size_t idx = (size_t) (row * 4 + col);
                const double identity = row == col ? 1.0 : 0.0;
                aTilde[idx] = (float) (2.0 * hA[idx] + identity);
            }
        }

        constexpr std::array<double, 4> b = { 1.0, 0.0, 0.0, 0.0 };
        const auto hB = multiply4x1(h, b);
        for (int i = 0; i < 4; ++i)
            bTilde[(size_t) i] = (float) (2.0 * hB[(size_t) i]);

        // C is [0, 0, 0, -1], so C * (H * A + I) is the negative last row.
        for (int col = 0; col < 4; ++col) {
            const size_t idx = (size_t) (3 * 4 + col);
            const double identity = col == 3 ? 1.0 : 0.0;
            cTilde[(size_t) col] = (float) -(hA[idx] + identity);
        }
    }

    float process(float x) noexcept
    {
        std::array<float, 4> v {};
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int col = 0; col < 4; ++col)
                sum += aTilde[(size_t) (row * 4 + col)] * v1[(size_t) col];
            v[(size_t) row] = sum + bTilde[(size_t) row] * x;
        }

        float y = 0.0f;
        for (int i = 0; i < 4; ++i)
            y += cTilde[(size_t) i] * v1[(size_t) i];

        applyNonlinearity(v);
        v1 = v;
        return y * makeup;
    }

private:
    static void setIdentity(std::array<float, 16>& matrix) noexcept
    {
        matrix.fill(0.0f);
        for (int i = 0; i < 4; ++i)
            matrix[(size_t) (i * 4 + i)] = 1.0f;
    }

    static std::array<double, 16> multiply4x4(const std::array<double, 16>& a, const std::array<double, 16>& b) noexcept
    {
        std::array<double, 16> result {};
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                double sum = 0.0;
                for (int k = 0; k < 4; ++k)
                    sum += a[(size_t) (row * 4 + k)] * b[(size_t) (k * 4 + col)];
                result[(size_t) (row * 4 + col)] = sum;
            }
        }
        return result;
    }

    static std::array<double, 4> multiply4x1(const std::array<double, 16>& a, const std::array<double, 4>& b) noexcept
    {
        std::array<double, 4> result {};
        for (int row = 0; row < 4; ++row) {
            double sum = 0.0;
            for (int k = 0; k < 4; ++k)
                sum += a[(size_t) (row * 4 + k)] * b[(size_t) k];
            result[(size_t) row] = sum;
        }
        return result;
    }

    static bool invert4x4(const std::array<double, 16>& matrix, std::array<double, 16>& inverse) noexcept
    {
        double augmented[4][8] {};

        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col)
                augmented[row][col] = matrix[(size_t) (row * 4 + col)];
            augmented[row][row + 4] = 1.0;
        }

        for (int col = 0; col < 4; ++col) {
            int pivotRow = col;
            double pivotAbs = std::abs(augmented[pivotRow][col]);
            for (int row = col + 1; row < 4; ++row) {
                const double candidate = std::abs(augmented[row][col]);
                if (candidate > pivotAbs) {
                    pivotRow = row;
                    pivotAbs = candidate;
                }
            }

            if (pivotAbs < 1.0e-12)
                return false;

            if (pivotRow != col) {
                for (int k = 0; k < 8; ++k)
                    std::swap(augmented[col][k], augmented[pivotRow][k]);
            }

            const double pivot = augmented[col][col];
            for (int k = 0; k < 8; ++k)
                augmented[col][k] /= pivot;

            for (int row = 0; row < 4; ++row) {
                if (row == col)
                    continue;

                const double factor = augmented[row][col];
                if (factor == 0.0)
                    continue;

                for (int k = 0; k < 8; ++k)
                    augmented[row][k] -= factor * augmented[col][k];
            }
        }

        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col)
                inverse[(size_t) (row * 4 + col)] = augmented[row][col + 4];
        }
        return true;
    }

    void applyNonlinearity(std::array<float, 4>& v) const noexcept
    {
        v[0] = std::tanh(v[0] * drive) * invDrive;
        v[2] = std::tanh(v[2] * drive) * invDrive;
        v[3] = std::tanh(v[3] * drive) * invDrive;
    }

    std::array<float, 16> aTilde {};
    std::array<float, 4> bTilde {};
    std::array<float, 4> cTilde {};
    std::array<float, 4> v1 {};

    float drive = 1.0f;
    float invDrive = 1.0f;
    float makeup = 1.0f;
};
