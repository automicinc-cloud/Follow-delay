#pragma once
#include "../util/Constants.h"
#include "../util/DataStructures.h"
#include <vector>
#include <algorithm>
#include <mutex>
#include <cmath>

namespace followdelay
{

class MotionGraphModel
{
public:
    MotionGraphModel()
    {
        // Default: gentle descending curve
        points.push_back ({ 0.0f, 0.72f });
        points.push_back ({ 0.35f, 0.62f });
        points.push_back ({ 1.0f, 0.12f });
        rebuildCurve();
    }

    // ── Point editing (UI thread) ─────────────────────────
    void setPoints (std::vector<GraphPoint> newPoints)
    {
        std::lock_guard<std::mutex> lock (mtx);
        points = std::move (newPoints);
        std::sort (points.begin(), points.end());
        clampPoints();
        rebuildCurve();
    }

    void addPoint (float x, float y)
    {
        std::lock_guard<std::mutex> lock (mtx);
        points.push_back ({ std::clamp (x, 0.0f, 1.0f), std::clamp (y, 0.0f, 1.0f) });
        std::sort (points.begin(), points.end());
        rebuildCurve();
    }

    void clearPoints()
    {
        std::lock_guard<std::mutex> lock (mtx);
        points.clear();
        points.push_back ({ 0.0f, 0.5f });
        points.push_back ({ 1.0f, 0.5f });
        rebuildCurve();
    }

    void resetToDefault()
    {
        std::lock_guard<std::mutex> lock (mtx);
        points.clear();
        points.push_back ({ 0.0f, 0.72f });
        points.push_back ({ 0.35f, 0.62f });
        points.push_back ({ 1.0f, 0.12f });
        rebuildCurve();
    }

    void setSmoothness (float s)
    {
        std::lock_guard<std::mutex> lock (mtx);
        smoothness = std::clamp (s, 0.0f, 1.0f);
        rebuildCurve();
    }

    // ── Snapshot access (audio thread safe after copy) ────
    MotionCurve getCurveSnapshot() const
    {
        std::lock_guard<std::mutex> lock (mtx);
        return curveSnapshot;
    }

    std::vector<GraphPoint> getPoints() const
    {
        std::lock_guard<std::mutex> lock (mtx);
        return points;
    }

    // ── Serialisation helpers ─────────────────────────────
    juce::var toVar() const
    {
        std::lock_guard<std::mutex> lock (mtx);
        auto arr = std::make_unique<juce::Array<juce::var>>();
        for (auto& p : points)
        {
            auto obj = std::make_unique<juce::DynamicObject>();
            obj->setProperty ("x", (double) p.x);
            obj->setProperty ("y", (double) p.y);
            arr->add (juce::var (obj.release()));
        }
        return juce::var (arr.release());
    }

    void fromVar (const juce::var& v)
    {
        if (!v.isArray()) return;
        std::lock_guard<std::mutex> lock (mtx);
        points.clear();
        for (int i = 0; i < v.size(); ++i)
        {
            auto& item = v[i];
            float x = (float)(double) item.getProperty ("x", 0.0);
            float y = (float)(double) item.getProperty ("y", 0.5);
            points.push_back ({ x, y });
        }
        if (points.size() < 2)
        {
            points.clear();
            points.push_back ({ 0.0f, 0.5f });
            points.push_back ({ 1.0f, 0.5f });
        }
        std::sort (points.begin(), points.end());
        clampPoints();
        rebuildCurve();
    }

private:
    void clampPoints()
    {
        for (auto& p : points)
        {
            p.x = std::clamp (p.x, 0.0f, 1.0f);
            p.y = std::clamp (p.y, 0.0f, 1.0f);
        }
    }

    void rebuildCurve()
    {
        curveSnapshot.samples.resize (kMotionCurveSize);
        curveSnapshot.valid = true;

        if (points.size() < 2)
        {
            std::fill (curveSnapshot.samples.begin(), curveSnapshot.samples.end(), 0.5f);
            return;
        }

        // Linear interpolation between points
        for (int i = 0; i < kMotionCurveSize; ++i)
        {
            float t = (float) i / (float)(kMotionCurveSize - 1);
            curveSnapshot.samples[i] = sampleAtTime (t);
        }

        // Smoothing passes
        if (smoothness > 0.0f)
        {
            int passes = (int)(smoothness * 16.0f) + 1;
            for (int p = 0; p < passes; ++p)
            {
                std::vector<float> temp (kMotionCurveSize);
                temp[0] = curveSnapshot.samples[0];
                temp[kMotionCurveSize - 1] = curveSnapshot.samples[kMotionCurveSize - 1];
                for (int j = 1; j < kMotionCurveSize - 1; ++j)
                    temp[j] = 0.25f * curveSnapshot.samples[j - 1]
                            + 0.50f * curveSnapshot.samples[j]
                            + 0.25f * curveSnapshot.samples[j + 1];
                curveSnapshot.samples = std::move (temp);
            }
        }
    }

    float sampleAtTime (float t) const
    {
        if (t <= points.front().x) return points.front().y;
        if (t >= points.back().x)  return points.back().y;

        for (size_t i = 1; i < points.size(); ++i)
        {
            if (t <= points[i].x)
            {
                float frac = (t - points[i - 1].x) / (points[i].x - points[i - 1].x + 1e-12f);
                return points[i - 1].y + frac * (points[i].y - points[i - 1].y);
            }
        }
        return points.back().y;
    }

    mutable std::mutex mtx;
    std::vector<GraphPoint> points;
    MotionCurve curveSnapshot;
    float smoothness = 0.55f;
};

} // namespace followdelay
