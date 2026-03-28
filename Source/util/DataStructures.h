#pragma once
#include <vector>
#include <cstdint>
#include "Constants.h"

namespace followdelay
{

// ── Trigger Event ──────────────────────────────────────────────
struct TriggerEvent
{
    int midiNote     = -1;
    int velocity     = 0;
    int channel      = 1;
    int64_t sampleTime = 0;
    double bpm       = 120.0;
    bool   transportValid = false;

    TriggerSourceEnum source = TriggerSourceEnum::MidiNote;

    bool    noteOffKnown = false;
    int64_t expectedNoteOffSample = 0;
    float   triggerConfidence = 1.0f;
};

// ── Held Note State ────────────────────────────────────────────
struct HeldNoteState
{
    int     noteNumber      = -1;
    int     velocity        = 0;
    int64_t noteOnSample    = 0;
    int64_t noteOffSample   = 0;
    bool    isHeld          = false;
};

// ── Learned Gesture Template ───────────────────────────────────
struct LearnedGestureTemplate
{
    bool valid = false;

    std::vector<float> pitchContour;
    std::vector<float> attackProfile;
    std::vector<float> releaseProfile;

    float originalDurationSeconds = 0.0f;
    float normalizedBendDistance  = 0.0f;
    float normalizedBendSpeed    = 0.0f;

    int   sourceNote     = -1;
    int   sourceVelocity = 0;
    float confidence     = 0.0f;
};

// ── Motion Curve ───────────────────────────────────────────────
struct MotionCurve
{
    std::vector<float> samples;  // normalised [0,1]
    bool valid = false;
};

// ── Motion Play Plan ───────────────────────────────────────────
struct MotionPlayPlan
{
    bool valid = false;

    SourceMode      sourceMode  = SourceMode::Draw;
    FollowModeEnum  followMode  = FollowModeEnum::Balanced;

    int64_t startSample           = 0;
    int64_t nominalDurationSamples = 0;
    int64_t fittedDurationSamples  = 0;

    float followAmount  = 0.5f;
    float warp          = 0.5f;
    float smoothness    = 0.5f;
    float fractalAmount = 0.0f;
    float stereoSpread  = 0.0f;

    bool restartFromZero      = true;
    bool updateExistingMotion = false;
};

// ── Runtime Motion State ───────────────────────────────────────
struct RuntimeMotionState
{
    bool    active          = false;
    int64_t startSample     = 0;
    int64_t elapsedSamples  = 0;
    int64_t durationSamples = 0;

    float currentValueL  = 0.0f;
    float currentValueR  = 0.0f;
    float playheadNorm   = 0.0f;
};

// ── Graph Point ────────────────────────────────────────────────
struct GraphPoint
{
    float x = 0.0f; // time [0,1]
    float y = 0.0f; // value [0,1]

    bool operator< (const GraphPoint& o) const { return x < o.x; }
};

} // namespace followdelay
