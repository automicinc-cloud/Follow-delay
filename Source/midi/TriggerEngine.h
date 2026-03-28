#pragma once
#include <JuceHeader.h>
#include "../util/Constants.h"
#include "../util/DataStructures.h"
#include <array>
#include <atomic>

namespace followdelay
{

class HeldNoteTracker
{
public:
    void noteOn (int note, int vel, int64_t sample)
    {
        auto& s = notes[note];
        s.noteNumber    = note;
        s.velocity      = vel;
        s.noteOnSample  = sample;
        s.noteOffSample = 0;
        s.isHeld        = true;
        ++heldCount;
    }

    void noteOff (int note, int64_t sample)
    {
        auto& s = notes[note];
        if (s.isHeld)
        {
            s.noteOffSample = sample;
            s.isHeld = false;
            --heldCount;
        }
    }

    const HeldNoteState& getNote (int n) const { return notes[n]; }
    int  getHeldCount() const { return heldCount; }
    bool isNoteHeld (int n) const { return notes[n].isHeld; }

    void reset()
    {
        for (auto& n : notes)
        {
            n = {};
        }
        heldCount = 0;
    }

    // Get the most recently triggered held note (any note)
    int getLatestHeldNote() const
    {
        int best = -1;
        int64_t latest = -1;
        for (int i = 0; i < 128; ++i)
        {
            if (notes[i].isHeld && notes[i].noteOnSample > latest)
            {
                best = i;
                latest = notes[i].noteOnSample;
            }
        }
        return best;
    }

private:
    std::array<HeldNoteState, 128> notes {};
    int heldCount = 0;
};

class TriggerEngine
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        tracker.reset();
        triggered.store (false);
    }

    void setTriggerSource  (TriggerSourceEnum s) { source = s; }
    void setTriggerNote    (int n)               { triggerNote = n; }
    void setTriggerRange   (int lo, int hi)      { rangeLow = lo; rangeHigh = hi; }
    void setVelocityThresh (int v)               { velThreshold = v; }
    void setRetriggerMode  (RetriggerModeEnum m) { retrigger = m; }
    void setMotionActive   (bool a)              { motionActive = a; }

    // Process a single MIDI message at a given sample offset.
    // Returns true if a trigger event was generated.
    bool processMidiEvent (const juce::MidiMessage& msg, int64_t absoluteSample,
                           TriggerEvent& outEvent)
    {
        if (msg.isNoteOn())
        {
            int note = msg.getNoteNumber();
            int vel  = msg.getVelocity();
            tracker.noteOn (note, vel, absoluteSample);

            if (vel < velThreshold) return false;

            bool matches = false;
            if (source == TriggerSourceEnum::MidiNote)
            {
                if (rangeLow == rangeHigh)
                    matches = (note == triggerNote);
                else
                    matches = (note >= rangeLow && note <= rangeHigh);
            }
            else // AnyHeldNote
            {
                matches = true;
            }

            if (!matches) return false;

            // Retrigger logic
            if (motionActive)
            {
                switch (retrigger)
                {
                    case RetriggerModeEnum::Ignore:
                        suppressedTrigger.store (true);
                        return false;

                    case RetriggerModeEnum::LegatoUpdate:
                        outEvent.midiNote     = note;
                        outEvent.velocity     = vel;
                        outEvent.sampleTime   = absoluteSample;
                        outEvent.noteOffKnown = false;
                        outEvent.triggerConfidence = 1.0f;
                        triggered.store (true);
                        return true; // caller handles legato vs restart

                    case RetriggerModeEnum::Restart:
                    default:
                        break;
                }
            }

            outEvent.midiNote     = note;
            outEvent.velocity     = vel;
            outEvent.sampleTime   = absoluteSample;
            outEvent.noteOffKnown = false;
            outEvent.triggerConfidence = 1.0f;
            triggered.store (true);
            return true;
        }
        else if (msg.isNoteOff())
        {
            int note = msg.getNoteNumber();
            tracker.noteOff (note, absoluteSample);
        }

        return false;
    }

    // UI-safe reads
    bool       wasTriggered()       { return triggered.exchange (false); }
    bool       wasSuppressed()      { return suppressedTrigger.exchange (false); }
    const HeldNoteTracker& getTracker() const { return tracker; }

private:
    double sr = 44100.0;
    HeldNoteTracker tracker;

    TriggerSourceEnum  source       = TriggerSourceEnum::MidiNote;
    int                triggerNote  = 60;
    int                rangeLow     = 60;
    int                rangeHigh    = 60;
    int                velThreshold = 1;
    RetriggerModeEnum  retrigger    = RetriggerModeEnum::Restart;
    bool               motionActive = false;

    std::atomic<bool>  triggered       { false };
    std::atomic<bool>  suppressedTrigger { false };
};

} // namespace followdelay
