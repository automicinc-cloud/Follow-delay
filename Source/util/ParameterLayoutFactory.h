#pragma once
#include <JuceHeader.h>
#include "Constants.h"

namespace followdelay
{

class ParameterLayoutFactory
{
public:
    static juce::AudioProcessorValueTreeState::ParameterLayout create()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        // ── Main ───────────────────────────────────────────
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamID::StartMs, 1 }, "Start",
            juce::NormalisableRange<float> (kMinDelayMs, kMaxDelayMs, 0.1f, 0.35f),
            420.0f, juce::AudioParameterFloatAttributes().withLabel ("ms")));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamID::EndMs, 1 }, "End",
            juce::NormalisableRange<float> (kMinDelayMs, kMaxDelayMs, 0.1f, 0.35f),
            80.0f, juce::AudioParameterFloatAttributes().withLabel ("ms")));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamID::Warp, 1 }, "Warp",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamID::Feedback, 1 }, "Feedback",
            juce::NormalisableRange<float> (0.0f, kMaxFeedbackGain), 0.35f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamID::Tone, 1 }, "Tone",
            juce::NormalisableRange<float> (20.0f, 18000.0f, 1.0f, 0.3f),
            6500.0f, juce::AudioParameterFloatAttributes().withLabel ("Hz")));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamID::Mix, 1 }, "Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.32f));

        // ── Motion ─────────────────────────────────────────
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { ParamID::SourceMode, 1 }, "Source Mode",
            juce::StringArray { "Draw", "Learn", "Draw+Fractal", "Learn+Fractal", "Fractal Only" }, 0));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamID::Smoothness, 1 }, "Smoothness",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.55f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamID::StereoSpread, 1 }, "Stereo",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.25f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamID::FollowAmount, 1 }, "Follow Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.65f));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { ParamID::FollowMode, 1 }, "Follow Mode",
            juce::StringArray { "Exact", "Balanced", "Loose" }, 1));

        // ── Fractal ────────────────────────────────────────
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamID::FractalAmount, 1 }, "Fractal",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.10f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamID::FractalRoughness, 1 }, "Fractal Roughness",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.55f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamID::FractalScale, 1 }, "Fractal Scale",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.50f));

        layout.add (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { ParamID::FractalSeed, 1 }, "Seed",
            1, 2147483647, 12345));

        // ── Trigger ────────────────────────────────────────
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { ParamID::TriggerSource, 1 }, "Trigger Source",
            juce::StringArray { "MIDI Note", "Any Held Note" }, 0));

        layout.add (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { ParamID::TriggerNote, 1 }, "Trigger Note",
            0, 127, 60));

        layout.add (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { ParamID::TriggerRangeLow, 1 }, "Trigger Range Low",
            0, 127, 60));

        layout.add (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { ParamID::TriggerRangeHigh, 1 }, "Trigger Range High",
            0, 127, 60));

        layout.add (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { ParamID::VelocityThreshold, 1 }, "Velocity Threshold",
            1, 127, 1));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { ParamID::RetriggerMode, 1 }, "Retrigger",
            juce::StringArray { "Restart", "Ignore", "Legato Update" }, 0));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ParamID::FollowMyTiming, 1 }, "Follow My Timing", true));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamID::MinHoldMs, 1 }, "Min Hold Threshold",
            juce::NormalisableRange<float> (10.0f, 2000.0f, 1.0f, 0.4f),
            80.0f, juce::AudioParameterFloatAttributes().withLabel ("ms")));

        // ── Learn ──────────────────────────────────────────
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ParamID::LearnArm, 1 }, "Learn", false));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ParamID::ClearGesture, 1 }, "Clear Gesture", false));

        // ── Safety ─────────────────────────────────────────
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ParamID::SafeSmoothing, 1 }, "Safe Mode", true));

        return layout;
    }
};

} // namespace followdelay
