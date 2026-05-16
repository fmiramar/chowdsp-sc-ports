#pragma once

#include "HighPassOnePole.h"
#include "utility.h"

class HighPassLadder
{
public:
    HighPassLadder() = default;

    void reset(double newSampleRate);
    double getSampleRate() const;
    void setCutoff(double cutoff);
    void setResonance(double resonance);

    inline double process(double x)
    {
        const double s1 = hp[0].getState();
        const double s2 = hp[1].getState();
        const double s3 = hp[2].getState();
        const double s4 = hp[3].getState();

        const double S = -G4 * s1 - G3 * s2 - G2 * s3 - G * s4;
        double y = (x - k * S) / (1.0 + k * G4);

        for (int i = 0; i < 4; ++i)
            y = hp[i].process(y);

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
    HighPassOnePole hp[4];
};
