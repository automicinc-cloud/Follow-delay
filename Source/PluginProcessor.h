#pragma once
#include <JuceHeader.h>
#include "util/Constants.h"
#include "util/DataStructures.h"
#include "util/ParameterLayoutFactory.h"
#include "util/SafeSmoother.h"
#include "dsp/DelayEngine.h"
#include "dsp/AdaptiveFitEngine.h"
#include "dsp/MotionPlaybackEngine.h"
#include "midi/TriggerEngine.h"
#include "midi/MidiGestureCapture.h"
#include "model/MotionGraphModel.h"
#include "model/FractalMotionGenerator.h"
#include "model/StateSerializer.h"
#include <atomic>

namespace followdelay
{

class FollowDelayAudioProcessor : public juce::AudioProcessor
{
public:
    FollowDelayAudioProcessor()
        : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          apvts (*this, nullptr, "FollowDelayState",
                 ParameterLayoutFactory::create())
    {
        // Cache parameter pointers
        pStartMs        = apvts.getRawParameterValue (ParamID::StartMs);
        pEndMs          = apvts.getRawParameterValue (ParamID::EndMs);
        pWarp           = apvts.getRawParameterValue (ParamID::Warp);
        pFeedback       = apvts.getRawParameterValue (ParamID::Feedback);
        pTone           = apvts.getRawParameterValue (ParamID::Tone);
        pMix            = apvts.getRawParameterValue (ParamID::Mix);
        pSourceMode     = apvts.getRawParameterValue (ParamID::SourceMode);
        pSmoothness     = apvts.getRawParameterValue (ParamID::Smoothness);
        pStereoSpread   = apvts.getRawParameterValue (ParamID::StereoSpread);
        pFollowAmount   = apvts.getRawParameterValue (ParamID::FollowAmount);
        pFollowMode     = apvts.getRawParameterValue (ParamID::FollowMode);
        pFractalAmount  = apvts.getRawParameterValue (ParamID::FractalAmount);
        pFractalRoughness = apvts.getRawParameterValue (ParamID::FractalRoughness);
        pFractalScale   = apvts.getRawParameterValue (ParamID::FractalScale);
        pFractalSeed    = apvts.getRawParameterValue (ParamID::FractalSeed);
        pTriggerSource  = apvts.getRawParameterValue (ParamID::TriggerSource);
        pTriggerNote    = apvts.getRawParameterValue (ParamID::TriggerNote);
        pTriggerRangeLow  = apvts.getRawParameterValue (ParamID::TriggerRangeLow);
        pTriggerRangeHigh = apvts.getRawParameterValue (ParamID::TriggerRangeHigh);
        pVelocityThreshold = apvts.getRawParameterValue (ParamID::VelocityThreshold);
        pRetriggerMode  = apvts.getRawParameterValue (ParamID::RetriggerMode);
        pFollowMyTiming = apvts.getRawParameterValue (ParamID::FollowMyTiming);
        pMinHoldMs      = apvts.getRawParameterValue (ParamID::MinHoldMs);
        pLearnArm       = apvts.getRawParameterValue (ParamID::LearnArm);
        pClearGesture   = apvts.getRawParameterValue (ParamID::ClearGesture);
        pSafeSmoothing  = apvts.getRawParameterValue (ParamID::SafeSmoothing);
    }

    ~FollowDelayAudioProcessor() override = default;

    // ── AudioProcessor interface ──────────────────────────────
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        sr = sampleRate;
        delayEngine.prepare (sampleRate, samplesPerBlock);
        triggerEngine.prepare (sampleRate);
        gestureCapture.prepare (sampleRate);
        adaptiveFit.prepare (sampleRate);
        motionPlayback.prepare (sampleRate);

        // Initial fractal generation
        regenerateFractal();

        // Push initial draw curve
        motionPlayback.setDrawCurve (graphModel.getCurveSnapshot());
    }

