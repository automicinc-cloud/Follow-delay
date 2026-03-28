#pragma once
#include <JuceHeader.h>
#include "../util/Constants.h"
#include "../util/DataStructures.h"
#include <vector>
#include <atomic>

namespace followdelay
{

class MidiGestureCapture
{
public:
    enum class State { Idle, Armed, Capturing, Finalizing, Learned };

    void prepare (double sampleRate) { sr = sampleRate; reset(); }

    void arm()
    {
        if (state == State::Idle || state == State::Learned)
        {
            state = State::Armed;
        }
    }

    void disarm()
    {
        if (state == State::Armed)
            state = State::Idle;
    }

    void reset()
    {
        state = State::Idle;
        capturedBend.clear();
        captureStartSample = 0;
        captureNote = -1;
        captureVelocity = 0;
    }

    void clearGesture()
    {
        gestureTemplate = {};
        reset();
    }

    // Called from audio thread on note-on
    void onNoteOn (int note, int vel, int64_t sample)
    {
        if (state != State::Armed) return;

        state = State::Capturing;
        captureStartSample = sample;
        captureNote = note;
        captureVelocity = vel;
        capturedBend.clear();
        capturedBend.push_back ({ 0.0f, 0.5f }); // time=0, centered bend
    }

    // Called from audio thread for pitch bend
    void onPitchBend (float normBend, int64_t sample)
    {
        if (state != State::Capturing) return;

        float t = (float)(sample - captureStartSample) / (float) sr;
        if (t > kMaxCaptureDuration)
        {
            finalize (sample);
            return;
        }
        capturedBend.push_back ({ t, normBend });
    }

    // Called from audio thread on note-off
    void onNoteOff (int note, int64_t sample)
    {
        if (state != State::Capturing) return;
        if (note != captureNote) return;

        finalize (sample);
    }

    State getState() const { return state; }

    const LearnedGestureTemplate& getTemplate() const { return gestureTemplate; }
    bool hasValidTemplate() const { return gestureTemplate.valid; }

private:
    void finalize (int64_t endSample)
    {
        state = State::Finalizing;

        float durationSec = (float)(endSample - captureStartSample) / (float) sr;

        if (durationSec < kMinGestureDuration || capturedBend.size() < 2)
        {
            state = State::Armed; // invalid, stay armed for retry
            return;
        }

        // Build template
        LearnedGestureTemplate templ;
        templ.valid = true;
        templ.sourceNote = captureNote;
        templ.sourceVelocity = captureVelocity;
        templ.originalDurationSeconds = durationSec;

        // Resample bend to normalized curve of kMotionCurveSize samples
        templ.pitchContour.resize (kMotionCurveSize);
        for (int i = 0; i < kMotionCurveSize; ++i)
        {
            float t = ((float) i / (float)(kMotionCurveSize - 1)) * durationSec;
            templ.pitchContour[i] = sampleBendAt (t);
        }

        // Compute metadata
        float minVal = 1.0f, maxVal = 0.0f;
        for (auto v : templ.pitchContour)
        {
            minVal = std::min (minVal, v);
            maxVal = std::max (maxVal, v);
        }
        templ.normalizedBendDistance = maxVal - minVal;

        if (durationSec > 0.0f)
            templ.normalizedBendSpeed = templ.normalizedBendDistance / durationSec;

        templ.confidence = std::min (1.0f, templ.normalizedBendDistance * 2.0f
                                            + (durationSec > 0.1f ? 0.3f : 0.0f));

        // Simple attack/release profiles
        int half = kMotionCurveSize / 2;
        templ.attackProfile.assign  (templ.pitchContour.begin(), templ.pitchContour.begin() + half);
        templ.releaseProfile.assign (templ.pitchContour.begin() + half, templ.pitchContour.end());

        gestureTemplate = std::move (templ);
        state = State::Learned;
    }

    float sampleBendAt (float timeSec) const
    {
        if (capturedBend.empty()) return 0.5f;
        if (timeSec <= capturedBend.front().first) return capturedBend.front().second;
        if (timeSec >= capturedBend.back().first)  return capturedBend.back().second;

        for (size_t i = 1; i < capturedBend.size(); ++i)
        {
            if (timeSec <= capturedBend[i].first)
            {
                float t0 = capturedBend[i - 1].first;
                float t1 = capturedBend[i].first;
                float v0 = capturedBend[i - 1].second;
                float v1 = capturedBend[i].second;
                float frac = (timeSec - t0) / (t1 - t0 + 1e-12f);
                return v0 + frac * (v1 - v0);
            }
        }
        return capturedBend.back().second;
    }

    double sr = 44100.0;
    State state = State::Idle;
    LearnedGestureTemplate gestureTemplate;

    std::vector<std::pair<float, float>> capturedBend; // {timeSec, normBend}
    int64_t captureStartSample = 0;
    int captureNote = -1;
    int captureVelocity = 0;
};

} // namespace followdelay
