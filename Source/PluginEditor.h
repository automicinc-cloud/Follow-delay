#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/MotionGraphComponent.h"
#include "ui/FollowDelayLookAndFeel.h"

namespace followdelay
{

class FollowDelayAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    FollowDelayAudioProcessorEditor (FollowDelayAudioProcessor& p)
        : AudioProcessorEditor (&p), processor (p),
          graphComponent (p.graphModel, p.uiPlayheadNorm, p.uiMotionActive)
    {
        setLookAndFeel (&lnf);
        setSize (kDefaultWindowW, kDefaultWindowH);

        // ── Knobs ─────────────────────────────────────────
        auto makeKnob = [&] (juce::Slider& s, const juce::String& name,
                             const juce::String& suffix = "")
        {
            s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            s.setTextValueSuffix (suffix);
            s.setName (name);
            addAndMakeVisible (s);
        };

        makeKnob (startKnob, "Start", " ms");
        makeKnob (endKnob,   "End",   " ms");
        makeKnob (warpKnob,  "Warp");
        makeKnob (feedbackKnob, "Feedback");
        makeKnob (toneKnob,  "Tone",  " Hz");
        makeKnob (mixKnob,   "Mix");
        makeKnob (fractalKnob,  "Fractal");
        makeKnob (smoothnessKnob, "Smoothness");
        makeKnob (stereoKnob, "Stereo");
        makeKnob (followAmountKnob, "Follow");

        // ── Labels ────────────────────────────────────────
        auto makeLabel = [&] (juce::Label& lbl, const juce::String& text)
        {
            lbl.setText (text, juce::dontSendNotification);
            lbl.setJustificationType (juce::Justification::centred);
            lbl.setFont (juce::Font (12.0f));
            addAndMakeVisible (lbl);
        };

        makeLabel (startLabel, "START");
        makeLabel (endLabel, "END");
        makeLabel (warpLabel, "WARP");
        makeLabel (feedbackLabel, "FEEDBACK");
        makeLabel (toneLabel, "TONE");
        makeLabel (mixLabel, "MIX");
        makeLabel (fractalLabel, "FRACTAL");
        makeLabel (smoothnessLabel, "SMOOTH");
        makeLabel (stereoLabel, "STEREO");
        makeLabel (followAmountLabel, "FOLLOW");

        // ── Combo boxes ───────────────────────────────────
        auto makeCombo = [&] (juce::ComboBox& cb, const juce::String& name)
        {
            cb.setName (name);
            addAndMakeVisible (cb);
        };

        sourceModeCombo.addItemList ({ "Draw", "Learn", "Draw+Fractal", "Learn+Fractal", "Fractal Only" }, 1);
        makeCombo (sourceModeCombo, "Source");

        followModeCombo.addItemList ({ "Exact", "Balanced", "Loose" }, 1);
        makeCombo (followModeCombo, "Follow Mode");

        triggerSourceCombo.addItemList ({ "MIDI Note", "Any Held Note" }, 1);
        makeCombo (triggerSourceCombo, "Trigger Source");

        retriggerCombo.addItemList ({ "Restart", "Ignore", "Legato Update" }, 1);
        makeCombo (retriggerCombo, "Retrigger");

        // ── Buttons ───────────────────────────────────────
        learnBtn.setButtonText ("LEARN");
        learnBtn.setClickingTogglesState (true);
        addAndMakeVisible (learnBtn);

        followTimingBtn.setButtonText ("FOLLOW TIMING");
        followTimingBtn.setClickingTogglesState (true);
        addAndMakeVisible (followTimingBtn);

        clearBtn.setButtonText ("CLEAR");
        addAndMakeVisible (clearBtn);

        resetBtn.setButtonText ("RESET");
        resetBtn.onClick = [&] { processor.graphModel.resetToDefault(); };
        addAndMakeVisible (resetBtn);

        advancedToggle.setButtonText ("ADVANCED");
        advancedToggle.setClickingTogglesState (true);
        advancedToggle.onClick = [&]
        {
            processor.advancedMode.store (advancedToggle.getToggleState());
            resized();
        };
        addAndMakeVisible (advancedToggle);

        // ── Trigger note knob ─────────────────────────────
        triggerNoteKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        triggerNoteKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 18);
        triggerNoteKnob.setName ("Trigger Note");
        addAndMakeVisible (triggerNoteKnob);

        makeLabel (triggerNoteLabel, "TRIGGER NOTE");

        // ── Graph ─────────────────────────────────────────
        addAndMakeVisible (graphComponent);

