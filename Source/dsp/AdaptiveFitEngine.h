#pragma once
#include "../util/Constants.h"
#include "../util/DataStructures.h"
#include <cmath>
#include <algorithm>

namespace followdelay
{

class AdaptiveFitEngine
{
public:
    void prepare (double sampleRate) { sr = sampleRate; }

    MotionPlayPlan buildPlan (const TriggerEvent& trigger,
                              SourceMode sourceMode,
                              FollowModeEnum followMode,
                              float followAmount,
                              float warp,
                              float smoothness,
                              float fractalAmount,
                              float stereoSpread,
                              bool  followMyTiming,
                              float minHoldMs,
                              float learnedDurationSec,
                              bool  motionCurrentlyActive,
                              RetriggerModeEnum retriggerMode)
    {
        MotionPlayPlan plan;
        plan.valid       = true;
        plan.sourceMode  = sourceMode;
        plan.followMode  = followMode;
        plan.startSample = trigger.sampleTime;
        plan.followAmount = followAmount;
        plan.warp         = warp;
        plan.smoothness   = smoothness;
        plan.fractalAmount = fractalAmount;
        plan.stereoSpread  = stereoSpread;

        // Step 1: nominal duration
        float nominalSec = 1.0f; // default gesture duration
        if (sourceMode == SourceMode::Learn || sourceMode == SourceMode::LearnFractal)
        {
            if (learnedDurationSec > kMinGestureDuration)
                nominalSec = learnedDurationSec;
        }
        plan.nominalDurationSamples = (int64_t)(nominalSec * sr);

        // Step 2: live target duration
        float liveTargetSec = nominalSec;
        if (followMyTiming && trigger.noteOffKnown)
        {
            float noteDurSec = (float)(trigger.expectedNoteOffSample - trigger.sampleTime) / (float) sr;
            noteDurSec = std::max (noteDurSec, minHoldMs * 0.001f);
            noteDurSec = std::clamp (noteDurSec, 0.05f, 10.0f);
            liveTargetSec = noteDurSec;
        }

        // Step 3: compute effective follow weight by mode
        float modeWeight = 0.65f;
        switch (followMode)
        {
            case FollowModeEnum::Exact:    modeWeight = 0.25f; break;
            case FollowModeEnum::Balanced: modeWeight = 0.65f; break;
            case FollowModeEnum::Loose:    modeWeight = 1.00f; break;
        }
        float effectiveFollow = followAmount * modeWeight;

        float fittedSec = nominalSec + effectiveFollow * (liveTargetSec - nominalSec);
        fittedSec = std::clamp (fittedSec, 0.05f, 10.0f);

        plan.fittedDurationSamples = (int64_t)(fittedSec * sr);

        // Retrigger handling
        if (motionCurrentlyActive && retriggerMode == RetriggerModeEnum::LegatoUpdate)
        {
            plan.restartFromZero      = false;
            plan.updateExistingMotion = true;
        }
        else
        {
            plan.restartFromZero      = true;
            plan.updateExistingMotion = false;
        }

        return plan;
    }

    // Update an active plan when note-off arrives mid-motion
    void updatePlanForNoteOff (MotionPlayPlan& plan, int64_t noteOffSample,
                               bool followMyTiming, float followAmount,
                               FollowModeEnum followMode, float minHoldMs)
    {
        if (!followMyTiming || !plan.valid) return;

        float noteDurSec = (float)(noteOffSample - plan.startSample) / (float) sr;
        noteDurSec = std::max (noteDurSec, minHoldMs * 0.001f);
        noteDurSec = std::clamp (noteDurSec, 0.05f, 10.0f);

        float nominalSec = (float) plan.nominalDurationSamples / (float) sr;

        float modeWeight = 0.65f;
        switch (followMode)
        {
            case FollowModeEnum::Exact:    modeWeight = 0.25f; break;
            case FollowModeEnum::Balanced: modeWeight = 0.65f; break;
            case FollowModeEnum::Loose:    modeWeight = 1.00f; break;
        }
        float effectiveFollow = followAmount * modeWeight;
        float fittedSec = nominalSec + effectiveFollow * (noteDurSec - nominalSec);
        fittedSec = std::clamp (fittedSec, 0.05f, 10.0f);

        plan.fittedDurationSamples = (int64_t)(fittedSec * sr);
    }

private:
    double sr = 44100.0;
};

} // namespace followdelay
