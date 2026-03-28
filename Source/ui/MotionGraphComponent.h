#pragma once
#include <JuceHeader.h>
#include "../model/MotionGraphModel.h"
#include "../util/DataStructures.h"
#include <atomic>

namespace followdelay
{

class MotionGraphComponent : public juce::Component, public juce::Timer
{
public:
    MotionGraphComponent (MotionGraphModel& model,
                          std::atomic<float>& playheadNorm,
                          std::atomic<bool>& motionActive)
        : graphModel (model), playhead (playheadNorm), active (motionActive)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (4.0f);

        // Background
        g.setColour (juce::Colour (0xFF1A1A2E));
        g.fillRoundedRectangle (bounds, 6.0f);

        // Grid
        g.setColour (juce::Colour (0xFF2A2A44));
        for (int i = 1; i < 4; ++i)
        {
            float y = bounds.getY() + bounds.getHeight() * (float) i / 4.0f;
            g.drawHorizontalLine ((int) y, bounds.getX(), bounds.getRight());
        }
        for (int i = 1; i < 8; ++i)
        {
            float x = bounds.getX() + bounds.getWidth() * (float) i / 8.0f;
            g.drawVerticalLine ((int) x, bounds.getY(), bounds.getBottom());
        }

        // Draw curve
        auto curve = graphModel.getCurveSnapshot();
        if (curve.valid && !curve.samples.empty())
        {
            juce::Path path;
            for (int i = 0; i < (int) curve.samples.size(); ++i)
            {
                float xNorm = (float) i / (float)(curve.samples.size() - 1);
                float yNorm = 1.0f - curve.samples[i]; // flip Y
                float px = bounds.getX() + xNorm * bounds.getWidth();
                float py = bounds.getY() + yNorm * bounds.getHeight();

                if (i == 0) path.startNewSubPath (px, py);
                else        path.lineTo (px, py);
            }

            g.setColour (juce::Colour (0xFF6C63FF));
            g.strokePath (path, juce::PathStrokeType (2.5f));

            // Glow
            g.setColour (juce::Colour (0x306C63FF));
            g.strokePath (path, juce::PathStrokeType (6.0f));
        }

        // Draw control points
        auto points = graphModel.getPoints();
        for (auto& pt : points)
        {
            float px = bounds.getX() + pt.x * bounds.getWidth();
            float py = bounds.getY() + (1.0f - pt.y) * bounds.getHeight();
            g.setColour (juce::Colours::white);
            g.fillEllipse (px - 5.0f, py - 5.0f, 10.0f, 10.0f);
            g.setColour (juce::Colour (0xFF6C63FF));
            g.fillEllipse (px - 3.5f, py - 3.5f, 7.0f, 7.0f);
        }

        // Playhead
        if (active.load())
        {
            float ph = playhead.load();
            float px = bounds.getX() + ph * bounds.getWidth();
            g.setColour (juce::Colour (0xFFFF6B6B));
            g.drawVerticalLine ((int) px, bounds.getY(), bounds.getBottom());
            g.fillEllipse (px - 4.0f, bounds.getY() - 4.0f, 8.0f, 8.0f);
        }

        // Border
        g.setColour (juce::Colour (0xFF3A3A5E));
        g.drawRoundedRectangle (bounds, 6.0f, 1.5f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (4.0f);
        float xNorm = (e.position.x - bounds.getX()) / bounds.getWidth();
        float yNorm = 1.0f - (e.position.y - bounds.getY()) / bounds.getHeight();

        // Check if near existing point
        auto points = graphModel.getPoints();
        dragIndex = -1;
        for (int i = 0; i < (int) points.size(); ++i)
        {
            float px = bounds.getX() + points[i].x * bounds.getWidth();
            float py = bounds.getY() + (1.0f - points[i].y) * bounds.getHeight();
            if (std::abs (e.position.x - px) < 12.0f && std::abs (e.position.y - py) < 12.0f)
            {
                dragIndex = i;
                return;
            }
        }

        // Add new point
        xNorm = std::clamp (xNorm, 0.0f, 1.0f);
        yNorm = std::clamp (yNorm, 0.0f, 1.0f);
        graphModel.addPoint (xNorm, yNorm);
        dragIndex = -1; // find it next mouse drag
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragIndex < 0) return;

        auto bounds = getLocalBounds().toFloat().reduced (4.0f);
        float xNorm = std::clamp ((e.position.x - bounds.getX()) / bounds.getWidth(), 0.0f, 1.0f);
        float yNorm = std::clamp (1.0f - (e.position.y - bounds.getY()) / bounds.getHeight(), 0.0f, 1.0f);

        auto points = graphModel.getPoints();
        if (dragIndex < (int) points.size())
        {
            points[dragIndex].x = xNorm;
            points[dragIndex].y = yNorm;
            graphModel.setPoints (points);
        }
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (4.0f);
        auto points = graphModel.getPoints();

        for (int i = 0; i < (int) points.size(); ++i)
        {
            float px = bounds.getX() + points[i].x * bounds.getWidth();
            float py = bounds.getY() + (1.0f - points[i].y) * bounds.getHeight();
            if (std::abs (e.position.x - px) < 12.0f && std::abs (e.position.y - py) < 12.0f)
            {
                if (points.size() > 2)
                {
                    points.erase (points.begin() + i);
                    graphModel.setPoints (points);
                }
                return;
            }
        }
    }

    void timerCallback() override
    {
        repaint();
    }

private:
    MotionGraphModel& graphModel;
    std::atomic<float>& playhead;
    std::atomic<bool>& active;
    int dragIndex = -1;
};

} // namespace followdelay
