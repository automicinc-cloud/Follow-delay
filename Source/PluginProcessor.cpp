#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace followdelay
{

juce::AudioProcessorEditor* FollowDelayAudioProcessor::createEditor()
{
    return new FollowDelayAudioProcessorEditor (*this);
}

} // namespace followdelay

// ── JUCE plugin instantiation ─────────────────────────────────
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new followdelay::FollowDelayAudioProcessor();
}
