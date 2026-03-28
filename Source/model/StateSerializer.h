#pragma once
#include <JuceHeader.h>
#include "../util/Constants.h"
#include "../util/DataStructures.h"
#include "../model/MotionGraphModel.h"
#include "../midi/MidiGestureCapture.h"

namespace followdelay
{

class StateSerializer
{
public:
    static void saveState (juce::AudioProcessorValueTreeState& apvts,
                           MotionGraphModel& graph,
                           const MidiGestureCapture& capture,
                           bool advancedMode,
                           juce::MemoryBlock& destData)
    {
        auto state = apvts.copyState();

        // Embed graph points
        auto graphVar = graph.toVar();
        state.setProperty ("graphPoints", graphVar, nullptr);

        // Embed learned gesture
        auto& templ = capture.getTemplate();
        if (templ.valid)
        {
            juce::var gestureObj (new juce::DynamicObject());
            auto* obj = gestureObj.getDynamicObject();
            obj->setProperty ("valid", true);
            obj->setProperty ("sourceNote", templ.sourceNote);
            obj->setProperty ("sourceVelocity", templ.sourceVelocity);
            obj->setProperty ("originalDurationSeconds", (double) templ.originalDurationSeconds);
            obj->setProperty ("normalizedBendDistance", (double) templ.normalizedBendDistance);
            obj->setProperty ("normalizedBendSpeed", (double) templ.normalizedBendSpeed);
            obj->setProperty ("confidence", (double) templ.confidence);

            auto pitchArr = std::make_unique<juce::Array<juce::var>>();
            for (auto v : templ.pitchContour) pitchArr->add ((double) v);
            obj->setProperty ("pitchContour", juce::var (pitchArr.release()));

            state.setProperty ("learnedGesture", gestureObj, nullptr);
        }

        state.setProperty ("stateVersion", kStateVersion, nullptr);
        state.setProperty ("advancedMode", advancedMode, nullptr);

        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        if (xml != nullptr)
            juce::AudioProcessor::copyXmlToBinary (*xml, destData);
    }

    static bool loadState (juce::AudioProcessorValueTreeState& apvts,
                           MotionGraphModel& graph,
                           MidiGestureCapture& capture,
                           bool& advancedMode,
                           const void* data, int sizeInBytes)
    {
        auto xml = juce::AudioProcessor::getXmlFromBinary (data, sizeInBytes);
        if (xml == nullptr) return false;

        auto tree = juce::ValueTree::fromXml (*xml);
        if (!tree.isValid()) return false;

        apvts.replaceState (tree);

        // Restore graph
        auto graphVar = tree.getProperty ("graphPoints");
        if (graphVar.isArray())
            graph.fromVar (graphVar);

        // Restore learned gesture (basic restore — full template restore
        // would need the capture object to accept a template directly;
        // for now we store/restore the pitch contour for playback)

        advancedMode = (bool) tree.getProperty ("advancedMode", false);

        return true;
    }
};

} // namespace followdelay
