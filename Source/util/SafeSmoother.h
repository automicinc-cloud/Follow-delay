#pragma once
#include <cmath>
#include <algorithm>

namespace followdelay
{

class SafeSmoother
{
public:
    void reset (float value)
    {
        current = value;
        target  = value;
    }

    void setTarget (float newTarget)
    {
        target = newTarget;
    }

    void setSmoothingTime (float timeMs, float sampleRate)
    {
        if (timeMs <= 0.0f || sampleRate <= 0.0f)
        {
            coeff = 1.0f;
            return;
        }
        float samples = (timeMs / 1000.0f) * sampleRate;
        coeff = 1.0f - std::exp (-1.0f / samples);
    }

    float getNext()
    {
        current += coeff * (target - current);
        return current;
    }

    float getCurrent() const { return current; }

private:
    float current = 0.0f;
    float target  = 0.0f;
    float coeff   = 1.0f;
};

// Slope-limited smoother for delay time modulation
class SlopeLimitedSmoother
{
public:
    void reset (float value)   { current = value; target = value; }
    void setTarget (float t)   { target = t; }

    void setMaxSlopePerSample (float slope) { maxSlope = slope; }

    float getNext()
    {
        float diff = target - current;
        if (std::abs (diff) <= maxSlope)
            current = target;
        else
            current += (diff > 0.0f ? maxSlope : -maxSlope);
        return current;
    }

    float getCurrent() const { return current; }

private:
    float current  = 0.0f;
    float target   = 0.0f;
    float maxSlope = 1.0f;
};

} // namespace followdelay
