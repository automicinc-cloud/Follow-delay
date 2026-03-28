#pragma once
#include <JuceHeader.h>

namespace followdelay
{

class FollowDelayLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FollowDelayLookAndFeel()
    {
        setColour (juce::Slider::rotarySliderFillColourId,    juce::Colour (0xFF6C63FF));
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xFF2A2A44));
        setColour (juce::Slider::thumbColourId,               juce::Colours::white);
        setColour (juce::Label::textColourId,                 juce::Colour (0xFFCCCCDD));
        setColour (juce::ComboBox::backgroundColourId,        juce::Colour (0xFF1E1E36));
        setColour (juce::ComboBox::textColourId,              juce::Colour (0xFFCCCCDD));
        setColour (juce::ComboBox::outlineColourId,           juce::Colour (0xFF3A3A5E));
        setColour (juce::TextButton::buttonColourId,          juce::Colour (0xFF2A2A44));
        setColour (juce::TextButton::textColourOffId,         juce::Colour (0xFFCCCCDD));
        setColour (juce::TextButton::buttonOnColourId,        juce::Colour (0xFF6C63FF));
        setColour (juce::TextButton::textColourOnId,          juce::Colours::white);
        setColour (juce::PopupMenu::backgroundColourId,       juce::Colour (0xFF1A1A2E));
        setColour (juce::PopupMenu::textColourId,             juce::Colour (0xFFCCCCDD));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xFF6C63FF));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
        float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto centre = bounds.getCentre();

        float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Track background
        juce::Path bgArc;
        bgArc.addCentredArc (centre.x, centre.y, radius - 4.0f, radius - 4.0f,
                             0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xFF2A2A44));
        g.strokePath (bgArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        // Value arc
        juce::Path valueArc;
        valueArc.addCentredArc (centre.x, centre.y, radius - 4.0f, radius - 4.0f,
                                0.0f, rotaryStartAngle, angle, true);
        g.setColour (juce::Colour (0xFF6C63FF));
        g.strokePath (valueArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

        // Thumb dot
        float thumbRadius = 5.0f;
        float thumbX = centre.x + (radius - 4.0f) * std::cos (angle - juce::MathConstants<float>::halfPi);
        float thumbY = centre.y + (radius - 4.0f) * std::sin (angle - juce::MathConstants<float>::halfPi);
        g.setColour (juce::Colours::white);
        g.fillEllipse (thumbX - thumbRadius, thumbY - thumbRadius,
                       thumbRadius * 2.0f, thumbRadius * 2.0f);

        // Centre label
        g.setColour (juce::Colour (0xFFCCCCDD));
        g.setFont (11.0f);
        juce::String text;
        if (slider.getTextValueSuffix().isNotEmpty())
            text = juce::String (slider.getValue(), 0) + slider.getTextValueSuffix();
        else
            text = juce::String (slider.getValue(), 2);
        g.drawText (text, bounds.reduced (radius * 0.35f), juce::Justification::centred, false);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        auto colour = button.getToggleState()
                    ? juce::Colour (0xFF6C63FF)
                    : juce::Colour (0xFF2A2A44);
        if (isButtonDown) colour = colour.brighter (0.2f);

        g.setColour (colour);
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (juce::Colour (0xFF3A3A5E));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
    }
};

} // namespace followdelay
