#include "LowPassLadder.h"

void LowPassLadder::reset(double newSampleRate)
{
    sampleRate = newSampleRate;
    for (auto& stage : lp)
        stage.reset(sampleRate);
}

double LowPassLadder::getSampleRate() const
{
    return sampleRate;
}

void LowPassLadder::setCutoff(double cutoff)
{
    const double omegaDigital = vogue_ladder_utility::TWO_PI * cutoff;
    const double omegaAnalog = vogue_ladder_utility::prewarp(omegaDigital, sampleRate);

    g = omegaAnalog / (2.0 * sampleRate);
    G = g / (1.0 + g);
    G2 = G * G;
    G3 = G2 * G;
    G4 = G3 * G;

    for (auto& stage : lp)
        stage.setG(G);
}

void LowPassLadder::setResonance(double resonance)
{
    k = resonance;
}
