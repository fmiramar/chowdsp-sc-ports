#include "HighPassOnePole.h"

void HighPassOnePole::reset(double newSampleRate)
{
    sampleRate = newSampleRate;
    state = 0.0;
}

double HighPassOnePole::getSampleRate() const
{
    return sampleRate;
}

void HighPassOnePole::setG(double newG)
{
    G = newG;
}

void HighPassOnePole::setG2(double newG2)
{
    g2 = newG2;
}

double HighPassOnePole::getState() const
{
    return state;
}
