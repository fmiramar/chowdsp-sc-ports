#pragma once

class LowPassOnePole
{
public:
    LowPassOnePole() = default;

    void reset(double newSampleRate);
    double getSampleRate() const;
    void setG(double newG);
    double getState() const;

    inline double process(double x)
    {
        const double v = G * (x - state);
        const double y = v + state;
        state = v + y;
        return y;
    }

private:
    double sampleRate { 0.0 };
    double state { 0.0 };
    double G { 0.0 };
};