    void releaseResources() override
    {
        delayEngine.reset();
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override
    {
        juce::ScopedNoDenormals noDenormals;

        auto totalNumInputChannels  = getTotalNumInputChannels();
        auto totalNumOutputChannels = getTotalNumOutputChannels();

        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
            buffer.clear (i, 0, buffer.getNumSamples());

        int numSamples = buffer.getNumSamples();

        // ── Read parameters ───────────────────────────────
        float startMs       = pStartMs->load();
        float endMs         = pEndMs->load();
        float warp          = pWarp->load();
        float feedback      = pFeedback->load();
        float tone          = pTone->load();
        float mix           = pMix->load();
        auto  sourceMode    = static_cast<SourceMode> ((int) pSourceMode->load());
        float smoothness    = pSmoothness->load();
        float stereoSpread  = pStereoSpread->load();
        float followAmount  = pFollowAmount->load();
        auto  followMode    = static_cast<FollowModeEnum> ((int) pFollowMode->load());
        float fracAmount    = pFractalAmount->load();
        float fracRoughness = pFractalRoughness->load();
        float fracScale     = pFractalScale->load();
        int   fracSeed      = (int) pFractalSeed->load();
        auto  trigSrc       = static_cast<TriggerSourceEnum> ((int) pTriggerSource->load());
        int   trigNote      = (int) pTriggerNote->load();
        int   trigRangeLo   = (int) pTriggerRangeLow->load();
        int   trigRangeHi   = (int) pTriggerRangeHigh->load();
        int   velThresh     = (int) pVelocityThreshold->load();
        auto  retrigger     = static_cast<RetriggerModeEnum> ((int) pRetriggerMode->load());
        bool  followTiming  = pFollowMyTiming->load() > 0.5f;
        float minHoldMs     = pMinHoldMs->load();
        bool  learnArm      = pLearnArm->load() > 0.5f;
        bool  clearGesture  = pClearGesture->load() > 0.5f;
        bool  safeMode      = pSafeSmoothing->load() > 0.5f;

        // ── Update engines with current params ────────────
        triggerEngine.setTriggerSource (trigSrc);
        triggerEngine.setTriggerNote (trigNote);
        triggerEngine.setTriggerRange (trigRangeLo, trigRangeHi);
        triggerEngine.setVelocityThresh (velThresh);
        triggerEngine.setRetriggerMode (retrigger);
        triggerEngine.setMotionActive (motionPlayback.isActive());

        delayEngine.setFeedback (feedback);
        delayEngine.setTone (tone);
        delayEngine.setMix (mix);
        delayEngine.setSafeMode (safeMode);

        // Learn mode control
        if (learnArm)
            gestureCapture.arm();
        else
            gestureCapture.disarm();

        if (clearGesture)
        {
            gestureCapture.clearGesture();
            // Reset the momentary param
            if (auto* p = apvts.getParameter (ParamID::ClearGesture))
                p->setValueNotifyingHost (0.0f);
        }

        // Fractal regeneration if params changed
        if (fracSeed != lastFracSeed || std::abs (fracRoughness - lastFracRoughness) > 0.001f
            || std::abs (fracScale - lastFracScale) > 0.001f)
        {
            regenerateFractalWith (fracSeed, fracRoughness, fracScale);
        }

        // Update curve snapshots
        graphModel.setSmoothness (smoothness);
        motionPlayback.setDrawCurve (graphModel.getCurveSnapshot());
        motionPlayback.setFractalCurve (fractalGen.getCurve());

        if (gestureCapture.hasValidTemplate())
        {
            // Build a MotionCurve from the learned template
            MotionCurve learnCurve;
            learnCurve.valid = true;
            learnCurve.samples = gestureCapture.getTemplate().pitchContour;
            motionPlayback.setLearnedCurve (learnCurve);
        }

        // ── Process MIDI events ───────────────────────────
        auto posInfo = getPlayHead() != nullptr ? getPlayHead()->getPosition() : juce::Optional<juce::AudioPlayHead::PositionInfo>();

        for (const auto metadata : midiMessages)
        {
            auto msg = metadata.getMessage();
            int64_t absSample = absoluteSamplePos + metadata.samplePosition;

            // Feed gesture capture
            if (msg.isNoteOn())
                gestureCapture.onNoteOn (msg.getNoteNumber(), msg.getVelocity(), absSample);
            else if (msg.isNoteOff())
                gestureCapture.onNoteOff (msg.getNoteNumber(), absSample);
            else if (msg.isPitchWheel())
            {
                float normBend = (float) msg.getPitchWheelValue() / 16383.0f;
                gestureCapture.onPitchBend (normBend, absSample);
            }

            // Feed trigger engine
            TriggerEvent trigEvent;
            if (triggerEngine.processMidiEvent (msg, absSample, trigEvent))
            {
                float learnedDur = gestureCapture.hasValidTemplate()
                                 ? gestureCapture.getTemplate().originalDurationSeconds
                                 : 1.0f;

                MotionPlayPlan plan = adaptiveFit.buildPlan (
                    trigEvent, sourceMode, followMode, followAmount,
                    warp, smoothness, fracAmount, stereoSpread,
                    followTiming, minHoldMs, learnedDur,
                    motionPlayback.isActive(), retrigger);

                if (plan.valid)
                {
                    if (plan.updateExistingMotion)
                        motionPlayback.updatePlan (plan);
                    else
                        motionPlayback.startMotion (plan);
                }

                lastTriggerNote.store (trigEvent.midiNote);
            }

            // Handle note-off for adaptive timing update
            if (msg.isNoteOff() && motionPlayback.isActive() && followTiming)
            {
                // Could update the active plan's duration here
            }
        }

        // ── Process audio sample by sample with motion ────
        float* leftData  = buffer.getWritePointer (0);
        float* rightData = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : leftData;

        // Generate per-sample modulated delay times
        std::vector<float> delayTimesL (numSamples);
        std::vector<float> delayTimesR (numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            float dL, dR;
            motionPlayback.processSample (startMs, endMs, dL, dR);
            delayTimesL[i] = dL;
            delayTimesR[i] = dR;
        }

        // Apply delay per-sample (set delay time then process one sample at a time)
        for (int i = 0; i < numSamples; ++i)
        {
            delayEngine.setDelayTimeMs (delayTimesL[i], delayTimesR[i]);

            float frameL[1] = { leftData[i] };
            float frameR[1] = { rightData[i] };
            delayEngine.processBlock (frameL, frameR, 1);
            leftData[i]  = frameL[0];
            rightData[i] = frameR[0];
        }

        absoluteSamplePos += numSamples;

        // Publish UI state
        uiPlayheadNorm.store (motionPlayback.getRuntimeState().playheadNorm);
        uiMotionActive.store (motionPlayback.isActive());
        uiLearnState.store (static_cast<int> (gestureCapture.getState()));
    }

    // ── State persistence ─────────────────────────────────
    void getStateInformation (juce::MemoryBlock& destData) override
    {
        StateSerializer::saveState (apvts, graphModel, gestureCapture,
                                    advancedMode.load(), destData);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        bool adv = false;
        StateSerializer::loadState (apvts, graphModel, gestureCapture, adv, data, sizeInBytes);
        advancedMode.store (adv);
        regenerateFractal();
    }

    // ── Editor ────────────────────────────────────────────
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // ── Misc ──────────────────────────────────────────────
    const juce::String getName() const override { return "Follow Delay"; }
    bool acceptsMidi()  const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;
        if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::mono()
            && layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
            return false;
        return true;
    }

