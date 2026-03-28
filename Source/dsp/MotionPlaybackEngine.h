#pragma once
#include "../util/Constants.h"
#include "../util/DataStructures.h"
#include "../util/SafeSmoother.h"
#include <cmath>
#include <algorithm>

namespace followdelay
{

class MotionPlaybackEngine
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        motionSmootherL.reset (0.0f);
        motionSmootherR.reset (0.0f);
        motionSmootherL.setSmoothingTime (3.0f, (float) sr);
        motionSmootherR.setSmoothingTime (3.0f, (float) sr);
    }

    void startMotion (const MotionPlayPlan& plan)
    {
        activePlan = plan;

        if (plan.restartFromZero)
        {
            runtimeState.elapsedSamples = 0;
            runtimeState.startSample    = plan.startSample;
        }

        runtimeState.durationSamples = plan.fittedDurationSamples;
        runtimeState.active = true;
    }

    void updatePlan (const MotionPlayPlan& plan)
    {
        activePlan = plan;
        runtimeState.durationSamples = plan.fittedDurationSamples;
    }

    bool isActive() const { return runtimeState.active; }

    const RuntimeMotionState& getRuntimeState() const { return runtimeState; }

    // Set the source curves (call before processBlock when they change)
    void setDrawCurve    (const MotionCurve& c) { drawCurve    = c; }
    void setLearnedCurve (const MotionCurve& c) { learnedCurve = c; }
    void setFractalCurve (const MotionCurve& c) { fractalCurve = c; }

    // Get L/R delay times in ms for one sample
    // startMs, endMs are the user's Start/End delay range
    void processSample (float startMs, float endMs,
                        float& outDelayL, float& outDelayR)
    {
        if (!runtimeState.active)
        {
            // No motion — output the "rest" delay time (Start value)
            outDelayL = startMs;
            outDelayR = startMs;
            return;
        }

        runtimeState.elapsedSamples++;

        if (runtimeState.durationSamples <= 0)
            runtimeState.durationSamples = 1;

        float playhead = (float) runtimeState.elapsedSamples
                       / (float) runtimeState.durationSamples;

        if (playhead >= 1.0f)
        {
            runtimeState.active = false;
            runtimeState.playheadNorm = 1.0f;
            runtimeState.currentValueL = 0.0f;
            runtimeState.currentValueR = 0.0f;
            outDelayL = endMs;
            outDelayR = endMs;
            return;
        }

        runtimeState.playheadNorm = playhead;

        // Apply warp (time remapping)
        float warpedTime = applyWarp (playhead, activePlan.warp);

        // Sample base curve
        float baseValue = sampleCurve (getBaseCurve(), warpedTime);

        // Blend fractal
        float fracVal = baseValue;
        if (activePlan.fractalAmount > 0.0f && fractalCurve.valid)
        {
            float fSample = sampleCurve (fractalCurve, warpedTime);
            fracVal = std::clamp (baseValue + activePlan.fractalAmount * (fSample - 0.5f),
                                  0.0f, 1.0f);
        }

        // Stereo spread
        float spread = activePlan.stereoSpread;
        float offsetL = fracVal;
        float offsetR = fracVal;
        if (spread > 0.0f)
        {
            float spreadShift = spread * 0.1f; // up to 10% phase offset
            float timeL = std::clamp (warpedTime - spreadShift, 0.0f, 1.0f);
            float timeR = std::clamp (warpedTime + spreadShift, 0.0f, 1.0f);
            float baseL = sampleCurve (getBaseCurve(), timeL);
            float baseR = sampleCurve (getBaseCurve(), timeR);

            if (activePlan.fractalAmount > 0.0f && fractalCurve.valid)
            {
                float fL = sampleCurve (fractalCurve, timeL);
                float fR = sampleCurve (fractalCurve, timeR);
                baseL = std::clamp (baseL + activePlan.fractalAmount * (fL - 0.5f), 0.0f, 1.0f);
                baseR = std::clamp (baseR + activePlan.fractalAmount * (fR - 0.5f), 0.0f, 1.0f);
            }
            offsetL = baseL;
            offsetR = baseR;
        }

        // Smooth
        motionSmootherL.setTarget (offsetL);
        motionSmootherR.setTarget (offsetR);

        float smoothL = motionSmootherL.getNext();
        float smoothR = motionSmootherR.getNext();

        runtimeState.currentValueL = smoothL;
        runtimeState.currentValueR = smoothR;

        // Map to delay time
        outDelayL = startMs + smoothL * (endMs - startMs);
        outDelayR = startMs + smoothR * (endMs - startMs);

        outDelayL = std::clamp (outDelayL, kMinDelayMs, kMaxDelayMs);
        outDelayR = std::clamp (outDelayR, kMinDelayMs, kMaxDelayMs);
    }

private:
    const MotionCurve& getBaseCurve() const
    {
        switch (activePlan.sourceMode)
        {
            case SourceMode::Learn:
            case SourceMode::LearnFractal:
                return learnedCurve.valid ? learnedCurve : drawCurve;

            case SourceMode::FractalOnly:
                return fractalCurve;

            case SourceMode::Draw:
            case SourceMode::DrawFractal:
            default:
                return drawCurve;
        }
    }

    static float sampleCurve (const MotionCurve& c, float t)
    {
        if (!c.valid || c.samples.empty()) return 0.5f;

        float pos = t * (float)(c.samples.size() - 1);
        int i0 = std::clamp ((int) pos, 0, (int) c.samples.size() - 1);
        int i1 = std::min (i0 + 1, (int) c.samples.size() - 1);
        float frac = pos - (float) i0;
        return c.samples[i0] + frac * (c.samples[i1] - c.samples[i0]);
    }

    static float applyWarp (float t, float warp)
    {
        // warp 0.0 = ease-in (slow start), 0.5 = linear, 1.0 = ease-out (fast start)
        if (std::abs (warp - 0.5f) < 0.01f) return t;

        float exponent = 1.0f;
        if (warp < 0.5f)
            exponent = 1.0f + (0.5f - warp) * 4.0f; // up to 3.0
        else
            exponent = 1.0f / (1.0f + (warp - 0.5f) * 4.0f); // down to ~0.33

        return std::pow (t, exponent);
    }

    double sr = 44100.0;
    MotionPlayPlan    activePlan;
    RuntimeMotionState runtimeState;

    MotionCurve drawCurve;
    MotionCurve learnedCurve;
    MotionCurve fractalCurve;

    SafeSmoother motionSmootherL;
    SafeSmoother motionSmootherR;
};

} // namespace followdelay
