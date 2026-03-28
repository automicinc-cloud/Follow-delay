#pragma once
#include <cmath>
#include <algorithm>

namespace followdelay
{

class FeedbackToneBlock
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        z1 = 0.0f;
        updateCoeff();
    }

    void setFrequency (float freqHz)
    {
        freq = std::clamp (freqHz, 20.0f, 18000.0f);
        updateCoeff();
    }

    void reset() { z1 = 0.0f; }

    float process (float in)
    {
        z1 += coeff * (in - z1);
        return z1;
    }

private:
    void updateCoeff()
    {
        if (sr <= 0.0) { coeff = 1.0f; return; }
        float wc = 2.0f * 3.14159265f * freq / (float) sr;
        coeff = wc / (1.0f + wc);
    }

    double sr    = 44100.0;
    float  freq  = 6500.0f;
    float  coeff = 1.0f;
    float  z1    = 0.0f;
};

} // namespace followdelay
