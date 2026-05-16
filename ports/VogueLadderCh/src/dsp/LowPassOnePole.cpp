#include "LowPassOnePole.h"

void LowPassOnePole::reset(double newSampleRate)
{
    sampleRate = newSampleRate;
    state = 0.0;
}

double LowPassOnePole::getSampleRate() const
{
    return sampleRate;
}

void LowPassOnePole::setG(double newG)
{
    G = newG;
}

double LowPassOnePole::getState() const
{
    return state;
}
