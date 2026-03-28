#pragma once
#include <JuceHeader.h>
#include "../util/Constants.h"
#include "../util/SafeSmoother.h"
#include "FeedbackToneBlock.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace followdelay
{

class DelayEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;

        int maxDelaySamples = static_cast<int> (kMaxDelayMs * 0.001 * sr) + 512;
        bufferL.assign (maxDelaySamples, 0.0f);
        bufferR.assign (maxDelaySamples, 0.0f);
        bufSize = maxDelaySamples;
        writePos = 0;

        toneL.prepare (sr);
        toneR.prepare (sr);

        delaySmootherL.reset (420.0f);
        delaySmootherR.reset (420.0f);
        delaySmootherL.setSmoothingTime (5.0f, (float) sr);
        delaySmootherR.setSmoothingTime (5.0f, (float) sr);

        feedbackSmoother.reset (0.35f);
        feedbackSmoother.setSmoothingTime (10.0f, (float) sr);

        mixSmoother.reset (0.32f);
        mixSmoother.setSmoothingTime (10.0f, (float) sr);

        // Slope limiter: max 0.5ms of delay-time change per sample
        float maxSlopePerSample = 0.5f;
        slopeL.reset (420.0f);
        slopeR.reset (420.0f);
        slopeL.setMaxSlopePerSample (maxSlopePerSample);
        slopeR.setMaxSlopePerSample (maxSlopePerSample);
    }

    void reset()
    {
        std::fill (bufferL.begin(), bufferL.end(), 0.0f);
        std::fill (bufferR.begin(), bufferR.end(), 0.0f);
        writePos = 0;
        toneL.reset();
        toneR.reset();
    }

    void setFeedback (float fb)   { feedbackSmoother.setTarget (std::min (fb, kFeedbackCeiling)); }
    void setTone     (float hz)   { toneL.setFrequency (hz); toneR.setFrequency (hz); }
    void setMix      (float m)    { mixSmoother.setTarget (m); }
    void setSafeMode (bool safe)  { safeMode = safe; }

    void setDelayTimeMs (float leftMs, float rightMs)
    {
        float clampedL = std::clamp (leftMs,  kMinDelayMs, kMaxDelayMs);
        float clampedR = std::clamp (rightMs, kMinDelayMs, kMaxDelayMs);
        delaySmootherL.setTarget (clampedL);
        delaySmootherR.setTarget (clampedR);
    }

    void processBlock (float* leftData, float* rightData, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float targetL = delaySmootherL.getNext();
            float targetR = delaySmootherR.getNext();

            if (safeMode)
            {
                slopeL.setTarget (targetL);
                slopeR.setTarget (targetR);
                targetL = slopeL.getNext();
                targetR = slopeR.getNext();
            }

            float fb  = feedbackSmoother.getNext();
            float mix = mixSmoother.getNext();

            float dryL = leftData[i];
            float dryR = rightData[i];

            // Read with cubic interpolation
            float wetL = readInterpolated (bufferL, targetL);
            float wetR = readInterpolated (bufferR, targetR);

            // Feedback path with tone
            float fbL = toneL.process (wetL) * fb;
            float fbR = toneR.process (wetR) * fb;

            // Soft clip feedback to prevent runaway
            fbL = softClip (fbL);
            fbR = softClip (fbR);

            // Write
            bufferL[writePos] = dryL + fbL;
            bufferR[writePos] = dryR + fbR;

            // Flush denormals
            bufferL[writePos] = flushDenorm (bufferL[writePos]);
            bufferR[writePos] = flushDenorm (bufferR[writePos]);

            writePos = (writePos + 1) % bufSize;

            // Output
            leftData[i]  = dryL * (1.0f - mix) + wetL * mix;
            rightData[i] = dryR * (1.0f - mix) + wetR * mix;
        }
    }

private:
    float readInterpolated (const std::vector<float>& buf, float delayMs) const
    {
        float delaySamples = delayMs * 0.001f * (float) sr;
        delaySamples = std::clamp (delaySamples, 1.0f, (float)(bufSize - 4));

        float readPos = (float) writePos - delaySamples;
        if (readPos < 0.0f) readPos += (float) bufSize;

        int   i0 = ((int) readPos - 1 + bufSize) % bufSize;
        int   i1 = ((int) readPos)               % bufSize;
        int   i2 = ((int) readPos + 1)            % bufSize;
        int   i3 = ((int) readPos + 2)            % bufSize;
        float frac = readPos - std::floor (readPos);

        float y0 = buf[i0], y1 = buf[i1], y2 = buf[i2], y3 = buf[i3];

        // Cubic (Hermite) interpolation
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    static float softClip (float x)
    {
        if (x > 1.0f)       return 1.0f - 1.0f / (1.0f + x);
        else if (x < -1.0f) return -1.0f + 1.0f / (1.0f - x);
        return x;
    }

    static float flushDenorm (float x)
    {
        return (std::abs (x) < 1.0e-15f) ? 0.0f : x;
    }

    double sr = 44100.0;
    std::vector<float> bufferL, bufferR;
    int bufSize  = 0;
    int writePos = 0;

    FeedbackToneBlock toneL, toneR;
    SafeSmoother delaySmootherL, delaySmootherR;
    SafeSmoother feedbackSmoother;
    SafeSmoother mixSmoother;
    SlopeLimitedSmoother slopeL, slopeR;
    bool safeMode = true;
};

} // namespace followdelay