    // ── Public accessors for editor ───────────────────────
    juce::AudioProcessorValueTreeState apvts;
    MotionGraphModel graphModel;

    std::atomic<float> uiPlayheadNorm { 0.0f };
    std::atomic<bool>  uiMotionActive { false };
    std::atomic<int>   uiLearnState   { 0 };
    std::atomic<int>   lastTriggerNote { -1 };
    std::atomic<bool>  advancedMode   { false };

private:
    void regenerateFractal()
    {
        int   seed  = (int) pFractalSeed->load();
        float rough = pFractalRoughness->load();
        float scale = pFractalScale->load();
        regenerateFractalWith (seed, rough, scale);
    }

    void regenerateFractalWith (int seed, float roughness, float scale)
    {
        fractalGen.generate (seed, roughness, scale);
        lastFracSeed = seed;
        lastFracRoughness = roughness;
        lastFracScale = scale;
    }

    double sr = 44100.0;
    int64_t absoluteSamplePos = 0;

    // DSP
    DelayEngine          delayEngine;
    AdaptiveFitEngine    adaptiveFit;
    MotionPlaybackEngine motionPlayback;

    // MIDI
    TriggerEngine       triggerEngine;
    MidiGestureCapture  gestureCapture;

    // Model
    FractalMotionGenerator fractalGen;

    // Fractal change detection
    int   lastFracSeed      = 0;
    float lastFracRoughness = -1.0f;
    float lastFracScale     = -1.0f;

    // Parameter pointers
    std::atomic<float>* pStartMs = nullptr;
    std::atomic<float>* pEndMs = nullptr;
    std::atomic<float>* pWarp = nullptr;
    std::atomic<float>* pFeedback = nullptr;
    std::atomic<float>* pTone = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pSourceMode = nullptr;
    std::atomic<float>* pSmoothness = nullptr;
    std::atomic<float>* pStereoSpread = nullptr;
    std::atomic<float>* pFollowAmount = nullptr;
    std::atomic<float>* pFollowMode = nullptr;
    std::atomic<float>* pFractalAmount = nullptr;
    std::atomic<float>* pFractalRoughness = nullptr;
    std::atomic<float>* pFractalScale = nullptr;
    std::atomic<float>* pFractalSeed = nullptr;
    std::atomic<float>* pTriggerSource = nullptr;
    std::atomic<float>* pTriggerNote = nullptr;
    std::atomic<float>* pTriggerRangeLow = nullptr;
    std::atomic<float>* pTriggerRangeHigh = nullptr;
    std::atomic<float>* pVelocityThreshold = nullptr;
    std::atomic<float>* pRetriggerMode = nullptr;
    std::atomic<float>* pFollowMyTiming = nullptr;
    std::atomic<float>* pMinHoldMs = nullptr;
    std::atomic<float>* pLearnArm = nullptr;
    std::atomic<float>* pClearGesture = nullptr;
    std::atomic<float>* pSafeSmoothing = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FollowDelayAudioProcessor)
};

} // namespace followdelay
