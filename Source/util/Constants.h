#pragma once
#include <JuceHeader.h>

namespace followdelay
{

// ── Parameter IDs ──────────────────────────────────────────────
namespace ParamID
{
    // Main
    inline constexpr const char* StartMs        = "start_ms";
    inline constexpr const char* EndMs          = "end_ms";
    inline constexpr const char* Warp           = "warp";
    inline constexpr const char* Feedback       = "feedback";
    inline constexpr const char* Tone           = "tone";
    inline constexpr const char* Mix            = "mix";

    // Motion
    inline constexpr const char* SourceMode     = "source_mode";
    inline constexpr const char* Smoothness     = "smoothness";
    inline constexpr const char* StereoSpread   = "stereo_spread";
    inline constexpr const char* FollowAmount   = "follow_amount";
    inline constexpr const char* FollowMode     = "follow_mode";

    // Fractal
    inline constexpr const char* FractalAmount    = "fractal_amount";
    inline constexpr const char* FractalRoughness = "fractal_roughness";
    inline constexpr const char* FractalScale     = "fractal_scale";
    inline constexpr const char* FractalSeed      = "fractal_seed";

    // Trigger
    inline constexpr const char* TriggerSource    = "trigger_source";
    inline constexpr const char* TriggerNote      = "trigger_note";
    inline constexpr const char* TriggerRangeLow  = "trigger_range_low";
    inline constexpr const char* TriggerRangeHigh = "trigger_range_high";
    inline constexpr const char* VelocityThreshold= "velocity_threshold";
    inline constexpr const char* RetriggerMode    = "retrigger_mode";
    inline constexpr const char* FollowMyTiming   = "follow_my_timing";
    inline constexpr const char* MinHoldMs        = "min_hold_ms";

    // Learn
    inline constexpr const char* LearnArm       = "learn_arm";
    inline constexpr const char* ClearGesture   = "clear_gesture";

    // Safety
    inline constexpr const char* SafeSmoothing  = "safe_smoothing";
}

// ── Enums ──────────────────────────────────────────────────────
enum class SourceMode
{
    Draw = 0,
    Learn,
    DrawFractal,
    LearnFractal,
    FractalOnly
};

enum class FollowModeEnum
{
    Exact = 0,
    Balanced,
    Loose
};

enum class TriggerSourceEnum
{
    MidiNote = 0,
    AnyHeldNote
};

enum class RetriggerModeEnum
{
    Restart = 0,
    Ignore,
    LegatoUpdate
};

// ── Constants ──────────────────────────────────────────────────
inline constexpr float  kMaxDelayMs         = 2000.0f;
inline constexpr float  kMinDelayMs         = 1.0f;
inline constexpr int    kMotionCurveSize    = 1024;
inline constexpr float  kMaxFeedbackGain    = 0.95f;
inline constexpr float  kFeedbackCeiling    = 0.98f; // internal hard limit
inline constexpr float  kDefaultBPM         = 120.0f;
inline constexpr float  kMaxCaptureDuration = 8.0f;  // seconds
inline constexpr float  kMinGestureDuration = 0.04f;  // seconds
inline constexpr int    kDefaultWindowW     = 1280;
inline constexpr int    kDefaultWindowH     = 820;
inline constexpr int    kStateVersion       = 1;

} // namespace followdelay