        // ── Status labels ─────────────────────────────────
        statusLabel.setFont (juce::Font (13.0f));
        statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xFF88DDAA));
        addAndMakeVisible (statusLabel);

        titleLabel.setText ("FOLLOW DELAY", juce::dontSendNotification);
        titleLabel.setFont (juce::Font (20.0f, juce::Font::bold));
        titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xFFCCCCEE));
        addAndMakeVisible (titleLabel);

        // ── Trigger indicator ─────────────────────────────
        triggerIndicator.setFont (juce::Font (14.0f, juce::Font::bold));
        triggerIndicator.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (triggerIndicator);

        // ── APVTS Attachments ─────────────────────────────
        startAtt      = std::make_unique<SliderAtt> (processor.apvts, ParamID::StartMs, startKnob);
        endAtt        = std::make_unique<SliderAtt> (processor.apvts, ParamID::EndMs, endKnob);
        warpAtt       = std::make_unique<SliderAtt> (processor.apvts, ParamID::Warp, warpKnob);
        feedbackAtt   = std::make_unique<SliderAtt> (processor.apvts, ParamID::Feedback, feedbackKnob);
        toneAtt       = std::make_unique<SliderAtt> (processor.apvts, ParamID::Tone, toneKnob);
        mixAtt        = std::make_unique<SliderAtt> (processor.apvts, ParamID::Mix, mixKnob);
        fractalAtt    = std::make_unique<SliderAtt> (processor.apvts, ParamID::FractalAmount, fractalKnob);
        smoothnessAtt = std::make_unique<SliderAtt> (processor.apvts, ParamID::Smoothness, smoothnessKnob);
        stereoAtt     = std::make_unique<SliderAtt> (processor.apvts, ParamID::StereoSpread, stereoKnob);
        followAmtAtt  = std::make_unique<SliderAtt> (processor.apvts, ParamID::FollowAmount, followAmountKnob);
        trigNoteAtt   = std::make_unique<SliderAtt> (processor.apvts, ParamID::TriggerNote, triggerNoteKnob);

        sourceModeAtt    = std::make_unique<ComboAtt> (processor.apvts, ParamID::SourceMode, sourceModeCombo);
        followModeAtt    = std::make_unique<ComboAtt> (processor.apvts, ParamID::FollowMode, followModeCombo);
        trigSourceAtt    = std::make_unique<ComboAtt> (processor.apvts, ParamID::TriggerSource, triggerSourceCombo);
        retriggerAtt     = std::make_unique<ComboAtt> (processor.apvts, ParamID::RetriggerMode, retriggerCombo);

        learnAtt         = std::make_unique<BtnAtt> (processor.apvts, ParamID::LearnArm, learnBtn);
        followTimingAtt  = std::make_unique<BtnAtt> (processor.apvts, ParamID::FollowMyTiming, followTimingBtn);
        clearAtt         = std::make_unique<BtnAtt> (processor.apvts, ParamID::ClearGesture, clearBtn);

        startTimerHz (15);
    }

    ~FollowDelayAudioProcessorEditor() override
    {
        setLookAndFeel (nullptr);
    }

    void paint (juce::Graphics& g) override
    {
        // Dark gradient background
        g.fillAll (juce::Colour (0xFF0F0F1A));

        auto bounds = getLocalBounds();

        // Top bar
        g.setColour (juce::Colour (0xFF161628));
        g.fillRect (bounds.removeFromTop (48));

        // Bottom status strip
        g.setColour (juce::Colour (0xFF111122));
        g.fillRect (getLocalBounds().removeFromBottom (36));

        // Panel dividers
        g.setColour (juce::Colour (0xFF2A2A44));
        int leftW = 220;
        g.drawVerticalLine (leftW, 48.0f, (float)(getHeight() - 36));
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bool advanced = processor.advancedMode.load();

        // Top bar
        auto topBar = bounds.removeFromTop (48);
        titleLabel.setBounds (topBar.removeFromLeft (200).reduced (12, 8));
        advancedToggle.setBounds (topBar.removeFromRight (120).reduced (8, 10));

        // Bottom strip
        auto bottomStrip = bounds.removeFromBottom (36);
        statusLabel.setBounds (bottomStrip.reduced (12, 4));
        triggerIndicator.setBounds (bottomStrip.removeFromRight (140).reduced (4));

        // Left panel (trigger controls)
        auto leftPanel = bounds.removeFromLeft (220);
        leftPanel.reduce (12, 12);

        auto makeRow = [&] (int h) { return leftPanel.removeFromTop (h); };

        triggerSourceCombo.setBounds (makeRow (30).reduced (2));
        leftPanel.removeFromTop (6);

        auto noteRow = makeRow (70);
        triggerNoteKnob.setBounds (noteRow.removeFromLeft (70));
        triggerNoteLabel.setBounds (noteRow.removeFromLeft (100).withHeight (20).translated (0, 25));
        leftPanel.removeFromTop (4);

        followTimingBtn.setBounds (makeRow (28).reduced (2));
        leftPanel.removeFromTop (4);

        retriggerCombo.setBounds (makeRow (28).reduced (2));
        leftPanel.removeFromTop (8);

        learnBtn.setBounds (makeRow (32).reduced (2));
        leftPanel.removeFromTop (4);

        if (advanced)
        {
            followModeCombo.setBounds (makeRow (28).reduced (2));
            followModeCombo.setVisible (true);
        }
        else
        {
            followModeCombo.setVisible (false);
        }

        // Right panel (knobs)
        auto rightPanel = bounds.removeFromRight (260);
        rightPanel.reduce (8, 12);

        int knobSize = 64;
        int labelH = 18;
        int rowH = knobSize + labelH + 4;

        auto placeKnob = [&] (juce::Slider& knob, juce::Label& label, juce::Rectangle<int>& area)
        {
            auto row = area.removeFromTop (rowH);
            auto knobArea = row.removeFromTop (knobSize).withSizeKeepingCentre (knobSize, knobSize);
            knob.setBounds (knobArea);
            label.setBounds (row.removeFromTop (labelH));
        };

        // 2 columns of knobs
        auto leftCol  = rightPanel.removeFromLeft (rightPanel.getWidth() / 2);
        auto rightCol = rightPanel;

        placeKnob (startKnob,    startLabel,    leftCol);
        placeKnob (endKnob,      endLabel,      leftCol);
        placeKnob (warpKnob,     warpLabel,     leftCol);
        placeKnob (feedbackKnob, feedbackLabel, leftCol);
        placeKnob (fractalKnob,  fractalLabel,  leftCol);

        placeKnob (mixKnob,           mixLabel,           rightCol);
        placeKnob (toneKnob,          toneLabel,          rightCol);
        placeKnob (smoothnessKnob,    smoothnessLabel,    rightCol);
        placeKnob (stereoKnob,        stereoLabel,        rightCol);
        placeKnob (followAmountKnob,  followAmountLabel,  rightCol);

        // Centre panel (graph + mode selector + buttons)
        bounds.reduce (8, 8);

        auto graphControls = bounds.removeFromTop (30);
        sourceModeCombo.setBounds (graphControls.removeFromLeft (180).reduced (2));
        clearBtn.setBounds (graphControls.removeFromRight (70).reduced (2));
        resetBtn.setBounds (graphControls.removeFromRight (70).reduced (2));

        graphComponent.setBounds (bounds.reduced (0, 4));
    }

    void timerCallback() override
    {
        // Update status text
        bool motionActive = processor.uiMotionActive.load();
        int learnState = processor.uiLearnState.load();
        int trigNote = processor.lastTriggerNote.load();

        juce::String status;
        if (learnState == 2) // Capturing
            status = "Capturing gesture...";
        else if (learnState == 1) // Armed
            status = "Learn armed — play a note";
        else if (motionActive)
            status = "Motion active";
        else
            status = "Ready";

        if (trigNote >= 0)
            status += "  |  Last trigger: " + juce::MidiMessage::getMidiNoteName (trigNote, true, true, 4);

        statusLabel.setText (status, juce::dontSendNotification);

        // Trigger flash
        if (processor.uiMotionActive.load())
        {
            triggerIndicator.setText ("TRIGGERED", juce::dontSendNotification);
            triggerIndicator.setColour (juce::Label::textColourId, juce::Colour (0xFFFF6B6B));
        }
        else
        {
            triggerIndicator.setText ("IDLE", juce::dontSendNotification);
            triggerIndicator.setColour (juce::Label::textColourId, juce::Colour (0xFF666688));
        }
    }

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BtnAtt    = juce::AudioProcessorValueTreeState::ButtonAttachment;

    FollowDelayAudioProcessor& processor;
    FollowDelayLookAndFeel lnf;

    // ── Components ────────────────────────────────────────
    MotionGraphComponent graphComponent;

    juce::Slider startKnob, endKnob, warpKnob, feedbackKnob, toneKnob, mixKnob;
    juce::Slider fractalKnob, smoothnessKnob, stereoKnob, followAmountKnob;
    juce::Slider triggerNoteKnob;

    juce::Label startLabel, endLabel, warpLabel, feedbackLabel, toneLabel, mixLabel;
    juce::Label fractalLabel, smoothnessLabel, stereoLabel, followAmountLabel;
    juce::Label triggerNoteLabel;
    juce::Label statusLabel, titleLabel, triggerIndicator;

    juce::ComboBox sourceModeCombo, followModeCombo, triggerSourceCombo, retriggerCombo;

    juce::TextButton learnBtn, followTimingBtn, clearBtn, resetBtn, advancedToggle;

    // ── Attachments ───────────────────────────────────────
    std::unique_ptr<SliderAtt> startAtt, endAtt, warpAtt, feedbackAtt, toneAtt, mixAtt;
    std::unique_ptr<SliderAtt> fractalAtt, smoothnessAtt, stereoAtt, followAmtAtt, trigNoteAtt;
    std::unique_ptr<ComboAtt>  sourceModeAtt, followModeAtt, trigSourceAtt, retriggerAtt;
    std::unique_ptr<BtnAtt>    learnAtt, followTimingAtt, clearAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FollowDelayAudioProcessorEditor)
};

} // namespace followdelay
