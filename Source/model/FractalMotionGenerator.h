#pragma once
#include "../util/Constants.h"
#include "../util/DataStructures.h"
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace followdelay
{

class FractalMotionGenerator
{
public:
    void generate (int seed, float roughness, float scale, int resolution = kMotionCurveSize)
    {
        curve.samples.resize (resolution);
        curve.valid = true;

        // Midpoint displacement algorithm
        std::vector<float> buf (resolution, 0.0f);
        uint32_t rng = static_cast<uint32_t> (seed);

        buf[0]              = nextRand (rng) * 0.5f + 0.25f;
        buf[resolution - 1] = nextRand (rng) * 0.5f + 0.25f;

        int step = resolution - 1;
        float displacement = scale * 0.5f;

        while (step > 1)
        {
            int halfStep = step / 2;
            for (int i = halfStep; i < resolution; i += step)
            {
                int left  = i - halfStep;
                int right = std::min (i + halfStep, resolution - 1);
                float mid = (buf[left] + buf[right]) * 0.5f;
                mid += (nextRand (rng) - 0.5f) * displacement;
                buf[i] = mid;
            }
            displacement *= std::pow (0.5f, 1.0f - roughness * 0.8f);
            step = halfStep;
        }

        // Normalize to [0, 1]
        float minV = *std::min_element (buf.begin(), buf.end());
        float maxV = *std::max_element (buf.begin(), buf.end());
        float range = maxV - minV;
        if (range < 1e-8f) range = 1.0f;

        for (int i = 0; i < resolution; ++i)
            curve.samples[i] = std::clamp ((buf[i] - minV) / range, 0.0f, 1.0f);
    }

    const MotionCurve& getCurve() const { return curve; }

private:
    float nextRand (uint32_t& state)
    {
        // xorshift32
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return (float)(state & 0x7FFFFFFF) / (float) 0x7FFFFFFF;
    }

    MotionCurve curve;
};

} // namespace followdelay
