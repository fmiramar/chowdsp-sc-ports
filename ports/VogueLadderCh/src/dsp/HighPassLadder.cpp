#include "HighPassLadder.h"

void HighPassLadder::reset(double newSampleRate)
{
    sampleRate = newSampleRate;
    for (auto& stage : hp)
        stage.reset(sampleRate);
}

double HighPassLadder::getSampleRate() const
{
    return sampleRate;
}

void HighPassLadder::setCutoff(double cutoff)
{
    const double omegaDigital = vogue_ladder_utility::TWO_PI * cutoff;
    const double omegaAnalog = vogue_ladder_utility::prewarp(omegaDigital, sampleRate);

    g = omegaAnalog / (2.0 * sampleRate);
    G = 1.0 / (1.0 + g);
    G2 = G * G;
    G3 = G2 * G;
    G4 = G3 * G;

    for (auto& stage : hp) {
        stage.setG(G);
        stage.setG2(g + g);
    }
}

void HighPassLadder::setResonance(double resonance)
{
    k = resonance;
}
