#pragma once

#include "LowPassOnePole.h"
#include "utility.h"

class LowPassLadder
{
public:
    LowPassLadder() = default;

    void reset(double newSampleRate);
    double getSampleRate() const;
    void setCutoff(double cutoff);
    void setResonance(double resonance);

    inline double process(double x)
    {
        const double s1 = lp[0].getState();
        const double s2 = lp[1].getState();
        const double s3 = lp[2].getState();
        const double s4 = lp[3].getState();

        const double S = (G3 * s1 + G2 * s2 + G * s3 + s4) / (1.0 + g);
        double y = (x - k * S) / (1.0 + k * G4);

        for (int i = 0; i < 4; ++i)
            y = lp[i].process(y);

        return y * (1.0 + k);
    }

private:
    double sampleRate { 0.0 };
    double k { 0.0 };
    double g { 0.0 };
    double G { 0.0 };
    double G2 { 0.0 };
    double G3 { 0.0 };
    double G4 { 0.0 };
    LowPassOnePole lp[4];
};
