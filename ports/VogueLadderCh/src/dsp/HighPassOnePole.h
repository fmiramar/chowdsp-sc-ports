#pragma once

class HighPassOnePole
{
public:
    HighPassOnePole() = default;

    void reset(double newSampleRate);
    double getSampleRate() const;
    void setG(double newG);
    void setG2(double newG2);
    double getState() const;

    inline double process(double x)
    {
        const double y = G * (x - state);
        state = state + y * g2;
        return y;
    }

private:
    double sampleRate { 0.0 };
    double state { 0.0 };
    double G { 0.0 };
    double g2 { 0.0 };
};
